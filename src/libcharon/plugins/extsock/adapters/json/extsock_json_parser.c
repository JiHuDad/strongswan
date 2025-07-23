/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_json_parser.h"
#include "../../common/extsock_common.h"
#include "../crypto/extsock_cert_loader.h"

#include <cjson/cJSON.h>
#include <daemon.h>
#include <library.h>
#include <config/ike_cfg.h>
#include <config/peer_cfg.h>
#include <config/child_cfg.h>
#include <credentials/auth_cfg.h>
#include <credentials/keys/shared_key.h>
#include <credentials/sets/mem_cred.h>
#include <utils/identification.h>
#include <utils/chunk.h>
#include <selectors/traffic_selector.h>
#include <collections/linked_list.h>
#include <utils/debug.h>
#include <ipsec/ipsec_types.h>

typedef struct private_extsock_json_parser_t private_extsock_json_parser_t;

/**
 * JSON 파싱 어댑터 내부 구조체
 */
struct private_extsock_json_parser_t {
    
    /**
     * 공개 인터페이스
     */
    extsock_json_parser_t public;
    
    /**
     * 인메모리 자격증명 세트 (PSK/인증서 저장용)
     */
    mem_cred_t *creds;
    
    /**
     * 인증서 로더
     */
    extsock_cert_loader_t *cert_loader;
};

/**
 * JSON 배열을 쉼표로 구분된 문자열로 변환
 */
static char* json_array_to_comma_separated_string(cJSON *json_array) 
{
    if (!json_array || !cJSON_IsArray(json_array) || cJSON_GetArraySize(json_array) == 0) {
        return strdup("%any");
    }

    chunk_t result = chunk_empty;
    chunk_t comma = chunk_from_str(",");
    cJSON *item;
    bool first = TRUE;

    cJSON_ArrayForEach(item, json_array) {
        if (cJSON_IsString(item) && item->valuestring && *(item->valuestring)) {
            chunk_t current_item_chunk = chunk_from_str(item->valuestring);
            if (first) {
                result = chunk_clone(current_item_chunk);
                first = FALSE;
            } else {
                result = chunk_cat("mcc", result, comma, current_item_chunk);
            }
        }
    }

    if (result.len == 0) {
        return strdup("%any");
    }

    // HIGH PRIORITY: 안전한 메모리 할당
    char *str_result = malloc(result.len + 1);
    if (!str_result) {
        chunk_free(&result);
        return NULL;
    }
    memcpy(str_result, result.ptr, result.len);
    str_result[result.len] = '\0';
    chunk_free(&result);

    return str_result;
}

/**
 * 문자열을 action_t로 변환
 */
static action_t string_to_action(const char* action_str) 
{
    if (!action_str) return ACTION_NONE;
    if (streq(action_str, "trap")) return ACTION_TRAP;
    if (streq(action_str, "start")) return ACTION_START;
    if (streq(action_str, "clear")) return ACTION_TRAP;
    if (streq(action_str, "hold")) return ACTION_TRAP;
    if (streq(action_str, "restart")) return ACTION_START;
    return ACTION_NONE;
}

/**
 * 문자열을 dscp_copy_t로 변환
 */
static dscp_copy_t string_to_dscp_copy(const char* dscp_str) 
{
    if (!dscp_str) return DSCP_COPY_OUT_ONLY;  // 기본값
    
    if (streq(dscp_str, "out")) return DSCP_COPY_OUT_ONLY;
    if (streq(dscp_str, "in")) return DSCP_COPY_IN_ONLY;
    if (streq(dscp_str, "yes")) return DSCP_COPY_YES;
    if (streq(dscp_str, "no")) return DSCP_COPY_NO;
    
    EXTSOCK_DBG(1, "Unknown DSCP copy mode: %s, using default 'out'", dscp_str);
    return DSCP_COPY_OUT_ONLY;  // 기본값
}

/**
 * 6비트 바이너리 문자열을 DSCP 값으로 변환
 */
