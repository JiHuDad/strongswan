// 플러그인 자체 헤더 파일을 포함합니다. 플러그인의 공개 인터페이스를 정의합니다.
#include "extsock_plugin.h"
// cJSON 라이브러리 헤더 파일을 포함합니다. JSON 파싱 및 생성을 위해 사용됩니다.
// 이 파일은 플러그인 디렉토리 내에 함께 위치하거나, 시스템 라이브러리 경로에 있어야 합니다.
#include "cJSON.h"

// strongSwan 데몬 및 라이브러리 관련 핵심 헤더 파일들을 포함합니다.
#include <daemon.h>      // 데몬 관련 기능 (charon 전역 객체 등)
#include <library.h>     // strongSwan 라이브러리 기본 기능
#include <threading/thread.h> // 스레드 생성 및 관리 기능
#include <sys/un.h>      // 유닉스 도메인 소켓 주소 구조체 (struct sockaddr_un)
#include <sys/socket.h>  // 소켓 관련 함수 (socket, connect, bind, listen, accept 등)
#include <unistd.h>      // 유닉스 표준 함수 (read, write, close, unlink 등)
#include <stdio.h>       // 표준 입출력 함수 (snprintf 등)
#include <string.h>      // 문자열 처리 함수 (strlen, strncmp, strncpy, memset 등)
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
    kernel_listener_t *listener;    // SAD (Security Association Database) 및 SPD (Security Policy Database)
                                    // 변경 이벤트를 감지하기 위한 커널 리스너 객체입니다.
};

// 외부 프로그램에 이벤트(JSON 형식의 문자열)를 전송하는 함수입니다.
/*static void send_event_to_external(const char *event_json)
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
        write(fd, event_json, strlen(event_json));
    }
    // 소켓 파일 디스크립터를 닫습니다.
    close(fd);
}*/

// SAD (Security Association Database) 변경 이벤트 정보를 JSON 문자열로 만들어 외부 프로그램에 전송하는 함수입니다.
/*static void send_sad_event(const char *event_type, kernel_ipsec_sa_id_t *id, kernel_ipsec_sa_t *sa)
{
    // 생성될 JSON 문자열을 저장하기 위한 버퍼입니다. 크기는 예상되는 최대 길이를 고려하여 설정합니다.
    char buf[512];
    // 출발지 및 목적지 IP 주소를 문자열 형태로 저장하기 위한 버퍼입니다.
    char src_str[64], dst_str[64];

    // kernel_ipsec_sa_id_t 구조체에 저장된 출발지 주소(sockaddr 형식)를 host_t 객체로 변환합니다.
    // host_t는 strongSwan에서 IP 주소를 다루는 내부 표현 방식입니다.
    host_t *src_host = host_create_from_sockaddr((struct sockaddr*)&id->src);
    // kernel_ipsec_sa_id_t 구조체에 저장된 목적지 주소를 host_t 객체로 변환합니다.
    host_t *dst_host = host_create_from_sockaddr((struct sockaddr*)&id->dst);
    // host_t 객체를 사람이 읽을 수 있는 IP 주소 문자열 형식으로 변환하여 버퍼에 저장합니다. (예: "192.168.0.1")
    // %H 포맷 지정자는 host_t 객체를 문자열로 변환합니다.
    snprintf(src_str, sizeof(src_str), "%H", src_host);
    snprintf(dst_str, sizeof(dst_str), "%H", dst_host);

    // JSON 형식의 문자열을 생성합니다.
    // 이벤트 타입("SAD_ADD", "SAD_DELETE"), SPI (Security Parameter Index), 프로토콜,
    // 출발지 주소, 목적지 주소 정보를 포함합니다.
    snprintf(buf, sizeof(buf),
        "{\"event\":\"%s\",\"spi\":%u,\"proto\":%u,\"src\":\"%s\",\"dst\":\"%s\"}",
        event_type,  // 이벤트의 종류 (예: "SAD_ADD", "SAD_DELETE")
        id->spi,     // SA를 고유하게 식별하는 32비트 정수 값입니다.
        id->proto,   // SA가 사용하는 프로토콜 (예: IPSEC_PROTO_ESP, IPSEC_PROTO_AH)
        src_str,     // 변환된 출발지 IP 주소 문자열입니다.
        dst_str);    // 변환된 목적지 IP 주소 문자열입니다.

    // 주소 변환을 위해 동적으로 할당된 host_t 객체의 메모리를 해제합니다.
    DESTROY_IF(src_host); // src_host가 NULL이 아니면 파괴(메모리 해제)합니다.
    DESTROY_IF(dst_host); // dst_host가 NULL이 아니면 파괴합니다.

    // 최종적으로 생성된 JSON 문자열을 외부 프로그램으로 전송합니다.
    //send_event_to_external(buf); // Temporarily commented out
}*/

