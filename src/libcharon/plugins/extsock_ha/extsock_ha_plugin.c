/*
 * Copyright (C) 2024 strongSwan Project
 *
 * External Socket Plugin with High Availability (HA) support
 * Includes ALL original extsock functionality plus SEGW failover
 */

#include "extsock_ha_plugin.h"

#include <daemon.h>
#include <library.h>
#include <threading/thread.h>
#include <threading/mutex.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <utils/debug.h>
#include <sys/types.h>
#include <stdint.h>

#include <config/ike_cfg.h>
#include <config/peer_cfg.h>
#include <config/child_cfg.h>
#include <settings/settings.h>
#include <sa/ike_sa_manager.h>
#include <sa/ike_sa.h>
#include <sa/ikev2/tasks/ike_dpd.h>
#include <kernel/kernel_listener.h>
#include <networking/host.h>
#include <credentials/sets/mem_cred.h>
#include <credentials/credential_manager.h>
#include <control/controller.h>
#include <bus/listeners/listener.h>
#include <sa/child_sa.h>
#include <selectors/traffic_selector.h>
#include <credentials/auth_cfg.h>
#include <credentials/keys/shared_key.h>
#include <utils/identification.h>
#include <utils/chunk.h>
#include <collections/linked_list.h>
#include <utils/enum.h>
#include <crypto/crypters/crypter.h>
#include <crypto/signers/signer.h>
#include <crypto/proposal/proposal.h>
#include <ipsec/ipsec_types.h>

#include <cjson/cJSON.h>

#define HA_HASH_SIZE 1021
#define SOCKET_PATH "/tmp/strongswan_extsock_ha"

typedef struct private_extsock_ha_plugin_t private_extsock_ha_plugin_t;

/**
 * HA 피어 설정 구조체 - 완전한 설정 백업
 */
typedef struct ha_peer_config_t {
    char *base_name;
    char *primary_peer_name;
    char *backup_peer_name;
    char *current_active_name;
    char *primary_segw_addr;
    char *backup_segw_addr;
    
    peer_cfg_create_t peer_template;
    ike_cfg_create_t ike_template;
    
    linked_list_t *local_auth_cfgs;
    linked_list_t *remote_auth_cfgs;
    linked_list_t *child_cfgs;
    linked_list_t *ike_proposals;
    
    bool using_backup;
    peer_cfg_t *active_peer_cfg;
    struct ha_peer_config_t *next;
} ha_peer_config_t;

/**
 * 플러그인 내부 구조체
 */
struct private_extsock_ha_plugin_t {
    extsock_ha_plugin_t public;
    mem_cred_t *creds;
    int sock_fd;
    thread_t *thread;
    bool running;
    linked_list_t *managed_peer_cfgs;
    mutex_t *peer_cfgs_mutex;
    ha_peer_config_t *ha_hash[HA_HASH_SIZE];
    mutex_t *ha_hash_mutex;
    listener_t listener;
};

static private_extsock_ha_plugin_t *g_ha_plugin = NULL;

// 전방 선언
static void send_event_to_external(const char *event_json);
static bool apply_ipsec_config(private_extsock_ha_plugin_t *this, const char *config_json);
static bool apply_ha_config(private_extsock_ha_plugin_t *this, const char *config_json);
static void handle_external_command(private_extsock_ha_plugin_t *this, char *cmd);
static void* socket_thread(private_extsock_ha_plugin_t *this);

// HA 관련 함수 전방 선언
static ha_peer_config_t* find_ha_config(private_extsock_ha_plugin_t *this, const char *base_name);
static void store_ha_config(private_extsock_ha_plugin_t *this, ha_peer_config_t *ha_config);
static bool create_peer_cfg_for_segw(private_extsock_ha_plugin_t *this, ha_peer_config_t *ha_config, 
                                     const char *peer_name, const char *segw_addr);
static bool perform_ha_failover(private_extsock_ha_plugin_t *this, const char *base_name);
static void free_ha_config(ha_peer_config_t *ha_config);
static linked_list_t* clone_auth_cfgs(linked_list_t *auth_cfgs);
static linked_list_t* clone_child_cfgs(linked_list_t *child_cfgs);
static linked_list_t* clone_proposals(linked_list_t *proposals);

// ===== 기존 extsock 기능들 (완전 보존) =====

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

    char *str_result = malloc(result.len + 1);
    if (!str_result) {
        chunk_free(&result);
        return strdup("%any");
    }
    memcpy(str_result, result.ptr, result.len);
    str_result[result.len] = '\0';
    chunk_free(&result);

    return str_result;
}

/**
 * JSON 배열로부터 proposal 목록 파싱
 */
static linked_list_t* parse_proposals_from_json_array(cJSON *json_array, protocol_id_t proto, bool is_ike) 
{
    if (!json_array || !cJSON_IsArray(json_array)) {
        return NULL;
    }

    linked_list_t *proposals = linked_list_create();
    cJSON *proposal_json;

    cJSON_ArrayForEach(proposal_json, json_array) {
        if (cJSON_IsString(proposal_json)) {
            proposal_t *proposal = proposal_create_from_string(proto, proposal_json->valuestring);
            if (proposal) {
                proposals->insert_last(proposals, proposal);
            }
        }
    }

    return proposals;
}

/**
 * 문자열을 action_t로 변환
 */
static action_t string_to_action(const char* action_str)
{
    if (!action_str) return ACTION_NONE;
    
    if (streq(action_str, "none")) return ACTION_NONE;
    if (streq(action_str, "route")) return ACTION_ROUTE;
    if (streq(action_str, "start")) return ACTION_START;
    if (streq(action_str, "restart")) return ACTION_RESTART;
    
    return ACTION_NONE;
}

/**
 * JSON 배열로부터 traffic selector 목록 파싱
 */
static linked_list_t* parse_ts_from_json_array(cJSON *json_array)
{
    if (!json_array || !cJSON_IsArray(json_array)) {
        return NULL;
    }

    linked_list_t *ts_list = linked_list_create();
    cJSON *ts_json;

    cJSON_ArrayForEach(ts_json, json_array) {
        if (cJSON_IsString(ts_json)) {
            char *ts_str = ts_json->valuestring;
            traffic_selector_t *ts = traffic_selector_create_from_cidr(ts_str, 0, 0, 65535);
            if (ts) {
                ts_list->insert_last(ts_list, ts);
            }
        }
    }

    return ts_list;
}

/**
 * JSON으로부터 IKE config 파싱
 */
