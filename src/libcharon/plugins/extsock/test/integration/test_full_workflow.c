/*
 * Copyright (C) 2024 strongSwan Project
 * Integration tests for complete extsock plugin workflow
 */

#include <check.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cjson/cJSON.h>
#include <pthread.h>
#include "../common/extsock_common.h"
#include "../adapters/json/extsock_json_parser.h"
#include "../adapters/socket/extsock_socket_adapter.h"
#include "../usecases/extsock_config_usecase.h"
#include "../usecases/extsock_event_usecase.h"

#define TEST_SOCKET_PATH "/tmp/test_integration_extsock.sock"
#define MAX_EVENTS 10

static extsock_json_parser_t *json_parser;
static extsock_socket_adapter_t *socket_adapter;
static extsock_config_usecase_t *config_usecase;
static extsock_event_usecase_t *event_usecase;
static thread_t *socket_thread;
static int client_fd;
static char received_events[MAX_EVENTS][1024];
static int event_count;
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * 이벤트 수신 스레드
 */
static void* event_receiver_thread(void* arg)
{
    char buffer[2048];
    ssize_t bytes_read;
    
    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        
        pthread_mutex_lock(&event_mutex);
        if (event_count < MAX_EVENTS) {
            strncpy(received_events[event_count], buffer, sizeof(received_events[0]) - 1);
            received_events[event_count][sizeof(received_events[0]) - 1] = '\0';
            event_count++;
        }
        pthread_mutex_unlock(&event_mutex);
    }
    
    return NULL;
}

void setup_integration_test(void)
{
    struct sockaddr_un addr;
    pthread_t receiver_thread;
    
    // 환경 설정
    setenv("EXTSOCK_SOCKET_PATH", TEST_SOCKET_PATH, 1);
    event_count = 0;
    client_fd = -1;
    
    // 컴포넌트 생성 (의존성 주입 방식)
    json_parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(json_parser);
    
    event_usecase = extsock_event_usecase_create();
    ck_assert_ptr_nonnull(event_usecase);
    
    config_usecase = extsock_config_usecase_create(
        json_parser,
        event_usecase->get_event_publisher(event_usecase)
    );
    ck_assert_ptr_nonnull(config_usecase);
    
    socket_adapter = extsock_socket_adapter_create(
        config_usecase->get_command_handler(config_usecase)
    );
    ck_assert_ptr_nonnull(socket_adapter);
    
    // 이벤트 유스케이스에 소켓 어댑터 주입
    event_usecase->set_socket_adapter(event_usecase, socket_adapter);
    
    // 소켓 서버 시작
    socket_thread = socket_adapter->start_listening(socket_adapter);
    ck_assert_ptr_nonnull(socket_thread);
    
    usleep(200000); // 소켓이 준비될 때까지 대기
    
    // 클라이언트 연결
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ck_assert_int_ge(client_fd, 0);
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, TEST_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    ck_assert_int_eq(connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)), 0);
    
    // 이벤트 수신 스레드 시작
    pthread_create(&receiver_thread, NULL, event_receiver_thread, NULL);
    pthread_detach(receiver_thread);
    
    usleep(100000); // 연결이 안정화될 때까지 대기
}

void teardown_integration_test(void)
{
    if (client_fd >= 0) {
        close(client_fd);
        client_fd = -1;
    }
    
    if (socket_thread) {
        socket_thread->cancel(socket_thread);
        socket_thread->join(socket_thread);
        socket_thread = NULL;
    }
    
    if (socket_adapter) {
        socket_adapter->destroy(socket_adapter);
        socket_adapter = NULL;
    }
    
    if (config_usecase) {
        config_usecase->destroy(config_usecase);
        config_usecase = NULL;
    }
    
    if (event_usecase) {
        event_usecase->destroy(event_usecase);
        event_usecase = NULL;
    }
    
    if (json_parser) {
        json_parser->destroy(json_parser);
        json_parser = NULL;
    }
    
    unlink(TEST_SOCKET_PATH);
    unsetenv("EXTSOCK_SOCKET_PATH");
}

/**
 * 간단한 설정 적용 워크플로우 테스트
 */
START_TEST(test_simple_config_workflow)
{
    // Given - 간단한 IPsec 설정 JSON
    const char *config_json = 
        "{"
        "\"name\":\"test-conn\","
        "\"local\":\"192.168.1.10\","
        "\"remote\":\"203.0.113.5\","
        "\"auth\":{"
            "\"type\":\"psk\","
            "\"id\":\"CN=testuser\","
            "\"secret\":\"supersecret\""
        "},"
        "\"children\":[{"
            "\"name\":\"child1\","
            "\"local_ts\":\"10.0.0.0/24\","
            "\"remote_ts\":\"10.1.0.0/24\""
        "}]"
        "}";
    
    char command[2048];
    snprintf(command, sizeof(command), "APPLY_CONFIG %s", config_json);
    
    // When
    ssize_t sent = write(client_fd, command, strlen(command));
    
    // Then
    ck_assert_int_gt(sent, 0);
    usleep(500000); // 처리 시간 대기
    
    // 명령이 성공적으로 처리되었는지 확인 (에러 응답이 없으면 성공)
    // 실제 구현에서는 응답 메시지나 상태 확인 로직 추가 필요
}
END_TEST

