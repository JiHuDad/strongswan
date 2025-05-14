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

// Forward declarations
static kernel_listener_t* kernel_listener_create(
    bool (*acquire)(kernel_listener_t *this, uint32_t reqid, kernel_acquire_data_t *data),
    bool (*expire)(kernel_listener_t *this, uint8_t protocol, uint32_t spi, host_t *dst, bool hard),
    bool (*mapping)(kernel_listener_t *this, uint8_t protocol, uint32_t spi, host_t *dst, host_t *remote),
    bool (*migrate)(kernel_listener_t *this, uint32_t reqid, traffic_selector_t *src_ts, traffic_selector_t *dst_ts, policy_dir_t direction, host_t *local, host_t *remote),
    bool (*roam)(kernel_listener_t *this, bool address),
    bool (*tun)(kernel_listener_t *this, tun_device_t *tun, bool created)
);

// Forward declarations for event sending functions
static void send_event_to_external(const char *event_json);
static void send_sad_event(const char *event_type, kernel_ipsec_sa_id_t *id);
static void send_spd_event(const char *event_type, kernel_ipsec_policy_id_t *id);
// Forward declaration for ts_to_string utility
static void ts_to_string(traffic_selector_t *ts, char *buf, size_t buflen);

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

// Function declarations
static void start_dpd(const char *ike_sa_name);
static void apply_ipsec_config(private_extsock_plugin_t *this, const char *config_json);

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

static void apply_ipsec_config(private_extsock_plugin_t *this, const char *config_json)
{
    DBG1(DBG_LIB, "apply_ipsec_config: received config: %s", config_json);
    cJSON *root = cJSON_Parse(config_json);
    if (!root)
    {
        DBG1(DBG_LIB, "apply_ipsec_config: Failed to parse JSON");
        return;
    }
    // Example: parse basic fields (name, local, remote, auth, proposal)
    cJSON *name = cJSON_GetObjectItem(root, "name");
    cJSON *local = cJSON_GetObjectItem(root, "local");
    cJSON *remote = cJSON_GetObjectItem(root, "remote");
    if (!name || !local || !remote)
    {
        DBG1(DBG_LIB, "apply_ipsec_config: Missing required fields");
        cJSON_Delete(root);
        return;
    }
    // For demonstration, just log the parsed values
    DBG1(DBG_LIB, "apply_ipsec_config: name=%s, local=%s, remote=%s", name->valuestring, local->valuestring, remote->valuestring);
    // TODO: Actually create and install IKE/CHILD config using strongSwan APIs
    // This would involve creating peer_cfg_t, ike_cfg_t, child_cfg_t, etc.
    // For now, just log and clean up
    cJSON_Delete(root);
}

//-------------------- SAD/SPD 이벤트 hook 콜백 함수들 --------------------

/**
 * SAD (Security Association Database)에 새로운 SA가 추가될 때 호출되는 콜백 함수입니다.
 * @param listener 이 콜백을 호출한 kernel_listener_t 객체입니다.
 * @param id 추가된 SA의 식별자 정보 (SPI, 프로토콜, IP 주소 등)를 담고 있는 구조체입니다.
 * @return 이벤트 처리가 성공적으로 완료되었으면 TRUE를 반환합니다.
 */
static bool acquire(kernel_listener_t *this, uint32_t reqid, kernel_acquire_data_t *data)
{
    if (data && data->src && data->dst) {
        kernel_ipsec_policy_id_t policy_id = {
            .dir = POLICY_OUT,  // Default to outbound policy
            .src_ts = data->src,
            .dst_ts = data->dst,
            .if_id = 0,         // No specific interface
            .interface = NULL,  // No specific interface
            .label = data->label
        };
        send_spd_event("SPD_ADD", &policy_id);
    }
    return TRUE;
}

static bool expire(kernel_listener_t *this, uint8_t protocol, uint32_t spi, host_t *dst, bool hard)
{
    kernel_ipsec_sa_id_t sa_id = {
        .spi = spi,
        .proto = protocol,
        .dst = dst,
        .src = NULL,  // Source address not available in expire event
        .mark = {0, 0},
        .if_id = 0
    };
    send_sad_event("SAD_DELETE", &sa_id);
    return TRUE;
}

/**
 * SAD/SPD 변경 이벤트를 감지하기 위한 kernel_listener_t 객체를 생성하고 등록하는 함수입니다.
 * @param this 플러그인의 내부 상태를 저장하는 private_extsock_plugin_t 구조체에 대한 포인터입니다.
 */
static void register_kernel_listener(private_extsock_plugin_t *this)
{
    // SAD/SPD 이벤트 발생 시 호출될 콜백 함수들을 지정하여 kernel_listener_t 객체를 생성합니다.
    // 사용하지 않는 이벤트 콜백은 NULL로 설정합니다.
    this->listener = kernel_listener_create(
        acquire,  // acquire
        expire,   // expire
        NULL,     // mapping
        NULL,     // migrate
        NULL,     // roam
        NULL      // tun
    );

    // 생성된 리스너 객체를 strongSwan 커널 인터페이스에 추가하여 이벤트 수신을 시작합니다.
    charon->kernel->add_listener(charon->kernel, this->listener);
}

