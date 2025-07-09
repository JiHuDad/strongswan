/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_socket_adapter.h"
#include "../../usecases/extsock_config_usecase.h"
#include "../../common/extsock_common.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <threading/thread.h>
#include <threading/mutex.h>

typedef struct private_extsock_socket_adapter_t private_extsock_socket_adapter_t;

/**
 * 소켓 어댑터 내부 구조체
 */
struct private_extsock_socket_adapter_t {
    
    /**
     * 공개 인터페이스
     */
    extsock_socket_adapter_t public;
    
    /**
     * 명령 처리기
     */
    extsock_command_handler_t *command_handler;

    /**
     * 설정 유스케이스
     */
    extsock_config_usecase_t *cfg_usecase;
    
    /**
     * 서버 소켓
     */
    int server_socket;
    
    /**
     * 클라이언트 소켓
     */
    int client_socket;
    
    /**
     * 소켓 실행 상태
     */
    bool running;
    
    /**
     * 뮤텍스
     */
    mutex_t *mutex;
};

/**
 * 소켓 스레드 함수 (기존 socket_thread 함수에서 이동)
 */
static void* socket_thread_function(void *data)
{
    private_extsock_socket_adapter_t *this = (private_extsock_socket_adapter_t*)data;
    struct sockaddr_un addr;
    char buffer[4096];
    
    // 소켓 생성
    this->server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->server_socket == -1) {
        EXTSOCK_DBG(1, "Failed to create socket: %s", strerror(errno));
        return NULL;
    }
    
    // 소켓 주소 설정
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    // 기존 소켓 파일 제거
    unlink(SOCKET_PATH);
    
    // 바인드
    if (bind(this->server_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        EXTSOCK_DBG(1, "Failed to bind socket: %s", strerror(errno));
        close(this->server_socket);
        return NULL;
    }
    
    // 리슨
    if (listen(this->server_socket, 1) == -1) {
        EXTSOCK_DBG(1, "Failed to listen on socket: %s", strerror(errno));
        close(this->server_socket);
        return NULL;
    }
    
    EXTSOCK_DBG(1, "Socket server listening on %s", SOCKET_PATH);
    this->running = TRUE;
    
    while (this->running) {
        // 클라이언트 연결 수락
        this->client_socket = accept(this->server_socket, NULL, NULL);
        if (this->client_socket == -1) {
            if (this->running) {
                EXTSOCK_DBG(1, "Failed to accept connection: %s", strerror(errno));
            }
            continue;
        }
        
        EXTSOCK_DBG(2, "Client connected");
        
        // MEDIUM PRIORITY: 스레드 안전성 - 뮤텍스로 클라이언트 소켓 보호
        if (this->mutex) {
            this->mutex->lock(this->mutex);
        }
        
        // 데이터 수신 및 처리
        while (this->running) {
            ssize_t bytes_received = recv(this->client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    EXTSOCK_DBG(2, "Client disconnected");
                } else {
                    EXTSOCK_DBG(1, "Receive error: %s", strerror(errno));
                }
                break;
            }
            
            buffer[bytes_received] = '\0';
            EXTSOCK_DBG(2, "Received command: %s", buffer);
            
            // 명령 처리
            if (this->command_handler && this->cfg_usecase) {
                this->command_handler->handle_command(this->cfg_usecase, buffer);
            }
        }
        
        close(this->client_socket);
        this->client_socket = -1;
        
        if (this->mutex) {
            this->mutex->unlock(this->mutex);
        }
    }
    
    return NULL;
}

