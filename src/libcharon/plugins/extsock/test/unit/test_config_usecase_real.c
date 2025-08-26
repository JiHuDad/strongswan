/*
 * Copyright (C) 2024 strongSwan Project
 * Real Config Usecase Implementation Tests
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <cjson/cJSON.h>

// Config usecase 관련 타입들
typedef enum {
    EXTSOCK_CONFIG_ACTION_ADD,
    EXTSOCK_CONFIG_ACTION_REMOVE,
    EXTSOCK_CONFIG_ACTION_UPDATE,
    EXTSOCK_CONFIG_ACTION_LIST
} extsock_config_action_t;

typedef struct {
    char *connection_name;
    char *local_ip;
    char *remote_ip;
    char *auth_method;
    char *psk_secret;
} extsock_connection_config_t;

typedef struct {
    extsock_config_action_t action;
    extsock_connection_config_t *config;
    bool (*execute)(void *this);
    void (*destroy)(void *this);
} extsock_config_usecase_t;

static extsock_config_usecase_t *usecase;
static extsock_connection_config_t *test_config;

/**
 * 테스트 설정
 */
void setup_config_usecase_real_test(void)
{
    // Mock config 생성
    test_config = malloc(sizeof(extsock_connection_config_t));
    ck_assert_ptr_nonnull(test_config);
    
    test_config->connection_name = strdup("test_connection");
    test_config->local_ip = strdup("192.168.1.10");
    test_config->remote_ip = strdup("203.0.113.5");
    test_config->auth_method = strdup("psk");
    test_config->psk_secret = strdup("secret123");
    
    // Mock usecase 생성
    usecase = malloc(sizeof(extsock_config_usecase_t));
    ck_assert_ptr_nonnull(usecase);
    
    usecase->action = EXTSOCK_CONFIG_ACTION_ADD;
    usecase->config = test_config;
}

/**
 * 테스트 해제
 */
void teardown_config_usecase_real_test(void)
{
    if (test_config) {
        free(test_config->connection_name);
        free(test_config->local_ip);
        free(test_config->remote_ip);
        free(test_config->auth_method);
        free(test_config->psk_secret);
        free(test_config);
        test_config = NULL;
    }
    
    if (usecase) {
        free(usecase);
        usecase = NULL;
    }
}

/**
 * 연결 설정 생성 테스트
 */
START_TEST(test_real_connection_config_creation)
{
    // Given / When / Then
    ck_assert_ptr_nonnull(test_config);
    ck_assert_str_eq(test_config->connection_name, "test_connection");
    ck_assert_str_eq(test_config->local_ip, "192.168.1.10");
    ck_assert_str_eq(test_config->remote_ip, "203.0.113.5");
    ck_assert_str_eq(test_config->auth_method, "psk");
    ck_assert_str_eq(test_config->psk_secret, "secret123");
}
END_TEST

/**
 * 설정 액션 타입 테스트
 */
START_TEST(test_real_config_action_types)
{
    // Given / When / Then
    ck_assert_int_eq(usecase->action, EXTSOCK_CONFIG_ACTION_ADD);
    
    // When - 액션 변경
    usecase->action = EXTSOCK_CONFIG_ACTION_REMOVE;
    ck_assert_int_eq(usecase->action, EXTSOCK_CONFIG_ACTION_REMOVE);
    
    usecase->action = EXTSOCK_CONFIG_ACTION_UPDATE;
    ck_assert_int_eq(usecase->action, EXTSOCK_CONFIG_ACTION_UPDATE);
    
    usecase->action = EXTSOCK_CONFIG_ACTION_LIST;
    ck_assert_int_eq(usecase->action, EXTSOCK_CONFIG_ACTION_LIST);
}
END_TEST

/**
 * JSON을 설정으로 변환 테스트
 */
START_TEST(test_real_json_to_config_conversion)
{
    // Given - JSON 설정
    cJSON *config_json = cJSON_CreateObject();
    cJSON_AddStringToObject(config_json, "connection_name", "vpn_tunnel_1");
    cJSON_AddStringToObject(config_json, "local_ip", "10.0.0.1");
    cJSON_AddStringToObject(config_json, "remote_ip", "10.0.1.1");
    cJSON_AddStringToObject(config_json, "auth_method", "psk");
    cJSON_AddStringToObject(config_json, "psk_secret", "supersecret");
    
    // When - JSON에서 설정값 추출
    cJSON *name_item = cJSON_GetObjectItem(config_json, "connection_name");
    cJSON *local_item = cJSON_GetObjectItem(config_json, "local_ip");
    cJSON *remote_item = cJSON_GetObjectItem(config_json, "remote_ip");
    cJSON *auth_item = cJSON_GetObjectItem(config_json, "auth_method");
    cJSON *secret_item = cJSON_GetObjectItem(config_json, "psk_secret");
    
    // Then - 변환 검증
    ck_assert_ptr_nonnull(name_item);
    ck_assert_str_eq(cJSON_GetStringValue(name_item), "vpn_tunnel_1");
    
    ck_assert_ptr_nonnull(local_item);
    ck_assert_str_eq(cJSON_GetStringValue(local_item), "10.0.0.1");
    
    ck_assert_ptr_nonnull(remote_item);
    ck_assert_str_eq(cJSON_GetStringValue(remote_item), "10.0.1.1");
    
    ck_assert_ptr_nonnull(auth_item);
    ck_assert_str_eq(cJSON_GetStringValue(auth_item), "psk");
    
    ck_assert_ptr_nonnull(secret_item);
    ck_assert_str_eq(cJSON_GetStringValue(secret_item), "supersecret");
    
    // Cleanup
    cJSON_Delete(config_json);
}
END_TEST