static ike_cfg_t* parse_ike_cfg_from_json(cJSON *ike_json)
{
    if (!ike_json) return NULL;

    cJSON *local_json = cJSON_GetObjectItem(ike_json, "local");
    cJSON *remote_json = cJSON_GetObjectItem(ike_json, "remote");
    cJSON *version_json = cJSON_GetObjectItem(ike_json, "version");
    cJSON *local_port_json = cJSON_GetObjectItem(ike_json, "local_port");
    cJSON *remote_port_json = cJSON_GetObjectItem(ike_json, "remote_port");

    char *local = cJSON_IsString(local_json) ? local_json->valuestring : "0.0.0.0";
    char *remote = cJSON_IsString(remote_json) ? remote_json->valuestring : "%any";
    int version = cJSON_IsNumber(version_json) ? (int)version_json->valuedouble : 2;
    int local_port = cJSON_IsNumber(local_port_json) ? (int)local_port_json->valuedouble : 500;
    int remote_port = cJSON_IsNumber(remote_port_json) ? (int)remote_port_json->valuedouble : 500;

    ike_cfg_create_t ike_cfg_data = {
        .version = version,
        .local = local,
        .remote = remote,
        .local_port = local_port,
        .remote_port = remote_port,
        .dscp = 0,
        .fragmentation = TRUE,
    };

    ike_cfg_t *ike_cfg = ike_cfg_create(&ike_cfg_data);
    if (!ike_cfg) {
        return NULL;
    }

    // IKE proposals 추가
    cJSON *proposals_json = cJSON_GetObjectItem(ike_json, "proposals");
    if (proposals_json) {
        linked_list_t *proposals = parse_proposals_from_json_array(proposals_json, PROTO_IKE, TRUE);
        if (proposals) {
            enumerator_t *enumerator = proposals->create_enumerator(proposals);
            proposal_t *proposal;
            while (enumerator->enumerate(enumerator, &proposal)) {
                ike_cfg->add_proposal(ike_cfg, proposal->clone(proposal, PROPOSAL_PREFER_SUPPLIED));
            }
            enumerator->destroy(enumerator);
            proposals->destroy_offset(proposals, offsetof(proposal_t, destroy));
        }
    }

    return ike_cfg;
}

/**
 * JSON으로부터 auth config 파싱
 */
static auth_cfg_t* parse_auth_cfg_from_json(cJSON *auth_json, bool is_local)
{
    if (!auth_json) return NULL;

    auth_cfg_t *auth_cfg = auth_cfg_create();

    cJSON *auth_method = cJSON_GetObjectItem(auth_json, "auth");
    if (auth_method && cJSON_IsString(auth_method)) {
        if (streq(auth_method->valuestring, "psk")) {
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK);
        } else if (streq(auth_method->valuestring, "pubkey")) {
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PUBKEY);
        } else if (streq(auth_method->valuestring, "eap")) {
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_EAP);
        }
    }

    cJSON *id_json = cJSON_GetObjectItem(auth_json, "id");
    if (id_json && cJSON_IsString(id_json)) {
        identification_t *id = identification_create_from_string(id_json->valuestring);
        if (id) {
            auth_cfg->add(auth_cfg, AUTH_RULE_IDENTITY, id);
        }
    }

    return auth_cfg;
}

/**
 * Traffic selector를 문자열로 변환
 */
static void ts_to_string(traffic_selector_t *ts, char *buf, size_t buflen)
{
    if (!ts || !buf || buflen == 0) {
        return;
    }

    host_t *from = ts->get_from_address(ts);
    host_t *to = ts->get_to_address(ts);
    uint16_t from_port = ts->get_from_port(ts);
    uint16_t to_port = ts->get_to_port(ts);

    if (from_port == to_port && from_port == 0) {
        snprintf(buf, buflen, "%H-%H", from, to);
    } else if (from_port == to_port) {
        snprintf(buf, buflen, "%H[%u]-%H[%u]", from, from_port, to, to_port);
    } else {
        snprintf(buf, buflen, "%H[%u-%u]-%H[%u-%u]", 
                from, from_port, from_port, to, to_port, to_port);
    }
}

/**
 * DPD 시작
 */
static void start_dpd(const char *ike_sa_name)
{
    ike_sa_t *ike_sa = charon->ike_sa_manager->checkout_by_name(
        charon->ike_sa_manager, (char*)ike_sa_name, ID_MATCH_PERFECT);
    if (!ike_sa) {
        DBG1(DBG_LIB, "start_dpd: IKE_SA '%s' not found", ike_sa_name);
        return;
    }
    
    DBG1(DBG_LIB, "start_dpd: Starting DPD for IKE_SA '%s'", ike_sa_name);
    ike_dpd_t *dpd = ike_dpd_create(TRUE);
    ike_sa->queue_task(ike_sa, (task_t*)dpd);
    charon->ike_sa_manager->checkin(charon->ike_sa_manager, ike_sa);
}

/**
 * JSON으로부터 child config 파싱 및 peer config에 추가
 */