static uint8_t parse_dscp_value(const char* dscp_str)
{
    if (!dscp_str) {
        return 0;  // 기본값
    }
    
    size_t len = strlen(dscp_str);
    if (len != 6) {
        EXTSOCK_DBG(1, "Invalid DSCP string length: %zu (expected 6), using default 0", len);
        return 0;  // 기본값
    }
    
    uint8_t dscp = 0;
    for (int i = 0; i < 6; i++) {
        if (dscp_str[i] == '1') {
            dscp |= (1 << (5 - i));
        } else if (dscp_str[i] != '0') {
            EXTSOCK_DBG(1, "Invalid DSCP character at position %d: '%c' (expected '0' or '1'), using default 0", i, dscp_str[i]);
            return 0;  // 잘못된 문자
        }
    }
    
    EXTSOCK_DBG(2, "Parsed DSCP value: %s -> 0x%02x", dscp_str, dscp);
    return dscp;
}

METHOD(extsock_json_parser_t, parse_proposals, linked_list_t *,
    private_extsock_json_parser_t *this, cJSON *json_array, protocol_id_t proto, bool is_ike)
{
    linked_list_t *proposals_list = linked_list_create();
    if (!proposals_list) return NULL;

    if (json_array && cJSON_IsArray(json_array)) {
        cJSON *prop_json;
        cJSON_ArrayForEach(prop_json, json_array) {
            if (cJSON_IsString(prop_json) && prop_json->valuestring) {
                proposal_t *p = proposal_create_from_string(proto, prop_json->valuestring);
                if (p) {
                    proposals_list->insert_last(proposals_list, p);
                } else {
                    EXTSOCK_DBG(1, "Failed to parse proposal string: %s for proto %d", 
                               prop_json->valuestring, proto);
                }
            }
        }
    }

    if (proposals_list->get_count(proposals_list) == 0) {
        EXTSOCK_DBG(1, "No proposals in JSON, adding defaults for proto %d (is_ike: %d)", proto, is_ike);
        if (is_ike) {
            proposal_t *first = proposal_create_default(PROTO_IKE);
            if (first) proposals_list->insert_last(proposals_list, first);
            proposal_t *second = proposal_create_default_aead(PROTO_IKE);
            if (second) proposals_list->insert_last(proposals_list, second);
        } else {
            proposal_t *first = proposal_create_default_aead(proto);
            if (first) proposals_list->insert_last(proposals_list, first);
            proposal_t *second = proposal_create_default(proto);
            if (second) proposals_list->insert_last(proposals_list, second);
        }
    }
    return proposals_list;
}

METHOD(extsock_json_parser_t, parse_traffic_selectors, linked_list_t *,
    private_extsock_json_parser_t *this, cJSON *json_array)
{
    linked_list_t *ts_list = linked_list_create();
    if (!ts_list) return NULL;

    if (json_array && cJSON_IsArray(json_array)) {
        cJSON *ts_json;
        cJSON_ArrayForEach(ts_json, json_array) {
            if (cJSON_IsString(ts_json) && ts_json->valuestring) {
                traffic_selector_t *ts = traffic_selector_create_from_cidr(
                    ts_json->valuestring, 0, 0, 0xFFFF);
                if (ts) {
                    ts_list->insert_last(ts_list, ts);
                } else {
                    EXTSOCK_DBG(1, "Failed to parse TS string as CIDR: %s", ts_json->valuestring);
                }
            }
        }
    }
    
    if (ts_list->get_count(ts_list) == 0) {
        traffic_selector_t* ts = traffic_selector_create_dynamic(0, 0, 0xFFFF);
        if (ts) ts_list->insert_last(ts_list, ts);
        EXTSOCK_DBG(1, "No traffic selectors in JSON or all failed to parse, adding dynamic TS");
    }
    return ts_list;
}

