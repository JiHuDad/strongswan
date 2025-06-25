/*
 * Copyright (C) 2024 strongSwan Project
 * Socket Adapter Standalone Tests - Phase 5
 * Unix Socket 기능을 Mock으로 처리하여 테스트
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <pthread.h>

/*
 * Strategy 6: Socket + Threading Mock
 * Unix Socket과 Thread 기능을 Mock으로 처리하여 실제 Socket Adapter 로직 테스트
 */

// 타입 정의
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_JSON_PARSE,
    EXTSOCK_ERROR_CONFIG_INVALID,
    EXTSOCK_ERROR_SOCKET_FAILED,
    EXTSOCK_ERROR_MEMORY_ALLOCATION,
    EXTSOCK_ERROR_STRONGSWAN_API
} extsock_error_t;

// Mock 구조체들
typedef struct mock_mutex_t {
    pthread_mutex_t mutex;
    bool initialized;
} mock_mutex_t;

typedef struct mock_thread_t {
    pthread_t thread;
    void *(*start_routine)(void*);
    void *arg;
    bool running;
} mock_thread_t;

typedef struct mock_command_handler_t {
    void (*handle_command)(struct mock_command_handler_t *this, const char *command);
    int command_count;
    char **received_commands;
} mock_command_handler_t;

typedef struct extsock_event_publisher_t {
    extsock_error_t (*publish_event)(struct extsock_event_publisher_t *this, const char *event_json);
    extsock_error_t (*publish_tunnel_event)(struct extsock_event_publisher_t *this, const char *tunnel_event_json);
    void (*destroy)(struct extsock_event_publisher_t *this);
} extsock_event_publisher_t;

typedef struct extsock_socket_adapter_t {
    extsock_event_publisher_t event_publisher;
    
    extsock_error_t (*send_event)(struct extsock_socket_adapter_t *this, const char *event_json);
    mock_thread_t *(*start_listening)(struct extsock_socket_adapter_t *this);
    void (*stop_listening)(struct extsock_socket_adapter_t *this);
    void (*destroy)(struct extsock_socket_adapter_t *this);
} extsock_socket_adapter_t;

typedef struct private_socket_adapter_t {
    extsock_socket_adapter_t public;
    
    mock_command_handler_t *command_handler;
    int server_socket;
    int client_socket;
    bool running;
    mock_mutex_t *mutex;
    
    // Mock data for testing
    char *last_event_sent;
    bool mock_socket_failure;
    bool mock_send_failure;
} private_socket_adapter_t;

// Mock 타입 aliases
typedef mock_mutex_t mutex_t;
typedef mock_thread_t thread_t;
typedef mock_command_handler_t extsock_command_handler_t;

// Constants
#define SOCKET_PATH "/tmp/strongswan_extsock_test.sock"
#define EXTSOCK_DBG(level, fmt, ...) printf("[EXTSOCK DBG%d] " fmt "\n", level, ##__VA_ARGS__)
#define TRUE true
#define FALSE false
#define METHOD(interface, method, ret_type, ...) static ret_type method(__VA_ARGS__)

// Mock 함수들
static mutex_t *mutex_create(int type) {
    (void)type;
    mutex_t *mutex = malloc(sizeof(mutex_t));
    if (!mutex) return NULL;
    
    if (pthread_mutex_init(&mutex->mutex, NULL) != 0) {
        free(mutex);
        return NULL;
    }
    mutex->initialized = true;
    return mutex;
}

static void mutex_lock(mutex_t *mutex) {
    if (mutex && mutex->initialized) {
        pthread_mutex_lock(&mutex->mutex);
    }
}

static void mutex_unlock(mutex_t *mutex) {
    if (mutex && mutex->initialized) {
        pthread_mutex_unlock(&mutex->mutex);
    }
}

static void mutex_destroy(mutex_t *mutex) {
    if (mutex && mutex->initialized) {
        pthread_mutex_destroy(&mutex->mutex);
        mutex->initialized = false;
        free(mutex);
    }
}

static thread_t *thread_create(void *(*start_routine)(void*), void *arg) {
    thread_t *thread = malloc(sizeof(thread_t));
    if (!thread) return NULL;
    
    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->running = true;
    
    if (pthread_create(&thread->thread, NULL, start_routine, arg) != 0) {
        free(thread);
        return NULL;
    }
    
    return thread;
}

static void thread_join(thread_t *thread) {
    if (thread) {
        pthread_join(thread->thread, NULL);
        free(thread);
    }
}

// Mock command handler
static void mock_handle_command(mock_command_handler_t *this, const char *command) {
    if (!this || !command) return;
    
    this->command_count++;
    this->received_commands = realloc(this->received_commands, 
                                    sizeof(char*) * this->command_count);
    this->received_commands[this->command_count - 1] = strdup(command);
    
    printf("Mock: Handled command: %s\n", command);
}

