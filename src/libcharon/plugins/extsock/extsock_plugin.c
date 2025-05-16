// 플러그인 자체 헤더 파일입니다.
#include "extsock_plugin.h"
// JSON 처리를 위한 cJSON 라이브러리입니다.
#include <cJSON.h>

// strongSwan 핵심 헤더 파일들입니다.
#include <daemon.h>      // 데몬 관련 (charon 객체)
#include <library.h>     // 라이브러리 기본 기능
#include <threading/thread.h> // 스레드 관련
#include <threading/mutex.h> // 뮤텍스 관련
#include <sys/un.h>      // 유닉스 도메인 소켓 주소
#include <sys/socket.h>  // 소켓 함수
#include <unistd.h>      // 유닉스 표준 함수
#include <stdio.h>       // 표준 입출력
#include <string.h>      // 문자열 처리
#include <errno.h>       // 에러 번호
#include <utils/debug.h> // 디버깅 유틸리티

// IPsec 설정 및 관리 관련 헤더 파일들입니다.
#include <config/ike_cfg.h>       // IKE 설정
#include <config/peer_cfg.h>      // 피어 설정
#include <config/child_cfg.h>     // CHILD_SA 설정
#include <settings/settings.h>    // 설정 접근
#include <sa/ike_sa_manager.h>    // IKE_SA 관리자
#include <sa/ike_sa.h>            // IKE_SA 객체
#include <sa/ikev2/tasks/ike_dpd.h> // DPD Task
#include <kernel/kernel_listener.h> // 커널 이벤트 리스너
#include <networking/host.h>      // 호스트 주소 변환
#include <credentials/sets/mem_cred.h> // 인메모리 자격증명
#include <credentials/credential_manager.h> // 자격증명 관리자
#include <control/controller.h>   // SA 제어
#include <bus/listeners/listener.h> // 이벤트 버스 리스너
#include <sa/child_sa.h>          // CHILD_SA 객체
#include <selectors/traffic_selector.h> // 트래픽 셀렉터
#include <credentials/auth_cfg.h> // 인증 설정
#include <credentials/keys/shared_key.h> // 공유키
#include <utils/identification.h>    // ID 객체
#include <utils/chunk.h>             // 데이터 청크
#include <utils/utils/string.h>      // 문자열 유틸리티
#include <collections/linked_list.h> // 연결 리스트

// 유닉스 도메인 소켓 파일 경로입니다.
#define SOCKET_PATH "/tmp/strongswan_extsock.sock"

// 플러그인 내부 데이터 구조체 전방 선언입니다.
typedef struct private_extsock_plugin_t private_extsock_plugin_t;

// 정적 함수 전방 선언입니다.
static void extsock_plugin_destroy(private_extsock_plugin_t *this); // 플러그인 소멸자
static void ts_to_string(traffic_selector_t *ts, char *buf, size_t buflen); // TS -> 문자열 변환
static void send_event_to_external(const char *event_json); // 외부로 이벤트 전송

// 플러그인 내부 데이터 구조체 정의입니다.
struct private_extsock_plugin_t {
    plugin_t public;                // 공개 플러그인 인터페이스
    mem_cred_t *creds;              // 인메모리 자격증명 세트
    int sock_fd;                    // 유닉스 도메인 소켓 파일 디스크립터
    thread_t *thread;               // 명령 수신 스레드
    bool running;                   // 스레드 실행 상태 플래그
    linked_list_t *managed_peer_cfgs; // 관리되는 피어 설정 목록
    mutex_t *peer_cfgs_mutex;         // 피어 설정 목록 접근 뮤텍스
};

// --- JSON 파싱 헬퍼 함수 전방 선언 ---
static ike_cfg_t* parse_ike_cfg_from_json(private_extsock_plugin_t *plugin, cJSON *ike_json);            // IKE 설정 파싱
static auth_cfg_t* parse_auth_cfg_from_json(private_extsock_plugin_t *plugin, cJSON *auth_json, bool is_local); // 인증 설정 파싱
static bool add_children_from_json(private_extsock_plugin_t *plugin, peer_cfg_t *peer_cfg, cJSON *children_json_array); // 자식 SA 설정 파싱 및 추가
static linked_list_t* parse_proposals_from_json_array(cJSON *json_array, protocol_id_t proto, bool is_ike); // 제안 목록 파싱
static linked_list_t* parse_ts_from_json_array(cJSON *json_array);                                       // 트래픽 셀렉터 목록 파싱
static char* json_array_to_comma_separated_string(cJSON *json_array);                                     // JSON 배열 -> 쉼표 구분 문자열
static action_t string_to_action(const char* action_str);                                                 // 문자열 -> action_t 변환
// --- 헬퍼 함수 전방 선언 종료 ---

