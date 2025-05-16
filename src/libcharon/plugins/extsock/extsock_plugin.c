// 플러그인 자체 헤더 파일을 포함합니다. 플러그인의 공개 인터페이스를 정의합니다.
#include "extsock_plugin.h"
// cJSON 라이브러리 헤더 파일을 포함합니다. JSON 파싱 및 생성을 위해 사용됩니다.
#include <cJSON.h>

// strongSwan 데몬 및 라이브러리 관련 핵심 헤더 파일들을 포함합니다.
#include <daemon.h>      // 데몬 관련 기능 (charon 전역 객체 등)
#include <library.h>     // strongSwan 라이브러리 기본 기능
#include <threading/thread.h> // 스레드 생성 및 관리 기능
#include <sys/un.h>      // 유닉스 도메인 소켓 주소 구조체 (struct sockaddr_un)
#include <sys/socket.h>  // 소켓 관련 함수 (socket, connect, bind, listen, accept 등)
#include <unistd.h>      // 유닉스 표준 함수 (read, write, close, unlink 등)
#include <stdio.h>       // 표준 입출력 함수 (snprintf 등)
#include <string.h>      // 문자열 처리 함수 (strlen, strncmp, strncpy, memset 등)
#include <errno.h>       // errno 변수와 에러 코드 정의
#include <utils/debug.h> // For LEVEL_INFO and other debug levels

// strongSwan 내부의 IPsec 설정, SA 관리, Task, Kernel 이벤트 관련 헤더 파일들을 포함합니다.
#include <config/ike_cfg.h>       // IKE 설정 (ike_cfg_t) 관련
#include <config/child_cfg.h>     // CHILD_SA 설정 (child_cfg_t) 관련
#include <sa/ike_sa_manager.h>    // IKE_SA 관리자 관련
#include <sa/ike_sa.h>            // IKE_SA 객체 관련
#include <sa/ikev2/tasks/ike_dpd.h>         // DPD(Dead Peer Detection) Task 관련
#include <kernel/kernel_listener.h> // Kernel 이벤트 리스너 (SAD/SPD 변경 감지) 관련
#include <networking/host.h>           // 주소(host_t)를 문자열로 변환하는 유틸리티 관련
#include <credentials/sets/mem_cred.h> // For mem_cred_t
#include <credentials/credential_manager.h> // For lib->credmgr
#include <control/controller.h> // For charon->controller->initiate
#include <bus/listeners/listener.h>
#include <sa/child_sa.h>
#include <config/peer_cfg.h>

// 추가 헤더 파일들
#include <collections/linked_list.h>
#include <sa/proposal.h>
#include <config/traffic_selector.h>
#include <credentials/auth_cfg.h>
#include <credentials/credential.h> // For secret_t, credential_t
#include <credentials/secret.h>     // For secret_create_psk
#include <identification.h>       // For identification_create_from_string
#include <utils/conversion.h>     // For chunk_from_str etc.
#include <string/utils.h>         // For streq

// 유닉스 도메인 소켓 파일의 경로를 정의합니다. 외부 프로그램과 통신 채널로 사용됩니다.
#define SOCKET_PATH "/tmp/strongswan_extsock.sock"

// 플러그인의 내부 상태를 나타내는 private 구조체의 전방 선언입니다.
typedef struct private_extsock_plugin_t private_extsock_plugin_t;

// Forward declaration for the destroy function
static void extsock_plugin_destroy(private_extsock_plugin_t *this);

// 플러그인의 내부 상태를 저장하는 구조체입니다.
struct private_extsock_plugin_t {
    plugin_t public;                // strongSwan 플러그인 시스템에 등록될 공개 인터페이스 구조체입니다.
    mem_cred_t *creds;              // Memory credential set for this plugin
    int sock_fd;                    // 외부 프로그램과의 통신을 위한 유닉스 도메인 소켓의 파일 디스크립터입니다.
    thread_t *thread;               // 외부 프로그램으로부터 명령을 수신하는 작업을 처리할 스레드 객체입니다.
    bool running;                   // 소켓 수신 스레드가 현재 실행 중인지 여부를 나타내는 플래그입니다.
};

// Function declarations
static void start_dpd(const char *ike_sa_name);
static void apply_ipsec_config(private_extsock_plugin_t *this, const char *config_json);

// --- Helper function declarations for parsing JSON ---
static ike_cfg_t* parse_ike_cfg_from_json(private_extsock_plugin_t *plugin, cJSON *ike_json);
static auth_cfg_t* parse_auth_cfg_from_json(private_extsock_plugin_t *plugin, cJSON *auth_json, bool is_local);
static bool add_children_from_json(private_extsock_plugin_t *plugin, peer_cfg_t *peer_cfg, cJSON *children_json_array);
static linked_list_t* parse_proposals_from_json_array(cJSON *json_array, protocol_id_t proto, bool is_ike);
static linked_list_t* parse_ts_from_json_array(cJSON *json_array);
static char* json_array_to_comma_separated_string(cJSON *json_array);
static action_t string_to_action(const char* action_str);
// --- End helper function declarations ---