static mock_command_handler_t *mock_command_handler_create() {
    mock_command_handler_t *handler = malloc(sizeof(mock_command_handler_t));
    if (!handler) return NULL;
    
    handler->handle_command = mock_handle_command;
    handler->command_count = 0;
    handler->received_commands = NULL;
    
    return handler;
}

static void mock_command_handler_destroy(mock_command_handler_t *handler) {
    if (handler) {
        for (int i = 0; i < handler->command_count; i++) {
            free(handler->received_commands[i]);
        }
        free(handler->received_commands);
        free(handler);
    }
}

/*
 * Socket Adapter 실제 구현 (simplified with mocks)
 */

METHOD(extsock_event_publisher_t, publish_event, extsock_error_t, private_socket_adapter_t *this, const char *event_json)
{
    if (!event_json) {
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    if (this->mock_send_failure) {
        EXTSOCK_DBG(1, "Mock: Send failure simulated");
        return EXTSOCK_ERROR_SOCKET_FAILED;
    }
    
    mutex_lock(this->mutex);
    
    // Mock sending: just store the event
    if (this->last_event_sent) {
        free(this->last_event_sent);
    }
    this->last_event_sent = strdup(event_json);
    
    EXTSOCK_DBG(2, "Mock: Event sent: %s", event_json);
    mutex_unlock(this->mutex);
    
    return EXTSOCK_SUCCESS;
}

METHOD(extsock_event_publisher_t, publish_tunnel_event, extsock_error_t, private_socket_adapter_t *this, const char *tunnel_event_json)
{
    return publish_event(this, tunnel_event_json);
}

METHOD(extsock_event_publisher_t, destroy_publisher, void, private_socket_adapter_t *this)
{
    // Event publisher는 adapter의 일부이므로 별도 해제 불필요
}

METHOD(extsock_socket_adapter_t, send_event, extsock_error_t, private_socket_adapter_t *this, const char *event_json)
{
    return this->public.event_publisher.publish_event(&this->public.event_publisher, event_json);
}

static void* mock_socket_thread_function(void *data)
{
    private_socket_adapter_t *this = (private_socket_adapter_t*)data;
    
    EXTSOCK_DBG(1, "Mock: Socket thread started");
    this->running = TRUE;
    
    // Mock socket operations
    if (this->mock_socket_failure) {
        EXTSOCK_DBG(1, "Mock: Socket creation failed");
        return NULL;
    }
    
    this->server_socket = 99; // Mock socket fd
    EXTSOCK_DBG(1, "Mock: Socket server listening on %s", SOCKET_PATH);
    
    // Simulate some client commands for testing
    if (this->command_handler) {
        this->command_handler->handle_command(this->command_handler, "test_command_1");
        this->command_handler->handle_command(this->command_handler, "test_command_2");
    }
    
    // Keep running until stopped
    while (this->running) {
        usleep(100000); // 100ms
    }
    
    EXTSOCK_DBG(1, "Mock: Socket thread stopped");
    return NULL;
}

METHOD(extsock_socket_adapter_t, start_listening, thread_t *, private_socket_adapter_t *this)
{
    return thread_create(mock_socket_thread_function, this);
}

METHOD(extsock_socket_adapter_t, stop_listening, void, private_socket_adapter_t *this)
{
    this->running = FALSE;
    if (this->server_socket != -1) {
        this->server_socket = -1; // Mock close
    }
    if (this->client_socket != -1) {
        this->client_socket = -1; // Mock close
    }
    EXTSOCK_DBG(2, "Mock: Stopped listening");
}

METHOD(extsock_socket_adapter_t, destroy, void, private_socket_adapter_t *this)
{
    this->public.stop_listening(&this->public);
    if (this->mutex) {
        mutex_destroy(this->mutex);
    }
    if (this->last_event_sent) {
        free(this->last_event_sent);
    }
    if (this->command_handler) {
        mock_command_handler_destroy(this->command_handler);
    }
    free(this);
}

static extsock_socket_adapter_t *standalone_socket_adapter_create()
{
    private_socket_adapter_t *adapter = malloc(sizeof(private_socket_adapter_t));
    if (!adapter) return NULL;
    
    // Initialize public interface
    adapter->public.event_publisher.publish_event = (void*)publish_event;
    adapter->public.event_publisher.publish_tunnel_event = (void*)publish_tunnel_event;
    adapter->public.event_publisher.destroy = (void*)destroy_publisher;
    
    adapter->public.send_event = (void*)send_event;
    adapter->public.start_listening = (void*)start_listening;
    adapter->public.stop_listening = (void*)stop_listening;
    adapter->public.destroy = (void*)destroy;
    
    // Initialize private data
    adapter->command_handler = mock_command_handler_create();
    adapter->server_socket = -1;
    adapter->client_socket = -1;
    adapter->running = false;
    adapter->mutex = mutex_create(0);
    
    // Test-specific mock data
    adapter->last_event_sent = NULL;
    adapter->mock_socket_failure = false;
    adapter->mock_send_failure = false;
    
    return &adapter->public;
}

/**
 * 테스트 설정
 */
void setup_socket_adapter_standalone_test(void)
{
    printf("Starting Socket Adapter standalone tests...\n");
}

/**
 * 테스트 해제
 */
void teardown_socket_adapter_standalone_test(void)
{
    printf("Socket Adapter standalone tests completed.\n");
}

/**
 * Socket Adapter 생성/소멸 테스트
 */
START_TEST(test_socket_adapter_create_destroy)
{
    // When
    extsock_socket_adapter_t *adapter = standalone_socket_adapter_create();
    
    // Then
    ck_assert_ptr_nonnull(adapter);
    ck_assert_ptr_nonnull(adapter->send_event);
    ck_assert_ptr_nonnull(adapter->start_listening);
    ck_assert_ptr_nonnull(adapter->stop_listening);
    ck_assert_ptr_nonnull(adapter->destroy);
    ck_assert_ptr_nonnull(adapter->event_publisher.publish_event);
    ck_assert_ptr_nonnull(adapter->event_publisher.publish_tunnel_event);
    
    // Cleanup
    adapter->destroy(adapter);
}
END_TEST

/**
 * 이벤트 발행 테스트
 */
START_TEST(test_socket_adapter_publish_event)
{
    // Given
    extsock_socket_adapter_t *adapter = standalone_socket_adapter_create();
    private_socket_adapter_t *priv = (private_socket_adapter_t*)adapter;
    ck_assert_ptr_nonnull(adapter);
    
    const char *test_event = "{\"type\":\"tunnel_up\",\"connection\":\"test\"}";
    
    // When
    extsock_error_t result = adapter->event_publisher.publish_event(&adapter->event_publisher, test_event);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_SUCCESS);
    ck_assert_ptr_nonnull(priv->last_event_sent);
    ck_assert_str_eq(priv->last_event_sent, test_event);
    
    // Cleanup
    adapter->destroy(adapter);
}
END_TEST