// 지정된 IKE_SA에 대해 DPD(Dead Peer Detection)를 시작합니다.
static void start_dpd(const char *ike_sa_name)
{
    // 이름으로 IKE_SA를 찾습니다.
    ike_sa_t *ike_sa = charon->ike_sa_manager->checkout_by_name(
        charon->ike_sa_manager, (char*)ike_sa_name, ID_MATCH_PERFECT);
    if (!ike_sa) // IKE_SA를 찾지 못하면 로그를 남기고 반환합니다.
    {
        DBG1(DBG_LIB, "start_dpd: IKE_SA '%s' not found", ike_sa_name);
        return;
    }
    DBG1(DBG_LIB, "start_dpd: Starting DPD for IKE_SA '%s'", ike_sa_name);
    ike_dpd_t *dpd = ike_dpd_create(TRUE); // DPD 태스크를 생성합니다.
    ike_sa->queue_task(ike_sa, (task_t*)dpd); // IKE_SA의 태스크 큐에 추가합니다.
}

// JSON 문자열 배열을 쉼표로 구분된 단일 C 문자열로 변환합니다.
// 배열이 비어있거나 유효하지 않으면 "%any"를 반환합니다.
static char* json_array_to_comma_separated_string(cJSON *json_array) {
    if (!json_array || !cJSON_IsArray(json_array) || cJSON_GetArraySize(json_array) == 0) {
        return strdup("%any"); // 기본값 처리
    }

    chunk_t result = chunk_empty; // 결과 청크
    chunk_t comma = chunk_from_str(","); // 쉼표 청크
    cJSON *item;
    bool first = TRUE; // 첫 항목 여부 플래그

    // 배열의 각 항목을 순회합니다.
    cJSON_ArrayForEach(item, json_array) {
        if (cJSON_IsString(item) && item->valuestring && *(item->valuestring)) { // 유효한 문자열 항목인 경우
            chunk_t current_item_chunk = chunk_from_str(item->valuestring);
            if (first) { // 첫 항목
                result = chunk_clone(current_item_chunk);
                first = FALSE;
            } else { // 이후 항목
                result = chunk_cat("mcc", result, comma, current_item_chunk); // 이전 결과에 쉼표와 현재 항목 추가
            }
        }
    }

    if (result.len == 0) { // 결과가 비어있으면
        return strdup("%any"); // 기본값 반환
    }

    // 청크를 NULL 종료 C 문자열로 변환합니다.
    char *str_result = malloc(result.len + 1);
    if (!str_result) { // 메모리 할당 실패
        chunk_free(&result);
        return strdup("%any"); // 대체값 반환
    }
    memcpy(str_result, result.ptr, result.len);
    str_result[result.len] = '\0';
    chunk_free(&result); // 원본 청크 해제

    return str_result;
}

// JSON 배열로부터 제안(proposal) 목록을 파싱합니다.
// 제안이 없으면 기본 제안을 추가합니다.
static linked_list_t* parse_proposals_from_json_array(cJSON *json_array, protocol_id_t proto, bool is_ike) {
    linked_list_t *proposals_list = linked_list_create(); // 제안 저장용 리스트
    if (!proposals_list) return NULL;

    if (json_array && cJSON_IsArray(json_array)) { // 유효한 JSON 배열인 경우
        cJSON *prop_json;
        cJSON_ArrayForEach(prop_json, json_array) { // 각 제안 문자열에 대해
            if (cJSON_IsString(prop_json) && prop_json->valuestring) {
                proposal_t *p = proposal_create_from_string(proto, prop_json->valuestring); // 문자열로부터 제안 객체 생성
                if (p) {
                    proposals_list->insert_last(proposals_list, p); // 리스트에 추가
                } else {
                    DBG1(DBG_LIB, "Failed to parse proposal string: %s for proto %d", prop_json->valuestring, proto);
                }
            }
        }
    }

    if (proposals_list->get_count(proposals_list) == 0) { // 파싱된 제안이 없으면 기본값 추가
        DBG1(DBG_LIB, "No proposals in JSON, adding defaults for proto %d (is_ike: %d)", proto, is_ike);
        if (is_ike) { // IKE 제안인 경우
            proposal_t *first = proposal_create_default(PROTO_IKE);
            if (first) proposals_list->insert_last(proposals_list, first);
            proposal_t *second = proposal_create_default_aead(PROTO_IKE);
            if (second) proposals_list->insert_last(proposals_list, second);
        } else { // ESP/AH 제안인 경우
            proposal_t *first = proposal_create_default_aead(proto);
            if (first) proposals_list->insert_last(proposals_list, first);
            proposal_t *second = proposal_create_default(proto);
            if (second) proposals_list->insert_last(proposals_list, second);
        }
    }
    return proposals_list;
}

// JSON 배열로부터 트래픽 셀렉터(TS) 목록을 파싱합니다.
// TS가 없으면 동적 TS (any)를 추가합니다.
static linked_list_t* parse_ts_from_json_array(cJSON *json_array) {
    linked_list_t *ts_list = linked_list_create(); // TS 저장용 리스트
    if (!ts_list) return NULL;

    if (json_array && cJSON_IsArray(json_array)) { // 유효한 JSON 배열인 경우
        cJSON *ts_json;
        cJSON_ArrayForEach(ts_json, json_array) { // 각 TS 문자열(CIDR)에 대해
            if (cJSON_IsString(ts_json) && ts_json->valuestring) {
                // CIDR 문자열로부터 TS 객체 생성 (모든 포트 범위)
                traffic_selector_t *ts = traffic_selector_create_from_cidr(ts_json->valuestring, 0, 0, 0xFFFF);
                if (ts) {
                    ts_list->insert_last(ts_list, ts); // 리스트에 추가
                } else {
                    DBG1(DBG_LIB, "Failed to parse TS string as CIDR: %s", ts_json->valuestring);
                }
            }
        }
    }
    
    if (ts_list->get_count(ts_list) == 0) { // 파싱된 TS가 없으면 동적 TS 추가
        traffic_selector_t* ts = traffic_selector_create_dynamic(0, 0, 0xFFFF);
        if (ts) ts_list->insert_last(ts_list, ts);
        DBG1(DBG_LIB, "No traffic selectors in JSON or all failed to parse, adding dynamic TS");
    }
    return ts_list;
}

