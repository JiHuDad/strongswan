/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Mock version of extsock_socket_adapter for adapter unit tests
 * TASK-008: Socket Adapter 실제 테스트
 * 
 * This is a simplified mock implementation that doesn't require actual socket operations
 * but provides the same interface for testing adapter layer functionality.
 */

#ifndef EXTSOCK_SOCKET_ADAPTER_MOCK_H_
#define EXTSOCK_SOCKET_ADAPTER_MOCK_H_

#include <stdbool.h>

// Mock types for testing (avoiding strongSwan dependencies)
typedef struct extsock_socket_adapter_t extsock_socket_adapter_t;
typedef struct extsock_config_usecase_t extsock_config_usecase_t;
typedef struct extsock_command_handler_t extsock_command_handler_t;
typedef struct extsock_event_publisher_t extsock_event_publisher_t;
typedef struct thread_t thread_t;

// Mock error type
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_SOCKET_CREATE = -1,
    EXTSOCK_ERROR_SOCKET_BIND = -2,
    EXTSOCK_ERROR_SOCKET_LISTEN = -3,
    EXTSOCK_ERROR_INVALID_PARAM = -4,
    EXTSOCK_ERROR_THREAD_CREATE = -5
} extsock_error_t;

/**
 * Mock Event Publisher structure (define before use)
 */
struct extsock_event_publisher_t {
    void (*publish_event)(extsock_event_publisher_t *this, const char *event);
    void (*destroy)(extsock_event_publisher_t *this);
};

/**
 * Mock Config Usecase structure
 */
struct extsock_config_usecase_t {
    char *config_data;
    bool is_valid;
};

/**
 * Mock Command Handler structure
 */
struct extsock_command_handler_t {
    void (*handle_command)(extsock_command_handler_t *this, const char *cmd);
    void (*destroy)(extsock_command_handler_t *this);
};

/**
 * Mock Thread structure for testing
 */
struct thread_t {
    int thread_id;
    bool is_running;
    char *name;
};

/**
 * Socket Adapter Mock Interface
 * 외부 소켓 통신을 시뮬레이션하는 Mock 어댑터
 */
struct extsock_socket_adapter_t {
    
    /**
     * 이벤트 발행자 인터페이스 구현 (Mock)
     */
    extsock_event_publisher_t event_publisher;
    
    /**
     * Mock 이벤트 전송
     *
     * @param this          인스턴스
     * @param event_json    전송할 이벤트 JSON
     * @return              성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*send_event)(extsock_socket_adapter_t *this,
                                 const char *event_json);
    
    /**
     * Mock 소켓 리스너 시작
     *
     * @param this      인스턴스
     * @return          Mock 소켓 스레드 인스턴스
     */
    thread_t* (*start_listening)(extsock_socket_adapter_t *this);
    
    /**
     * Mock 소켓 리스너 중지
     *
     * @param this      인스턴스
     */
    void (*stop_listening)(extsock_socket_adapter_t *this);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_socket_adapter_t *this);
};

/**
 * Mock Thread creation functions
 */
thread_t *mock_thread_create(const char *name);
void mock_thread_destroy(thread_t *thread);

/**
 * Mock Event Publisher creation functions
 */
extsock_event_publisher_t *mock_event_publisher_create(void);

/**
 * Mock Config Usecase creation functions
 */
extsock_config_usecase_t *mock_config_usecase_create(void);

/**
 * Mock Command Handler creation functions
 */
extsock_command_handler_t *mock_command_handler_create(void);

/**
 * Socket Adapter Mock 생성 (Mock version)
 *
 * @param cfg_usecase       Mock 설정 유스케이스
 * @return                  Mock 소켓 어댑터 인스턴스
 */
extsock_socket_adapter_t *extsock_socket_adapter_create(
    extsock_config_usecase_t *cfg_usecase);

#endif /** EXTSOCK_SOCKET_ADAPTER_MOCK_H_ @}*/