// Function definitions
static void start_dpd(const char *ike_sa_name)
{
    ike_sa_t *ike_sa = charon->ike_sa_manager->checkout_by_name(
        charon->ike_sa_manager, (char*)ike_sa_name, ID_MATCH_PERFECT);
    if (!ike_sa)
    {
        DBG1(DBG_LIB, "start_dpd: IKE_SA '%s' not found", ike_sa_name);
        return;
    }
    DBG1(DBG_LIB, "start_dpd: Starting DPD for IKE_SA '%s'", ike_sa_name);
    ike_dpd_t *dpd = ike_dpd_create(TRUE);
    ike_sa->queue_task(ike_sa, (task_t*)dpd);
}

// Helper to convert a JSON array of strings to a single comma-separated string
static char* json_array_to_comma_separated_string(cJSON *json_array) {
    if (!json_array || !cJSON_IsArray(json_array) || cJSON_GetArraySize(json_array) == 0) {
        return strdup("%any"); // Default if not specified or empty
    }
    string_t *str = string_create(NULL);
    cJSON *item;
    bool first = TRUE;
    cJSON_ArrayForEach(item, json_array) {
        if (cJSON_IsString(item) && item->valuestring) {
            if (!first) {
                string_append_char(str, ',');
            }
            string_append_chars(str, item->valuestring, strlen(item->valuestring));
            first = FALSE;
        }
    }
    if (string_length(str) == 0) {
        string_destroy(str);
        return strdup("%any");
    }
    return string_detach(str);
}

// Helper to parse proposal strings from JSON array
static linked_list_t* parse_proposals_from_json_array(cJSON *json_array, protocol_id_t proto, bool is_ike) {
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
                    DBG1(DBG_LIB, "Failed to parse proposal string: %s for proto %d", prop_json->valuestring, proto);
                }
            }
        }
    }

    // Add default proposals if none were parsed successfully
    if (proposals_list->get_count(proposals_list) == 0) {
        DBG1(DBG_LIB, "No proposals in JSON, adding defaults for proto %d (is_ike: %d)", proto, is_ike);
        if (is_ike) { // PROTO_IKE
            proposal_t *first = proposal_create_default(PROTO_IKE);
            if (first) proposals_list->insert_last(proposals_list, first);
            // AEAD is preferred for IKE by default in newer setups, but vici adds both
            proposal_t *second = proposal_create_default_aead(PROTO_IKE);
            if (second) proposals_list->insert_last(proposals_list, second);
        } else { // PROTO_ESP or PROTO_AH
            proposal_t *first = proposal_create_default_aead(proto); // e.g. PROTO_ESP
            if (first) proposals_list->insert_last(proposals_list, first);
            proposal_t *second = proposal_create_default(proto);
            if (second) proposals_list->insert_last(proposals_list, second);
        }
    }
    return proposals_list;
}

// Helper to parse traffic selector strings from JSON array
static linked_list_t* parse_ts_from_json_array(cJSON *json_array) {
    linked_list_t *ts_list = linked_list_create();
    if (!ts_list) return NULL;

    if (json_array && cJSON_IsArray(json_array)) {
        cJSON *ts_json;
        cJSON_ArrayForEach(ts_json, json_array) {
            if (cJSON_IsString(ts_json) && ts_json->valuestring) {
                // traffic_selector_create_from_string is deprecated.
                // Use traffic_selector_create_from_cidr or traffic_selector_create_from_range
                // For simplicity, we'll assume CIDR format for now.
                traffic_selector_t *ts = traffic_selector_create_from_cidr(ts_json->valuestring, 0, 0, 0xFFFF);
                if (ts) {
                    ts_list->insert_last(ts_list, ts);
                } else {
                    DBG1(DBG_LIB, "Failed to parse TS string as CIDR: %s", ts_json->valuestring);
                }
            }
        }
    }
    
    if (ts_list->get_count(ts_list) == 0) { // Default to dynamic if none provided or parsing failed
        traffic_selector_t* ts = traffic_selector_create_dynamic(0, 0, 0xFFFF);
        if (ts) ts_list->insert_last(ts_list, ts);
        DBG1(DBG_LIB, "No traffic selectors in JSON or all failed to parse, adding dynamic TS");
    }
    return ts_list;
}