METHOD(extsock_json_parser_t, parse_ike_config, ike_cfg_t *,
    private_extsock_json_parser_t *this, cJSON *ike_json)
{
    // HIGH PRIORITY: NULL 체크 강화
    EXTSOCK_CHECK_NULL_RET_NULL(ike_json);

    ike_cfg_create_t ike_create_cfg = {0};

    // 로컬 및 원격 주소 파싱
    cJSON *j_local_addrs = cJSON_GetObjectItem(ike_json, "local_addrs");
    ike_create_cfg.local = json_array_to_comma_separated_string(j_local_addrs);
    cJSON *j_remote_addrs = cJSON_GetObjectItem(ike_json, "remote_addrs");
    ike_create_cfg.remote = json_array_to_comma_separated_string(j_remote_addrs);
    
    // IKE 버전 파싱
    cJSON *j_version = cJSON_GetObjectItem(ike_json, "version");
    if (j_version && cJSON_IsNumber(j_version)) {
        ike_create_cfg.version = j_version->valueint;
    } else {
        ike_create_cfg.version = IKE_ANY; 
    }
    
    // DSCP 설정 파싱
    cJSON *j_dscp = cJSON_GetObjectItem(ike_json, "dscp");
    if (j_dscp && cJSON_IsString(j_dscp) && j_dscp->valuestring) {
        ike_create_cfg.dscp = parse_dscp_value(j_dscp->valuestring);
        EXTSOCK_DBG(2, "IKE DSCP set to: 0x%02x", ike_create_cfg.dscp);
    } else {
        ike_create_cfg.dscp = 0;  // 기본값
        EXTSOCK_DBG(2, "IKE DSCP using default: 0x00");
    }
    
    // 포트 설정 - 안전한 포트 가져오기
    if (charon && charon->socket) {
        ike_create_cfg.local_port = charon->socket->get_port(charon->socket, FALSE);
    } else {
        ike_create_cfg.local_port = IKEV2_UDP_PORT;
        EXTSOCK_DBG(1, "Warning: charon->socket not available, using default port");
    }
    ike_create_cfg.remote_port = IKEV2_UDP_PORT;

    // HIGH PRIORITY: strongSwan API 안전 호출
    ike_cfg_t *ike_cfg = EXTSOCK_SAFE_STRONGSWAN_CREATE(ike_cfg_create, &ike_create_cfg);
    
    // 메모리 정리
    EXTSOCK_SAFE_FREE(ike_create_cfg.local);
    EXTSOCK_SAFE_FREE(ike_create_cfg.remote);

    if (!ike_cfg) {
        EXTSOCK_DBG(1, "Failed to create ike_cfg");
        return NULL;
    }

    // IKE 제안 파싱 및 추가
    cJSON *j_proposals = cJSON_GetObjectItem(ike_json, "proposals");
    linked_list_t *ike_proposals = this->public.parse_proposals(&this->public, j_proposals, PROTO_IKE, TRUE);
    if (ike_proposals) {
        proposal_t *prop;
        while (ike_proposals->remove_first(ike_proposals, (void**)&prop) == SUCCESS) {
            if (prop) {  // NULL 체크 추가
                ike_cfg->add_proposal(ike_cfg, prop);
            }
        }
        ike_proposals->destroy(ike_proposals);
    }
    
    return ike_cfg;
}



/**
 * CHILD SA lifetime 설정 파싱
 */
static lifetime_cfg_t* parse_child_lifetime(cJSON *child_json)
{
    lifetime_cfg_t *lifetime = malloc_thing(lifetime_cfg_t);
    memset(lifetime, 0, sizeof(*lifetime));
    
    cJSON *j_lifetime = cJSON_GetObjectItem(child_json, "lifetime");
    if (!j_lifetime) {
        // 기본값 설정
        lifetime->time.rekey = 3600;  // 1시간
        lifetime->time.life = 7200;   // 2시간
        return lifetime;
    }
    
    cJSON *j_rekey = cJSON_GetObjectItem(j_lifetime, "rekey_time");
    if (j_rekey && cJSON_IsNumber(j_rekey)) {
        lifetime->time.rekey = j_rekey->valueint * 0.9;
        lifetime->time.life = lifetime->time.rekey + 20;
        lifetime->time.jitter = 20;
        EXTSOCK_DBG(1, "Child rekey_time set to %u seconds", lifetime->time.rekey);
        EXTSOCK_DBG(1, "Child life_time set to %u seconds", lifetime->time.life);
        EXTSOCK_DBG(1, "Child jitter set to %u seconds", lifetime->time.jitter);
    }

    /*
    cJSON *j_life = cJSON_GetObjectItem(j_lifetime, "life_time");
    if (j_life && cJSON_IsNumber(j_life)) {
        lifetime->time.life = j_life->valueint;
    }
    
    cJSON *j_rekey_bytes = cJSON_GetObjectItem(j_lifetime, "rekey_bytes");
    if (j_rekey_bytes && cJSON_IsNumber(j_rekey_bytes)) {
        lifetime->bytes.rekey = j_rekey_bytes->valueint;
    }
    
    cJSON *j_life_bytes = cJSON_GetObjectItem(j_lifetime, "life_bytes");
    if (j_life_bytes && cJSON_IsNumber(j_life_bytes)) {
        lifetime->bytes.life = j_life_bytes->valueint;
    }
    */
    
    // packets, jitter 등 추가 설정...
    
    return lifetime;
}