// JSON 객체로부터 IKE 설정을 파싱하여 ike_cfg_t 객체를 생성합니다.
static ike_cfg_t* parse_ike_cfg_from_json(private_extsock_plugin_t *plugin, cJSON *ike_json) {
    if (!ike_json) return NULL; // 입력 JSON이 없으면 NULL 반환

    ike_cfg_create_t ike_create_cfg = {0}; // IKE 설정 생성용 구조체 초기화

    // 로컬 및 원격 주소 파싱
    cJSON *j_local_addrs = cJSON_GetObjectItem(ike_json, "local_addrs");
    ike_create_cfg.local = json_array_to_comma_separated_string(j_local_addrs);
    cJSON *j_remote_addrs = cJSON_GetObjectItem(ike_json, "remote_addrs");
    ike_create_cfg.remote = json_array_to_comma_separated_string(j_remote_addrs);
    
    // IKE 버전 파싱 (기본값: IKE_ANY)
    cJSON *j_version = cJSON_GetObjectItem(ike_json, "version");
    if (j_version && cJSON_IsNumber(j_version)) {
        ike_create_cfg.version = j_version->valueint;
    } else {
        ike_create_cfg.version = IKE_ANY; 
    }
    ike_create_cfg.local_port = 0; // 로컬 포트 (any)
    ike_create_cfg.remote_port = 0; // 원격 포트 (any)

    // ike_cfg 객체 생성
    ike_cfg_t *ike_cfg = ike_cfg_create(&ike_create_cfg);
    free(ike_create_cfg.local); // 할당된 주소 문자열 메모리 해제
    free(ike_create_cfg.remote);

    if (!ike_cfg) { // 생성 실패 시 로그 남기고 NULL 반환
        DBG1(DBG_LIB, "Failed to create ike_cfg");
        return NULL;
    }

    // IKE 제안 파싱 및 추가
    cJSON *j_proposals = cJSON_GetObjectItem(ike_json, "proposals");
    linked_list_t *ike_proposals = parse_proposals_from_json_array(j_proposals, PROTO_IKE, TRUE);
    if (ike_proposals) {
        proposal_t *prop;
        while (ike_proposals->remove_first(ike_proposals, (void**)&prop) == SUCCESS) { // 리스트에서 하나씩 꺼내어
            ike_cfg->add_proposal(ike_cfg, prop); // ike_cfg에 추가
        }
        ike_proposals->destroy(ike_proposals); // 제안 목록 리스트 해제
    }
    return ike_cfg;
}