// SPD (Security Policy Database) 변경 이벤트 정보를 JSON 문자열로 만들어 외부 프로그램에 전송하는 함수입니다.
/*static void send_spd_event(const char *event_type, kernel_ipsec_policy_id_t *id, kernel_ipsec_policy_t *policy)
{
    // 생성될 JSON 문자열을 저장하기 위한 버퍼입니다.
    char buf[512];
    // 출발지 및 목적지 IP 주소/네트워크를 문자열 형태로 저장하기 위한 버퍼입니다.
    char src_str[64], dst_str[64];

    // kernel_ipsec_policy_id_t 구조체에 저장된 출발지 선택자 주소를 host_t 객체로 변환합니다.
    host_t *src_host = host_create_from_sockaddr((struct sockaddr*)&id->src);
    // kernel_ipsec_policy_id_t 구조체에 저장된 목적지 선택자 주소를 host_t 객체로 변환합니다.
    host_t *dst_host = host_create_from_sockaddr((struct sockaddr*)&id->dst);
    // host_t 객체를 사람이 읽을 수 있는 IP 주소/네트워크 문자열 형식으로 변환합니다. (예: "10.0.0.0/24")
    snprintf(src_str, sizeof(src_str), "%H", src_host);
    snprintf(dst_str, sizeof(dst_str), "%H", dst_host);

    // 정책의 방향(direction)을 나타내는 정수 값을 문자열로 변환합니다.
    const char *dir_str = (id->dir == POLICY_IN) ? "in" :       // Inbound (수신) 정책
                         (id->dir == POLICY_OUT) ? "out" :     // Outbound (송신) 정책
                         "fwd";                                // Forward (전달) 정책 (일반적으로 잘 사용되지 않음)

    // JSON 형식의 문자열을 생성합니다.
    // 이벤트 타입("SPD_ADD", "SPD_DELETE"), 정책 ID (reqid), 프로토콜,
    // 출발지 선택자, 목적지 선택자, 정책 방향 정보를 포함합니다.
    snprintf(buf, sizeof(buf),
        "{\"event\":\"%s\",\"id\":%u,\"proto\":%u,\"src\":\"%s\",\"dst\":\"%s\",\"dir\":\"%s\"}",
        event_type,  // 이벤트의 종류 (예: "SPD_ADD", "SPD_DELETE")
        id->reqid,   // 정책을 식별하는 요청 ID. CHILD_SA 설정 시 사용된 reqid와 연관될 수 있습니다.
        id->proto,   // 정책이 적용되는 프로토콜 (0이면 모든 프로토콜).
        src_str,     // 변환된 출발지 선택자 문자열입니다.
        dst_str,     // 변환된 목적지 선택자 문자열입니다.
        dir_str);    // 변환된 정책 방향 문자열입니다. ("in", "out", "fwd")

    // 주소 변환을 위해 동적으로 할당된 host_t 객체의 메모리를 해제합니다.
    DESTROY_IF(src_host);
    DESTROY_IF(dst_host);

    // 최종적으로 생성된 JSON 문자열을 외부 프로그램으로 전송합니다.
    //send_event_to_external(buf); // Temporarily commented out
}*/