static bool add_children_from_json(peer_cfg_t *peer_cfg, cJSON *children_json_array)
{
    if (!peer_cfg || !children_json_array || !cJSON_IsArray(children_json_array)) {
        return FALSE;
    }

    cJSON *child_json;
    cJSON_ArrayForEach(child_json, children_json_array) {
        cJSON *name_json = cJSON_GetObjectItem(child_json, "name");
        if (!name_json || !cJSON_IsString(name_json)) {
            continue;
        }

        child_cfg_create_t child_cfg_data = {
            .lifetime = {
                .time = {
                    .life = 3600,
                    .rekey = 3300,
                    .jitter = 300
                },
                .bytes = {
                    .life = 0,
                    .rekey = 0,
                    .jitter = 0
                },
                .packets = {
                    .life = 0,
                    .rekey = 0,
                    .jitter = 0
                }
            },
            .mode = MODE_TUNNEL,
            .action = ACTION_NONE,
            .dpd_action = ACTION_RESTART,
            .close_action = ACTION_RESTART,
            .reqid = 0,
            .mark_in = { .value = 0, .mask = 0 },
            .mark_out = { .value = 0, .mask = 0 },
            .tfc = 0
        };

        // Mode 파싱
        cJSON *mode_json = cJSON_GetObjectItem(child_json, "mode");
        if (mode_json && cJSON_IsString(mode_json)) {
            if (streq(mode_json->valuestring, "tunnel")) {
                child_cfg_data.mode = MODE_TUNNEL;
            } else if (streq(mode_json->valuestring, "transport")) {
                child_cfg_data.mode = MODE_TRANSPORT;
            }
        }

        // Action 파싱
        cJSON *action_json = cJSON_GetObjectItem(child_json, "action");
        if (action_json && cJSON_IsString(action_json)) {
            child_cfg_data.action = string_to_action(action_json->valuestring);
        }

        child_cfg_t *child_cfg = child_cfg_create(name_json->valuestring, &child_cfg_data);
        if (!child_cfg) {
            continue;
        }

        // Local traffic selectors 추가
        cJSON *local_ts_json = cJSON_GetObjectItem(child_json, "local_ts");
        if (local_ts_json) {
            linked_list_t *local_ts_list = parse_ts_from_json_array(local_ts_json);
            if (local_ts_list) {
                enumerator_t *enumerator = local_ts_list->create_enumerator(local_ts_list);
                traffic_selector_t *ts;
                while (enumerator->enumerate(enumerator, &ts)) {
                    child_cfg->add_traffic_selector(child_cfg, TRUE, ts);
                }
                enumerator->destroy(enumerator);
                local_ts_list->destroy(local_ts_list);
            }
        }

        // Remote traffic selectors 추가
        cJSON *remote_ts_json = cJSON_GetObjectItem(child_json, "remote_ts");
        if (remote_ts_json) {
            linked_list_t *remote_ts_list = parse_ts_from_json_array(remote_ts_json);
            if (remote_ts_list) {
                enumerator_t *enumerator = remote_ts_list->create_enumerator(remote_ts_list);
                traffic_selector_t *ts;
                while (enumerator->enumerate(enumerator, &ts)) {
                    child_cfg->add_traffic_selector(child_cfg, FALSE, ts);
                }
                enumerator->destroy(enumerator);
                remote_ts_list->destroy(remote_ts_list);
            }
        }

        // ESP proposals 추가
        cJSON *esp_proposals_json = cJSON_GetObjectItem(child_json, "esp_proposals");
        if (esp_proposals_json) {
            linked_list_t *esp_proposals = parse_proposals_from_json_array(esp_proposals_json, PROTO_ESP, FALSE);
            if (esp_proposals) {
                enumerator_t *enumerator = esp_proposals->create_enumerator(esp_proposals);
                proposal_t *proposal;
                while (enumerator->enumerate(enumerator, &proposal)) {
                    child_cfg->add_proposal(child_cfg, proposal->clone(proposal, PROPOSAL_PREFER_SUPPLIED));
                }
                enumerator->destroy(enumerator);
                esp_proposals->destroy_offset(esp_proposals, offsetof(proposal_t, destroy));
            }
        }

        peer_cfg->add_child_cfg(peer_cfg, child_cfg);
    }

    return TRUE;
}

/**
 * IPsec 설정 적용 (기존 extsock 핵심 기능)
 */
static bool apply_ipsec_config(private_extsock_ha_plugin_t *this, const char *config_json)
{
    if (!config_json) {
        DBG1(DBG_CFG, "Invalid JSON config");
        return FALSE;
    }

    cJSON *json = cJSON_Parse(config_json);
    if (!json) {
        DBG1(DBG_CFG, "Failed to parse JSON config");
        return FALSE;
    }

    // Peer name 가져오기
    cJSON *peer_name_json = cJSON_GetObjectItem(json, "peer_name");
    if (!peer_name_json || !cJSON_IsString(peer_name_json)) {
        DBG1(DBG_CFG, "Missing or invalid peer_name");
        cJSON_Delete(json);
        return FALSE;
    }
    char *peer_name = peer_name_json->valuestring;

    // IKE config 파싱
    cJSON *ike_json = cJSON_GetObjectItem(json, "ike");
    ike_cfg_t *ike_cfg = parse_ike_cfg_from_json(ike_json);
    if (!ike_cfg) {
        DBG1(DBG_CFG, "Failed to parse IKE config");
        cJSON_Delete(json);
        return FALSE;
    }

    // Peer config 생성
    peer_cfg_create_t peer_cfg_data = {
        .cert_policy = CERT_SEND_IF_ASKED,
        .unique = UNIQUE_REPLACE,
        .keyingtries = 3,
        .rekey_time = 3600,
        .reauth_time = 0,
        .jitter_time = 300,
        .over_time = 300,
        .dpd_delay = 30,
        .dpd_timeout = 120,
        .dpd_action = ACTION_RESTART,
        .mediation = FALSE,
        .mediated_by = NULL,
        .peer_id = NULL,
    };

    peer_cfg_t *peer_cfg = peer_cfg_create(peer_name, ike_cfg, &peer_cfg_data);
    if (!peer_cfg) {
        DBG1(DBG_CFG, "Failed to create peer config");
        ike_cfg->destroy(ike_cfg);
        cJSON_Delete(json);
        return FALSE;
    }

    // Local auth config 추가
    cJSON *local_auth_json = cJSON_GetObjectItem(json, "local_auth");
    if (local_auth_json) {
        auth_cfg_t *local_auth = parse_auth_cfg_from_json(local_auth_json, TRUE);
        if (local_auth) {
            peer_cfg->add_auth_cfg(peer_cfg, local_auth, TRUE);
        }
    }

    // Remote auth config 추가
    cJSON *remote_auth_json = cJSON_GetObjectItem(json, "remote_auth");
    if (remote_auth_json) {
        auth_cfg_t *remote_auth = parse_auth_cfg_from_json(remote_auth_json, FALSE);
        if (remote_auth) {
            peer_cfg->add_auth_cfg(peer_cfg, remote_auth, FALSE);
        }
    }

    // Children 추가
    cJSON *children_json = cJSON_GetObjectItem(json, "children");
    if (children_json) {
        add_children_from_json(peer_cfg, children_json);
    }

    // 관리 목록에 추가
    this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
    this->managed_peer_cfgs->insert_last(this->managed_peer_cfgs, peer_cfg);
    this->peer_cfgs_mutex->unlock(this->peer_cfgs_mutex);

    // 연결 시도
    enumerator_t *child_enumerator = peer_cfg->create_child_cfg_enumerator(peer_cfg);
    child_cfg_t *child_cfg;
    bool success = FALSE;
    
    while (child_enumerator->enumerate(child_enumerator, &child_cfg)) {
        status_t status = charon->controller->initiate(
            charon->controller, peer_cfg, child_cfg, NULL, NULL, LEVEL_CTRL, 0, FALSE);
        
        if (status == SUCCESS) {
            success = TRUE;
            DBG1(DBG_CFG, "Successfully initiated connection for peer '%s'", peer_name);
        } else {
            DBG1(DBG_CFG, "Failed to initiate connection for peer '%s', status: %d", peer_name, status);
        }
    }
    child_enumerator->destroy(child_enumerator);

    cJSON_Delete(json);
    return success;
}

/**
 * 외부 명령 처리 (기존 extsock 기능)
 */