// JSON 객체로부터 인증 설정을 파싱하여 auth_cfg_t 객체를 생성합니다.
// is_local 플래그는 로컬/원격 인증 구분에 사용됩니다.
static auth_cfg_t* parse_auth_cfg_from_json(private_extsock_plugin_t *plugin, cJSON *auth_json, bool is_local) {
    if (!auth_json) return NULL; // 입력 JSON 없으면 NULL 반환

    auth_cfg_t *auth_cfg = auth_cfg_create(); // 인증 설정 객체 생성
    if (!auth_cfg) {
        DBG1(DBG_LIB, "Failed to create auth_cfg");
        return NULL;
    }

    // JSON에서 인증 관련 항목(타입, ID, 시크릿) 가져오기
    cJSON *j_auth_type = cJSON_GetObjectItem(auth_json, "auth");
    cJSON *j_id = cJSON_GetObjectItem(auth_json, "id");
    cJSON *j_secret = cJSON_GetObjectItem(auth_json, "secret");

    if (j_auth_type && cJSON_IsString(j_auth_type)) { // 인증 타입이 문자열로 지정된 경우
        const char *auth_type_str = j_auth_type->valuestring;
        if (streq(auth_type_str, "psk")) { // PSK 인증
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK); // PSK 클래스 설정
            identification_t *psk_identity = NULL;
            if (j_id && cJSON_IsString(j_id)) { // ID가 지정된 경우
                 psk_identity = identification_create_from_string(j_id->valuestring);
                 auth_cfg->add(auth_cfg, AUTH_RULE_IDENTITY, identification_create_from_string(j_id->valuestring)); // ID 설정
            } else { // ID 없으면 "%any"
                psk_identity = identification_create_from_string("%any");
            }

            if (psk_identity) { // ID 객체 생성 성공 시
                if (j_secret && cJSON_IsString(j_secret)) { // 시크릿이 지정된 경우
                    const char *secret_str = j_secret->valuestring;
                    chunk_t secret_chunk = chunk_from_str((char*)secret_str);
                    shared_key_t *psk_key = shared_key_create(SHARED_IKE, chunk_clone(secret_chunk)); // 공유키 객체 생성
                    if (psk_key) {
                        // 플러그인의 자격증명 세트에 공유키 추가 (psk_key, psk_identity 소유권 이전)
                        plugin->creds->add_shared(plugin->creds, psk_key, psk_identity, NULL);
                    } else {
                        DBG1(DBG_LIB, "Failed to create PSK key for ID: %s", j_id ? j_id->valuestring : "%any");
                        psk_identity->destroy(psk_identity); // 키 생성 실패 시 ID 객체 해제
                    }
                } else {
                    DBG1(DBG_LIB, "PSK auth specified but 'secret' missing or not a string for ID: %s", j_id ? j_id->valuestring : "%any");
                    psk_identity->destroy(psk_identity); // 시크릿 누락 시 ID 객체 해제
                }
            } else {
                 DBG1(DBG_LIB, "Failed to create PSK identity for PSK auth.");
            }
        } else if (streq(auth_type_str, "pubkey")) { // 공개키 인증
            auth_cfg->add(auth_cfg, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PUBKEY); // 공개키 클래스 설정
            if (j_id && cJSON_IsString(j_id)) { // ID가 지정된 경우
                identification_t *pubkey_id = identification_create_from_string(j_id->valuestring);
                if (pubkey_id) {
                    auth_cfg->add(auth_cfg, AUTH_RULE_IDENTITY, pubkey_id); // ID 설정
                } else {
                     DBG1(DBG_LIB, "Failed to create identification for pubkey ID: %s", j_id->valuestring);
                }
            } else if (is_local) { // 로컬 인증인데 ID가 없는 경우
                 DBG1(DBG_LIB, "Pubkey auth for local peer specified but 'id' missing or not a string.");
            }
        } else { // 지원하지 않는 인증 타입
            DBG1(DBG_LIB, "Unsupported auth type: %s", auth_type_str);
            auth_cfg->destroy(auth_cfg);
            return NULL;
        }
    } else { // 인증 타입 누락
        DBG1(DBG_LIB, "'auth' type missing in auth config");
        auth_cfg->destroy(auth_cfg);
        return NULL;
    }
    return auth_cfg;
}

// 문자열로 된 액션 이름을 action_t 열거형 값으로 변환합니다.
static action_t string_to_action(const char* action_str) {
    if (!action_str) return ACTION_NONE;
    if (streq(action_str, "trap")) return ACTION_TRAP;
    if (streq(action_str, "start")) return ACTION_START;
    // "clear", "hold", "restart"는 strongSwan 내부 정책에 따라 매핑 (예시)
    if (streq(action_str, "clear")) return ACTION_TRAP;
    if (streq(action_str, "hold")) return ACTION_TRAP;
    if (streq(action_str, "restart")) return ACTION_START;
    return ACTION_NONE; // 그 외에는 ACTION_NONE
}

// JSON 배열로부터 자식 SA(CHILD_SA) 설정들을 파싱하여 peer_cfg에 추가합니다.
static bool add_children_from_json(private_extsock_plugin_t *plugin, peer_cfg_t *peer_cfg, cJSON *children_json_array) {
    if (!children_json_array || !cJSON_IsArray(children_json_array)) { // 자식 SA 설정이 없거나 배열이 아니면 성공 처리
        return TRUE;
    }

    cJSON *child_json;
    cJSON_ArrayForEach(child_json, children_json_array) { // 각 자식 SA 설정에 대해
        if (!cJSON_IsObject(child_json)) continue; // 객체가 아니면 건너뛰기

        // 자식 SA 이름 파싱
        cJSON *j_name = cJSON_GetObjectItem(child_json, "name");
        if (!j_name || !cJSON_IsString(j_name) || !j_name->valuestring) {
            DBG1(DBG_LIB, "Child config missing 'name'");
            continue;
        }
        const char *child_name_str = j_name->valuestring;

        child_cfg_create_t child_create_cfg = {0}; // 자식 SA 생성용 구조체 초기화
        
        // 시작 액션(start_action) 파싱 (기본값: ACTION_NONE)
        cJSON *j_start_action = cJSON_GetObjectItem(child_json, "start_action");
        if (j_start_action && cJSON_IsString(j_start_action)) {
            child_create_cfg.start_action = string_to_action(j_start_action->valuestring);
        } else {
            child_create_cfg.start_action = ACTION_NONE; 
        }
        
        // DPD 액션(dpd_action) 파싱 (기본값: ACTION_NONE)
        cJSON *j_dpd_action = cJSON_GetObjectItem(child_json, "dpd_action");
        if (j_dpd_action && cJSON_IsString(j_dpd_action)) {
            child_create_cfg.dpd_action = string_to_action(j_dpd_action->valuestring);
        } else {
            child_create_cfg.dpd_action = ACTION_NONE; 
        }

        // 자식 SA 설정(child_cfg_t) 객체 생성
        child_cfg_t *child_cfg = child_cfg_create((char*)child_name_str, &child_create_cfg);
        if (!child_cfg) { // 생성 실패 시
            DBG1(DBG_LIB, "Failed to create child_cfg: %s", child_name_str);
            continue;
        }

        // 로컬 트래픽 셀렉터 파싱 및 추가
        cJSON *j_local_ts = cJSON_GetObjectItem(child_json, "local_ts");
        linked_list_t *local_ts_list = parse_ts_from_json_array(j_local_ts);
        if (local_ts_list) {
            traffic_selector_t *ts;
            while (local_ts_list->remove_first(local_ts_list, (void**)&ts) == SUCCESS) {
                child_cfg->add_traffic_selector(child_cfg, TRUE, ts); // TRUE는 로컬 TS임을 의미
            }
            local_ts_list->destroy(local_ts_list);
        }

        // 원격 트래픽 셀렉터 파싱 및 추가
        cJSON *j_remote_ts = cJSON_GetObjectItem(child_json, "remote_ts");
        linked_list_t *remote_ts_list = parse_ts_from_json_array(j_remote_ts);
        if (remote_ts_list) {
            traffic_selector_t *ts;
            while (remote_ts_list->remove_first(remote_ts_list, (void**)&ts) == SUCCESS) {
                child_cfg->add_traffic_selector(child_cfg, FALSE, ts); // FALSE는 원격 TS임을 의미
            }
            remote_ts_list->destroy(remote_ts_list);
        }

        // ESP 제안 파싱 및 추가
        cJSON *j_esp_proposals = cJSON_GetObjectItem(child_json, "esp_proposals");
        linked_list_t *esp_proposals_list = parse_proposals_from_json_array(j_esp_proposals, PROTO_ESP, FALSE);
        if (esp_proposals_list) {
            proposal_t *prop;
            while (esp_proposals_list->remove_first(esp_proposals_list, (void**)&prop) == SUCCESS) {
                child_cfg->add_proposal(child_cfg, prop);
            }
            esp_proposals_list->destroy(esp_proposals_list);
        }
        
        peer_cfg->add_child_cfg(peer_cfg, child_cfg); // 부모 peer_cfg에 자식 SA 설정 추가
        DBG2(DBG_LIB, "Added child_cfg: %s to peer_cfg: %s", child_name_str, peer_cfg->get_name(peer_cfg));
    }
    return TRUE; // 모든 자식 SA 처리 성공
}