// JSON 문자열을 파싱하여 strongSwan에 IKE/CHILD 설정을 동적으로 추가/적용하는 함수입니다.
static void apply_ipsec_config(private_extsock_plugin_t *plugin_this, const char *json)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        DBG1(DBG_LIB, "Failed to parse JSON for IPsec config");
        return;
    }
    const char *conn_name_const = cJSON_GetObjectItem(root, "name")->valuestring;
    const char *local_const = cJSON_GetObjectItem(root, "local")->valuestring;
    const char *remote_const = cJSON_GetObjectItem(root, "remote")->valuestring;

    char *conn_name = strdup(conn_name_const);
    char *local_addr = strdup(local_const);
    char *remote_addr = strdup(remote_const);

    // IKE/ESP proposal 파싱
    const char *ike_proposal = NULL, *esp_proposal = NULL;
    cJSON *ike_prop_json = cJSON_GetObjectItem(root, "ike_proposal");
    if (ike_prop_json && cJSON_IsString(ike_prop_json)) {
        ike_proposal = ike_prop_json->valuestring;
    }
    cJSON *esp_prop_json = cJSON_GetObjectItem(root, "esp_proposal");
    if (esp_prop_json && cJSON_IsString(esp_prop_json)) {
        esp_proposal = esp_prop_json->valuestring;
    }

    // DPD, rekey_time 등 추가 옵션 파싱
    int dpd = 30, rekey_time = 3600, jitter_time = 300, over_time = 300;
    cJSON *dpd_json = cJSON_GetObjectItem(root, "dpd");
    if (dpd_json && cJSON_IsNumber(dpd_json)) {
        dpd = dpd_json->valueint;
    }
    cJSON *rekey_json = cJSON_GetObjectItem(root, "rekey_time");
    if (rekey_json && cJSON_IsNumber(rekey_json)) {
        rekey_time = rekey_json->valueint;
    }
    cJSON *jitter_json = cJSON_GetObjectItem(root, "jitter_time");
    if (jitter_json && cJSON_IsNumber(jitter_json)) {
        jitter_time = jitter_json->valueint;
    }
    cJSON *over_json = cJSON_GetObjectItem(root, "over_time");
    if (over_json && cJSON_IsNumber(over_json)) {
        over_time = over_json->valueint;
    }

    // IKE 설정 생성
    ike_cfg_create_t ike = {
        .version = IKEV2, .local = local_addr, .local_port = 500,
        .remote = remote_addr, .remote_port = 500, .no_certreq = FALSE,
    };
    ike_cfg_t *ike_cfg = ike_cfg_create(&ike);
    if (ike_proposal) {
        ike_cfg->add_proposal(ike_cfg, proposal_create_from_string(PROTO_IKE, ike_proposal));
    } else {
        ike_cfg->add_proposal(ike_cfg, proposal_create_default(PROTO_IKE));
        ike_cfg->add_proposal(ike_cfg, proposal_create_default_aead(PROTO_IKE));
    }

    // Peer 설정 생성
    peer_cfg_create_t peer = {
        .cert_policy = CERT_NEVER_SEND, .unique = UNIQUE_REPLACE, .keyingtries = 1,
        .rekey_time = rekey_time, .jitter_time = jitter_time, .over_time = over_time, .dpd = dpd,
    };
    peer_cfg_t *peer_cfg = peer_cfg_create(conn_name, ike_cfg, &peer);

    // 인증 방식 파싱 (auth: {type, ...})
    cJSON *auth_json = cJSON_GetObjectItem(root, "auth");
    if (auth_json && cJSON_IsObject(auth_json)) {
        const char *auth_type = cJSON_GetObjectItem(auth_json, "type")->valuestring;
        if (strcmp(auth_type, "cert") == 0) {
            // 인증서 기반 인증
            const char *local_id_const = cJSON_GetObjectItem(auth_json, "local_id")->valuestring;
            const char *remote_id_const = cJSON_GetObjectItem(auth_json, "remote_id")->valuestring;
            const char *cert_file = cJSON_GetObjectItem(auth_json, "cert_file")->valuestring;
            const char *key_file = cJSON_GetObjectItem(auth_json, "key_file")->valuestring;

            char *local_id_str = strdup(local_id_const);
            char *remote_id_str = strdup(remote_id_const);

            // 인증서/키를 strongSwan credential manager에 등록
            certificate_t *cert = lib->creds->create(lib->creds, CRED_CERTIFICATE, CERT_X509, BUILD_FROM_FILE, cert_file, BUILD_END);
            private_key_t *key = lib->creds->create(lib->creds, CRED_PRIVATE_KEY, KEY_ANY, BUILD_FROM_FILE, key_file, BUILD_END);
            if (cert) {
                plugin_this->creds->add_cert_ref(plugin_this->creds, TRUE, cert);
            }
            if (key) {
                plugin_this->creds->add_key(plugin_this->creds, key);
            }
            // 로컬 인증 설정
            auth_cfg_t *auth = auth_cfg_create();
            auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PUBKEY);
            auth->add(auth, AUTH_RULE_IDENTITY, identification_create_from_string(local_id_str));
            peer_cfg->add_auth_cfg(peer_cfg, auth, TRUE);
            // 원격 인증 설정
            auth = auth_cfg_create();
            auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PUBKEY);
            auth->add(auth, AUTH_RULE_IDENTITY, identification_create_from_string(remote_id_str));
            peer_cfg->add_auth_cfg(peer_cfg, auth, FALSE);
            free(local_id_str);
            free(remote_id_str);
        } else if (strcmp(auth_type, "psk") == 0) {
            // PSK 기반 인증
            const char *psk = cJSON_GetObjectItem(root, "psk")->valuestring;
            const char *local_id_const = cJSON_GetObjectItem(auth_json, "local_id") ? cJSON_GetObjectItem(auth_json, "local_id")->valuestring : local_const;
            const char *remote_id_const = cJSON_GetObjectItem(auth_json, "remote_id") ? cJSON_GetObjectItem(auth_json, "remote_id")->valuestring : remote_const;

            char *local_id_str = strdup(local_id_const);
            char *remote_id_str = strdup(remote_id_const);

            // 로컬 인증 설정
            auth_cfg_t *auth = auth_cfg_create();
            auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK);
            auth->add(auth, AUTH_RULE_IDENTITY, identification_create_from_string(local_id_str));
            peer_cfg->add_auth_cfg(peer_cfg, auth, TRUE);
            // 원격 인증 설정
            auth = auth_cfg_create();
            auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK);
            auth->add(auth, AUTH_RULE_IDENTITY, identification_create_from_string(remote_id_str));
            peer_cfg->add_auth_cfg(peer_cfg, auth, FALSE);
            // PSK 등록
            identification_t *local_id_obj_psk = identification_create_from_string(local_id_str);
            identification_t *remote_id_obj_psk = identification_create_from_string(remote_id_str);
            shared_key_t *psk_obj = shared_key_create(SHARED_IKE, chunk_create((u_char*)psk, strlen(psk)));
            if (local_id_obj_psk && remote_id_obj_psk && psk_obj) {
                plugin_this->creds->add_shared(plugin_this->creds, psk_obj, local_id_obj_psk, remote_id_obj_psk, NULL);
            } else {
                DESTROY_IF(local_id_obj_psk);
                DESTROY_IF(remote_id_obj_psk);
                DESTROY_IF(psk_obj);
                DBG1(DBG_LIB, "Failed to create objects for PSK sharing (auth_json)");
            }
            free(local_id_str);
            free(remote_id_str);
        }
    } else {
        // 기본: PSK 방식 (호환성)
        const char *psk = cJSON_GetObjectItem(root, "psk")->valuestring;
        // 로컬 인증 설정
        auth_cfg_t *auth = auth_cfg_create();
        auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK);
        auth->add(auth, AUTH_RULE_IDENTITY, identification_create_from_string(local_addr));
        peer_cfg->add_auth_cfg(peer_cfg, auth, TRUE);
        // 원격 인증 설정
        auth = auth_cfg_create();
        auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK);
        auth->add(auth, AUTH_RULE_IDENTITY, identification_create_from_string(remote_addr));
        peer_cfg->add_auth_cfg(peer_cfg, auth, FALSE);
        // PSK 등록
        identification_t *local_obj_psk_default = identification_create_from_string(local_addr);
        identification_t *remote_obj_psk_default = identification_create_from_string(remote_addr);
        shared_key_t *psk_obj_default = shared_key_create(SHARED_IKE, chunk_create((u_char*)psk, strlen(psk)));
        if (local_obj_psk_default && remote_obj_psk_default && psk_obj_default) {
            plugin_this->creds->add_shared(plugin_this->creds, psk_obj_default, local_obj_psk_default, remote_obj_psk_default, NULL);
        } else {
            DESTROY_IF(local_obj_psk_default);
            DESTROY_IF(remote_obj_psk_default);
            DESTROY_IF(psk_obj_default);
            DBG1(DBG_LIB, "Failed to create objects for PSK sharing (default)");
        }
    }

    // 여러 CHILD_SA 지원 (children: array)
    cJSON *children_json = cJSON_GetObjectItem(root, "children");
    if (children_json && cJSON_IsArray(children_json)) {
        int n = cJSON_GetArraySize(children_json);
        for (int i = 0; i < n; i++) {
            cJSON *child_json = cJSON_GetArrayItem(children_json, i);
            const char *child_name_const = cJSON_GetObjectItem(child_json, "name")->valuestring;
            const char *local_ts_const = cJSON_GetObjectItem(child_json, "local_ts")->valuestring;
            const char *remote_ts_const = cJSON_GetObjectItem(child_json, "remote_ts")->valuestring;

            char *child_name_str = strdup(child_name_const);
            char *local_ts_str = strdup(local_ts_const);
            char *remote_ts_str = strdup(remote_ts_const);

            child_cfg_create_t child_cfg_data = {
                .mode = MODE_TUNNEL,
                .lifetime = { .time = { .life = 3600, .rekey = 3000, .jitter = 300 } }
            };
            child_cfg_t *child_cfg = child_cfg_create(child_name_str, &child_cfg_data);
            // ESP proposal 적용
            if (esp_proposal) {
                child_cfg->add_proposal(child_cfg, proposal_create_from_string(PROTO_ESP, esp_proposal));
            } else {
                child_cfg->add_proposal(child_cfg, proposal_create_default_aead(PROTO_ESP));
                child_cfg->add_proposal(child_cfg, proposal_create_default(PROTO_ESP));
            }
            child_cfg->add_traffic_selector(child_cfg, TRUE, traffic_selector_create_from_cidr(local_ts_str, 0, 0, 65535));
            child_cfg->add_traffic_selector(child_cfg, FALSE, traffic_selector_create_from_cidr(remote_ts_str, 0, 0, 65535));
            peer_cfg->add_child_cfg(peer_cfg, child_cfg);

            free(child_name_str);
            free(local_ts_str);
            free(remote_ts_str);
        }
    } else {
        // 기존 단일 child 처리 (child: object)
        cJSON *child_json = cJSON_GetObjectItem(root, "child");
        if (child_json && cJSON_IsObject(child_json)) {
            const char *child_name_const = cJSON_GetObjectItem(child_json, "name")->valuestring;
            const char *local_ts_const = cJSON_GetObjectItem(child_json, "local_ts")->valuestring;
            const char *remote_ts_const = cJSON_GetObjectItem(child_json, "remote_ts")->valuestring;

            char *child_name_str = strdup(child_name_const);
            char *local_ts_str = strdup(local_ts_const);
            char *remote_ts_str = strdup(remote_ts_const);

            child_cfg_create_t child_cfg_data = {
                .mode = MODE_TUNNEL,
                .lifetime = { .time = { .life = 3600, .rekey = 3000, .jitter = 300 } }
            };
            child_cfg_t *child_cfg = child_cfg_create(child_name_str, &child_cfg_data);
            if (esp_proposal) {
                child_cfg->add_proposal(child_cfg, proposal_create_from_string(PROTO_ESP, esp_proposal));
            } else {
                child_cfg->add_proposal(child_cfg, proposal_create_default_aead(PROTO_ESP));
                child_cfg->add_proposal(child_cfg, proposal_create_default(PROTO_ESP));
            }
            child_cfg->add_traffic_selector(child_cfg, TRUE, traffic_selector_create_from_cidr(local_ts_str, 0, 0, 65535));
            child_cfg->add_traffic_selector(child_cfg, FALSE, traffic_selector_create_from_cidr(remote_ts_str, 0, 0, 65535));
            peer_cfg->add_child_cfg(peer_cfg, child_cfg);

            free(child_name_str);
            free(local_ts_str);
            free(remote_ts_str);
        }
    }

    child_cfg_t *child_cfg_first = NULL;
    enumerator_t *enumerator = peer_cfg->create_child_cfg_enumerator(peer_cfg);
    if (enumerator->enumerate(enumerator, &child_cfg_first)) {
        // child_cfg_first is obtained, get a reference if initiate needs its own
        // For controller->initiate, it manages references internally.
    }
    enumerator->destroy(enumerator);

    status_t status = charon->controller->initiate(charon->controller, peer_cfg,
                               child_cfg_first, /* child_cfg_first can be NULL */
                               controller_cb_empty, NULL,
                               0, /* Temporarily using 0 for log level */
                               0, FALSE);
    if (status == SUCCESS) {
        DBG1(DBG_LIB, "IPsec config applied and SA initiated successfully: %s", conn_name);
    } else {
        DBG1(DBG_LIB, "Failed to initiate SA for %s, status: %d", conn_name, status);
        // peer_cfg might need explicit destruction if initiate did not consume it
        // However, controller's job usually handles this.
        // If child_cfg_first was ref'd, it might also need release here on failure.
    }
    free(conn_name);
    free(local_addr);
    free(remote_addr);
    cJSON_Delete(root);
}

