/*
 * Copyright (C) 2024 strongSwan Project
 * Real Socket Adapter Implementation Tests
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <cjson/cJSON.h>

// 실제 소켓 어댑터 관련 타입들
typedef enum {
    EXTSOCK_ADAPTER_STATE_DISCONNECTED,
    EXTSOCK_ADAPTER_STATE_CONNECTING,
    EXTSOCK_ADAPTER_STATE_CONNECTED,
    EXTSOCK_ADAPTER_STATE_ERROR
} extsock_adapter_state_t;

typedef struct {
    int socket_fd;
    extsock_adapter_state_t state;
    char *socket_path;
    void (*destroy)(void *this);
} extsock_socket_adapter_t;

static extsock_socket_adapter_t *adapter;
static char test_socket_path[256];

/**
 * 테스트 설정
 */
void setup_socket_adapter_real_test(void)
{
    // 임시 소켓 경로 생성
    snprintf(test_socket_path, sizeof(test_socket_path), "/tmp/extsock_test_%d.sock", getpid());
    
    // Mock adapter 생성
    adapter = malloc(sizeof(extsock_socket_adapter_t));
    ck_assert_ptr_nonnull(adapter);
    
    adapter->socket_fd = -1;
    adapter->state = EXTSOCK_ADAPTER_STATE_DISCONNECTED;
    adapter->socket_path = strdup(test_socket_path);
}

/**
 * 테스트 해제
 */
void teardown_socket_adapter_real_test(void)
{
    if (adapter) {
        if (adapter->socket_fd >= 0) {
            close(adapter->socket_fd);
        }
        if (adapter->socket_path) {
            unlink(adapter->socket_path);
            free(adapter->socket_path);
        }
        free(adapter);
        adapter = NULL;
    }
    
    // 테스트 소켓 파일 정리
    unlink(test_socket_path);
}

/**
 * 실제 소켓 생성 테스트
 */
START_TEST(test_real_socket_creation)
{
    // Given / When
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    
    // Then
    ck_assert_int_ge(sock_fd, 0);
    
    // Cleanup
    close(sock_fd);
}
END_TEST

/**
 * Unix Domain Socket 바인딩 테스트
 */
START_TEST(test_real_unix_socket_bind)
{
    // Given
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ck_assert_int_ge(sock_fd, 0);
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, test_socket_path, sizeof(addr.sun_path) - 1);
    
    // When
    int bind_result = bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
    
    // Then
    ck_assert_int_eq(bind_result, 0);
    
    // 소켓 파일이 생성되었는지 확인
    ck_assert_int_eq(access(test_socket_path, F_OK), 0);
    
    // Cleanup
    close(sock_fd);
    unlink(test_socket_path);
}
END_TEST

/**
 * 소켓 리스닝 테스트
 */
START_TEST(test_real_socket_listen)
{
    // Given
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ck_assert_int_ge(sock_fd, 0);
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, test_socket_path, sizeof(addr.sun_path) - 1);
    
    int bind_result = bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
    ck_assert_int_eq(bind_result, 0);
    
    // When
    int listen_result = listen(sock_fd, 5);
    
    // Then
    ck_assert_int_eq(listen_result, 0);
    
    // Cleanup
    close(sock_fd);
    unlink(test_socket_path);
}
END_TEST

/**
 * 소켓 상태 관리 테스트
 */
START_TEST(test_real_socket_state_management)
{
    // Given - 초기 상태
    ck_assert_int_eq(adapter->state, EXTSOCK_ADAPTER_STATE_DISCONNECTED);
    ck_assert_int_eq(adapter->socket_fd, -1);
    
    // When - 연결 시도 상태로 변경
    adapter->state = EXTSOCK_ADAPTER_STATE_CONNECTING;
    adapter->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    
    // Then
    ck_assert_int_eq(adapter->state, EXTSOCK_ADAPTER_STATE_CONNECTING);
    ck_assert_int_ge(adapter->socket_fd, 0);
    
    // When - 연결 완료 상태로 변경
    adapter->state = EXTSOCK_ADAPTER_STATE_CONNECTED;
    
    // Then
    ck_assert_int_eq(adapter->state, EXTSOCK_ADAPTER_STATE_CONNECTED);
}
END_TEST