static void handle_external_command(private_extsock_ha_plugin_t *this, char *cmd)
{
    if (!cmd || strlen(cmd) == 0) {
        return;
    }

    DBG1(DBG_CFG, "Received external command: %s", cmd);

    cJSON *json = cJSON_Parse(cmd);
    if (!json) {
        DBG1(DBG_CFG, "Failed to parse command JSON");
        return;
    }

    cJSON *action = cJSON_GetObjectItem(json, "action");
    if (!action || !cJSON_IsString(action)) {
        DBG1(DBG_CFG, "Missing or invalid action");
        cJSON_Delete(json);
        return;
    }

    if (streq(action->valuestring, "configure")) {
        apply_ipsec_config(this, cmd);
    } else if (streq(action->valuestring, "configure_ha")) {
        apply_ha_config(this, cmd);
    } else if (streq(action->valuestring, "start_dpd")) {
        cJSON *ike_sa_name = cJSON_GetObjectItem(json, "ike_sa_name");
        if (ike_sa_name && cJSON_IsString(ike_sa_name)) {
            start_dpd(ike_sa_name->valuestring);
        }
    } else if (streq(action->valuestring, "manual_failover")) {
        cJSON *base_name_json = cJSON_GetObjectItem(json, "base_name");
        if (base_name_json && cJSON_IsString(base_name_json)) {
            perform_ha_failover(this, base_name_json->valuestring);
        }
    }

    cJSON_Delete(json);
}

/**
 * 소켓 스레드 (기존 extsock 기능)
 */
static void* socket_thread(private_extsock_ha_plugin_t *this)
{
    struct sockaddr_un addr;
    int client_fd;
    char buffer[4096];
    ssize_t bytes_read;

    // 소켓 생성
    this->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->sock_fd < 0) {
        DBG1(DBG_CFG, "Failed to create socket: %s", strerror(errno));
        return NULL;
    }

    // 소켓 주소 설정
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // 기존 소켓 파일 제거
    unlink(SOCKET_PATH);

    // 소켓 바인딩
    if (bind(this->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        DBG1(DBG_CFG, "Failed to bind socket: %s", strerror(errno));
        close(this->sock_fd);
        this->sock_fd = -1;
        return NULL;
    }

    // 소켓 리스닝
    if (listen(this->sock_fd, 5) == -1) {
        DBG1(DBG_CFG, "Failed to listen on socket: %s", strerror(errno));
        close(this->sock_fd);
        this->sock_fd = -1;
        return NULL;
    }

    DBG1(DBG_CFG, "External socket listening on %s", SOCKET_PATH);

    // 클라이언트 연결 처리
    while (this->running) {
        client_fd = accept(this->sock_fd, NULL, NULL);
        if (client_fd < 0) {
            if (this->running) {
                DBG1(DBG_CFG, "Failed to accept connection: %s", strerror(errno));
            }
            continue;
        }

        // 명령 읽기
        bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            handle_external_command(this, buffer);
        }

        close(client_fd);
    }

    return NULL;
}

// ===== HA 관련 함수들 =====