// 외부의 요청에 따라 특정 IKE_SA에 대해 DPD (Dead Peer Detection) Task를 시작하는 함수입니다.
static void start_dpd(const char *ike_name_const)
{
    char *ike_name = strdup(ike_name_const);
    // 주어진 이름으로 IKE_SA를 찾습니다. (IKEv2 SA만 대상으로 함)
    ike_sa_t *ike_sa = charon->ike_sa_manager->checkout_by_name(
        charon->ike_sa_manager, ike_name, FALSE);

    // IKE_SA를 찾았으면
    if (ike_sa) {
        // DPD를 시작하는 Task (dpd_initiator_create)를 생성합니다.
        task_t *dpd_task = (task_t*)ike_dpd_create(TRUE);
        // 생성된 DPD Task를 해당 IKE_SA의 작업 큐에 추가합니다.
        ike_sa->queue_task(ike_sa, dpd_task);
        // IKE_SA 사용이 끝났으므로 참조 카운터를 감소시킵니다.
        charon->ike_sa_manager->checkin(charon->ike_sa_manager, ike_sa);
        // DPD Task 추가 성공 로그를 출력합니다.
        DBG1(DBG_LIB, "DPD task queued successfully: %s", ike_name);
    } else {
        // 해당 이름의 IKE_SA를 찾지 못했으면 에러 로그를 출력합니다.
        DBG1(DBG_LIB, "IKE_SA not found for DPD: %s", ike_name);
    }
    free(ike_name);
}