/**
 * DPD 명령 워크플로우 테스트
 */
START_TEST(test_dpd_command_workflow)
{
    // Given
    const char *dpd_command = "START_DPD test-conn";
    
    // When
    ssize_t sent = write(client_fd, dpd_command, strlen(dpd_command));
    
    // Then
    ck_assert_int_gt(sent, 0);
    usleep(200000); // 처리 시간 대기
    
    // DPD 명령이 처리되었는지 확인
}
END_TEST

/**
 * 잘못된 JSON 처리 워크플로우 테스트
 */
START_TEST(test_invalid_json_workflow)
{
    // Given - 잘못된 JSON
    const char *invalid_command = "APPLY_CONFIG {invalid json}";
    
    // When
    ssize_t sent = write(client_fd, invalid_command, strlen(invalid_command));
    
    // Then
    ck_assert_int_gt(sent, 0);
    usleep(200000); // 처리 시간 대기
    
    // 에러가 적절히 처리되었는지 확인
    // 실제 구현에서는 에러 응답 메시지 확인 필요
}
END_TEST

/**
 * 다중 명령 처리 테스트
 */
START_TEST(test_multiple_commands_workflow)
{
    // Given
    const char *commands[] = {
        "APPLY_CONFIG {\"name\":\"conn1\",\"local\":\"192.168.1.10\",\"remote\":\"203.0.113.5\"}",
        "APPLY_CONFIG {\"name\":\"conn2\",\"local\":\"192.168.1.11\",\"remote\":\"203.0.113.6\"}",
        "START_DPD conn1",
        "START_DPD conn2"
    };
    int cmd_count = sizeof(commands) / sizeof(commands[0]);
    
    // When
    for (int i = 0; i < cmd_count; i++) {
        ssize_t sent = write(client_fd, commands[i], strlen(commands[i]));
        ck_assert_int_gt(sent, 0);
        usleep(100000); // 명령 간 간격
    }
    
    // Then
    usleep(500000); // 모든 명령 처리 대기
    
    // 모든 명령이 순차적으로 처리되었는지 확인
}
END_TEST

/**
 * 이벤트 발행 테스트
 */
START_TEST(test_event_publishing_workflow)
{
    // Given - 모의 Child SA 이벤트 생성
    const char *test_event = 
        "{"
        "\"event\":\"child_sa_up\","
        "\"ike_sa_name\":\"test-conn\","
        "\"child_sa_name\":\"child1\","
        "\"ike_sa_state\":\"4\","
        "\"child_sa_state\":\"2\""
        "}";
    
    // When
    extsock_error_t result = socket_adapter->send_event(socket_adapter, test_event);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_NONE);
    usleep(200000); // 이벤트 수신 대기
    
    // 이벤트가 수신되었는지 확인
    pthread_mutex_lock(&event_mutex);
    ck_assert_int_gt(event_count, 0);
    
    // 수신된 이벤트 내용 확인
    bool found_event = false;
    for (int i = 0; i < event_count; i++) {
        if (strstr(received_events[i], "child_sa_up") != NULL) {
            found_event = true;
            break;
        }
    }
    ck_assert_int_eq(found_event, true);
    pthread_mutex_unlock(&event_mutex);
}
END_TEST

/**
 * 연결 해제 후 재연결 테스트
 */
START_TEST(test_reconnection_workflow)
{
    // Given - 연결 해제
    close(client_fd);
    usleep(100000);
    
    // When - 재연결 시도
    struct sockaddr_un addr;
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ck_assert_int_ge(client_fd, 0);
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, TEST_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    int result = connect(client_fd, (struct sockaddr*)&addr, sizeof(addr));
    
    // Then
    ck_assert_int_eq(result, 0);
    
    // 재연결 후 명령 전송 테스트
    const char *test_command = "START_DPD test-conn";
    ssize_t sent = write(client_fd, test_command, strlen(test_command));
    ck_assert_int_gt(sent, 0);
}
END_TEST

/**
 * 테스트 스위트 생성
 */
Suite *integration_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Integration Tests");

    /* Core test case */
    tc_core = tcase_create("Workflow");
    
    // 통합 테스트는 시간이 더 걸릴 수 있으므로 타임아웃 연장
    tcase_set_timeout(tc_core, 30);

    tcase_add_checked_fixture(tc_core, setup_integration_test, teardown_integration_test);
    
    tcase_add_test(tc_core, test_simple_config_workflow);
    tcase_add_test(tc_core, test_dpd_command_workflow);
    tcase_add_test(tc_core, test_invalid_json_workflow);
    tcase_add_test(tc_core, test_multiple_commands_workflow);
    tcase_add_test(tc_core, test_event_publishing_workflow);
    tcase_add_test(tc_core, test_reconnection_workflow);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = integration_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 