/**
 * 등록된 kernel_listener_t 객체를 해제하고 strongSwan 커널에서 제거하는 함수입니다.
 * @param this 플러그인의 내부 상태를 저장하는 private_extsock_plugin_t 구조체에 대한 포인터입니다.
 */
static void unregister_kernel_listener(private_extsock_plugin_t *this)
{
    // 리스너 객체가 유효한 경우(NULL이 아닌 경우)에만 해제 작업을 수행합니다.
    if (this->listener) {
        // strongSwan 커널 인터페이스에서 리스너를 제거합니다.
        charon->kernel->remove_listener(charon->kernel, this->listener);
        // 리스너 객체 자체의 메모리를 해제합니다.
        // this->listener->destroy(this->listener); // destroy 멤버가 없으므로 제거
        this->listener = NULL;
    }
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

    // SAD/SPD 변경 이벤트를 수신하기 위해 커널 리스너를 등록합니다.
    register_kernel_listener(this);
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
        unregister_kernel_listener(this);
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
    unregister_kernel_listener(this);

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

// SAD (Security Association Database) 및 SPD (Security Policy Database) 변경 이벤트 정보를 JSON 문자열로 만들어 외부 프로그램에 전송하는 함수입니다.
static void send_sad_event(const char *event_type, kernel_ipsec_sa_id_t *id)
{
    char buf[512];
    char src_str[64], dst_str[64];
    host_t *src_host = host_create_from_sockaddr((struct sockaddr*)&id->src);
    host_t *dst_host = host_create_from_sockaddr((struct sockaddr*)&id->dst);
    snprintf(src_str, sizeof(src_str), "%H", src_host);
    snprintf(dst_str, sizeof(dst_str), "%H", dst_host);
    snprintf(buf, sizeof(buf),
        "{\"event\":\"%s\",\"spi\":%u,\"proto\":%u,\"src\":\"%s\",\"dst\":\"%s\"}",
        event_type,
        id->spi,
        id->proto,
        src_str,
        dst_str);
    DESTROY_IF(src_host);
    DESTROY_IF(dst_host);
    send_event_to_external(buf);
}

static void send_spd_event(const char *event_type, kernel_ipsec_policy_id_t *id)
{
    char buf[512];
    char src_buf[128], dst_buf[128];
    ts_to_string(id->src_ts, src_buf, sizeof(src_buf));
    ts_to_string(id->dst_ts, dst_buf, sizeof(dst_buf));
    const char *dir_str = (id->dir == POLICY_IN) ? "in" :
                         (id->dir == POLICY_OUT) ? "out" :
                         "fwd";
    snprintf(buf, sizeof(buf),
        "{\"event\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\",\"dir\":\"%s\"}",
        event_type,
        src_buf,
        dst_buf,
        dir_str);
    send_event_to_external(buf);
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

// Forward declarations
static kernel_listener_t* kernel_listener_create(
    bool (*acquire)(kernel_listener_t *this, uint32_t reqid, kernel_acquire_data_t *data),
    bool (*expire)(kernel_listener_t *this, uint8_t protocol, uint32_t spi, host_t *dst, bool hard),
    bool (*mapping)(kernel_listener_t *this, uint8_t protocol, uint32_t spi, host_t *dst, host_t *remote),
    bool (*migrate)(kernel_listener_t *this, uint32_t reqid, traffic_selector_t *src_ts, traffic_selector_t *dst_ts, policy_dir_t direction, host_t *local, host_t *remote),
    bool (*roam)(kernel_listener_t *this, bool address),
    bool (*tun)(kernel_listener_t *this, tun_device_t *tun, bool created)
);

// kernel_listener_create 함수 구현
static kernel_listener_t* kernel_listener_create(
    bool (*acquire)(kernel_listener_t *this, uint32_t reqid, kernel_acquire_data_t *data),
    bool (*expire)(kernel_listener_t *this, uint8_t protocol, uint32_t spi, host_t *dst, bool hard),
    bool (*mapping)(kernel_listener_t *this, uint8_t protocol, uint32_t spi, host_t *dst, host_t *remote),
    bool (*migrate)(kernel_listener_t *this, uint32_t reqid, traffic_selector_t *src_ts, traffic_selector_t *dst_ts, policy_dir_t direction, host_t *local, host_t *remote),
    bool (*roam)(kernel_listener_t *this, bool address),
    bool (*tun)(kernel_listener_t *this, tun_device_t *tun, bool created))
{
    kernel_listener_t *listener = malloc(sizeof(kernel_listener_t));
    if (listener) {
        listener->acquire = acquire;
        listener->expire = expire;
        listener->mapping = mapping;
        listener->migrate = migrate;
        listener->roam = roam;
        listener->tun = tun;
    }
    return listener;
} 