static uint32_t hash_string(const char *str)
{
    if (!str) return 0;
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static char* extract_base_name(const char *peer_name)
{
    if (!peer_name) return NULL;
    
    char *base = strdup(peer_name);
    if (!base) return NULL;
    
    char *suffix = strstr(base, "_primary");
    if (!suffix) {
        suffix = strstr(base, "_backup");
    }
    
    if (suffix) {
        *suffix = '\0';
    }
    
    return base;
}

/**
 * auth_cfg 리스트 복제
 */
static linked_list_t* clone_auth_cfgs(linked_list_t *auth_cfgs)
{
    if (!auth_cfgs) return NULL;
    
    linked_list_t *cloned = linked_list_create();
    enumerator_t *enumerator = auth_cfgs->create_enumerator(auth_cfgs);
    auth_cfg_t *auth_cfg;
    
    while (enumerator->enumerate(enumerator, &auth_cfg)) {
        cloned->insert_last(cloned, auth_cfg->clone(auth_cfg));
    }
    enumerator->destroy(enumerator);
    
    return cloned;
}

/**
 * child_cfg 리스트 복제
 */
static linked_list_t* clone_child_cfgs(linked_list_t *child_cfgs)
{
    if (!child_cfgs) return NULL;
    
    linked_list_t *cloned = linked_list_create();
    enumerator_t *enumerator = child_cfgs->create_enumerator(child_cfgs);
    child_cfg_t *child_cfg;
    
    while (enumerator->enumerate(enumerator, &child_cfg)) {
        cloned->insert_last(cloned, child_cfg->get_ref(child_cfg));
    }
    enumerator->destroy(enumerator);
    
    return cloned;
}

/**
 * proposal 리스트 복제
 */
static linked_list_t* clone_proposals(linked_list_t *proposals)
{
    if (!proposals) return NULL;
    
    linked_list_t *cloned = linked_list_create();
    enumerator_t *enumerator = proposals->create_enumerator(proposals);
    proposal_t *proposal;
    
    while (enumerator->enumerate(enumerator, &proposal)) {
        cloned->insert_last(cloned, proposal->clone(proposal, PROPOSAL_PREFER_SUPPLIED));
    }
    enumerator->destroy(enumerator);
    
    return cloned;
}

/**
 * HA 설정 검색
 */
static ha_peer_config_t* find_ha_config(private_extsock_ha_plugin_t *this, const char *base_name)
{
    if (!base_name) return NULL;
    
    uint32_t hash = hash_string(base_name) % HA_HASH_SIZE;
    ha_peer_config_t *result = NULL;
    
    this->ha_hash_mutex->lock(this->ha_hash_mutex);
    
    ha_peer_config_t *current = this->ha_hash[hash];
    while (current) {
        if (current->base_name && streq(current->base_name, base_name)) {
            result = current;  // 원본 포인터 반환 (복사 안함)
            break;
        }
        current = current->next;
    }
    
    this->ha_hash_mutex->unlock(this->ha_hash_mutex);
    return result;
}

/**
 * HA 설정 저장
 */
static void store_ha_config(private_extsock_ha_plugin_t *this, ha_peer_config_t *ha_config)
{
    if (!ha_config || !ha_config->base_name) return;
    
    uint32_t hash = hash_string(ha_config->base_name) % HA_HASH_SIZE;
    
    this->ha_hash_mutex->lock(this->ha_hash_mutex);
    
    // 기존 설정이 있는지 확인
    ha_peer_config_t **current = &this->ha_hash[hash];
    while (*current) {
        if ((*current)->base_name && streq((*current)->base_name, ha_config->base_name)) {
            // 기존 설정 교체
            ha_peer_config_t *old = *current;
            *current = ha_config;
            ha_config->next = old->next;
            free_ha_config(old);
            this->ha_hash_mutex->unlock(this->ha_hash_mutex);
            return;
        }
        current = &(*current)->next;
    }
    
    // 새로운 설정 추가
    ha_config->next = this->ha_hash[hash];
    this->ha_hash[hash] = ha_config;
    
    this->ha_hash_mutex->unlock(this->ha_hash_mutex);
}

/**
 * HA 설정 메모리 해제
 */
static void free_ha_config(ha_peer_config_t *ha_config)
{
    if (!ha_config) return;
    
    free(ha_config->base_name);
    free(ha_config->primary_peer_name);
    free(ha_config->backup_peer_name);
    free(ha_config->primary_segw_addr);
    free(ha_config->backup_segw_addr);
    
    if (ha_config->local_auth_cfgs) {
        ha_config->local_auth_cfgs->destroy_offset(ha_config->local_auth_cfgs, 
                                                  offsetof(auth_cfg_t, destroy));
    }
    if (ha_config->remote_auth_cfgs) {
        ha_config->remote_auth_cfgs->destroy_offset(ha_config->remote_auth_cfgs, 
                                                   offsetof(auth_cfg_t, destroy));
    }
    if (ha_config->child_cfgs) {
        ha_config->child_cfgs->destroy_offset(ha_config->child_cfgs, 
                                             offsetof(child_cfg_t, destroy));
    }
    if (ha_config->ike_proposals) {
        ha_config->ike_proposals->destroy_offset(ha_config->ike_proposals, 
                                                offsetof(proposal_t, destroy));
    }
    
    free(ha_config);
}

/**
 * 특정 SEGW에 대한 peer_cfg 생성
 */
static bool create_peer_cfg_for_segw(private_extsock_ha_plugin_t *this, ha_peer_config_t *ha_config, 
                                     const char *peer_name, const char *segw_addr)
{
    if (!ha_config || !peer_name || !segw_addr) return FALSE;
    
    // IKE config 생성 (템플릿 기반)
    ike_cfg_create_t ike_data = ha_config->ike_template;
    ike_data.remote = segw_addr;  // SEGW 주소만 변경
    
    ike_cfg_t *ike_cfg = ike_cfg_create(&ike_data);
    if (!ike_cfg) {
        DBG1(DBG_CFG, "Failed to create IKE config for peer %s", peer_name);
        return FALSE;
    }
    
    // IKE proposals 추가
    if (ha_config->ike_proposals) {
        enumerator_t *enumerator = ha_config->ike_proposals->create_enumerator(ha_config->ike_proposals);
        proposal_t *proposal;
        while (enumerator->enumerate(enumerator, &proposal)) {
            ike_cfg->add_proposal(ike_cfg, proposal->clone(proposal, PROPOSAL_PREFER_SUPPLIED));
        }
        enumerator->destroy(enumerator);
    }
    
    // peer config 생성
    peer_cfg_t *peer_cfg = peer_cfg_create((char*)peer_name, ike_cfg, &ha_config->peer_template);
    if (!peer_cfg) {
        DBG1(DBG_CFG, "Failed to create peer config for %s", peer_name);
        ike_cfg->destroy(ike_cfg);
        return FALSE;
    }
    
    // 인증 설정 추가
    if (ha_config->local_auth_cfgs) {
        enumerator_t *enumerator = ha_config->local_auth_cfgs->create_enumerator(ha_config->local_auth_cfgs);
        auth_cfg_t *auth_cfg;
        while (enumerator->enumerate(enumerator, &auth_cfg)) {
            peer_cfg->add_auth_cfg(peer_cfg, auth_cfg->clone(auth_cfg), TRUE);
        }
        enumerator->destroy(enumerator);
    }
    
    if (ha_config->remote_auth_cfgs) {
        enumerator_t *enumerator = ha_config->remote_auth_cfgs->create_enumerator(ha_config->remote_auth_cfgs);
        auth_cfg_t *auth_cfg;
        while (enumerator->enumerate(enumerator, &auth_cfg)) {
            peer_cfg->add_auth_cfg(peer_cfg, auth_cfg->clone(auth_cfg), FALSE);
        }
        enumerator->destroy(enumerator);
    }
    
    // child 설정 추가
    if (ha_config->child_cfgs) {
        enumerator_t *enumerator = ha_config->child_cfgs->create_enumerator(ha_config->child_cfgs);
        child_cfg_t *child_cfg;
        while (enumerator->enumerate(enumerator, &child_cfg)) {
            peer_cfg->add_child_cfg(peer_cfg, child_cfg->get_ref(child_cfg));
        }
        enumerator->destroy(enumerator);
    }
    
    // 관리 목록에 추가
    this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
    this->managed_peer_cfgs->insert_last(this->managed_peer_cfgs, peer_cfg);
    this->peer_cfgs_mutex->unlock(this->peer_cfgs_mutex);
    
    // HA 설정 업데이트 (현재 활성 peer_cfg)
    ha_config->active_peer_cfg = peer_cfg;
    
    DBG1(DBG_CFG, "Successfully created peer config '%s' for SEGW %s", peer_name, segw_addr);
    return TRUE;
}

/**
 * HA failover 수행 - 핵심 로직
 */
static bool perform_ha_failover(private_extsock_ha_plugin_t *this, const char *base_name)
{
    ha_peer_config_t *ha_config = find_ha_config(this, base_name);
    if (!ha_config) {
        DBG1(DBG_CFG, "No HA config found for %s", base_name);
        return FALSE;
    }
    
    const char *new_peer_name;
    const char *new_segw_addr;
    const char *old_segw_addr;
    
    if (ha_config->using_backup) {
        // Backup에서 Primary로 복구
        new_peer_name = ha_config->primary_peer_name;
        new_segw_addr = ha_config->primary_segw_addr;
        old_segw_addr = ha_config->backup_segw_addr;
        DBG1(DBG_CFG, "HA: Failing back to primary SEGW for %s", base_name);
    } else {
        // Primary에서 Backup으로 전환
        new_peer_name = ha_config->backup_peer_name;
        new_segw_addr = ha_config->backup_segw_addr;
        old_segw_addr = ha_config->primary_segw_addr;
        DBG1(DBG_CFG, "HA: Failing over to backup SEGW for %s", base_name);
    }
    
    // 1. 기존 IKE SA들 종료 (같은 base_name을 가진 모든 연결)
    enumerator_t *sa_enumerator = charon->controller->create_ike_sa_enumerator(charon->controller, TRUE);
    ike_sa_t *ike_sa;
    linked_list_t *sas_to_terminate = linked_list_create();
    
    while (sa_enumerator->enumerate(sa_enumerator, &ike_sa)) {
        peer_cfg_t *existing_peer_cfg = ike_sa->get_peer_cfg(ike_sa);
        if (existing_peer_cfg) {
            char *existing_base = extract_base_name(existing_peer_cfg->get_name(existing_peer_cfg));
            if (existing_base && streq(existing_base, base_name)) {
                uint32_t ike_sa_id = ike_sa->get_unique_id(ike_sa);
                sas_to_terminate->insert_last(sas_to_terminate, (void*)(uintptr_t)ike_sa_id);
                DBG1(DBG_CFG, "HA: Marking IKE_SA %u for termination", ike_sa_id);
            }
            free(existing_base);
        }
    }
    sa_enumerator->destroy(sa_enumerator);
    
    // IKE SA들 종료
    enumerator_t *term_enumerator = sas_to_terminate->create_enumerator(sas_to_terminate);
    void *sa_id_ptr;
    while (term_enumerator->enumerate(term_enumerator, &sa_id_ptr)) {
        uint32_t sa_id = (uint32_t)(uintptr_t)sa_id_ptr;
        charon->controller->terminate_ike(charon->controller, sa_id, NULL, NULL, 0);
        DBG1(DBG_CFG, "HA: Terminated IKE_SA %u for failover", sa_id);
    }
    term_enumerator->destroy(term_enumerator);
    sas_to_terminate->destroy(sas_to_terminate);
    
    // 2. 새로운 peer_cfg 생성 및 연결 시작
    bool success = FALSE;
    if (create_peer_cfg_for_segw(this, ha_config, new_peer_name, new_segw_addr)) {
        // 3. HA 상태 업데이트
        this->ha_hash_mutex->lock(this->ha_hash_mutex);
        ha_config->using_backup = !ha_config->using_backup;
        ha_config->current_active_name = (char*)new_peer_name;
        this->ha_hash_mutex->unlock(this->ha_hash_mutex);
        
        // 4. 새로운 연결 시도
        peer_cfg_t *new_peer_cfg = ha_config->active_peer_cfg;
        if (new_peer_cfg) {
            enumerator_t *child_enumerator = new_peer_cfg->create_child_cfg_enumerator(new_peer_cfg);
            child_cfg_t *child_cfg;
            
            while (child_enumerator->enumerate(child_enumerator, &child_cfg)) {
                status_t status = charon->controller->initiate(
                    charon->controller, new_peer_cfg, child_cfg, NULL, NULL, LEVEL_CTRL, 0, FALSE);
                
                if (status == SUCCESS) {
                    success = TRUE;
                    DBG1(DBG_CFG, "HA: Successfully initiated new connection for peer '%s'", new_peer_name);
                } else {
                    DBG1(DBG_CFG, "HA: Failed to initiate connection for peer '%s', status: %d", new_peer_name, status);
                }
            }
            child_enumerator->destroy(child_enumerator);
        }
        
        // 5. HA 이벤트 알림
        cJSON *ha_event = cJSON_CreateObject();
        cJSON_AddStringToObject(ha_event, "event", "ha_failover");
        cJSON_AddStringToObject(ha_event, "base_name", base_name);
        cJSON_AddStringToObject(ha_event, "from_segw", old_segw_addr);
        cJSON_AddStringToObject(ha_event, "to_segw", new_segw_addr);
        cJSON_AddStringToObject(ha_event, "new_peer_name", new_peer_name);
        cJSON_AddBoolToObject(ha_event, "success", success);
        
        char *ha_json_str = cJSON_Print(ha_event);
        if (ha_json_str) {
            send_event_to_external(ha_json_str);
            free(ha_json_str);
        }
        cJSON_Delete(ha_event);
        
        DBG1(DBG_CFG, "HA: Failover completed for %s from %s to %s", 
             base_name, old_segw_addr, new_segw_addr);
    }
    
    return success;
}

/**
 * HA 설정 적용 (새로운 기능)
 */
static bool apply_ha_config(private_extsock_ha_plugin_t *this, const char *config_json)
{
    if (!config_json) {
        DBG1(DBG_CFG, "Invalid HA JSON config");
        return FALSE;
    }

    cJSON *json = cJSON_Parse(config_json);
    if (!json) {
        DBG1(DBG_CFG, "Failed to parse HA JSON config");
        return FALSE;
    }

    // Base name 가져오기
    cJSON *base_name_json = cJSON_GetObjectItem(json, "base_name");
    if (!base_name_json || !cJSON_IsString(base_name_json)) {
        DBG1(DBG_CFG, "Missing or invalid base_name in HA config");
        cJSON_Delete(json);
        return FALSE;
    }
    char *base_name = base_name_json->valuestring;

    // SEGW 주소들 가져오기
    cJSON *primary_segw_json = cJSON_GetObjectItem(json, "primary_segw");
    cJSON *backup_segw_json = cJSON_GetObjectItem(json, "backup_segw");
    if (!primary_segw_json || !cJSON_IsString(primary_segw_json) ||
        !backup_segw_json || !cJSON_IsString(backup_segw_json)) {
        DBG1(DBG_CFG, "Missing SEGW addresses in HA config");
        cJSON_Delete(json);
        return FALSE;
    }

    // HA 설정 구조체 생성
    ha_peer_config_t *ha_config = malloc(sizeof(ha_peer_config_t));
    if (!ha_config) {
        cJSON_Delete(json);
        return FALSE;
    }
    memset(ha_config, 0, sizeof(ha_peer_config_t));

    // 기본 정보 설정
    ha_config->base_name = strdup(base_name);
    ha_config->primary_segw_addr = strdup(primary_segw_json->valuestring);
    ha_config->backup_segw_addr = strdup(backup_segw_json->valuestring);
    
    // 동적 peer 이름 생성
    asprintf(&ha_config->primary_peer_name, "%s_primary", base_name);
    asprintf(&ha_config->backup_peer_name, "%s_backup", base_name);
    ha_config->current_active_name = ha_config->primary_peer_name;
    ha_config->using_backup = FALSE;

    // IKE 템플릿 파싱
    cJSON *ike_json = cJSON_GetObjectItem(json, "ike");
    if (ike_json) {
        cJSON *local_json = cJSON_GetObjectItem(ike_json, "local");
        cJSON *version_json = cJSON_GetObjectItem(ike_json, "version");
        cJSON *local_port_json = cJSON_GetObjectItem(ike_json, "local_port");
        cJSON *remote_port_json = cJSON_GetObjectItem(ike_json, "remote_port");

        ha_config->ike_template.version = cJSON_IsNumber(version_json) ? (int)version_json->valuedouble : 2;
        ha_config->ike_template.local = cJSON_IsString(local_json) ? local_json->valuestring : "0.0.0.0";
        ha_config->ike_template.remote = "%any";  // 나중에 SEGW 주소로 대체
        ha_config->ike_template.local_port = cJSON_IsNumber(local_port_json) ? (int)local_port_json->valuedouble : 500;
        ha_config->ike_template.remote_port = cJSON_IsNumber(remote_port_json) ? (int)remote_port_json->valuedouble : 500;
        ha_config->ike_template.dscp = 0;
        ha_config->ike_template.fragmentation = TRUE;
        
        // IKE proposals 저장
        cJSON *proposals_json = cJSON_GetObjectItem(ike_json, "proposals");
        if (proposals_json) {
            ha_config->ike_proposals = parse_proposals_from_json_array(proposals_json, PROTO_IKE, TRUE);
        }
    }

    // Peer 템플릿 설정
    ha_config->peer_template.cert_policy = CERT_SEND_IF_ASKED;
    ha_config->peer_template.unique = UNIQUE_REPLACE;
    ha_config->peer_template.keyingtries = 3;
    ha_config->peer_template.rekey_time = 3600;
    ha_config->peer_template.reauth_time = 0;
    ha_config->peer_template.jitter_time = 300;
    ha_config->peer_template.over_time = 300;
    ha_config->peer_template.dpd_delay = 30;
    ha_config->peer_template.dpd_timeout = 120;
    ha_config->peer_template.dpd_action = ACTION_RESTART;
    ha_config->peer_template.mediation = FALSE;
    ha_config->peer_template.mediated_by = NULL;
    ha_config->peer_template.peer_id = NULL;

    // 인증 설정 저장
    cJSON *local_auth_json = cJSON_GetObjectItem(json, "local_auth");
    if (local_auth_json) {
        auth_cfg_t *local_auth = parse_auth_cfg_from_json(local_auth_json, TRUE);
        if (local_auth) {
            ha_config->local_auth_cfgs = linked_list_create();
            ha_config->local_auth_cfgs->insert_last(ha_config->local_auth_cfgs, local_auth);
        }
    }

    cJSON *remote_auth_json = cJSON_GetObjectItem(json, "remote_auth");
    if (remote_auth_json) {
        auth_cfg_t *remote_auth = parse_auth_cfg_from_json(remote_auth_json, FALSE);
        if (remote_auth) {
            ha_config->remote_auth_cfgs = linked_list_create();
            ha_config->remote_auth_cfgs->insert_last(ha_config->remote_auth_cfgs, remote_auth);
        }
    }

    // Child 설정들 저장
    cJSON *children_json = cJSON_GetObjectItem(json, "children");
    if (children_json && cJSON_IsArray(children_json)) {
        ha_config->child_cfgs = linked_list_create();
        
        cJSON *child_json;
        cJSON_ArrayForEach(child_json, children_json) {
            cJSON *name_json = cJSON_GetObjectItem(child_json, "name");
            if (!name_json || !cJSON_IsString(name_json)) continue;

            child_cfg_create_t child_cfg_data = {
                .lifetime = {
                    .time = { .life = 3600, .rekey = 3300, .jitter = 300 },
                    .bytes = { .life = 0, .rekey = 0, .jitter = 0 },
                    .packets = { .life = 0, .rekey = 0, .jitter = 0 }
                },
                .mode = MODE_TUNNEL,
                .action = ACTION_NONE,
                .dpd_action = ACTION_RESTART,
                .close_action = ACTION_RESTART,
                .reqid = 0,
                .mark_in = { .value = 0, .mask = 0 },
                .mark_out = { .value = 0, .mask = 0 },
                .tfc = 0
            };

            child_cfg_t *child_cfg = child_cfg_create(name_json->valuestring, &child_cfg_data);
            if (child_cfg) {
                // Traffic selectors 추가
                cJSON *local_ts_json = cJSON_GetObjectItem(child_json, "local_ts");
                if (local_ts_json) {
                    linked_list_t *local_ts_list = parse_ts_from_json_array(local_ts_json);
                    if (local_ts_list) {
                        enumerator_t *enumerator = local_ts_list->create_enumerator(local_ts_list);
                        traffic_selector_t *ts;
                        while (enumerator->enumerate(enumerator, &ts)) {
                            child_cfg->add_traffic_selector(child_cfg, TRUE, ts);
                        }
                        enumerator->destroy(enumerator);
                        local_ts_list->destroy(local_ts_list);
                    }
                }

                cJSON *remote_ts_json = cJSON_GetObjectItem(child_json, "remote_ts");
                if (remote_ts_json) {
                    linked_list_t *remote_ts_list = parse_ts_from_json_array(remote_ts_json);
                    if (remote_ts_list) {
                        enumerator_t *enumerator = remote_ts_list->create_enumerator(remote_ts_list);
                        traffic_selector_t *ts;
                        while (enumerator->enumerate(enumerator, &ts)) {
                            child_cfg->add_traffic_selector(child_cfg, FALSE, ts);
                        }
                        enumerator->destroy(enumerator);
                        remote_ts_list->destroy(remote_ts_list);
                    }
                }

                ha_config->child_cfgs->insert_last(ha_config->child_cfgs, child_cfg);
            }
        }
    }

    // HA 설정 저장
    store_ha_config(this, ha_config);

    // Primary SEGW로 초기 연결 생성
    bool success = create_peer_cfg_for_segw(this, ha_config, 
                                           ha_config->primary_peer_name, 
                                           ha_config->primary_segw_addr);
    if (success) {
        // 연결 시도
        peer_cfg_t *peer_cfg = ha_config->active_peer_cfg;
        if (peer_cfg) {
            enumerator_t *child_enumerator = peer_cfg->create_child_cfg_enumerator(peer_cfg);
            child_cfg_t *child_cfg;
            
            while (child_enumerator->enumerate(child_enumerator, &child_cfg)) {
                status_t status = charon->controller->initiate(
                    charon->controller, peer_cfg, child_cfg, NULL, NULL, LEVEL_CTRL, 0, FALSE);
                
                if (status == SUCCESS) {
                    DBG1(DBG_CFG, "HA: Successfully initiated initial connection for '%s'", 
                         ha_config->primary_peer_name);
                } else {
                    DBG1(DBG_CFG, "HA: Failed to initiate initial connection, status: %d", status);
                }
            }
            child_enumerator->destroy(child_enumerator);
        }
    }

    cJSON_Delete(json);
    DBG1(DBG_CFG, "HA configuration applied for base_name: %s", base_name);
    return success;
}

/**
 * 외부로 이벤트 전송 (기존 extsock 기능)
 */
static void send_event_to_external(const char *event_json)
{
    if (!event_json) return;
    
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return;
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        write(sock, event_json, strlen(event_json));
    }
    
    close(sock);
}