METHOD(extsock_json_parser_t, parse_auth_config, auth_cfg_t *,
    private_extsock_json_parser_t *this, cJSON *auth_json, bool is_local)
{
    // HIGH PRIORITY: NULL 체크 강화
    EXTSOCK_CHECK_NULL_RET_NULL(auth_json);

    auth_cfg_t *auth_cfg = EXTSOCK_SAFE_STRONGSWAN_CREATE(auth_cfg_create);
    if (!auth_cfg) {
        EXTSOCK_DBG(1, "Failed to create auth_cfg");
        return NULL;
    }

    cJSON *j_auth_type = cJSON_GetObjectItem(auth_json, "auth");
    cJSON *j_id = cJSON_GetObjectItem(auth_json, "id");
    cJSON *j_secret = cJSON_GetObjectItem(auth_json, "secret");

    if (j_auth_type && cJSON_IsString(j_auth_type) && j_auth_type->valuestring) {
        const char *auth_type_str = j_auth_type->valuestring;
        if (streq(auth_type_str, "psk")) {
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK);
            identification_t *psk_identity = NULL;
            
            if (j_id && cJSON_IsString(j_id) && j_id->valuestring) {
                // HIGH PRIORITY: strongSwan API 안전 호출
                psk_identity = EXTSOCK_SAFE_STRONGSWAN_CREATE(identification_create_from_string, j_id->valuestring);
                if (psk_identity) {
                    identification_t *auth_identity = EXTSOCK_SAFE_STRONGSWAN_CREATE(identification_create_from_string, j_id->valuestring);
                    if (auth_identity) {
                        auth_cfg->add(auth_cfg, AUTH_RULE_IDENTITY, auth_identity);
                    }
                }
            } else {
                psk_identity = EXTSOCK_SAFE_STRONGSWAN_CREATE(identification_create_from_string, "%any");
            }

            if (psk_identity) {
                if (j_secret && cJSON_IsString(j_secret) && j_secret->valuestring) {
                    const char *secret_str = j_secret->valuestring;
                    chunk_t secret_chunk = chunk_from_str((char*)secret_str);
                    shared_key_t *psk_key = EXTSOCK_SAFE_STRONGSWAN_CREATE(shared_key_create, SHARED_IKE, chunk_clone(secret_chunk));
                    if (psk_key && this->creds) {
                        this->creds->add_shared(this->creds, psk_key, psk_identity, NULL);
                    } else {
                        EXTSOCK_DBG(1, "Failed to create PSK key for ID: %s", j_id ? j_id->valuestring : "%any");
                        if (psk_identity) psk_identity->destroy(psk_identity);
                    }
                } else {
                    EXTSOCK_DBG(1, "PSK auth specified but 'secret' missing for ID: %s", j_id ? j_id->valuestring : "%any");
                    if (psk_identity) psk_identity->destroy(psk_identity);
                }
            }
        } else if (streq(auth_type_str, "pubkey")) {
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PUBKEY);
            if (j_id && cJSON_IsString(j_id) && j_id->valuestring) {
                identification_t *pubkey_id = EXTSOCK_SAFE_STRONGSWAN_CREATE(identification_create_from_string, j_id->valuestring);
                if (pubkey_id) {
                    auth_cfg->add(auth_cfg, AUTH_RULE_IDENTITY, pubkey_id);
                }
            }
        } else if (streq(auth_type_str, "cert")) {
            // 인증서 기반 인증
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PUBKEY);
            
            // JSON에서 인증서 관련 필드들 추출
            cJSON *j_cert = cJSON_GetObjectItem(auth_json, "cert");
            cJSON *j_private_key = cJSON_GetObjectItem(auth_json, "private_key");
            cJSON *j_private_key_passphrase = cJSON_GetObjectItem(auth_json, "private_key_passphrase");
            cJSON *j_ca_cert = cJSON_GetObjectItem(auth_json, "ca_cert");
            cJSON *j_enable_ocsp = cJSON_GetObjectItem(auth_json, "enable_ocsp");
            cJSON *j_enable_crl = cJSON_GetObjectItem(auth_json, "enable_crl");
            
            certificate_t *cert = NULL;
            private_key_t *private_key = NULL;
            certificate_t *ca_cert = NULL;
            
            // 인증서 파일 로딩
            if (j_cert && cJSON_IsString(j_cert) && j_cert->valuestring) {
                cert = this->cert_loader->load_certificate(this->cert_loader, j_cert->valuestring);
                if (cert) {
                    EXTSOCK_DBG(2, "Certificate loaded from: %s", j_cert->valuestring);
                    
                    // 인증서에서 identity 추출 (ID가 별도 지정되지 않은 경우)
                    if (!j_id || !cJSON_IsString(j_id) || !j_id->valuestring) {
                        identification_t *cert_subject = cert->get_subject(cert);
                        if (cert_subject) {
                            auth_cfg->add(auth_cfg, AUTH_RULE_IDENTITY, cert_subject->clone(cert_subject));
                            EXTSOCK_DBG(3, "Using certificate subject as identity: %Y", cert_subject);
                        }
                    } else {
                        identification_t *specified_id = EXTSOCK_SAFE_STRONGSWAN_CREATE(identification_create_from_string, j_id->valuestring);
                        if (specified_id) {
                            auth_cfg->add(auth_cfg, AUTH_RULE_IDENTITY, specified_id);
                            EXTSOCK_DBG(3, "Using specified identity: %Y", specified_id);
                        }
                    }
                    
                    // credential store에 인증서 추가
                    this->creds->add_cert(this->creds, TRUE, cert);
                } else {
                    EXTSOCK_DBG(1, "Failed to load certificate from: %s", j_cert->valuestring);
                }
            }
            
            // 개인키 파일 로딩 (로컬 인증에만 필요)
            if (is_local && j_private_key && cJSON_IsString(j_private_key) && j_private_key->valuestring) {
                const char *passphrase = NULL;
                if (j_private_key_passphrase && cJSON_IsString(j_private_key_passphrase) && j_private_key_passphrase->valuestring) {
                    passphrase = j_private_key_passphrase->valuestring;
                }

                EXTSOCK_DBG(2, "Loading private key from: %s (passphrase: %s)", 
                    j_private_key->valuestring, passphrase ? "provided" : "none");
                
                // Try to load with provided password first, then auto-resolution
                if (passphrase) {
                    EXTSOCK_DBG(2, "Setting password for private key decryption");
                    this->cert_loader->set_password(this->cert_loader, (char*)passphrase);
                }

                private_key = this->cert_loader->load_private_key_auto(this->cert_loader,
                                                                  j_private_key->valuestring);

                if (!private_key && passphrase) {
                    EXTSOCK_DBG(2, "Auto loading failed, trying with explicit password");
                    private_key = this->cert_loader->load_private_key(this->cert_loader, j_private_key->valuestring, (char*)passphrase);
                }

                if (private_key) {
                    EXTSOCK_DBG(1, "Private key loaded from: %s", j_private_key->valuestring);
                    
                    // 개인키와 인증서 매칭 확인
                    if (cert && !this->cert_loader->verify_key_cert_match(this->cert_loader, private_key, cert)) {
                        EXTSOCK_DBG(1, "WARNING: Private key and certificate do not match!");
                    }
                    
                    // credential store에 개인키 추가
                    this->creds->add_key(this->creds, private_key);
                } else {
                    EXTSOCK_DBG(1, "Failed to load private key from: %s", j_private_key->valuestring);
                }

                this->cert_loader->set_password(this->cert_loader, NULL); // Reset password
            }

            // Support Multiple CA Certificates
            linked_list_t *ca_certs_list = linked_list_create();
            int ca_count = 0;

            // Single CA cert (backwards compatibility)
            if (j_ca_cert && cJSON_IsString(j_ca_cert) && j_ca_cert->valuestring) {
                ca_cert = this->cert_loader->load_certificate(this->cert_loader, j_ca_cert->valuestring);
                if (ca_cert) {
                    ca_certs_list->insert_last(ca_certs_list, ca_cert);
                    ca_count++;
                    EXTSOCK_DBG(2, "CA certificate loaded from: %s", j_ca_cert->valuestring);
                } else {
                    EXTSOCK_DBG(1, "Failed to load CA certificate from: %s", j_ca_cert->valuestring);
                }
            }

            // NEW : Multiple CA certs array support
            cJSON *j_ca_certs = cJSON_GetObjectItem(auth_json, "ca_certs");
            if (j_ca_certs && cJSON_IsArray(j_ca_certs)) {
                cJSON *ca_cert_item;
                cJSON_ArrayForEach(ca_cert_item, j_ca_certs) {
                    if (cJSON_IsString(ca_cert_item) && ca_cert_item->valuestring) {
                        certificate_t *ca = this->cert_loader->load_certificate(this->cert_loader, ca_cert_item->valuestring);
                        if (ca) {
                            ca_certs_list->insert_last(ca_certs_list, ca);
                            ca_count++;
                            EXTSOCK_DBG(2, "CA certificate loaded from: %s", ca_cert_item->valuestring);
                        } else {
                            EXTSOCK_DBG(1, "Failed to load CA certificate from: %s", ca_cert_item->valuestring);
                        }
                    }
                }
            }

            // NEW : Structured CA chain support (root, intermediate, etc.)
            cJSON *j_ca_chain = cJSON_GetObjectItem(auth_json, "ca_chain");
            if (j_ca_chain && cJSON_IsObject(j_ca_chain)) {
                cJSON *j_root_ca = cJSON_GetObjectItem(j_ca_chain, "root_ca");
                if (j_root_ca && cJSON_IsString(j_root_ca) && j_root_ca->valuestring) {
                    certificate_t *root_ca = this->cert_loader->load_certificate(this->cert_loader, j_root_ca->valuestring);
                    if (root_ca) {
                        ca_certs_list->insert_last(ca_certs_list, root_ca);
                        ca_count++;
                        EXTSOCK_DBG(2, "Root CA certificate loaded from: %s", j_root_ca->valuestring);
                    } else {
                        EXTSOCK_DBG(1, "Failed to load Root CA certificate from: %s", j_root_ca->valuestring);
                    }
                }

                // Intermediate CA
                cJSON *j_intermediate_cas = cJSON_GetObjectItem(j_ca_chain, "intermediate_cas");
                if (j_intermediate_cas && cJSON_IsArray(j_intermediate_cas)) {
                    cJSON *intermediate_ca_item;
                    int intermediate_ca_count = 0;
                    cJSON_ArrayForEach(intermediate_ca_item, j_intermediate_cas) {
                        if (cJSON_IsString(intermediate_ca_item) && intermediate_ca_item->valuestring) {
                            certificate_t *intermediate_ca = this->cert_loader->load_certificate(this->cert_loader, intermediate_ca_item->valuestring);
                            if (intermediate_ca) {
                                ca_certs_list->insert_last(ca_certs_list, intermediate_ca);
                                ca_count++;
                                intermediate_ca_count++;
                                EXTSOCK_DBG(2, "Intermediate CA certificate loaded from: %s", intermediate_ca_item->valuestring);
                            } else {
                                EXTSOCK_DBG(1, "Failed to load Intermediate CA certificate from: %s", intermediate_ca_item->valuestring);
                            }
                        }
                    }
                    EXTSOCK_DBG(1, "Loaded %d intermediate CA certificates", intermediate_ca_count);
                }
            }

            EXTSOCK_DBG(1, "Loaded %d CA certificates", ca_count);

            // Phase 3: Advanced trust chain validation
            if (ca_count > 0 && cert) {
                auth_cfg_t *trust_chain;
                bool online_validation = TRUE;  // Enable by default

                // Check for online validation preferences
                bool enable_ocsp = TRUE;  // Default enabled
                bool enable_crl = TRUE;   // Default enabled

                if (j_enable_ocsp && cJSON_IsBool(j_enable_ocsp)) {
                    enable_ocsp = cJSON_IsTrue(j_enable_ocsp);
                }
                if (j_enable_crl && cJSON_IsBool(j_enable_crl)) {
                    enable_crl = cJSON_IsTrue(j_enable_crl);
                }

                online_validation = enable_ocsp || enable_crl;

                // Configure cert loader for online validation
                this->cert_loader->set_online_validation(this->cert_loader, online_validation);

                EXTSOCK_DBG(1, "Building advanced trust chain with %d CA(s), OCSP(%s)/CRL(%s) validation", 
                            ca_count, enable_ocsp ? "enabled" : "disabled",
                            enable_crl ? "enabled" : "disabled");

                trust_chain = this->cert_loader->build_trust_chain(this->cert_loader, 
                                                                  cert, ca_certs_list, online_validation);
                
                if (trust_chain) {
                    enumerator_t *trust_enum;
                    auth_rule_t rule;
                    void *value;
                    
                    EXTSOCK_DBG(1, "Advanced trust chain validation successful");

                    // Extract validation results
                    trust_enum = trust_chain->create_enumerator(trust_chain);
                    while (trust_enum->enumerate(trust_enum, &rule, &value)) {
                        switch (rule) {
                            case AUTH_RULE_OCSP_VALIDATION:
                                EXTSOCK_DBG(2, "OCSP validation result: %d", (cert_validation_t)(uintptr_t)value);
                                break;
                            case AUTH_RULE_CRL_VALIDATION:
                                EXTSOCK_DBG(2, "CRL validation result: %d", (cert_validation_t)(uintptr_t)value);
                                break;
                            case AUTH_RULE_CA_CERT:
                                EXTSOCK_DBG(2, "Trusted CA found in chain");
                                break;
                            case AUTH_RULE_IM_CERT:
                                EXTSOCK_DBG(2, "Intermediate CA found in chain");
                                break;
                            default:
                                break;
                        }
                    }
                    trust_enum->destroy(trust_enum);
                    trust_chain->destroy(trust_chain);
                } else {
                    EXTSOCK_DBG(1, "Advanced trust chain validation failed");
                }
            }

            // Add all CA certificates to credential store
            if (ca_count > 0) {
                certificate_t *ca_cert_item;
                enumerator_t *ca_enum = ca_certs_list->create_enumerator(ca_certs_list);
                while (ca_enum->enumerate(ca_enum, (void**)&ca_cert_item)) {
                    this->creds->add_cert(this->creds, TRUE, ca_cert_item);
                    if (ca_cert_item->issued_by(ca_cert_item, ca_cert_item, NULL)) {
                        auth_cfg->add(auth_cfg, AUTH_RULE_CA_CERT, ca_cert_item->get_ref(ca_cert_item));
                    } else {
                        auth_cfg->add(auth_cfg, AUTH_RULE_IM_CERT, ca_cert_item->get_ref(ca_cert_item));
                    }

                    EXTSOCK_DBG(3, "CA certificate added to credential store and auth config: %Y", ca_cert_item);
                }
                ca_enum->destroy(ca_enum);
            }

            // Clean up CA list (certificates are now referenced by credential store)
            ca_certs_list->destroy(ca_certs_list);

            if (!cert && !ca_cert) {
                EXTSOCK_DBG(1, "Certificate authentication specified but no certificate files provided");
            }
        } else {
            EXTSOCK_DBG(1, "Unsupported auth type: %s", auth_type_str);
            auth_cfg->destroy(auth_cfg);
            return NULL;
        }
    } else {
        EXTSOCK_DBG(1, "'auth' type missing in auth config");
        auth_cfg->destroy(auth_cfg);
        return NULL;
    }
    return auth_cfg;
}