static ike_cfg_t* parse_ike_cfg_from_json(private_extsock_plugin_t *plugin, cJSON *ike_json) {
    if (!ike_json) return NULL;

    ike_cfg_create_t ike_create_cfg = {0}; // Initialize with zeros

    cJSON *j_local_addrs = cJSON_GetObjectItem(ike_json, "local_addrs");
    ike_create_cfg.local = json_array_to_comma_separated_string(j_local_addrs);

    cJSON *j_remote_addrs = cJSON_GetObjectItem(ike_json, "remote_addrs");
    ike_create_cfg.remote = json_array_to_comma_separated_string(j_remote_addrs);
    
    cJSON *j_version = cJSON_GetObjectItem(ike_json, "version");
    if (j_version && cJSON_IsNumber(j_version)) {
        ike_create_cfg.version = j_version->valueint;
    } else {
        ike_create_cfg.version = IKE_ANY; // Default
    }
    // Default ports (charon will use standard 500/4500 or configured ones)
    ike_create_cfg.local_port = 0; 
    ike_create_cfg.remote_port = 0;


    ike_cfg_t *ike_cfg = ike_cfg_create(&ike_create_cfg);
    // Strings from json_array_to_comma_separated_string were allocated with strdup/string_detach
    // ike_cfg_create copies them, so we need to free our copies.
    free(ike_create_cfg.local);
    free(ike_create_cfg.remote);

    if (!ike_cfg) {
        DBG1(DBG_LIB, "Failed to create ike_cfg");
        return NULL;
    }

    cJSON *j_proposals = cJSON_GetObjectItem(ike_json, "proposals");
    linked_list_t *ike_proposals = parse_proposals_from_json_array(j_proposals, PROTO_IKE, TRUE);
    if (ike_proposals) {
        proposal_t *prop;
        while (ike_proposals->remove_first(ike_proposals, (void**)&prop) == SUCCESS) {
            ike_cfg->add_proposal(ike_cfg, prop);
        }
        ike_proposals->destroy(ike_proposals);
    }
    return ike_cfg;
}

static auth_cfg_t* parse_auth_cfg_from_json(private_extsock_plugin_t *plugin, cJSON *auth_json, bool is_local) {
    if (!auth_json) return NULL;

    auth_cfg_t *auth_cfg = auth_cfg_create();
    if (!auth_cfg) {
        DBG1(DBG_LIB, "Failed to create auth_cfg");
        return NULL;
    }

    cJSON *j_auth_type = cJSON_GetObjectItem(auth_json, "auth");
    cJSON *j_id = cJSON_GetObjectItem(auth_json, "id");
    cJSON *j_secret = cJSON_GetObjectItem(auth_json, "secret");

    if (j_auth_type && cJSON_IsString(j_auth_type)) {
        const char *auth_type_str = j_auth_type->valuestring;
        if (streq(auth_type_str, "psk")) {
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK);
            if (j_secret && cJSON_IsString(j_secret)) {
                const char *secret_str = j_secret->valuestring;
                identification_t *psk_identity = NULL;
                // Associate PSK with the provided ID, or %any if no ID for this auth block
                if (j_id && cJSON_IsString(j_id)) {
                     psk_identity = identification_create_from_string(j_id->valuestring);
                     // Also add this ID to the auth_cfg for matching
                     auth_cfg->add(auth_cfg, AUTH_RULE_IDENTITY, identification_create_from_string(j_id->valuestring));
                } else {
                    psk_identity = identification_create_any();
                }

                if (psk_identity) {
                    chunk_t secret_chunk = chunk_from_str((char*)secret_str); // a cast is needed for char*
                    secret_t *secret_obj = secret_create_psk(secret_chunk, psk_identity, NULL); // peer_id is NULL
                    if (secret_obj) {
                        if (!plugin->creds->add_secret(plugin->creds, secret_obj)) {
                            DBG1(DBG_LIB, "Failed to add PSK to plugin's mem_cred for ID: %s", j_id ? j_id->valuestring : "%any");
                        }
                        secret_obj->destroy(secret_obj); // add_secret clones it
                    }
                    psk_identity->destroy(psk_identity);
                }
            } else {
                DBG1(DBG_LIB, "PSK auth specified but 'secret' missing or not a string");
            }
        } else if (streq(auth_type_str, "pubkey")) {
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PUBKEY);
            if (j_id && cJSON_IsString(j_id)) {
                identification_t *pubkey_id = identification_create_from_string(j_id->valuestring);
                if (pubkey_id) {
                    auth_cfg->add(auth_cfg, AUTH_RULE_IDENTITY, pubkey_id);
                } else {
                     DBG1(DBG_LIB, "Failed to create identification for pubkey ID: %s", j_id->valuestring);
                }
            } else if (is_local) {
                 DBG1(DBG_LIB, "Pubkey auth for local peer specified but 'id' missing or not a string. Certificate subject might be used if cert is loaded.");
            }
            // Certificates are assumed to be pre-loaded via other means or by path (not implemented here)
            // and associated with the ID.
        } else {
            DBG1(DBG_LIB, "Unsupported auth type: %s", auth_type_str);
            auth_cfg->destroy(auth_cfg);
            return NULL;
        }
    } else {
        // Default to pubkey if no auth type, or some other sensible default.
        // For now, if no specific auth is given, it might rely on defaults or fail later.
        // Let's assume if section exists, type must be specified.
        DBG1(DBG_LIB, "'auth' type missing in auth config");
        auth_cfg->destroy(auth_cfg);
        return NULL;
    }

    return auth_cfg;
}