//-------------------- SAD/SPD 이벤트 hook 콜백 함수들 --------------------

/**
 * SAD (Security Association Database)에 새로운 SA가 추가될 때 호출되는 콜백 함수입니다.
 * @param listener 이 콜백을 호출한 kernel_listener_t 객체입니다.
 * @param id 추가된 SA의 식별자 정보 (SPI, 프로토콜, IP 주소 등)를 담고 있는 구조체입니다.
 * @param sa 추가된 SA의 상세 정보 (암호화 키, 알고리즘 등)를 담고 있는 구조체입니다.
 * @return 이벤트 처리가 성공적으로 완료되었으면 TRUE를 반환합니다.
 */
/*static bool sad_add(kernel_listener_t *listener, kernel_ipsec_sa_id_t *id, kernel_ipsec_sa_t *sa)
{
    // "SAD_ADD" 이벤트 타입과 함께 SA 식별자 및 상세 정보를 JSON으로 변환하여 외부로 전송합니다.
    // sa 매개변수는 여기서는 사용되지 않지만, 필요시 SA의 더 상세한 정보를 추출하는 데 사용할 수 있습니다.
    //send_sad_event("SAD_ADD", id, sa); // Temporarily commented out
    // 이벤트 처리가 성공적으로 완료되었음을 strongSwan 커널에 알립니다.
    return TRUE;
}*/