/**
 * Tunnel 상태 변경 이벤트 리스너 (기존 기능 + HA)
 */
static bool ha_child_updown(listener_t *this, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up)
{
    private_extsock_ha_plugin_t *plugin = g_ha_plugin;
    if (!plugin) return TRUE;
    
    // 기존 extsock 기능: 외부로 이벤트 전송
    cJSON *event = cJSON_CreateObject();
    cJSON_AddStringToObject(event, "event", "tunnel_state");
    cJSON_AddStringToObject(event, "state", up ? "up" : "down");
    
    if (ike_sa && child_sa) {
        peer_cfg_t *peer_cfg = ike_sa->get_peer_cfg(ike_sa);
        if (peer_cfg) {
            cJSON_AddStringToObject(event, "peer_name", peer_cfg->get_name(peer_cfg));
        }
        cJSON_AddStringToObject(event, "child_name", child_sa->get_name(child_sa));
        
        // Traffic Selector 정보 추가 (기존 기능)
        linked_list_t *local_ts = child_sa->get_traffic_selectors(child_sa, TRUE);
        linked_list_t *remote_ts = child_sa->get_traffic_selectors(child_sa, FALSE);
        
        if (local_ts && local_ts->get_count(local_ts) > 0) {
            cJSON *local_ts_array = cJSON_CreateArray();
            enumerator_t *enumerator = local_ts->create_enumerator(local_ts);
            traffic_selector_t *ts;
            while (enumerator->enumerate(enumerator, &ts)) {
                char ts_str[256];
                ts_to_string(ts, ts_str, sizeof(ts_str));
                cJSON_AddItemToArray(local_ts_array, cJSON_CreateString(ts_str));
            }
            enumerator->destroy(enumerator);
            cJSON_AddItemToObject(event, "local_ts", local_ts_array);
        }
        
        if (remote_ts && remote_ts->get_count(remote_ts) > 0) {
            cJSON *remote_ts_array = cJSON_CreateArray();
            enumerator_t *enumerator = remote_ts->create_enumerator(remote_ts);
            traffic_selector_t *ts;
            while (enumerator->enumerate(enumerator, &ts)) {
                char ts_str[256];
                ts_to_string(ts, ts_str, sizeof(ts_str));
                cJSON_AddItemToArray(remote_ts_array, cJSON_CreateString(ts_str));
            }
            enumerator->destroy(enumerator);
            cJSON_AddItemToObject(event, "remote_ts", remote_ts_array);
        }
    }
    
    char *json_str = cJSON_Print(event);
    if (json_str) {
        send_event_to_external(json_str);
        free(json_str);
    }
    cJSON_Delete(event);
    
    // HA failover 로직 (tunnel down 시)
    if (!up && ike_sa && child_sa) {
        peer_cfg_t *peer_cfg = ike_sa->get_peer_cfg(ike_sa);
        if (peer_cfg) {
            const char *peer_name = peer_cfg->get_name(peer_cfg);
            char *base_name = extract_base_name(peer_name);
            
            if (base_name) {
                // HA 설정이 있는지 확인
                ha_peer_config_t *ha_config = find_ha_config(plugin, base_name);
                if (ha_config) {
                    DBG1(DBG_CFG, "HA: Tunnel DOWN detected for peer '%s', base_name '%s'", 
                         peer_name, base_name);
                    DBG1(DBG_CFG, "HA: Attempting automatic failover...");
                    
                    // 비동기적으로 failover 수행 (이벤트 리스너에서 직접 하지 않음)
                    // 대신 간단한 지연 후 수행
                    sleep(1);  // 기존 연결이 완전히 정리될 시간 제공
                    
                    bool failover_success = perform_ha_failover(plugin, base_name);
                    if (failover_success) {
                        DBG1(DBG_CFG, "HA: Automatic failover completed successfully for %s", base_name);
                    } else {
                        DBG1(DBG_CFG, "HA: Automatic failover failed for %s", base_name);
                    }
                } else {
                    DBG2(DBG_CFG, "HA: No HA configuration found for base_name '%s'", base_name);
                }
                free(base_name);
            }
        }
    }
    
    return TRUE;
}