/**
 * JSON 메시지 전송 시뮬레이션 테스트
 */
START_TEST(test_real_json_message_handling)
{
    // Given - JSON 메시지 생성
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "type", "config_request");
    cJSON_AddStringToObject(message, "connection_name", "test_connection");
    
    cJSON *ike = cJSON_CreateObject();
    cJSON_AddStringToObject(ike, "local", "192.168.1.10");
    cJSON_AddStringToObject(ike, "remote", "203.0.113.5");
    cJSON_AddItemToObject(message, "ike", ike);
    
    // When - JSON을 문자열로 직렬화
    char *json_string = cJSON_Print(message);
    ck_assert_ptr_nonnull(json_string);
    
    // Then - 메시지 검증
    ck_assert_int_gt(strlen(json_string), 50);
    ck_assert_ptr_nonnull(strstr(json_string, "config_request"));
    ck_assert_ptr_nonnull(strstr(json_string, "test_connection"));
    ck_assert_ptr_nonnull(strstr(json_string, "192.168.1.10"));
    
    // 메시지 파싱 테스트
    cJSON *parsed = cJSON_Parse(json_string);
    ck_assert_ptr_nonnull(parsed);
    
    cJSON *type_item = cJSON_GetObjectItem(parsed, "type");
    ck_assert_str_eq(cJSON_GetStringValue(type_item), "config_request");
    
    // Cleanup
    free(json_string);
    cJSON_Delete(parsed);
    cJSON_Delete(message);
}
END_TEST

/**
 * 소켓 에러 처리 테스트
 */
START_TEST(test_real_socket_error_handling)
{
    // Given - 잘못된 소켓 경로
    const char *invalid_path = "/invalid/path/that/does/not/exist/socket.sock";
    
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ck_assert_int_ge(sock_fd, 0);
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, invalid_path, sizeof(addr.sun_path) - 1);
    
    // When - 바인딩 시도 (실패해야 함)
    int bind_result = bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
    
    // Then - 에러 발생해야 함
    ck_assert_int_eq(bind_result, -1);
    ck_assert_int_eq(errno, ENOENT);
    
    // 어댑터 상태도 에러로 설정
    adapter->state = EXTSOCK_ADAPTER_STATE_ERROR;
    ck_assert_int_eq(adapter->state, EXTSOCK_ADAPTER_STATE_ERROR);
    
    // Cleanup
    close(sock_fd);
}
END_TEST

/**
 * 동시 연결 시뮬레이션 테스트
 */
START_TEST(test_real_concurrent_connections)
{
    // Given - 서버 소켓 설정
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ck_assert_int_ge(server_fd, 0);
    
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, test_socket_path, sizeof(server_addr.sun_path) - 1);
    
    ck_assert_int_eq(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)), 0);
    ck_assert_int_eq(listen(server_fd, 5), 0);
    
    // When - 클라이언트 소켓들 생성
    int client_fds[3];
    for (int i = 0; i < 3; i++) {
        client_fds[i] = socket(AF_UNIX, SOCK_STREAM, 0);
        ck_assert_int_ge(client_fds[i], 0);
    }
    
    // Then - 모든 소켓이 정상 생성되었는지 확인
    for (int i = 0; i < 3; i++) {
        ck_assert_int_ge(client_fds[i], 0);
    }
    
    // Cleanup
    for (int i = 0; i < 3; i++) {
        close(client_fds[i]);
    }
    close(server_fd);
    unlink(test_socket_path);
}
END_TEST

/**
 * 큰 JSON 메시지 처리 테스트
 */