/**
 * SAD에서 SA가 삭제될 때 호출되는 콜백 함수입니다.
 * @param listener 이 콜백을 호출한 kernel_listener_t 객체입니다.
 * @param id 삭제된 SA의 식별자 정보를 담고 있는 구조체입니다.
 * @return 이벤트 처리가 성공적으로 완료되었으면 TRUE를 반환합니다.
 */
/*static bool sad_delete(kernel_listener_t *listener, kernel_ipsec_sa_id_t *id)
{
    // "SAD_DELETE" 이벤트 타입과 함께 SA 식별자 정보를 JSON으로 변환하여 외부로 전송합니다.
    // SA가 이미 삭제된 후이므로, sa 객체는 NULL로 전달합니다.
    //send_sad_event("SAD_DELETE", id, NULL); // Temporarily commented out
    // 이벤트 처리가 성공적으로 완료되었음을 알립니다.
    return TRUE;
}*/

/**
 * SPD (Security Policy Database)에 새로운 정책이 추가될 때 호출되는 콜백 함수입니다.
 * @param listener 이 콜백을 호출한 kernel_listener_t 객체입니다.
 * @param id 추가된 정책의 식별자 정보 (reqid, 프로토콜, IP 선택자, 방향 등)를 담고 있는 구조체입니다.
 * @param policy 추가된 정책의 상세 정보 (적용 규칙, 우선순위 등)를 담고 있는 구조체입니다.
 * @return 이벤트 처리가 성공적으로 완료되었으면 TRUE를 반환합니다.
 */