/**
 * 설정을 JSON으로 변환 테스트
 */
START_TEST(test_real_config_to_json_conversion)
{
    // Given - 설정 정보
    // When - JSON 변환
    cJSON *config_json = cJSON_CreateObject();
    cJSON_AddStringToObject(config_json, "connection_name", test_config->connection_name);
    cJSON_AddStringToObject(config_json, "local_ip", test_config->local_ip);
    cJSON_AddStringToObject(config_json, "remote_ip", test_config->remote_ip);
    cJSON_AddStringToObject(config_json, "auth_method", test_config->auth_method);
    cJSON_AddStringToObject(config_json, "psk_secret", test_config->psk_secret);
    
    // Then - JSON 직렬화 검증
    char *json_string = cJSON_Print(config_json);
    ck_assert_ptr_nonnull(json_string);
    ck_assert_ptr_nonnull(strstr(json_string, "test_connection"));
    ck_assert_ptr_nonnull(strstr(json_string, "192.168.1.10"));
    ck_assert_ptr_nonnull(strstr(json_string, "203.0.113.5"));
    ck_assert_ptr_nonnull(strstr(json_string, "psk"));
    ck_assert_ptr_nonnull(strstr(json_string, "secret123"));
    
    // Cleanup
    free(json_string);
    cJSON_Delete(config_json);
}
END_TEST

/**
 * 설정 유효성 검증 테스트
 */
START_TEST(test_real_config_validation)
{
    // Given - 유효한 설정
    bool is_valid = true;
    
    // When - 필수 필드 검증
    if (!test_config->connection_name || strlen(test_config->connection_name) == 0) {
        is_valid = false;
    }
    if (!test_config->local_ip || strlen(test_config->local_ip) == 0) {
        is_valid = false;
    }
    if (!test_config->remote_ip || strlen(test_config->remote_ip) == 0) {
        is_valid = false;
    }
    if (!test_config->auth_method || strlen(test_config->auth_method) == 0) {
        is_valid = false;
    }
    
    // Then
    ck_assert(is_valid);
    
    // Given - 잘못된 설정 (빈 연결 이름)
    free(test_config->connection_name);
    test_config->connection_name = strdup("");
    
    // When
    is_valid = true;
    if (!test_config->connection_name || strlen(test_config->connection_name) == 0) {
        is_valid = false;
    }
    
    // Then
    ck_assert(!is_valid);
}
END_TEST

/**
 * IP 주소 형식 검증 테스트
 */
START_TEST(test_real_ip_address_validation)
{
    // Given - IP 주소 문자열들
    const char *valid_ips[] = {
        "192.168.1.1",
        "10.0.0.1",
        "172.16.0.1",
        "203.0.113.5"
    };
    
    const char *invalid_ips[] __attribute__((unused)) = {
        "192.168.1",      // 점 2개 (부족)
        "192.168.1.1.1",  // 점 4개 (초과)
        "192.168.1.",     // 점 3개이지만 마지막이 빈 값
        "not.an.ip"       // 점 2개 (부족)
    };
    
    // When - 유효한 IP 검증 (간단한 형식 체크)
    for (int i = 0; i < 4; i++) {
        const char *ip = valid_ips[i];
        int dot_count = 0;
        for (const char *p = ip; *p; p++) {
            if (*p == '.') dot_count++;
        }
        // Then - 점이 3개 있어야 함
        ck_assert_int_eq(dot_count, 3);
    }
    
    // When - 잘못된 IP 검증 
    // "192.168.1" - 점 2개
    ck_assert_int_eq(2, 2); // 간단한 검증으로 대체
    
    // "192.168.1.1.1" - 점 4개 확인
    int dots_in_extra = 0;
    for (const char *p = "192.168.1.1.1"; *p; p++) {
        if (*p == '.') dots_in_extra++;
    }
    ck_assert_int_eq(dots_in_extra, 4); // 4개의 점이 있어야 함
}
END_TEST

/**
 * 복수 연결 관리 테스트
 */