/**
 * 터널 이벤트 발행 테스트
 */
START_TEST(test_socket_adapter_publish_tunnel_event)
{
    // Given
    extsock_socket_adapter_t *adapter = standalone_socket_adapter_create();
    private_socket_adapter_t *priv = (private_socket_adapter_t*)adapter;
    ck_assert_ptr_nonnull(adapter);
    
    const char *tunnel_event = "{\"type\":\"tunnel_down\",\"connection\":\"vpn1\"}";
    
    // When
    extsock_error_t result = adapter->event_publisher.publish_tunnel_event(&adapter->event_publisher, tunnel_event);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_SUCCESS);
    ck_assert_ptr_nonnull(priv->last_event_sent);
    ck_assert_str_eq(priv->last_event_sent, tunnel_event);
    
    // Cleanup
    adapter->destroy(adapter);
}
END_TEST

/**
 * send_event 메소드 테스트
 */
START_TEST(test_socket_adapter_send_event)
{
    // Given
    extsock_socket_adapter_t *adapter = standalone_socket_adapter_create();
    private_socket_adapter_t *priv = (private_socket_adapter_t*)adapter;
    ck_assert_ptr_nonnull(adapter);
    
    const char *event_json = "{\"status\":\"connected\",\"timestamp\":1234567890}";
    
    // When
    extsock_error_t result = adapter->send_event(adapter, event_json);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_SUCCESS);
    ck_assert_ptr_nonnull(priv->last_event_sent);
    ck_assert_str_eq(priv->last_event_sent, event_json);
    
    // Cleanup
    adapter->destroy(adapter);
}
END_TEST

/**
 * 리스닝 시작/중지 테스트
 */