// ===== 플러그인 인터페이스 =====

METHOD(plugin_t, get_name, char*,
       private_extsock_ha_plugin_t *this)
{
    return "extsock-ha";
}

METHOD(plugin_t, get_features, int,
       private_extsock_ha_plugin_t *this, plugin_feature_t **features)
{
    static plugin_feature_t f[] = {
        PLUGIN_CALLBACK((plugin_feature_callback_t)NULL, NULL),
            PLUGIN_PROVIDE(CUSTOM, "extsock-ha"),
    };
    *features = f;
    return countof(f);
}

METHOD(plugin_t, destroy, void,
       private_extsock_ha_plugin_t *this)
{
    if (this->running) {
        this->running = FALSE;
        if (this->thread) {
            this->thread->cancel(this->thread);
            this->thread = NULL;
        }
    }
    
    if (this->sock_fd >= 0) {
        close(this->sock_fd);
        unlink(SOCKET_PATH);
    }
    
    charon->bus->remove_listener(charon->bus, &this->listener);
    
    if (this->managed_peer_cfgs) {
        this->managed_peer_cfgs->destroy_offset(this->managed_peer_cfgs, 
                                               offsetof(peer_cfg_t, destroy));
    }
    
    if (this->peer_cfgs_mutex) {
        this->peer_cfgs_mutex->destroy(this->peer_cfgs_mutex);
    }
    
    if (this->ha_hash_mutex) {
        this->ha_hash_mutex->destroy(this->ha_hash_mutex);
    }
    
    for (int i = 0; i < HA_HASH_SIZE; i++) {
        ha_peer_config_t *current = this->ha_hash[i];
        while (current) {
            ha_peer_config_t *next = current->next;
            free_ha_config(current);
            current = next;
        }
    }
    
    if (this->creds) {
        charon->credentials->remove_set(charon->credentials, &this->creds->set);
        this->creds->destroy(this->creds);
    }
    
    g_ha_plugin = NULL;
    free(this);
}

plugin_t *extsock_ha_plugin_create()
{
    private_extsock_ha_plugin_t *this;
    
    INIT(this,
        .public = {
            .plugin = {
                .get_name = _get_name,
                .get_features = _get_features,
                .destroy = _destroy,
            },
        },
        .sock_fd = -1,
        .running = TRUE,
        .listener = {
            .child_updown = _ha_child_updown,
        },
    );
    
    g_ha_plugin = this;
    
    this->peer_cfgs_mutex = mutex_create(MUTEX_TYPE_DEFAULT);
    this->ha_hash_mutex = mutex_create(MUTEX_TYPE_DEFAULT);
    this->managed_peer_cfgs = linked_list_create();
    
    for (int i = 0; i < HA_HASH_SIZE; i++) {
        this->ha_hash[i] = NULL;
    }
    
    this->creds = mem_cred_create();
    charon->credentials->add_set(charon->credentials, &this->creds->set);
    
    charon->bus->add_listener(charon->bus, &this->listener);
    
    // 소켓 스레드 시작
    this->thread = thread_create((thread_main_t)socket_thread, this);

    DBG1(DBG_CFG, "ExternalSocket HA plugin initialized successfully");
    return &this->public.plugin;
} 