static action_t string_to_action(const char* action_str) {
    if (!action_str) return ACTION_NONE;
    if (streq(action_str, "trap")) return ACTION_TRAP;
    if (streq(action_str, "start")) return ACTION_START;
    if (streq(action_str, "clear")) return ACTION_CLEAR; // DPD
    if (streq(action_str, "hold")) return ACTION_HOLD;   // DPD
    if (streq(action_str, "restart")) return ACTION_RESTART; // DPD
    return ACTION_NONE;
}

static bool add_children_from_json(private_extsock_plugin_t *plugin, peer_cfg_t *peer_cfg, cJSON *children_json_array) {
    if (!children_json_array || !cJSON_IsArray(children_json_array)) {
        // No children array, or not an array. This is acceptable.
        return TRUE;
    }

    cJSON *child_json;
    cJSON_ArrayForEach(child_json, children_json_array) {
        if (!cJSON_IsObject(child_json)) continue;

        cJSON *j_name = cJSON_GetObjectItem(child_json, "name");
        if (!j_name || !cJSON_IsString(j_name) || !j_name->valuestring) {
            DBG1(DBG_LIB, "Child config missing 'name'");
            continue;
        }
        const char *child_name_str = j_name->valuestring;

        child_cfg_create_t child_create_cfg = {0}; // Initialize

        // Parse other child config options (mode, rekey_time, etc. defaults are fine for now)
        // Example: child_create_cfg.mode = MODE_TUNNEL; (default)
        
        cJSON *j_start_action = cJSON_GetObjectItem(child_json, "start_action");
        if (j_start_action && cJSON_IsString(j_start_action)) {
            child_create_cfg.start_action = string_to_action(j_start_action->valuestring);
        } else {
            child_create_cfg.start_action = ACTION_NONE; // Default
        }
        
        cJSON *j_dpd_action = cJSON_GetObjectItem(child_json, "dpd_action");
        if (j_dpd_action && cJSON_IsString(j_dpd_action)) {
            child_create_cfg.dpd_action = string_to_action(j_dpd_action->valuestring);
        } else {
            child_create_cfg.dpd_action = ACTION_NONE; // Default for DPD
        }


        child_cfg_t *child_cfg = child_cfg_create((char*)child_name_str, &child_create_cfg);
        if (!child_cfg) {
            DBG1(DBG_LIB, "Failed to create child_cfg: %s", child_name_str);
            continue;
        }

        cJSON *j_local_ts = cJSON_GetObjectItem(child_json, "local_ts");
        linked_list_t *local_ts_list = parse_ts_from_json_array(j_local_ts);
        if (local_ts_list) {
            traffic_selector_t *ts;
            while (local_ts_list->remove_first(local_ts_list, (void**)&ts) == SUCCESS) {
                child_cfg->add_traffic_selector(child_cfg, TRUE, ts);
            }
            local_ts_list->destroy(local_ts_list);
        }

        cJSON *j_remote_ts = cJSON_GetObjectItem(child_json, "remote_ts");
        linked_list_t *remote_ts_list = parse_ts_from_json_array(j_remote_ts);
        if (remote_ts_list) {
            traffic_selector_t *ts;
            while (remote_ts_list->remove_first(remote_ts_list, (void**)&ts) == SUCCESS) {
                child_cfg->add_traffic_selector(child_cfg, FALSE, ts);
            }
            remote_ts_list->destroy(remote_ts_list);
        }

        cJSON *j_esp_proposals = cJSON_GetObjectItem(child_json, "esp_proposals");
        linked_list_t *esp_proposals_list = parse_proposals_from_json_array(j_esp_proposals, PROTO_ESP, FALSE);
        if (esp_proposals_list) {
            proposal_t *prop;
            while (esp_proposals_list->remove_first(esp_proposals_list, (void**)&prop) == SUCCESS) {
                child_cfg->add_proposal(child_cfg, prop);
            }
            esp_proposals_list->destroy(esp_proposals_list);
        }
        
        peer_cfg->add_child_cfg(peer_cfg, child_cfg);
        DBG2(DBG_LIB, "Added child_cfg: %s to peer_cfg: %s", child_name_str, peer_cfg->get_name(peer_cfg));
    }
    return TRUE;
}