METHOD(extsock_event_publisher_t, publish_event, extsock_error_t,
    private_extsock_socket_adapter_t *this, const char *event_json)
{
    // HIGH PRIORITY: NULL 체크 강화
    EXTSOCK_CHECK_NULL_RET(this, EXTSOCK_ERROR_CONFIG_INVALID);
    EXTSOCK_CHECK_NULL_RET(event_json, EXTSOCK_ERROR_CONFIG_INVALID);
    
    if (!this->mutex) {
        EXTSOCK_DBG(1, "Mutex not available for socket operations");
        return EXTSOCK_ERROR_STRONGSWAN_API;
    }
    
    this->mutex->lock(this->mutex);
    
    // MEDIUM PRIORITY: 스레드 안전성 강화
    if (this->client_socket != -1) {
        // SIGPIPE 방지를 위한 MSG_NOSIGNAL 플래그 사용
        ssize_t sent = send(this->client_socket, event_json, strlen(event_json), MSG_NOSIGNAL);
        if (sent == -1) {
            if (errno == EPIPE || errno == ECONNRESET) {
                EXTSOCK_DBG(2, "Client disconnected during send, closing socket");
                close(this->client_socket);
                this->client_socket = -1;
            } else {
                EXTSOCK_DBG(1, "Failed to send event: %s", strerror(errno));
            }
            this->mutex->unlock(this->mutex);
            return EXTSOCK_ERROR_STRONGSWAN_API;
        }
        EXTSOCK_DBG(2, "Event sent: %s", event_json);
    } else {
        EXTSOCK_DBG(2, "No client connected, event not sent");
    }
    
    this->mutex->unlock(this->mutex);
    return EXTSOCK_SUCCESS;
}

METHOD(extsock_event_publisher_t, publish_tunnel_event, extsock_error_t,
    private_extsock_socket_adapter_t *this, const char *tunnel_event_json)
{
    return this->public.event_publisher.publish_event(&this->public.event_publisher, tunnel_event_json);
}

METHOD(extsock_event_publisher_t, destroy_publisher, void,
    private_extsock_socket_adapter_t *this)
{
    // 이벤트 발행자는 어댑터의 일부이므로 별도 해제 불필요
}

METHOD(extsock_socket_adapter_t, send_event, extsock_error_t,
    private_extsock_socket_adapter_t *this, const char *event_json)
{
    return this->public.event_publisher.publish_event(&this->public.event_publisher, event_json);
}

METHOD(extsock_socket_adapter_t, start_listening, thread_t *,
    private_extsock_socket_adapter_t *this)
{
    return thread_create(socket_thread_function, this);
}

METHOD(extsock_socket_adapter_t, stop_listening, void,
    private_extsock_socket_adapter_t *this)
{
    // MEDIUM PRIORITY: 안전한 소켓 종료
    if (!this) return;
    
    this->running = FALSE;
    
    if (this->mutex) {
        this->mutex->lock(this->mutex);
    }
    
    if (this->server_socket != -1) {
        close(this->server_socket);
        this->server_socket = -1;
    }
    if (this->client_socket != -1) {
        close(this->client_socket);
        this->client_socket = -1;
    }
    
    if (this->mutex) {
        this->mutex->unlock(this->mutex);
    }
}

METHOD(extsock_socket_adapter_t, destroy, void,
    private_extsock_socket_adapter_t *this)
{
    if (!this) return;
    
    this->public.stop_listening(&this->public);
    EXTSOCK_SAFE_DESTROY(this->mutex);
    free(this);
}

/**
 * 소켓 어댑터 생성
 */
extsock_socket_adapter_t *extsock_socket_adapter_create(
    extsock_config_usecase_t *cfg_usecase)
{
    private_extsock_socket_adapter_t *this;

    // cfg_usecase에서 command_handler 가져오기
    extsock_command_handler_t *command_handler = cfg_usecase->get_command_handler(cfg_usecase);
    if (!command_handler) {
        EXTSOCK_DBG(1, "Failed to get command handler from config usecase");
        return NULL;
    }

    INIT(this,
        .public = {
            .event_publisher = {
                .publish_event = _publish_event,
                .publish_tunnel_event = _publish_tunnel_event,
                .destroy = _destroy_publisher,
            },
            .send_event = _send_event,
            .start_listening = _start_listening,
            .stop_listening = _stop_listening,
            .destroy = _destroy,
        },
        .command_handler = command_handler,
        .server_socket = -1,
        .client_socket = -1,
        .running = FALSE,
        .mutex = mutex_create(MUTEX_TYPE_DEFAULT),
        .cfg_usecase = cfg_usecase,
    );

    return &this->public;
} 