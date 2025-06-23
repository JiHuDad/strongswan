/*
 * Copyright (C) 2024 strongSwan Project
 * Unit tests for Socket Adapter
 */

#include <check.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "../adapters/socket/extsock_socket_adapter.h"
#include "../common/extsock_common.h"

#define TEST_SOCKET_PATH "/tmp/test_strongswan_extsock.sock"

static extsock_socket_adapter_t *socket_adapter;
static int test_client_fd;

/**
 * Mock Command Handler for testing
 */
typedef struct mock_command_handler_t {
    extsock_command_handler_t public;
    char *last_command;
    extsock_error_t return_error;
} mock_command_handler_t;

static extsock_error_t mock_handle_command(mock_command_handler_t *this, const char *command)
{
    free(this->last_command);
    this->last_command = strdup(command);
    return this->return_error;
}

static extsock_error_t mock_handle_dpd_command(mock_command_handler_t *this, const char *ike_sa_name)
{
    char *dpd_command;
    asprintf(&dpd_command, "START_DPD %s", ike_sa_name);
    free(this->last_command);
    this->last_command = dpd_command;
    return this->return_error;
}

static void mock_destroy(mock_command_handler_t *this)
{
    free(this->last_command);
    free(this);
}

static extsock_command_handler_t *create_mock_command_handler()
{
    mock_command_handler_t *mock;
    
    INIT(mock,
        .public = {
            .handle_command = (void*)mock_handle_command,
            .handle_dpd_command = (void*)mock_handle_dpd_command,
            .destroy = (void*)mock_destroy,
        },
        .last_command = NULL,
        .return_error = EXTSOCK_ERROR_NONE,
    );
    
    return &mock->public;
}

void setup_socket_adapter_test(void)
{
    extsock_command_handler_t *mock_handler = create_mock_command_handler();
    
    // 테스트용 소켓 경로 사용
    setenv("EXTSOCK_SOCKET_PATH", TEST_SOCKET_PATH, 1);
    
    socket_adapter = extsock_socket_adapter_create(mock_handler);
    ck_assert_ptr_nonnull(socket_adapter);
    
    test_client_fd = -1;
}

void teardown_socket_adapter_test(void)
{
    if (test_client_fd >= 0) {
        close(test_client_fd);
        test_client_fd = -1;
    }
    
    if (socket_adapter) {
        socket_adapter->destroy(socket_adapter);
        socket_adapter = NULL;
    }
    
    // 테스트 소켓 파일 정리
    unlink(TEST_SOCKET_PATH);
    unsetenv("EXTSOCK_SOCKET_PATH");
}

/**
 * 테스트 클라이언트 연결 생성
 */
static int create_test_client()
{
    int fd;
    struct sockaddr_un addr;
    
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, TEST_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    // 논블로킹으로 설정하여 테스트에서 블록되지 않도록 함
    fcntl(fd, F_SETFL, O_NONBLOCK);
    
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (errno != EINPROGRESS) {
            close(fd);
            return -1;
        }
    }
    
    return fd;
}

/**
 * 소켓 어댑터 생성 테스트
 */
START_TEST(test_socket_adapter_creation)
{
    // socket_adapter는 setup에서 생성됨
    ck_assert_ptr_nonnull(socket_adapter);
}
END_TEST

/**
 * 소켓 리스닝 시작 테스트
 */
START_TEST(test_start_listening)
{
    // When
    thread_t *thread = socket_adapter->start_listening(socket_adapter);
    
    // Then
    ck_assert_ptr_nonnull(thread);
    
    // 잠시 대기하여 소켓이 바인드되도록 함
    usleep(100000); // 100ms
    
    // 소켓 파일이 생성되었는지 확인
    ck_assert_int_eq(access(TEST_SOCKET_PATH, F_OK), 0);
    
    // 스레드 정리
    thread->cancel(thread);
    thread->join(thread);
}
END_TEST

/**
 * 클라이언트 연결 테스트
 */
START_TEST(test_client_connection)
{
    // Given
    thread_t *thread = socket_adapter->start_listening(socket_adapter);
    ck_assert_ptr_nonnull(thread);
    
    usleep(100000); // 소켓이 준비될 때까지 대기
    
    // When
    test_client_fd = create_test_client();
    
    // Then
    ck_assert_int_ge(test_client_fd, 0);
    
    // 정리
    thread->cancel(thread);
    thread->join(thread);
}
END_TEST

/**
 * 이벤트 전송 테스트
 */
START_TEST(test_send_event)
{
    // Given
    const char *test_event = "{\"event\":\"test\",\"data\":\"value\"}";
    thread_t *thread = socket_adapter->start_listening(socket_adapter);
    ck_assert_ptr_nonnull(thread);
    
    usleep(100000);
    test_client_fd = create_test_client();
    ck_assert_int_ge(test_client_fd, 0);
    
    usleep(100000); // 연결이 설정될 때까지 대기
    
    // When
    extsock_error_t result = socket_adapter->send_event(socket_adapter, test_event);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_NONE);
    
    // 정리
    thread->cancel(thread);
    thread->join(thread);
}
END_TEST

/**
 * NULL 이벤트 전송 처리 테스트
 */
START_TEST(test_send_event_null)
{
    // When
    extsock_error_t result = socket_adapter->send_event(socket_adapter, NULL);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * 빈 문자열 이벤트 전송 처리 테스트
 */
START_TEST(test_send_event_empty)
{
    // When
    extsock_error_t result = socket_adapter->send_event(socket_adapter, "");
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * 소켓 어댑터 해제 테스트
 */
START_TEST(test_socket_adapter_destroy)
{
    // Given
    thread_t *thread = socket_adapter->start_listening(socket_adapter);
    ck_assert_ptr_nonnull(thread);
    
    // When
    socket_adapter->destroy(socket_adapter);
    socket_adapter = NULL; // teardown에서 다시 해제하지 않도록
    
    // Then
    // 소켓 파일이 정리되었는지 확인
    usleep(100000); // 정리 시간 대기
    ck_assert_int_ne(access(TEST_SOCKET_PATH, F_OK), 0);
}
END_TEST

/**
 * 테스트 스위트 생성
 */
Suite *socket_adapter_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Socket Adapter");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup_socket_adapter_test, teardown_socket_adapter_test);
    
    tcase_add_test(tc_core, test_socket_adapter_creation);
    tcase_add_test(tc_core, test_start_listening);
    tcase_add_test(tc_core, test_client_connection);
    tcase_add_test(tc_core, test_send_event);
    tcase_add_test(tc_core, test_send_event_null);
    tcase_add_test(tc_core, test_send_event_empty);
    tcase_add_test(tc_core, test_socket_adapter_destroy);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = socket_adapter_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 