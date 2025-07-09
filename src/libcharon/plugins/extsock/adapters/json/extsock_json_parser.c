/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_json_parser.h"
#include "../../common/extsock_common.h"
#include "../../domain/extsock_config_entity.h"

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
     * 인메모리 자격증명 세트 (PSK 저장용)
     */
    mem_cred_t *creds;
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

    // 🔴 HIGH PRIORITY: 안전한 메모리 할당
    char *str_result = malloc(result.len + 1);
    if (!str_result) {
        chunk_free(&result);
        // malloc 실패 시 fallback 문자열도 안전하게 할당
        char *fallback = strdup("%any");
        return fallback;  // strdup이 실패하면 NULL 반환 (적절한 에러 처리)
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
        lifetime->time.rekey = j_rekey->valueint;
    }
    
    cJSON *j_life = cJSON_GetObjectItem(j_lifetime, "life_time");
    if (j_life && cJSON_IsNumber(j_life)) {
        lifetime->time.life = j_life->valueint;
    }
    
    /*
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
    );

    if (this->creds) {
        lib->credmgr->add_set(lib->credmgr, &this->creds->set);
    }

    return &this->public;
} 