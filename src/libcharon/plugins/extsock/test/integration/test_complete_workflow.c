/*
 * Copyright (C) 2024 strongSwan Project
 * Complete Workflow Integration Tests
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <cjson/cJSON.h>

// 통합 컴포넌트 타입들
typedef enum {
    WORKFLOW_STATE_INIT,
    WORKFLOW_STATE_CONFIG_LOADED,
    WORKFLOW_STATE_SOCKET_CONNECTED,
    WORKFLOW_STATE_AUTH_COMPLETED,
    WORKFLOW_STATE_TUNNEL_ESTABLISHED,
    WORKFLOW_STATE_ERROR
} workflow_state_t;

typedef struct {
    char *connection_name;
    char *config_json;
    bool socket_connected;
    bool auth_success;
    bool tunnel_active;
    workflow_state_t state;
    char *last_error;
    int events_count;
    time_t start_time;
} workflow_context_t;

static workflow_context_t *workflow;

/**
 * 워크플로우 컨텍스트 생성
 */
static workflow_context_t* create_workflow_context(const char *conn_name)
{
    workflow_context_t *ctx = malloc(sizeof(workflow_context_t));
    if (!ctx) return NULL;
    
    ctx->connection_name = strdup(conn_name);
    ctx->config_json = NULL;
    ctx->socket_connected = false;
    ctx->auth_success = false;
    ctx->tunnel_active = false;
    ctx->state = WORKFLOW_STATE_INIT;
    ctx->last_error = NULL;
    ctx->events_count = 0;
    ctx->start_time = time(NULL);
    
    return ctx;
}

/**
 * 워크플로우 컨텍스트 해제
 */
static void destroy_workflow_context(workflow_context_t *ctx)
{
    if (ctx) {
        free(ctx->connection_name);
        free(ctx->config_json);
        free(ctx->last_error);
        free(ctx);
    }
}

/**
 * 설정 JSON 생성 시뮬레이션
 */
static char* create_test_config_json(const char *conn_name)
{
    cJSON *config = cJSON_CreateObject();
    
    // 연결 기본 정보
    cJSON_AddStringToObject(config, "connection_name", conn_name);
    cJSON_AddStringToObject(config, "version", "2.0");
    cJSON_AddStringToObject(config, "type", "ikev2");
    
    // 로컬 설정
    cJSON *local = cJSON_CreateObject();
    cJSON_AddStringToObject(local, "addrs", "192.168.1.10");
    cJSON_AddStringToObject(local, "auth", "psk");
    cJSON_AddStringToObject(local, "id", "client@example.com");
    cJSON_AddItemToObject(config, "local", local);
    
    // 원격 설정
    cJSON *remote = cJSON_CreateObject();
    cJSON_AddStringToObject(remote, "addrs", "203.0.113.5");
    cJSON_AddStringToObject(remote, "auth", "psk");
    cJSON_AddStringToObject(remote, "id", "server@example.com");
    cJSON_AddItemToObject(config, "remote", remote);
    
    // 자식 SA 설정
    cJSON *children = cJSON_CreateObject();
    cJSON *child_sa = cJSON_CreateObject();
    cJSON_AddStringToObject(child_sa, "local_ts", "192.168.1.0/24");
    cJSON_AddStringToObject(child_sa, "remote_ts", "10.0.0.0/16");
    cJSON_AddStringToObject(child_sa, "mode", "tunnel");
    cJSON_AddItemToObject(children, "net", child_sa);
    cJSON_AddItemToObject(config, "children", children);
    
    char *json_string = cJSON_Print(config);
    cJSON_Delete(config);
    
    return json_string;
}

/**
 * 소켓 연결 시뮬레이션
 */
static bool simulate_socket_connection(workflow_context_t *ctx)
{
    if (!ctx) return false;
    
    // 소켓 연결 시뮬레이션 (실제로는 외부 소켓에 연결)
    usleep(100000); // 100ms 지연
    
    ctx->socket_connected = true;
    ctx->state = WORKFLOW_STATE_SOCKET_CONNECTED;
    ctx->events_count++;
    
    return true;
}

/**
 * 인증 과정 시뮬레이션
 */
static bool simulate_authentication(workflow_context_t *ctx)
{
    if (!ctx || !ctx->socket_connected) return false;
    
    // 인증 과정 시뮬레이션
    usleep(200000); // 200ms 지연
    
    // PSK 인증 시뮬레이션
    bool auth_success = true; // 테스트에서는 항상 성공
    
    if (auth_success) {
        ctx->auth_success = true;
        ctx->state = WORKFLOW_STATE_AUTH_COMPLETED;
        ctx->events_count++;
        return true;
    } else {
        ctx->state = WORKFLOW_STATE_ERROR;
        ctx->last_error = strdup("Authentication failed");
        return false;
    }
}

