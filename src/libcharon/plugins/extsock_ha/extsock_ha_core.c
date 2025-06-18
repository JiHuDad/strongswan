/*
 * Copyright (C) 2024 strongSwan Project
 *
 * External Socket HA Plugin - Core functionality
 * Includes all original extsock features plus HA failover
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

/**
 * JSON 배열을 쉼표로 구분된 문자열로 변환 (기존 extsock 기능)
 */
char* json_array_to_comma_separated_string(cJSON *json_array) 
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
 * JSON 배열로부터 proposal 목록 파싱 (기존 extsock 기능)
 */
linked_list_t* parse_proposals_from_json_array(cJSON *json_array, protocol_id_t proto, bool is_ike) 
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
 * 문자열을 action_t로 변환 (기존 extsock 기능)
 */
action_t string_to_action(const char* action_str)
{
    if (!action_str) return ACTION_NONE;
    
    if (streq(action_str, "none")) return ACTION_NONE;
    if (streq(action_str, "route")) return ACTION_ROUTE;
    if (streq(action_str, "start")) return ACTION_START;
    if (streq(action_str, "restart")) return ACTION_RESTART;
    
    return ACTION_NONE;
}

/**
 * JSON 배열로부터 traffic selector 목록 파싱 (기존 extsock 기능)
 */
linked_list_t* parse_ts_from_json_array(cJSON *json_array)
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
 * JSON으로부터 IKE config 파싱 (기존 extsock 기능)
 */
ike_cfg_t* parse_ike_cfg_from_json(cJSON *ike_json)
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
 * JSON으로부터 auth config 파싱 (기존 extsock 기능)
 */
auth_cfg_t* parse_auth_cfg_from_json(cJSON *auth_json, bool is_local)
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

    cJSON *cert_json = cJSON_GetObjectItem(auth_json, "cert");
    if (cert_json && cJSON_IsString(cert_json)) {
        // Certificate handling would go here
    }

    return auth_cfg;
}

/**
 * Traffic selector를 문자열로 변환 (기존 extsock 기능)
 */
void ts_to_string(traffic_selector_t *ts, char *buf, size_t buflen)
{
    if (!ts || !buf || buflen == 0) {
        return;
    }

    host_t *from = ts->get_from_address(ts);
    host_t *to = ts->get_to_address(ts);
    uint16_t from_port = ts->get_from_port(ts);
    uint16_t to_port = ts->get_to_port(ts);
    uint8_t protocol = ts->get_protocol(ts);

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
 * DPD 시작 (기존 extsock 기능)
 */
void start_dpd(const char *ike_sa_name)
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
 * JSON으로부터 child config 파싱 및 peer config에 추가 (기존 extsock 기능)
 */
bool add_children_from_json(peer_cfg_t *peer_cfg, cJSON *children_json_array)
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

        cJSON *dpd_action_json = cJSON_GetObjectItem(child_json, "dpd_action");
        if (dpd_action_json && cJSON_IsString(dpd_action_json)) {
            child_cfg_data.dpd_action = string_to_action(dpd_action_json->valuestring);
        }

        cJSON *close_action_json = cJSON_GetObjectItem(child_json, "close_action");
        if (close_action_json && cJSON_IsString(close_action_json)) {
            child_cfg_data.close_action = string_to_action(close_action_json->valuestring);
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