// 수신된 JSON 형식의 IPsec 설정을 적용합니다.
static void apply_ipsec_config(private_extsock_plugin_t *this, const char *config_json)
{
    DBG1(DBG_LIB, "apply_ipsec_config: received config: %s", config_json); // 수신된 설정 로그
    cJSON *root = cJSON_Parse(config_json); // JSON 파싱
    if (!root) { // 파싱 실패
        DBG1(DBG_LIB, "apply_ipsec_config: Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return;
    }

    // 연결 이름 파싱
    cJSON *j_conn_name = cJSON_GetObjectItem(root, "name");
    if (!j_conn_name || !cJSON_IsString(j_conn_name) || !j_conn_name->valuestring) {
        DBG1(DBG_LIB, "apply_ipsec_config: Missing connection 'name' in JSON");
        cJSON_Delete(root);
        return;
    }
    const char *conn_name_str = j_conn_name->valuestring;

    // IKE 설정 파싱
    cJSON *j_ike_cfg = cJSON_GetObjectItem(root, "ike_cfg");
    ike_cfg_t *ike_cfg = parse_ike_cfg_from_json(this, j_ike_cfg);
    if (!ike_cfg) { // IKE 설정 파싱 실패
        DBG1(DBG_LIB, "apply_ipsec_config: Failed to parse ike_cfg section for %s", conn_name_str);
        cJSON_Delete(root);
        return;
    }

    peer_cfg_create_t peer_create_cfg = {0}; // 피어 설정 생성용 구조체
    // TODO: 필요시 JSON에서 DPD, 재키 설정 등 추가 피어 설정 파싱
    
    // 피어 설정(peer_cfg_t) 객체 생성
    peer_cfg_t *peer_cfg = peer_cfg_create((char*)conn_name_str, ike_cfg, &peer_create_cfg);
    if (!peer_cfg) { // 피어 설정 생성 실패
        DBG1(DBG_LIB, "apply_ipsec_config: Failed to create peer_cfg for %s", conn_name_str);
        ike_cfg->destroy(ike_cfg); // 생성된 IKE 설정 해제
        cJSON_Delete(root);
        return;
    }

    // 로컬 인증 설정 파싱 및 추가 (없으면 기본값 사용)
    cJSON *j_local_auth = cJSON_GetObjectItem(root, "local_auth");
    if (j_local_auth) {
        auth_cfg_t *local_auth_cfg = parse_auth_cfg_from_json(this, j_local_auth, TRUE);
        if (local_auth_cfg) {
            peer_cfg->add_auth_cfg(peer_cfg, local_auth_cfg, TRUE); // TRUE는 로컬 인증
        } else {
            DBG1(DBG_LIB, "apply_ipsec_config: Failed to parse local_auth for %s", conn_name_str);
        }
    } else { // 기본 로컬 인증 (any)
        auth_cfg_t *default_local_auth = auth_cfg_create();
        default_local_auth->add(default_local_auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_ANY);
        peer_cfg->add_auth_cfg(peer_cfg, default_local_auth, TRUE);
    }

    // 원격 인증 설정 파싱 및 추가 (없으면 기본값 사용)
    cJSON *j_remote_auth = cJSON_GetObjectItem(root, "remote_auth");
    if (j_remote_auth) {
        auth_cfg_t *remote_auth_cfg = parse_auth_cfg_from_json(this, j_remote_auth, FALSE);
        if (remote_auth_cfg) {
            peer_cfg->add_auth_cfg(peer_cfg, remote_auth_cfg, FALSE); // FALSE는 원격 인증
        } else {
            DBG1(DBG_LIB, "apply_ipsec_config: Failed to parse remote_auth for %s", conn_name_str);
        }
    } else { // 기본 원격 인증 (any)
        auth_cfg_t *default_remote_auth = auth_cfg_create();
        default_remote_auth->add(default_remote_auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_ANY);
        peer_cfg->add_auth_cfg(peer_cfg, default_remote_auth, FALSE);
    }

    // 자식 SA 설정 파싱 및 추가
    cJSON *j_children = cJSON_GetObjectItem(root, "children");
    if (!add_children_from_json(this, peer_cfg, j_children)) {
        DBG1(DBG_LIB, "apply_ipsec_config: Error processing children for %s", conn_name_str);
        // 치명적 오류 시 정리 로직 고려
    }

    DBG1(DBG_LIB, "Successfully parsed peer_cfg '%s' from JSON.", peer_cfg->get_name(peer_cfg));

    // 뮤텍스 잠금 후 관리 목록에 피어 설정 추가
    this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
    this->managed_peer_cfgs->insert_last(this->managed_peer_cfgs, peer_cfg); // 피어 설정 저장
    DBG1(DBG_LIB, "Stored peer_cfg '%s' in managed list.", peer_cfg->get_name(peer_cfg));

    // "start_action"이 ACTION_START인 자식 SA에 대해 SA 개시
    enumerator_t *child_enum = peer_cfg->create_child_cfg_enumerator(peer_cfg);
    child_cfg_t *current_child;
    if (child_enum) {
        while (child_enum->enumerate(child_enum, &current_child)) { // 각 자식 SA 순회
            if (current_child->get_start_action(current_child) == ACTION_START) { // 시작 액션 확인
                DBG1(DBG_LIB, "Initiating CHILD_SA '%s' for peer '%s'",
                     current_child->get_name(current_child), peer_cfg->get_name(peer_cfg));
                // SA 개시 요청
                charon->controller->initiate(charon->controller,
                                             peer_cfg, current_child,
                                             NULL, NULL, 0, 0, FALSE);
            }
        }
        child_enum->destroy(child_enum); // 열거자 해제
    }
    this->peer_cfgs_mutex->unlock(this->peer_cfgs_mutex); // 뮤텍스 해제

    cJSON_Delete(root); // JSON 객체 트리 해제
}

// 외부 프로그램으로부터 수신된 명령을 처리합니다.
static void handle_external_command(private_extsock_plugin_t *this, char *cmd)
{
    // "START_DPD <ike_sa_name>" 명령 처리
    if (strncmp(cmd, "START_DPD ", 10) == 0) {
        start_dpd(cmd + 10); // IKE_SA 이름 부분 전달
    }
    // "APPLY_CONFIG <json_config>" 명령 처리
    else if (strncmp(cmd, "APPLY_CONFIG ", 13) == 0) {
        apply_ipsec_config(this, cmd + 13); // JSON 설정 부분 전달
    }
    // TODO: 다른 외부 명령 처리 로직 추가 가능
}

// 유닉스 도메인 소켓을 통해 외부 명령을 수신하는 스레드 함수입니다.
static void* socket_thread(private_extsock_plugin_t *this)
{
    struct sockaddr_un addr; // 소켓 주소 구조체
    char buf[1024]; // 수신 버퍼

    // 유닉스 도메인 소켓 생성 (스트림 방식)
    this->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->sock_fd < 0) { // 생성 실패
        DBG1(DBG_LIB, "Failed to create Unix domain socket");
        return NULL;
    }

    unlink(SOCKET_PATH); // 기존 소켓 파일이 있으면 삭제 (bind 오류 방지)

    // 소켓 주소 설정
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    // 소켓에 주소 바인딩
    if (bind(this->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        DBG1(DBG_LIB, "Failed to bind Unix domain socket to %s", SOCKET_PATH);
        close(this->sock_fd);
        this->sock_fd = -1;
        return NULL;
    }

    // 소켓 리슨 (연결 요청 대기), 백로그 5
    if (listen(this->sock_fd, 5) == -1) {
        DBG1(DBG_LIB, "Failed to listen on Unix domain socket");
        close(this->sock_fd);
        this->sock_fd = -1;
        unlink(SOCKET_PATH); // 리슨 실패 시 소켓 파일도 삭제
        return NULL;
    }

    DBG1(DBG_LIB, "Unix domain socket listening on %s", SOCKET_PATH);
    this->running = TRUE; // 스레드 실행 플래그 설정

    // 스레드 실행 루프
    while (this->running) {
        // 클라이언트 연결 요청 수락 (블로킹)
        int client_fd = accept(this->sock_fd, NULL, NULL);
        if (client_fd < 0) { // accept 실패
            if (!this->running) break; // 종료 요청 시 루프 탈출
            DBG1(DBG_LIB, "Accept failed on Unix domain socket");
            continue;
        }

        // 클라이언트로부터 데이터 읽기 (최대 버퍼 크기 - 1)
        ssize_t len = read(client_fd, buf, sizeof(buf)-1);
        if (len > 0) { // 데이터 수신 성공
            buf[len] = '\0'; // NULL 종료 문자열로 만듦
            DBG2(DBG_LIB, "Received command from external program: %s", buf);
            handle_external_command(this, buf); // 명령 처리 함수 호출
        } else if (len == 0) { // 클라이언트 연결 종료
             DBG2(DBG_LIB, "Client disconnected");
        } else { // 읽기 오류
            DBG1(DBG_LIB, "Read error from client socket");
        }
        close(client_fd); // 클라이언트 소켓 닫기
    }
    return NULL; // 스레드 종료
}

// 플러그인 이름 반환 함수입니다.
static char* extsock_plugin_get_name(plugin_t* plugin) {
    (void)plugin; // 사용 안함
    return "extsock";
}

// 플러그인 기능 정보 반환 함수입니다. (현재 기능 없음)
static int extsock_plugin_get_features(plugin_t* plugin, plugin_feature_t *features[]) {
    (void)plugin; // 사용 안함
    *features = NULL;
    return 0;
}

// CHILD_SA UP/DOWN 이벤트 리스너 함수입니다.
// 터널 상태 변경 시 외부로 JSON 이벤트를 전송합니다.
static bool extsock_child_updown(listener_t *this, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up)
{
    char buf[1024]; // 이벤트 JSON 저장 버퍼
    uint32_t spi = child_sa->get_spi(child_sa, TRUE); // 인바운드 SPI
    uint8_t proto = child_sa->get_protocol(child_sa); // 프로토콜 (ESP/AH)
    const char *proto_str = (proto == IPPROTO_ESP) ? "esp" : (proto == IPPROTO_AH) ? "ah" : "unknown";
    traffic_selector_t *local_ts, *remote_ts;
    // CHILD_SA의 정책(TS) 열거
    enumerator_t *ts_enum = child_sa->create_policy_enumerator(child_sa);
    if (ts_enum && ts_enum->enumerate(ts_enum, &local_ts, &remote_ts)) { // TS 정보 가져오기 성공 시
        char local_buf[128], remote_buf[128];
        ts_to_string(local_ts, local_buf, sizeof(local_buf)); // 로컬 TS 문자열 변환
        ts_to_string(remote_ts, remote_buf, sizeof(remote_buf)); // 원격 TS 문자열 변환
        // 이벤트 JSON 문자열 생성
        snprintf(buf, sizeof(buf),
            "{\"event\":\"tunnel_%s\",\"spi\":%u,\"proto\":\"%s\",\"local_ts\":\"%s\",\"remote_ts\":\"%s\"}",
            up ? "up" : "down", spi, proto_str, local_buf, remote_buf);
        send_event_to_external(buf); // 외부로 이벤트 전송
    }
    if (ts_enum) ts_enum->destroy(ts_enum); // 열거자 해제
    return TRUE; // 항상 성공 반환 (리스너 체인 계속)
}

// CHILD_SA UP/DOWN 이벤트 처리를 위한 리스너 인스턴스입니다.
static listener_t extsock_listener = {
    .child_updown = extsock_child_updown,
};

// extsock 플러그인 생성 함수입니다. strongSwan 로드 시 호출됩니다.
plugin_t* extsock_plugin_create()
{
    // 플러그인 내부 데이터 구조체 메모리 할당 및 초기화
    private_extsock_plugin_t *this = calloc(1, sizeof(*this));

    this->sock_fd = -1; // 소켓 디스크립터 초기화

    // 공개 플러그인 인터페이스 함수 포인터 설정
    this->public.destroy = (void*)extsock_plugin_destroy;
    this->public.get_name = extsock_plugin_get_name;
    this->public.get_features = extsock_plugin_get_features;

    // 이벤트 버스에 CHILD_SA UP/DOWN 리스너 등록
    charon->bus->add_listener(charon->bus, &extsock_listener);

    // 인메모리 자격증명 세트 생성 및 등록
    this->creds = mem_cred_create();
    if (this->creds) {
        lib->credmgr->add_set(lib->credmgr, &this->creds->set);
    } else { // 생성 실패 시
        DBG1(DBG_LIB, "Failed to create mem_cred_t for extsock plugin");
        free(this);
        return NULL; // 플러그인 생성 실패
    }

    // 관리되는 피어 설정 목록 생성
    this->managed_peer_cfgs = linked_list_create();
    if (!this->managed_peer_cfgs) {
        DBG1(DBG_LIB, "Failed to create managed_peer_cfgs list");
        lib->credmgr->remove_set(lib->credmgr, &this->creds->set); // 등록된 자격증명 세트 해제
        this->creds->destroy(this->creds); // 자격증명 세트 해제
        free(this);
        return NULL;
    }

    // 피어 설정 목록 접근 뮤텍스 생성
    this->peer_cfgs_mutex = mutex_create(MUTEX_TYPE_DEFAULT);
    if (!this->peer_cfgs_mutex) {
        DBG1(DBG_LIB, "Failed to create peer_cfgs_mutex");
        this->managed_peer_cfgs->destroy(this->managed_peer_cfgs); // 피어 목록 해제
        lib->credmgr->remove_set(lib->credmgr, &this->creds->set);
        this->creds->destroy(this->creds);
        free(this);
        return NULL;
    }

    // 외부 명령 수신 스레드 생성 및 시작
    this->thread = thread_create((thread_main_t)socket_thread, this);
    if (!this->thread) { // 스레드 생성 실패 시
        DBG1(DBG_LIB, "Failed to create socket listener thread");
        // 생성된 리소스들 해제
        if (this->creds) {
            lib->credmgr->remove_set(lib->credmgr, &this->creds->set);
            this->creds->destroy(this->creds);
        }
        if(this->managed_peer_cfgs) this->managed_peer_cfgs->destroy(this->managed_peer_cfgs);
        if(this->peer_cfgs_mutex) this->peer_cfgs_mutex->destroy(this->peer_cfgs_mutex);
        charon->bus->remove_listener(charon->bus, &extsock_listener); // 리스너 해제
        free(this);
        return NULL; // 플러그인 생성 실패
    }

    DBG1(DBG_LIB, "extsock plugin loaded successfully"); // 플러그인 로드 성공 로그
    return &this->public; // 공개 플러그인 인터페이스 반환
}

// extsock 플러그인 소멸자 함수입니다. strongSwan 언로드 시 호출됩니다.
void extsock_plugin_destroy(private_extsock_plugin_t *this)
{
    this->running = FALSE; // 소켓 스레드 루프 종료 유도

    if (this->thread) { // 스레드가 존재하면
        if (this->sock_fd >= 0) { // 소켓이 열려있으면
            shutdown(this->sock_fd, SHUT_RDWR); // 소켓 읽기/쓰기 종료 (accept 블로킹 해제용)
            close(this->sock_fd); // 소켓 닫기
            this->sock_fd = -1;
        }
        this->thread->join(this->thread); // 스레드 종료 대기
        // this->thread->destroy(this->thread); // join이 처리하므로 보통 불필요
        this->thread = NULL;
    }

    unlink(SOCKET_PATH); // 유닉스 도메인 소켓 파일 삭제

    // 이벤트 버스에서 리스너 제거
    charon->bus->remove_listener(charon->bus, &extsock_listener);

    // 관리되는 피어 설정들 해제
    if (this->managed_peer_cfgs) {
        this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
        peer_cfg_t *cfg_to_destroy;
        // 리스트에서 모든 peer_cfg를 꺼내어 각각 destroy 호출
        while(this->managed_peer_cfgs->remove_first(this->managed_peer_cfgs, (void**)&cfg_to_destroy) == SUCCESS) {
            cfg_to_destroy->destroy(cfg_to_destroy);
        }
        this->peer_cfgs_mutex->unlock(this->peer_cfgs_mutex);
        this->managed_peer_cfgs->destroy(this->managed_peer_cfgs); // 리스트 자체 해제
        this->managed_peer_cfgs = NULL;
    }

    // 뮤텍스 해제
    if (this->peer_cfgs_mutex) {
        this->peer_cfgs_mutex->destroy(this->peer_cfgs_mutex);
        this->peer_cfgs_mutex = NULL;
    }

    // 자격증명 세트 해제
    if (this->creds) {
        lib->credmgr->remove_set(lib->credmgr, &this->creds->set);
        this->creds->destroy(this->creds);
        this->creds = NULL;
    }

    DBG1(DBG_LIB, "extsock plugin unloaded successfully"); // 플러그인 언로드 성공 로그
    free(this); // 플러그인 내부 데이터 구조체 메모리 해제
}
 
// 외부 프로그램으로 JSON 형식의 이벤트를 전송합니다.
static void send_event_to_external(const char *event_json)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0); // 새 소켓 생성
    if (fd < 0) {
        DBG1(DBG_LIB, "send_event_to_external: Failed to create socket: %s", strerror(errno));
        return;
    }
    struct sockaddr_un addr; // 소켓 주소
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    // 외부 프로그램(수신측) 소켓에 연결 시도
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        // 연결 성공 시 이벤트 JSON 전송
        ssize_t written = write(fd, event_json, strlen(event_json));
        if (written < 0) { // 쓰기 실패
            DBG1(DBG_LIB, "Failed to write to socket: %s", strerror(errno));
        }
    } else { // 연결 실패
         DBG1(DBG_LIB, "send_event_to_external: Failed to connect to socket %s: %s", SOCKET_PATH, strerror(errno));
    }
    close(fd); // 소켓 닫기
}

// traffic_selector_t 객체를 문자열로 변환하여 버퍼에 저장합니다.
// strongSwan의 %R 포맷 지정자를 사용합니다.
static void ts_to_string(traffic_selector_t *ts, char *buf, size_t buflen)
{
    if (ts && buf && buflen > 0) // 유효한 입력값인지 확인
    {
        snprintf(buf, buflen, "%R", ts); // %R은 strongSwan 내부 타입 포맷팅
    }
    else if (buf && buflen > 0) // TS가 NULL이어도 버퍼는 초기화
    {
        buf[0] = '\0'; // 빈 문자열로 설정
    }
} 