/*static bool spd_add(kernel_listener_t *listener, kernel_ipsec_policy_id_t *id, kernel_ipsec_policy_t *policy)
{
    // "SPD_ADD" 이벤트 타입과 함께 정책 식별자 및 상세 정보를 JSON으로 변환하여 외부로 전송합니다.
    // policy 매개변수는 여기서는 사용되지 않지만, 필요시 정책의 더 상세한 정보를 추출하는 데 사용할 수 있습니다.
    //send_spd_event("SPD_ADD", id, policy); // Temporarily commented out
    // 이벤트 처리가 성공적으로 완료되었음을 알립니다.
    return TRUE;
}*/

/**
 * SPD에서 정책이 삭제될 때 호출되는 콜백 함수입니다.
 * @param listener 이 콜백을 호출한 kernel_listener_t 객체입니다.
 * @param id 삭제된 정책의 식별자 정보를 담고 있는 구조체입니다.
 * @return 이벤트 처리가 성공적으로 완료되었으면 TRUE를 반환합니다.
 */
/*static bool spd_delete(kernel_listener_t *listener, kernel_ipsec_policy_id_t *id)
{
    // "SPD_DELETE" 이벤트 타입과 함께 정책 식별자 정보를 JSON으로 변환하여 외부로 전송합니다.
    // 정책이 이미 삭제된 후이므로, policy 객체는 NULL로 전달합니다.
    //send_spd_event("SPD_DELETE", id, NULL); // Temporarily commented out
    // 이벤트 처리가 성공적으로 완료되었음을 알립니다.
    return TRUE;
}*/