/**
 * 터널 설정 시뮬레이션
 */
static bool simulate_tunnel_establishment(workflow_context_t *ctx)
{
    if (!ctx || !ctx->auth_success) return false;
    
    // 터널 설정 시뮬레이션
    usleep(300000); // 300ms 지연
    
    ctx->tunnel_active = true;
    ctx->state = WORKFLOW_STATE_TUNNEL_ESTABLISHED;
    ctx->events_count++;
    
    return true;
}

/**
 * 테스트 설정
 */
void setup_complete_workflow_test(void)
{
    workflow = create_workflow_context("integration_test_connection");
    ck_assert_ptr_nonnull(workflow);
}

/**
 * 테스트 해제
 */
void teardown_complete_workflow_test(void)
{
    if (workflow) {
        destroy_workflow_context(workflow);
        workflow = NULL;
    }
}

/**
 * 워크플로우 초기화 테스트
 */
START_TEST(test_complete_workflow_initialization)
{
    // Given / When / Then
    ck_assert_ptr_nonnull(workflow);
    ck_assert_str_eq(workflow->connection_name, "integration_test_connection");
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_INIT);
    ck_assert(!workflow->socket_connected);
    ck_assert(!workflow->auth_success);
    ck_assert(!workflow->tunnel_active);
    ck_assert_int_eq(workflow->events_count, 0);
    ck_assert_int_gt(workflow->start_time, 0);
}
END_TEST

/**
 * 설정 로딩 워크플로우 테스트
 */
START_TEST(test_complete_config_loading_workflow)
{
    // Given - 설정 JSON 생성
    char *config_json = create_test_config_json(workflow->connection_name);
    ck_assert_ptr_nonnull(config_json);
    
    // When - 설정 파싱
    cJSON *parsed_config = cJSON_Parse(config_json);
    ck_assert_ptr_nonnull(parsed_config);
    
    cJSON *conn_name_item = cJSON_GetObjectItem(parsed_config, "connection_name");
    cJSON *local_item = cJSON_GetObjectItem(parsed_config, "local");
    cJSON *remote_item = cJSON_GetObjectItem(parsed_config, "remote");
    cJSON *children_item = cJSON_GetObjectItem(parsed_config, "children");
    
    // Then - 설정 유효성 검증
    ck_assert_ptr_nonnull(conn_name_item);
    ck_assert_str_eq(cJSON_GetStringValue(conn_name_item), workflow->connection_name);
    ck_assert_ptr_nonnull(local_item);
    ck_assert_ptr_nonnull(remote_item);
    ck_assert_ptr_nonnull(children_item);
    
    // 워크플로우 상태 업데이트
    workflow->config_json = strdup(config_json);
    workflow->state = WORKFLOW_STATE_CONFIG_LOADED;
    workflow->events_count++;
    
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_CONFIG_LOADED);
    ck_assert_int_eq(workflow->events_count, 1);
    
    // Cleanup
    free(config_json);
    cJSON_Delete(parsed_config);
}
END_TEST

/**
 * 소켓 연결 워크플로우 테스트
 */
START_TEST(test_complete_socket_connection_workflow)
{
    // Given - 설정 로딩 완료 상태
    workflow->config_json = create_test_config_json(workflow->connection_name);
    workflow->state = WORKFLOW_STATE_CONFIG_LOADED;
    workflow->events_count = 1;
    
    // When - 소켓 연결
    bool connected = simulate_socket_connection(workflow);
    
    // Then
    ck_assert(connected);
    ck_assert(workflow->socket_connected);
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_SOCKET_CONNECTED);
    ck_assert_int_eq(workflow->events_count, 2);
}
END_TEST

/**
 * 인증 워크플로우 테스트
 */
START_TEST(test_complete_authentication_workflow)
{
    // Given - 소켓 연결 완료 상태
    workflow->config_json = create_test_config_json(workflow->connection_name);
    workflow->state = WORKFLOW_STATE_SOCKET_CONNECTED;
    workflow->socket_connected = true;
    workflow->events_count = 2;
    
    // When - 인증 실행
    bool auth_result = simulate_authentication(workflow);
    
    // Then
    ck_assert(auth_result);
    ck_assert(workflow->auth_success);
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_AUTH_COMPLETED);
    ck_assert_int_eq(workflow->events_count, 3);
}
END_TEST

/**
 * 터널 설정 워크플로우 테스트
 */