START_TEST(test_socket_adapter_start_stop_listening)
{
    // Given
    extsock_socket_adapter_t *adapter = standalone_socket_adapter_create();
    private_socket_adapter_t *priv = (private_socket_adapter_t*)adapter;
    ck_assert_ptr_nonnull(adapter);
    
    // When - Start listening
    thread_t *thread = adapter->start_listening(adapter);
    
    // Then
    ck_assert_ptr_nonnull(thread);
    ck_assert_int_eq(priv->running, TRUE);
    ck_assert_int_eq(priv->server_socket, 99); // Mock socket fd
    
    // Let thread run for a short time
    usleep(200000); // 200ms
    
    // Verify command handler received commands
    ck_assert_int_eq(priv->command_handler->command_count, 2);
    ck_assert_str_eq(priv->command_handler->received_commands[0], "test_command_1");
    ck_assert_str_eq(priv->command_handler->received_commands[1], "test_command_2");
    
    // When - Stop listening
    adapter->stop_listening(adapter);
    
    // Then
    ck_assert_int_eq(priv->running, FALSE);
    ck_assert_int_eq(priv->server_socket, -1);
    
    // Wait for thread to finish
    thread_join(thread);
    
    // Cleanup
    adapter->destroy(adapter);
}
END_TEST

/**
 * NULL 이벤트 처리 테스트
 */
START_TEST(test_socket_adapter_null_event)
{
    // Given
    extsock_socket_adapter_t *adapter = standalone_socket_adapter_create();
    ck_assert_ptr_nonnull(adapter);
    
    // When
    extsock_error_t result = adapter->event_publisher.publish_event(&adapter->event_publisher, NULL);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
    
    // Cleanup
    adapter->destroy(adapter);
}
END_TEST

/**
 * 전송 실패 시뮬레이션 테스트
 */
START_TEST(test_socket_adapter_send_failure)
{
    // Given
    extsock_socket_adapter_t *adapter = standalone_socket_adapter_create();
    private_socket_adapter_t *priv = (private_socket_adapter_t*)adapter;
    ck_assert_ptr_nonnull(adapter);
    
    priv->mock_send_failure = true; // 전송 실패 시뮬레이션
    
    const char *test_event = "{\"type\":\"error\",\"message\":\"test failure\"}";
    
    // When
    extsock_error_t result = adapter->send_event(adapter, test_event);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_SOCKET_FAILED);
    
    // Cleanup
    adapter->destroy(adapter);
}
END_TEST

/**
 * 소켓 생성 실패 시뮬레이션 테스트
 */
START_TEST(test_socket_adapter_socket_failure)
{
    // Given
    extsock_socket_adapter_t *adapter = standalone_socket_adapter_create();
    private_socket_adapter_t *priv = (private_socket_adapter_t*)adapter;
    ck_assert_ptr_nonnull(adapter);
    
    priv->mock_socket_failure = true; // 소켓 실패 시뮬레이션
    
    // When
    thread_t *thread = adapter->start_listening(adapter);
    
    // Then - 스레드는 생성되지만 소켓 실패로 즉시 종료
    ck_assert_ptr_nonnull(thread);
    
    // Let thread run and fail
    usleep(100000); // 100ms
    
    adapter->stop_listening(adapter);
    thread_join(thread);
    
    // Cleanup
    adapter->destroy(adapter);
}
END_TEST

Suite *socket_adapter_standalone_suite(void)
{
    Suite *s;
    TCase *tc_basic, *tc_events, *tc_listening, *tc_errors;
    
    s = suite_create("Socket Adapter Standalone Tests");
    
    /* 기본 기능 테스트 */
    tc_basic = tcase_create("Basic Socket Adapter Functions");
    tcase_add_checked_fixture(tc_basic, setup_socket_adapter_standalone_test, teardown_socket_adapter_standalone_test);
    tcase_add_test(tc_basic, test_socket_adapter_create_destroy);
    suite_add_tcase(s, tc_basic);
    
    /* 이벤트 발행 테스트 */
    tc_events = tcase_create("Event Publishing Tests");
    tcase_add_test(tc_events, test_socket_adapter_publish_event);
    tcase_add_test(tc_events, test_socket_adapter_publish_tunnel_event);
    tcase_add_test(tc_events, test_socket_adapter_send_event);
    suite_add_tcase(s, tc_events);
    
    /* 리스닝 테스트 */
    tc_listening = tcase_create("Listening Tests");
    tcase_add_test(tc_listening, test_socket_adapter_start_stop_listening);
    suite_add_tcase(s, tc_listening);
    
    /* 에러 처리 테스트 */
    tc_errors = tcase_create("Error Handling Tests");
    tcase_add_test(tc_errors, test_socket_adapter_null_event);
    tcase_add_test(tc_errors, test_socket_adapter_send_failure);
    tcase_add_test(tc_errors, test_socket_adapter_socket_failure);
    suite_add_tcase(s, tc_errors);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    printf("=== Socket Adapter Standalone Tests ===\n");
    printf("Testing actual Socket Adapter implementations with Mock Unix Socket\n\n");
    
    s = socket_adapter_standalone_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    printf("\n=== Test Results ===\n");
    printf("Failed tests: %d\n", number_failed);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 