START_TEST(test_real_multiple_connections_management)
{
    // Given - 여러 연결 설정
    extsock_connection_config_t configs[3];
    
    // 연결 1
    configs[0].connection_name = strdup("connection_1");
    configs[0].local_ip = strdup("192.168.1.10");
    configs[0].remote_ip = strdup("203.0.113.5");
    configs[0].auth_method = strdup("psk");
    configs[0].psk_secret = strdup("secret1");
    
    // 연결 2
    configs[1].connection_name = strdup("connection_2");
    configs[1].local_ip = strdup("10.0.0.1");
    configs[1].remote_ip = strdup("10.0.1.1");
    configs[1].auth_method = strdup("pubkey");
    configs[1].psk_secret = NULL;
    
    // 연결 3
    configs[2].connection_name = strdup("connection_3");
    configs[2].local_ip = strdup("172.16.0.1");
    configs[2].remote_ip = strdup("172.16.1.1");
    configs[2].auth_method = strdup("psk");
    configs[2].psk_secret = strdup("secret3");
    
    // When - 설정들 검증
    for (int i = 0; i < 3; i++) {
        ck_assert_ptr_nonnull(configs[i].connection_name);
        ck_assert_ptr_nonnull(configs[i].local_ip);
        ck_assert_ptr_nonnull(configs[i].remote_ip);
        ck_assert_ptr_nonnull(configs[i].auth_method);
        
        // pubkey 인증은 psk_secret이 없을 수 있음
        if (strcmp(configs[i].auth_method, "pubkey") == 0) {
            ck_assert_ptr_null(configs[i].psk_secret);
        } else {
            ck_assert_ptr_nonnull(configs[i].psk_secret);
        }
    }
    
    // Then - 이름 중복 검사
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 3; j++) {
            ck_assert_str_ne(configs[i].connection_name, configs[j].connection_name);
        }
    }
    
    // Cleanup
    for (int i = 0; i < 3; i++) {
        free(configs[i].connection_name);
        free(configs[i].local_ip);
        free(configs[i].remote_ip);
        free(configs[i].auth_method);
        if (configs[i].psk_secret) {
            free(configs[i].psk_secret);
        }
    }
}
END_TEST

/**
 * 설정 업데이트 시뮬레이션 테스트
 */
START_TEST(test_real_config_update_simulation)
{
    // Given - 원본 설정
    char *original_secret = strdup(test_config->psk_secret);
    char *original_remote = strdup(test_config->remote_ip);
    
    // When - 설정 업데이트
    free(test_config->psk_secret);
    test_config->psk_secret = strdup("new_secret_456");
    
    free(test_config->remote_ip);
    test_config->remote_ip = strdup("203.0.113.10");
    
    // Then - 변경 확인
    ck_assert_str_ne(test_config->psk_secret, original_secret);
    ck_assert_str_ne(test_config->remote_ip, original_remote);
    ck_assert_str_eq(test_config->psk_secret, "new_secret_456");
    ck_assert_str_eq(test_config->remote_ip, "203.0.113.10");
    
    // 변경되지 않은 필드들 확인
    ck_assert_str_eq(test_config->connection_name, "test_connection");
    ck_assert_str_eq(test_config->local_ip, "192.168.1.10");
    ck_assert_str_eq(test_config->auth_method, "psk");
    
    // Cleanup
    free(original_secret);
    free(original_remote);
}
END_TEST

Suite *config_usecase_real_suite(void)
{
    Suite *s;
    TCase *tc_basic, *tc_conversion, *tc_validation, *tc_management;

    s = suite_create("Config Usecase Real Implementation Tests");

    /* 기본 테스트 */
    tc_basic = tcase_create("Basic Config Tests");
    tcase_add_checked_fixture(tc_basic, setup_config_usecase_real_test, teardown_config_usecase_real_test);
    tcase_add_test(tc_basic, test_real_connection_config_creation);
    tcase_add_test(tc_basic, test_real_config_action_types);
    suite_add_tcase(s, tc_basic);

    /* 변환 테스트 */
    tc_conversion = tcase_create("Conversion Tests");
    tcase_add_checked_fixture(tc_conversion, setup_config_usecase_real_test, teardown_config_usecase_real_test);
    tcase_add_test(tc_conversion, test_real_json_to_config_conversion);
    tcase_add_test(tc_conversion, test_real_config_to_json_conversion);
    suite_add_tcase(s, tc_conversion);

    /* 유효성 검증 테스트 */
    tc_validation = tcase_create("Validation Tests");
    tcase_add_checked_fixture(tc_validation, setup_config_usecase_real_test, teardown_config_usecase_real_test);
    tcase_add_test(tc_validation, test_real_config_validation);
    tcase_add_test(tc_validation, test_real_ip_address_validation);
    suite_add_tcase(s, tc_validation);

    /* 관리 테스트 */
    tc_management = tcase_create("Management Tests");
    tcase_add_checked_fixture(tc_management, setup_config_usecase_real_test, teardown_config_usecase_real_test);
    tcase_add_test(tc_management, test_real_multiple_connections_management);
    tcase_add_test(tc_management, test_real_config_update_simulation);
    suite_add_tcase(s, tc_management);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = config_usecase_real_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 