static void apply_ipsec_config(private_extsock_plugin_t *this, const char *config_json)
{
    DBG1(DBG_LIB, "apply_ipsec_config: received config: %s", config_json);
    cJSON *root = cJSON_Parse(config_json);
    if (!root)
    {
        DBG1(DBG_LIB, "apply_ipsec_config: Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return;
    }

    cJSON *j_conn_name = cJSON_GetObjectItem(root, "name");
    if (!j_conn_name || !cJSON_IsString(j_conn_name) || !j_conn_name->valuestring) {
        DBG1(DBG_LIB, "apply_ipsec_config: Missing connection 'name' in JSON");
        cJSON_Delete(root);
        return;
    }
    const char *conn_name_str = j_conn_name->valuestring;

    // 1. Parse IKE Config
    cJSON *j_ike_cfg = cJSON_GetObjectItem(root, "ike_cfg");
    ike_cfg_t *ike_cfg = parse_ike_cfg_from_json(this, j_ike_cfg);
    if (!ike_cfg) {
        DBG1(DBG_LIB, "apply_ipsec_config: Failed to parse ike_cfg section for %s", conn_name_str);
        cJSON_Delete(root);
        return;
    }

    // 2. Create Peer Config Shell
    peer_cfg_create_t peer_create_cfg = {0}; // Initialize with defaults
    // TODO: Populate peer_create_cfg from JSON if needed (e.g., DPD, rekey times etc.)
    // Example: peer_create_cfg.unique = UNIQUE_NO;
    //          peer_create_cfg.dpd = 30; // DPD delay
    
    peer_cfg_t *peer_cfg = peer_cfg_create((char*)conn_name_str, ike_cfg, &peer_create_cfg);
    if (!peer_cfg) {
        DBG1(DBG_LIB, "apply_ipsec_config: Failed to create peer_cfg for %s", conn_name_str);
        ike_cfg->destroy(ike_cfg); // ike_cfg was not adopted by peer_cfg
        cJSON_Delete(root);
        return;
    }
    // ike_cfg is now owned by peer_cfg

    // 3. Parse and Add Local Auth Config
    cJSON *j_local_auth = cJSON_GetObjectItem(root, "local_auth");
    if (j_local_auth) {
        auth_cfg_t *local_auth_cfg = parse_auth_cfg_from_json(this, j_local_auth, TRUE);
        if (local_auth_cfg) {
            peer_cfg->add_auth_cfg(peer_cfg, local_auth_cfg, TRUE); // TRUE for local
        } else {
            DBG1(DBG_LIB, "apply_ipsec_config: Failed to parse local_auth for %s", conn_name_str);
            // Continue without this auth, or error out? For now, continue.
        }
    } else { // Add a default local auth if not specified (e.g. pubkey any)
        auth_cfg_t *default_local_auth = auth_cfg_create();
        default_local_auth->add(default_local_auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_ANY);
        peer_cfg->add_auth_cfg(peer_cfg, default_local_auth, TRUE);
    }


    // 4. Parse and Add Remote Auth Config
    cJSON *j_remote_auth = cJSON_GetObjectItem(root, "remote_auth");
    if (j_remote_auth) {
        auth_cfg_t *remote_auth_cfg = parse_auth_cfg_from_json(this, j_remote_auth, FALSE);
        if (remote_auth_cfg) {
            peer_cfg->add_auth_cfg(peer_cfg, remote_auth_cfg, FALSE); // FALSE for remote
        } else {
            DBG1(DBG_LIB, "apply_ipsec_config: Failed to parse remote_auth for %s", conn_name_str);
        }
    } else { // Add a default remote auth if not specified
        auth_cfg_t *default_remote_auth = auth_cfg_create();
        default_remote_auth->add(default_remote_auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_ANY);
        peer_cfg->add_auth_cfg(peer_cfg, default_remote_auth, FALSE);
    }

    // 5. Parse and Add Children Configs
    cJSON *j_children = cJSON_GetObjectItem(root, "children");
    if (!add_children_from_json(this, peer_cfg, j_children)) {
        DBG1(DBG_LIB, "apply_ipsec_config: Error processing children for %s", conn_name_str);
        // Potentially cleanup and return
    }

    DBG1(DBG_LIB, "Successfully created peer_cfg '%s' from JSON.", peer_cfg->get_name(peer_cfg));
    // TODO: Implement robust "applying" of the peer_cfg.
    // This could involve:
    // 1. Storing it in this plugin's managed list.
    // 2. Making this plugin a `backend_t` provider.
    // 3. Interfacing with another plugin like `vici` to load it (complex).
    // For now, we will just destroy it to avoid memory leaks.
    // If you need to initiate it, you would do something like:
    /*
    enumerator_t *child_enum = peer_cfg->create_child_cfg_enumerator(peer_cfg);
    child_cfg_t *child_to_start = NULL, *current_child;
    if (child_enum->enumerate(child_enum, &current_child)) { // get first child
        if (current_child->get_start_action(current_child) == ACTION_START) {
             child_to_start = current_child;
        }
    }
    child_enum->destroy(child_enum);
    if (child_to_start) {
        DBG1(DBG_LIB, "Initiating CHILD_SA '%s' for peer '%s'", child_to_start->get_name(child_to_start), peer_cfg->get_name(peer_cfg));
        charon->controller->initiate(charon->controller, peer_cfg->get_ref(peer_cfg), child_to_start->get_ref(child_to_start), NULL, NULL, 0, 0, FALSE);
    }
    */
    // Since we are not storing/applying it permanently for now:
    peer_cfg->destroy(peer_cfg);
    DBG1(DBG_LIB, "peer_cfg '%s' was parsed and then destroyed (pending robust apply mechanism).", conn_name_str);

    cJSON_Delete(root);
}

//-------------------- 소켓을 통해 수신된 외부 명령 처리 함수 --------------------
static void handle_external_command(private_extsock_plugin_t *this, char *cmd)
{
    // 수신된 명령 문자열이 "START_DPD "로 시작하는지 비교합니다.
    if (strncmp(cmd, "START_DPD ", 10) == 0) {
        // "START_DPD " 다음의 문자열(IKE_SA 이름)을 start_dpd 함수에 전달합니다.
        start_dpd(cmd + 10);
    }
    // 수신된 명령 문자열이 "APPLY_CONFIG "로 시작하는지 비교합니다.
    else if (strncmp(cmd, "APPLY_CONFIG ", 13) == 0) {
        // "APPLY_CONFIG " 다음의 문자열(JSON 설정)을 apply_ipsec_config 함수에 전달합니다.
        apply_ipsec_config(this, cmd + 13);
    }
    // 여기에 다른 종류의 외부 명령을 처리하는 로직을 추가할 수 있습니다.
}

//-------------------- 플러그인 소켓 스레드 함수 --------------------

/**
 * 외부 프로그램으로부터 유닉스 도메인 소켓을 통해 명령을 수신하는 스레드의 메인 함수입니다.
 * @param this 플러그인의 내부 상태를 저장하는 private_extsock_plugin_t 구조체에 대한 포인터입니다.
 * @return 스레드 종료 시 NULL을 반환합니다.
 */
static void* socket_thread(private_extsock_plugin_t *this)
{
    // 유닉스 도메인 소켓 주소 정보를 담을 구조체입니다.
    struct sockaddr_un addr;
    // 클라이언트로부터 수신한 데이터를 저장할 버퍼입니다.
    char buf[1024]; // 충분한 크기로 설정합니다.

    // 유닉스 도메인 소켓을 생성합니다. (AF_UNIX: 로컬 통신, SOCK_STREAM: 스트림 방식)
    this->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    // 소켓 생성 실패 시 에러 로그를 남기고 스레드를 종료합니다.
    if (this->sock_fd < 0) {
        DBG1(DBG_LIB, "Failed to create Unix domain socket");
        return NULL;
    }

    // 이전에 같은 경로로 소켓 파일이 존재할 경우를 대비하여 삭제합니다. (bind 오류 방지)
    unlink(SOCKET_PATH);

    // 소켓 주소 구조체를 0으로 초기화합니다.
    memset(&addr, 0, sizeof(addr));
    // 주소 패밀리를 유닉스 도메인 소켓으로 설정합니다.
    addr.sun_family = AF_UNIX;
    // 소켓 파일 경로를 주소 구조체에 복사합니다.
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    // 생성된 소켓에 주소를 할당(bind)합니다.
    // bind 실패 시 에러 로그를 남기고, 소켓을 닫은 후 스레드를 종료합니다.
    if (bind(this->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        DBG1(DBG_LIB, "Failed to bind Unix domain socket to %s", SOCKET_PATH);
        close(this->sock_fd); // 소켓을 닫습니다.
        this->sock_fd = -1;   // sock_fd를 유효하지 않은 값으로 설정합니다.
        return NULL;
    }

    // 소켓을 연결 요청 대기 상태로 만듭니다(listen). 동시에 최대 5개의 연결 요청을 큐에 저장할 수 있습니다.
    if (listen(this->sock_fd, 5) == -1) {
        DBG1(DBG_LIB, "Failed to listen on Unix domain socket");
        close(this->sock_fd);
        this->sock_fd = -1;
        unlink(SOCKET_PATH); // bind 실패 시 소켓 파일도 삭제
        return NULL;
    }

    DBG1(DBG_LIB, "Unix domain socket listening on %s", SOCKET_PATH);

    // 스레드가 실행 중임을 나타내는 플래그를 설정합니다.
    this->running = TRUE;
    // this->running 플래그가 TRUE인 동안 루프를 계속 실행합니다. (플러그인 종료 시 FALSE로 설정됨)
    while (this->running) {
        // 클라이언트의 연결 요청을 수락(accept)합니다. 연결될 때까지 대기(blocking)합니다.
        // accept는 새로운 소켓 디스크립터(client_fd)를 반환하여 클라이언트와 통신합니다.
        int client_fd = accept(this->sock_fd, NULL, NULL);
        // accept 실패 시 (예: 소켓이 닫힌 경우) 루프의 다음 반복으로 넘어갑니다.
        if (client_fd < 0) {
            // running 플래그가 FALSE가 되어 루프를 탈출해야 하는지 확인합니다.
            if (!this->running) break;
            DBG1(DBG_LIB, "Accept failed on Unix domain socket");
            continue;
        }

        // 클라이언트로부터 데이터를 읽습니다(read). 최대 buf 크기 - 1 만큼 읽습니다 (NULL 종단 문자 공간 확보).
        ssize_t len = read(client_fd, buf, sizeof(buf)-1);
        // 읽은 데이터가 있으면
        if (len > 0) {
            // 수신한 데이터를 NULL로 종단하여 문자열로 만듭니다.
            buf[len] = '\0';
            // 수신된 명령 로그 출력 (디버깅용)
            DBG2(DBG_LIB, "Received command from external program: %s", buf);
            // 수신된 명령을 처리하는 함수를 호출합니다.
            handle_external_command(this, buf);
        } else if (len == 0) {
            // 클라이언트가 연결을 닫은 경우 (EOF)
             DBG2(DBG_LIB, "Client disconnected");
        } else {
            // 읽기 오류 발생
            DBG1(DBG_LIB, "Read error from client socket");
        }
        // 클라이언트와의 통신을 위한 소켓을 닫습니다.
        close(client_fd);
    }
    // 루프가 종료되면 (this->running이 FALSE가 되면) 스레드를 반환합니다.
    return NULL;
}

//-------------------- 플러그인 생성 및 파괴 함수 --------------------

// Plugin metadata functions
static char* extsock_plugin_get_name(plugin_t* plugin) {
    (void)plugin; // Unused
    return "extsock";
}

static int extsock_plugin_get_features(plugin_t* plugin, plugin_feature_t *features[]) {
    (void)plugin; // Unused
    *features = NULL;
    return 0;
}

// child_updown listener for tunnel up/down notification
static bool extsock_child_updown(listener_t *this, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up)
{
    char buf[1024];
    uint32_t spi = child_sa->get_spi(child_sa, TRUE); // inbound SPI
    uint8_t proto = child_sa->get_protocol(child_sa);
    const char *proto_str = (proto == IPPROTO_ESP) ? "esp" : (proto == IPPROTO_AH) ? "ah" : "unknown";
    traffic_selector_t *local_ts, *remote_ts;
    enumerator_t *ts_enum = child_sa->create_policy_enumerator(child_sa);
    if (ts_enum && ts_enum->enumerate(ts_enum, &local_ts, &remote_ts)) {
        char local_buf[128], remote_buf[128];
        ts_to_string(local_ts, local_buf, sizeof(local_buf));
        ts_to_string(remote_ts, remote_buf, sizeof(remote_buf));
        snprintf(buf, sizeof(buf),
            "{\"event\":\"tunnel_%s\",\"spi\":%u,\"proto\":\"%s\",\"local_ts\":\"%s\",\"remote_ts\":\"%s\"}",
            up ? "up" : "down", spi, proto_str, local_buf, remote_buf);
        send_event_to_external(buf);
    }
    if (ts_enum) ts_enum->destroy(ts_enum);
    return TRUE;
}

// Listener instance
static listener_t extsock_listener = {
    .child_updown = extsock_child_updown,
};

/**
 * 플러그인이 strongSwan에 로드될 때 호출되는 생성자 함수입니다.
 * 플러그인의 초기화 작업을 수행합니다.
 * @return 성공적으로 생성된 플러그인 객체(plugin_t*)를 반환합니다. 실패 시 NULL을 반환할 수 있습니다 (여기서는 항상 성공).
 */
plugin_t* extsock_plugin_create()
{
    // 플러그인의 내부 상태를 저장할 private_extsock_plugin_t 구조체의 메모리를 동적으로 할당하고 0으로 초기화합니다.
    private_extsock_plugin_t *this = calloc(1, sizeof(*this));

    // 소켓 파일 디스크립터를 초기값(-1)으로 설정합니다.
    this->sock_fd = -1;

    // 플러그인의 공개 인터페이스(plugin_t)에 소멸자 함수(extsock_plugin_destroy)를 등록합니다.
    this->public.destroy = (void*)extsock_plugin_destroy;
    // 플러그인 이름을 설정합니다 (디버깅 및 로깅에 사용됨).
    this->public.get_name = extsock_plugin_get_name;
    // 플러그인 기능 함수를 설정합니다.
    this->public.get_features = extsock_plugin_get_features;

    // bus에 child_updown 리스너 등록
    charon->bus->add_listener(charon->bus, &extsock_listener);

    this->creds = mem_cred_create();
    if (this->creds) {
        lib->credmgr->add_set(lib->credmgr, &this->creds->set);
    } else {
        DBG1(DBG_LIB, "Failed to create mem_cred_t for extsock plugin");
        // Consider proper error handling if mem_cred_create fails
        free(this); // Free allocated memory for the plugin struct
        return NULL; // Indicate plugin creation failure
    }

    // 외부 명령 수신을 위한 소켓 스레드를 생성하고 시작합니다.
    // thread_create는 스레드 메인 함수(socket_thread)와 그 함수에 전달될 인자(this)를 받습니다.
    this->thread = thread_create((thread_main_t)socket_thread, this);
    // 스레드 생성 실패 시 로그를 남깁니다 (실제로는 오류 처리 및 리소스 해제 필요).
    if (!this->thread) {
        DBG1(DBG_LIB, "Failed to create socket listener thread");
        // 스레드 생성 실패 시, 이미 등록한 커널 리스너를 해제하고, 할당된 메모리도 해제해야 합니다.
        if (this->creds) {
            lib->credmgr->remove_set(lib->credmgr, &this->creds->set);
            this->creds->destroy(this->creds);
            this->creds = NULL;
        }
        free(this);
        return NULL; // 플러그인 생성 실패를 알립니다.
    }

    // 플러그인 로드 성공 로그를 출력합니다.
    DBG1(DBG_LIB, "extsock plugin loaded successfully");

    // 생성된 플러그인 객체의 공개 인터페이스 부분(plugin_t)의 주소를 반환합니다.
    return &this->public;
}

/**
 * 플러그인이 strongSwan에서 언로드될 때 호출되는 소멸자 함수입니다.
 * 플러그인이 사용한 모든 리소스를 해제합니다.
 * @param this 파괴될 플러그인 객체(private_extsock_plugin_t*)입니다. (실제로는 plugin_t*로 전달되지만, 내부적으로 private 구조체임)
 */
void extsock_plugin_destroy(private_extsock_plugin_t *this)
{
    // 스레드가 실행 중임을 나타내는 플래그를 FALSE로 설정하여 socket_thread 루프를 종료하도록 유도합니다.
    this->running = FALSE;

    // 소켓 스레드가 존재하면
    if (this->thread) {
        // 소켓 스레드에 취소 요청을 보냅니다. (accept()와 같은 blocking call에서 벗어나도록 함)
        // 실제 취소 메커니즘은 스레드 구현 및 blocking call의 종류에 따라 다를 수 있습니다.
        // 가장 간단한 방법은 소켓을 닫아 accept()에서 오류를 발생시키는 것입니다.
        if (this->sock_fd >= 0) {
             // shutdown을 사용하여 accept()를 즉시 중단시킬 수 있습니다.
            shutdown(this->sock_fd, SHUT_RDWR);
            close(this->sock_fd); // 그 후 소켓을 닫습니다.
            this->sock_fd = -1;   // sock_fd를 유효하지 않은 값으로 설정합니다.
        }
        // 소켓 스레드가 종료될 때까지 대기합니다.
        this->thread->join(this->thread);
        // 소켓 스레드 객체의 메모리를 해제합니다.
        // thread_destroy(this->thread); // This line is removed as join/detach should handle destruction.
        this->thread = NULL; // 포인터를 NULL로 설정합니다.
    }

    // 유닉스 도메인 소켓 파일이 존재하면 삭제합니다.
    // sock_fd가 여전히 유효한 값(>=0)을 가지고 있다면 위에서 close 되지 않았다는 의미일 수 있으므로,
    // 여기서 한 번 더 확인하고 닫은 후 삭제합니다. (하지만 위에서 이미 처리됨)
    // unlink는 파일 시스템에서 소켓 파일을 제거합니다.
    unlink(SOCKET_PATH);

    // 등록된 SAD/SPD 커널 리스너를 해제합니다.
    charon->bus->remove_listener(charon->bus, &extsock_listener);

    // 플러그인 언로드 성공 로그를 출력합니다.
    DBG1(DBG_LIB, "extsock plugin unloaded successfully");

    // 플러그인 내부 상태 구조체(private_extsock_plugin_t)의 메모리를 해제합니다.
    if (this->creds) {
        lib->credmgr->remove_set(lib->credmgr, &this->creds->set);
        this->creds->destroy(this->creds);
        this->creds = NULL;
    }
    free(this);
}

// 외부 프로그램에 이벤트(JSON 형식의 문자열)를 전송하는 함수입니다.
static void send_event_to_external(const char *event_json)
{
    // 유닉스 도메인 소켓을 생성합니다. (AF_UNIX: 로컬 통신, SOCK_STREAM: TCP와 유사한 스트림 방식)
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    // 소켓 주소 정보를 담을 구조체입니다.
    struct sockaddr_un addr;
    // 주소 구조체를 0으로 초기화합니다.
    memset(&addr, 0, sizeof(addr));
    // 주소 패밀리를 유닉스 도메인 소켓으로 설정합니다.
    addr.sun_family = AF_UNIX;
    // 소켓 파일 경로를 주소 구조체에 복사합니다. 경로 길이 초과를 방지합니다.
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    // 생성된 소켓을 사용하여 외부 프로그램(서버 역할)에 연결을 시도합니다.
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        // 연결에 성공하면, 이벤트 JSON 문자열을 소켓을 통해 전송합니다.
        ssize_t written = write(fd, event_json, strlen(event_json));
        if (written < 0) {
            DBG1(DBG_LIB, "Failed to write to socket: %s", strerror(errno));
        }
    }
    // 소켓 파일 디스크립터를 닫습니다.
    close(fd);
}

// Utility for traffic selector to string
static void ts_to_string(traffic_selector_t *ts, char *buf, size_t buflen)
{
    if (ts && buf && buflen > 0)
    {
        snprintf(buf, buflen, "%R", ts);
    }
    else if (buf && buflen > 0)
    {
        buf[0] = '\0';
    }
} 