METHOD(extsock_json_parser_t, parse_child_configs, bool,
    private_extsock_json_parser_t *this, peer_cfg_t *peer_cfg, cJSON *children_json_array)
{
    if (!children_json_array || !cJSON_IsArray(children_json_array)) {
        return TRUE;
    }

    cJSON *child_json;
    cJSON_ArrayForEach(child_json, children_json_array) {
        if (!cJSON_IsObject(child_json)) continue;

        cJSON *j_name = cJSON_GetObjectItem(child_json, "name");
        if (!j_name || !cJSON_IsString(j_name) || !j_name->valuestring) {
            EXTSOCK_DBG(1, "Child config missing 'name'");
            continue;
        }
        const char *child_name_str = j_name->valuestring;

        child_cfg_create_t child_create_cfg = {0};
        
        cJSON *j_start_action = cJSON_GetObjectItem(child_json, "start_action");
        if (j_start_action && cJSON_IsString(j_start_action)) {
            child_create_cfg.start_action = string_to_action(j_start_action->valuestring);
        } else {
            child_create_cfg.start_action = ACTION_NONE; 
        }
        
        cJSON *j_dpd_action = cJSON_GetObjectItem(child_json, "dpd_action");
        if (j_dpd_action && cJSON_IsString(j_dpd_action)) {
            child_create_cfg.dpd_action = string_to_action(j_dpd_action->valuestring);
        } else {
            child_create_cfg.dpd_action = ACTION_NONE; 
        }

        // DSCP 복사 모드 파싱
        cJSON *j_copy_dscp = cJSON_GetObjectItem(child_json, "copy_dscp");
        if (j_copy_dscp && cJSON_IsString(j_copy_dscp) && j_copy_dscp->valuestring) {
            child_create_cfg.copy_dscp = string_to_dscp_copy(j_copy_dscp->valuestring);
            EXTSOCK_DBG(2, "CHILD DSCP copy mode set to: %s", j_copy_dscp->valuestring);
        } else {
            child_create_cfg.copy_dscp = DSCP_COPY_OUT_ONLY;  // 기본값
            EXTSOCK_DBG(2, "CHILD DSCP copy mode using default: out");
        }

        // CHILD SA lifetime 설정 파싱
        lifetime_cfg_t *lifetime = parse_child_lifetime(child_json);
        if (lifetime) {
            child_create_cfg.lifetime = *lifetime;
            free(lifetime);
        }

        child_cfg_t *child_cfg = child_cfg_create((char*)child_name_str, &child_create_cfg);
        if (!child_cfg) {
            EXTSOCK_DBG(1, "Failed to create child_cfg: %s", child_name_str);
            continue;
        }

        // 로컬 트래픽 셀렉터
        cJSON *j_local_ts = cJSON_GetObjectItem(child_json, "local_ts");
        linked_list_t *local_ts_list = this->public.parse_traffic_selectors(&this->public, j_local_ts);
        if (local_ts_list) {
            traffic_selector_t *ts;
            while (local_ts_list->remove_first(local_ts_list, (void**)&ts) == SUCCESS) {
                child_cfg->add_traffic_selector(child_cfg, TRUE, ts);
            }
            local_ts_list->destroy(local_ts_list);
        }

        // 원격 트래픽 셀렉터
        cJSON *j_remote_ts = cJSON_GetObjectItem(child_json, "remote_ts");
        linked_list_t *remote_ts_list = this->public.parse_traffic_selectors(&this->public, j_remote_ts);
        if (remote_ts_list) {
            traffic_selector_t *ts;
            while (remote_ts_list->remove_first(remote_ts_list, (void**)&ts) == SUCCESS) {
                child_cfg->add_traffic_selector(child_cfg, FALSE, ts);
            }
            remote_ts_list->destroy(remote_ts_list);
        }

        // ESP 제안
        cJSON *j_esp_proposals = cJSON_GetObjectItem(child_json, "esp_proposals");
        linked_list_t *esp_proposals_list = this->public.parse_proposals(&this->public, j_esp_proposals, PROTO_ESP, FALSE);
        if (esp_proposals_list) {
            proposal_t *prop;
            while (esp_proposals_list->remove_first(esp_proposals_list, (void**)&prop) == SUCCESS) {
                child_cfg->add_proposal(child_cfg, prop);
            }
            esp_proposals_list->destroy(esp_proposals_list);
        }
        
        peer_cfg->add_child_cfg(peer_cfg, child_cfg);
        EXTSOCK_DBG(2, "Added child_cfg: %s to peer_cfg: %s", child_name_str, peer_cfg->get_name(peer_cfg));
    }
    return TRUE;
}