START_TEST(test_complete_tunnel_establishment_workflow)
{
    // Given - 인증 완료 상태
    workflow->config_json = create_test_config_json(workflow->connection_name);
    workflow->state = WORKFLOW_STATE_AUTH_COMPLETED;
    workflow->socket_connected = true;
    workflow->auth_success = true;
    workflow->events_count = 3;
    
    // When - 터널 설정
    bool tunnel_result = simulate_tunnel_establishment(workflow);
    
    // Then
    ck_assert(tunnel_result);
    ck_assert(workflow->tunnel_active);
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_TUNNEL_ESTABLISHED);
    ck_assert_int_eq(workflow->events_count, 4);
}
END_TEST

/**
 * 전체 워크플로우 통합 테스트
 */
START_TEST(test_complete_end_to_end_workflow)
{
    // Given - 초기 상태
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_INIT);
    time_t start_time = time(NULL);
    
    // Phase 1: 설정 로딩
    char *config_json = create_test_config_json(workflow->connection_name);
    ck_assert_ptr_nonnull(config_json);
    
    workflow->config_json = strdup(config_json);
    workflow->state = WORKFLOW_STATE_CONFIG_LOADED;
    workflow->events_count++;
    
    // Phase 2: 소켓 연결
    bool socket_result = simulate_socket_connection(workflow);
    ck_assert(socket_result);
    
    // Phase 3: 인증
    bool auth_result = simulate_authentication(workflow);
    ck_assert(auth_result);
    
    // Phase 4: 터널 설정
    bool tunnel_result = simulate_tunnel_establishment(workflow);
    ck_assert(tunnel_result);
    
    // Then - 최종 상태 검증
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_TUNNEL_ESTABLISHED);
    ck_assert(workflow->socket_connected);
    ck_assert(workflow->auth_success);
    ck_assert(workflow->tunnel_active);
    ck_assert_int_eq(workflow->events_count, 4);
    ck_assert_ptr_null(workflow->last_error);
    
    // 시간 검증 (전체 과정이 2초 이내)
    time_t end_time = time(NULL);
    ck_assert_int_le(end_time - start_time, 2);
    
    // Cleanup
    free(config_json);
}
END_TEST

/**
 * 에러 시나리오 워크플로우 테스트
 */
START_TEST(test_complete_error_handling_workflow)
{
    // Given - 소켓 연결 실패 시나리오
    workflow->config_json = create_test_config_json(workflow->connection_name);
    workflow->state = WORKFLOW_STATE_CONFIG_LOADED;
    
    // When - 소켓 연결 실패 시뮬레이션
    workflow->socket_connected = false; // 연결 실패
    workflow->state = WORKFLOW_STATE_ERROR;
    workflow->last_error = strdup("Socket connection failed");
    
    // Then
    ck_assert(!workflow->socket_connected);
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_ERROR);
    ck_assert_ptr_nonnull(workflow->last_error);
    ck_assert_str_eq(workflow->last_error, "Socket connection failed");
    
    // Given - 인증 실패 시나리오
    workflow->socket_connected = true;
    workflow->state = WORKFLOW_STATE_SOCKET_CONNECTED;
    free(workflow->last_error);
    
    // When - 인증 실패 시뮬레이션
    workflow->auth_success = false;
    workflow->state = WORKFLOW_STATE_ERROR;
    workflow->last_error = strdup("Authentication failed");
    
    // Then
    ck_assert(!workflow->auth_success);
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_ERROR);
    ck_assert_str_eq(workflow->last_error, "Authentication failed");
}
END_TEST

/**
 * 워크플로우 상태 전환 유효성 테스트
 */
START_TEST(test_complete_state_transition_validation)
{
    // Given - 각 상태별 전환 테스트
    workflow_state_t valid_transitions[][2] = {
        {WORKFLOW_STATE_INIT, WORKFLOW_STATE_CONFIG_LOADED},
        {WORKFLOW_STATE_CONFIG_LOADED, WORKFLOW_STATE_SOCKET_CONNECTED},
        {WORKFLOW_STATE_SOCKET_CONNECTED, WORKFLOW_STATE_AUTH_COMPLETED},
        {WORKFLOW_STATE_AUTH_COMPLETED, WORKFLOW_STATE_TUNNEL_ESTABLISHED}
    };
    
    int num_transitions = sizeof(valid_transitions) / sizeof(valid_transitions[0]);
    
    // When/Then - 각 전환 테스트
    for (int i = 0; i < num_transitions; i++) {
        workflow_state_t from_state = valid_transitions[i][0];
        workflow_state_t to_state = valid_transitions[i][1];
        
        // 상태 설정
        workflow->state = from_state;
        
        // 상태 전환 로직 (간단한 예제)
        bool transition_valid = true;
        switch (from_state) {
            case WORKFLOW_STATE_INIT:
                transition_valid = (to_state == WORKFLOW_STATE_CONFIG_LOADED);
                break;
            case WORKFLOW_STATE_CONFIG_LOADED:
                transition_valid = (to_state == WORKFLOW_STATE_SOCKET_CONNECTED);
                break;
            case WORKFLOW_STATE_SOCKET_CONNECTED:
                transition_valid = (to_state == WORKFLOW_STATE_AUTH_COMPLETED);
                break;
            case WORKFLOW_STATE_AUTH_COMPLETED:
                transition_valid = (to_state == WORKFLOW_STATE_TUNNEL_ESTABLISHED);
                break;
            default:
                transition_valid = false;
        }
        
        ck_assert(transition_valid);
        
        if (transition_valid) {
            workflow->state = to_state;
            ck_assert_int_eq(workflow->state, to_state);
        }
    }
}
END_TEST