/**
 * SAD/SPD 변경 이벤트를 감지하기 위한 kernel_listener_t 객체를 생성하고 등록하는 함수입니다.
 * @param this 플러그인의 내부 상태를 저장하는 private_extsock_plugin_t 구조체에 대한 포인터입니다.
 */
/*static void register_kernel_listener(private_extsock_plugin_t *this)
{
    // SAD/SPD 이벤트 발생 시 호출될 콜백 함수들을 지정하여 kernel_listener_t 객체를 생성합니다.
    // 사용하지 않는 이벤트 콜백은 NULL로 설정합니다.
    //this->listener = kernel_listener_create(
    //    sad_add, sad_delete, spd_add, spd_delete, // 사용할 콜백 함수들
    //    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL // 사용하지 않는 콜백들
    //);
    // 생성된 리스너 객체를 strongSwan 커널 인터페이스에 추가하여 이벤트 수신을 시작합니다.
    //charon->kernel->add_listener(charon->kernel, this->listener);
}*/

/**
 * 등록된 kernel_listener_t 객체를 해제하고 strongSwan 커널에서 제거하는 함수입니다.
 * @param this 플러그인의 내부 상태를 저장하는 private_extsock_plugin_t 구조체에 대한 포인터입니다.
 */
/*static void unregister_kernel_listener(private_extsock_plugin_t *this)
{
    // 리스너 객체가 유효한 경우(NULL이 아닌 경우)에만 해제 작업을 수행합니다.
    if (this->listener) {
        // strongSwan 커널 인터페이스에서 리스너를 제거합니다.
        //charon->kernel->remove_listener(charon->kernel, this->listener);
        // 리스너 객체 자체의 메모리를 해제합니다.
        //this->listener->destroy(this->listener); // API 변경 가능성
        // 해제된 리스너 포인터를 NULL로 설정하여 dangling pointer 문제를 방지합니다.
        //this->listener = NULL;
    }
}*/

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
    // 주소 패밀리를 유닉스 도메인으로 설정합니다.
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
    // 커널 리스너 포인터를 NULL로 초기화합니다.
    this->listener = NULL;

    // 플러그인의 공개 인터페이스(plugin_t)에 소멸자 함수(extsock_plugin_destroy)를 등록합니다.
    // strongSwan이 플러그인을 언로드할 때 이 함수가 호출됩니다.
    // (void*) 캐스팅은 함수 포인터 타입 일치를 위함입니다.
    this->public.destroy = (void*)extsock_plugin_destroy;
    // 플러그인 이름을 설정합니다 (디버깅 및 로깅에 사용됨).
    this->public.get_name = extsock_plugin_get_name;
    // 플러그인 기능 함수를 설정합니다.
    this->public.get_features = extsock_plugin_get_features;

    // SAD/SPD 변경 이벤트를 수신하기 위해 커널 리스너를 등록합니다.
    //register_kernel_listener(this); // Temporarily commented out

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
        //unregister_kernel_listener(this);
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
    //unregister_kernel_listener(this); // Temporarily commented out

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
 