METHOD(extsock_json_parser_t, parse_config_entity, extsock_config_entity_t *,
    private_extsock_json_parser_t *this, const char *config_json)
{
    // 이 메서드는 domain layer 구현 후 완성됩니다
    EXTSOCK_DBG(1, "parse_config_entity not yet implemented");
    return NULL;
}

METHOD(extsock_json_parser_t, destroy, void,
    private_extsock_json_parser_t *this)
{
    if (this->cert_loader) {
        this->cert_loader->destroy(this->cert_loader);
    }
    free(this);
}

/**
 * JSON 파싱 어댑터 생성
 */
extsock_json_parser_t *extsock_json_parser_create()
{
    private_extsock_json_parser_t *this;

    INIT(this,
        .public = {
            .parse_ike_config = _parse_ike_config,
            .parse_auth_config = _parse_auth_config,
            .parse_proposals = _parse_proposals,
            .parse_traffic_selectors = _parse_traffic_selectors,
            .parse_child_configs = _parse_child_configs,
            .parse_config_entity = _parse_config_entity,
            .destroy = _destroy,
        },
        .creds = mem_cred_create(),
        .cert_loader = extsock_cert_loader_create(),
    );

    if (this->creds) {
        lib->credmgr->add_set(lib->credmgr, &this->creds->set);
    }
    
    if (!this->cert_loader) {
        EXTSOCK_DBG(1, "Failed to create certificate loader");
        if (this->creds) {
            lib->credmgr->remove_set(lib->credmgr, &this->creds->set);
            this->creds->destroy(this->creds);
        }
        free(this);
        return NULL;
    }

    EXTSOCK_DBG(2, "JSON parser created with certificate support");
    return &this->public;
} 