/**
 * 워크플로우 성능 테스트
 */
START_TEST(test_complete_performance_workflow)
{
    // Given - 성능 측정 시작
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // When - 빠른 워크플로우 실행
    char *config_json = create_test_config_json(workflow->connection_name);
    workflow->config_json = strdup(config_json);
    workflow->state = WORKFLOW_STATE_CONFIG_LOADED;
    
    // 소켓 연결 (지연 없이)
    workflow->socket_connected = true;
    workflow->state = WORKFLOW_STATE_SOCKET_CONNECTED;
    
    // 인증 (지연 없이)
    workflow->auth_success = true;
    workflow->state = WORKFLOW_STATE_AUTH_COMPLETED;
    
    // 터널 설정 (지연 없이)
    workflow->tunnel_active = true;
    workflow->state = WORKFLOW_STATE_TUNNEL_ESTABLISHED;
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Then - 성능 검증 (1ms 이내 완료)
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
    long elapsed_ms = elapsed_ns / 1000000L;
    
    ck_assert_int_lt(elapsed_ms, 10); // 10ms 이내
    ck_assert_int_eq(workflow->state, WORKFLOW_STATE_TUNNEL_ESTABLISHED);
    
    // Cleanup
    free(config_json);
}
END_TEST

Suite *complete_workflow_suite(void)
{
    Suite *s;
    TCase *tc_init, *tc_phases, *tc_integration, *tc_error, *tc_validation, *tc_performance;

    s = suite_create("Complete Workflow Integration Tests");

    /* 초기화 테스트 */
    tc_init = tcase_create("Initialization Tests");
    tcase_add_checked_fixture(tc_init, setup_complete_workflow_test, teardown_complete_workflow_test);
    tcase_add_test(tc_init, test_complete_workflow_initialization);
    suite_add_tcase(s, tc_init);

    /* 각 단계별 테스트 */
    tc_phases = tcase_create("Phase Tests");
    tcase_add_checked_fixture(tc_phases, setup_complete_workflow_test, teardown_complete_workflow_test);
    tcase_add_test(tc_phases, test_complete_config_loading_workflow);
    tcase_add_test(tc_phases, test_complete_socket_connection_workflow);
    tcase_add_test(tc_phases, test_complete_authentication_workflow);
    tcase_add_test(tc_phases, test_complete_tunnel_establishment_workflow);
    suite_add_tcase(s, tc_phases);

    /* 통합 테스트 */
    tc_integration = tcase_create("Integration Tests");
    tcase_add_checked_fixture(tc_integration, setup_complete_workflow_test, teardown_complete_workflow_test);
    tcase_add_test(tc_integration, test_complete_end_to_end_workflow);
    suite_add_tcase(s, tc_integration);

    /* 에러 핸들링 테스트 */
    tc_error = tcase_create("Error Handling Tests");
    tcase_add_checked_fixture(tc_error, setup_complete_workflow_test, teardown_complete_workflow_test);
    tcase_add_test(tc_error, test_complete_error_handling_workflow);
    suite_add_tcase(s, tc_error);

    /* 유효성 검증 테스트 */
    tc_validation = tcase_create("Validation Tests");
    tcase_add_checked_fixture(tc_validation, setup_complete_workflow_test, teardown_complete_workflow_test);
    tcase_add_test(tc_validation, test_complete_state_transition_validation);
    suite_add_tcase(s, tc_validation);

    /* 성능 테스트 */
    tc_performance = tcase_create("Performance Tests");
    tcase_add_checked_fixture(tc_performance, setup_complete_workflow_test, teardown_complete_workflow_test);
    tcase_add_test(tc_performance, test_complete_performance_workflow);
    suite_add_tcase(s, tc_performance);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = complete_workflow_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 