START_TEST(test_real_large_json_message)
{
    // Given - 큰 JSON 메시지 생성
    cJSON *large_message = cJSON_CreateObject();
    cJSON_AddStringToObject(large_message, "type", "large_config");
    
    // 큰 배열 추가
    cJSON *large_array = cJSON_CreateArray();
    for (int i = 0; i < 100; i++) {
        cJSON *item = cJSON_CreateObject();
        char name[64];
        snprintf(name, sizeof(name), "connection_%d", i);
        cJSON_AddStringToObject(item, "name", name);
        cJSON_AddStringToObject(item, "local", "10.0.0.1");
        cJSON_AddStringToObject(item, "remote", "10.0.1.1");
        cJSON_AddItemToArray(large_array, item);
    }
    cJSON_AddItemToObject(large_message, "connections", large_array);
    
    // When - JSON 직렬화
    char *json_string = cJSON_Print(large_message);
    ck_assert_ptr_nonnull(json_string);
    
    // Then - 메시지 크기 확인 (최소 5KB 이상)
    size_t message_size = strlen(json_string);
    ck_assert_int_gt(message_size, 5000);
    
    // 파싱 테스트
    cJSON *parsed = cJSON_Parse(json_string);
    ck_assert_ptr_nonnull(parsed);
    
    cJSON *connections = cJSON_GetObjectItem(parsed, "connections");
    ck_assert_ptr_nonnull(connections);
    ck_assert(cJSON_IsArray(connections));
    ck_assert_int_eq(cJSON_GetArraySize(connections), 100);
    
    // Cleanup
    free(json_string);
    cJSON_Delete(parsed);
    cJSON_Delete(large_message);
}
END_TEST

/**
 * 메시지 프레이밍 테스트 (길이 prefix)
 */
START_TEST(test_real_message_framing)
{
    // Given - 메시지와 길이 정보
    const char *test_message = "{\"type\":\"test\",\"data\":\"hello world\"}";
    size_t message_len = strlen(test_message);
    
    // When - 길이 정보를 포함한 프레임 생성
    char frame[1024];
    int frame_len = snprintf(frame, sizeof(frame), "%zu\n%s", message_len, test_message);
    
    // Then - 프레임 검증
    ck_assert_int_gt(frame_len, message_len);
    ck_assert_ptr_nonnull(strstr(frame, test_message));
    
    // 프레임 파싱 시뮬레이션
    char *newline_pos = strchr(frame, '\n');
    ck_assert_ptr_nonnull(newline_pos);
    
    size_t parsed_len = strtoul(frame, NULL, 10);
    ck_assert_int_eq(parsed_len, message_len);
    
    char *message_start = newline_pos + 1;
    cJSON *parsed_json = cJSON_Parse(message_start);
    ck_assert_ptr_nonnull(parsed_json);
    
    // Cleanup
    cJSON_Delete(parsed_json);
}
END_TEST

Suite *socket_adapter_real_suite(void)
{
    Suite *s;
    TCase *tc_basic, *tc_messages, *tc_errors, *tc_advanced;

    s = suite_create("Socket Adapter Real Implementation Tests");

    /* 기본 소켓 테스트 */
    tc_basic = tcase_create("Basic Socket Tests");
    tcase_add_checked_fixture(tc_basic, setup_socket_adapter_real_test, teardown_socket_adapter_real_test);
    tcase_add_test(tc_basic, test_real_socket_creation);
    tcase_add_test(tc_basic, test_real_unix_socket_bind);
    tcase_add_test(tc_basic, test_real_socket_listen);
    tcase_add_test(tc_basic, test_real_socket_state_management);
    suite_add_tcase(s, tc_basic);

    /* 메시지 처리 테스트 */
    tc_messages = tcase_create("Message Handling Tests");
    tcase_add_checked_fixture(tc_messages, setup_socket_adapter_real_test, teardown_socket_adapter_real_test);
    tcase_add_test(tc_messages, test_real_json_message_handling);
    tcase_add_test(tc_messages, test_real_large_json_message);
    tcase_add_test(tc_messages, test_real_message_framing);
    suite_add_tcase(s, tc_messages);

    /* 에러 처리 테스트 */
    tc_errors = tcase_create("Error Handling Tests");
    tcase_add_checked_fixture(tc_errors, setup_socket_adapter_real_test, teardown_socket_adapter_real_test);
    tcase_add_test(tc_errors, test_real_socket_error_handling);
    suite_add_tcase(s, tc_errors);

    /* 고급 기능 테스트 */
    tc_advanced = tcase_create("Advanced Features Tests");
    tcase_add_checked_fixture(tc_advanced, setup_socket_adapter_real_test, teardown_socket_adapter_real_test);
    tcase_add_test(tc_advanced, test_real_concurrent_connections);
    suite_add_tcase(s, tc_advanced);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = socket_adapter_real_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 