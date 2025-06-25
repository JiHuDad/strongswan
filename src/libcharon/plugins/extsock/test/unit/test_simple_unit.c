/*
 * Copyright (C) 2024 strongSwan Project
 * Simple Unit Test for ExternalSocket Plugin - Real Implementation Test
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <library.h>

// 실제 extsock 에러 처리 함수들 포함
#include "../common/extsock_errors.h"

/**
 * 실제 extsock 에러 생성 함수 테스트
 */
START_TEST(test_extsock_error_create)
{
    // Given & When
    extsock_error_info_t *error1 = extsock_error_create(EXTSOCK_ERROR_JSON_PARSE, "JSON parse error");
    extsock_error_info_t *error2 = extsock_error_create(EXTSOCK_ERROR_SOCKET_FAILED, "Socket error occurred");
    extsock_error_info_t *error3 = extsock_error_create(EXTSOCK_ERROR_CONFIG_INVALID, NULL);
    
    // Then
    ck_assert_ptr_nonnull(error1);
    ck_assert_int_eq(error1->code, EXTSOCK_ERROR_JSON_PARSE);
    ck_assert_ptr_nonnull(error1->message);
    ck_assert_str_eq(error1->message, "JSON parse error");
    
    ck_assert_ptr_nonnull(error2);
    ck_assert_int_eq(error2->code, EXTSOCK_ERROR_SOCKET_FAILED);
    ck_assert_ptr_nonnull(error2->message);
    ck_assert_str_eq(error2->message, "Socket error occurred");
    
    ck_assert_ptr_nonnull(error3);
    ck_assert_int_eq(error3->code, EXTSOCK_ERROR_CONFIG_INVALID);
    ck_assert_ptr_null(error3->message);
    
    // Cleanup
    extsock_error_destroy(error1);
    extsock_error_destroy(error2);
    extsock_error_destroy(error3);
}
END_TEST

/**
 * 실제 extsock 에러 해제 함수 테스트
 */
START_TEST(test_extsock_error_destroy)
{
    // Given
    extsock_error_info_t *error = extsock_error_create(EXTSOCK_ERROR_CONFIG_INVALID, "Test message");
    ck_assert_ptr_nonnull(error);
    
    // When & Then - 크래시되지 않으면 성공
    extsock_error_destroy(error);
    
    // NULL 포인터로 호출해도 크래시되지 않는지 테스트
    extsock_error_destroy(NULL);
}
END_TEST

/**
 * 다양한 에러 코드로 에러 생성 테스트
 */
START_TEST(test_extsock_error_various_codes)
{
    // Given & When
    extsock_error_info_t *errors[5];
    errors[0] = extsock_error_create(EXTSOCK_ERROR_JSON_PARSE, "JSON parse issue");
    errors[1] = extsock_error_create(EXTSOCK_ERROR_CONFIG_INVALID, "Config issue");
    errors[2] = extsock_error_create(EXTSOCK_ERROR_SOCKET_FAILED, "Socket issue");
    errors[3] = extsock_error_create(EXTSOCK_ERROR_MEMORY_ALLOCATION, "Memory issue");
    errors[4] = extsock_error_create(EXTSOCK_ERROR_STRONGSWAN_API, "strongSwan API issue");
    
    // Then
    for (int i = 0; i < 5; i++) {
        ck_assert_ptr_nonnull(errors[i]);
        ck_assert_ptr_nonnull(errors[i]->message);
    }
    
    ck_assert_int_eq(errors[0]->code, EXTSOCK_ERROR_JSON_PARSE);
    ck_assert_int_eq(errors[1]->code, EXTSOCK_ERROR_CONFIG_INVALID);
    ck_assert_int_eq(errors[2]->code, EXTSOCK_ERROR_SOCKET_FAILED);
    ck_assert_int_eq(errors[3]->code, EXTSOCK_ERROR_MEMORY_ALLOCATION);
    ck_assert_int_eq(errors[4]->code, EXTSOCK_ERROR_STRONGSWAN_API);
    
    // Cleanup
    for (int i = 0; i < 5; i++) {
        extsock_error_destroy(errors[i]);
    }
}
END_TEST

/**
 * 긴 에러 메시지 처리 테스트
 */
START_TEST(test_extsock_error_long_message)
{
    // Given
    char long_message[1000];
    for (int i = 0; i < 999; i++) {
        long_message[i] = 'A' + (i % 26);
    }
    long_message[999] = '\0';
    
    // When
    extsock_error_info_t *error = extsock_error_create(EXTSOCK_ERROR_MEMORY_ALLOCATION, long_message);
    
    // Then
    ck_assert_ptr_nonnull(error);
    ck_assert_int_eq(error->code, EXTSOCK_ERROR_MEMORY_ALLOCATION);
    ck_assert_ptr_nonnull(error->message);
    ck_assert_str_eq(error->message, long_message);
    
    // Cleanup
    extsock_error_destroy(error);
}
END_TEST

/**
 * 메모리 할당 실패 시뮬레이션 테스트 (정상 환경에서는 성공)
 */
START_TEST(test_extsock_error_memory_conditions)
{
    // Given & When - 여러 에러를 동시에 생성
    extsock_error_info_t *errors[100];
    int created_count = 0;
    
    for (int i = 0; i < 100; i++) {
        char message[50];
        snprintf(message, sizeof(message), "Error message %d", i);
        errors[i] = extsock_error_create(EXTSOCK_ERROR_JSON_PARSE, message);
        if (errors[i]) {
            created_count++;
        }
    }
    
    // Then - 정상 환경에서는 모두 성공해야 함
    ck_assert_int_eq(created_count, 100);
    
    // Cleanup
    for (int i = 0; i < 100; i++) {
        if (errors[i]) {
            extsock_error_destroy(errors[i]);
        }
    }
}
END_TEST

/**
 * JSON 파싱 기본 테스트 (기존 테스트 유지)
 */
START_TEST(test_json_parsing_basic)
{
    // Given
    const char *json_str = "{\"name\":\"test\",\"value\":123}";
    
    // When
    cJSON *json = cJSON_Parse(json_str);
    
    // Then
    ck_assert_ptr_nonnull(json);
    
    cJSON *name = cJSON_GetObjectItem(json, "name");
    ck_assert_ptr_nonnull(name);
    ck_assert(cJSON_IsString(name));
    ck_assert_str_eq(cJSON_GetStringValue(name), "test");
    
    cJSON *value = cJSON_GetObjectItem(json, "value");
    ck_assert_ptr_nonnull(value);
    ck_assert(cJSON_IsNumber(value));
    ck_assert_int_eq(cJSON_GetNumberValue(value), 123);
    
    // Cleanup
    cJSON_Delete(json);
}
END_TEST

/**
 * JSON 생성 테스트
 */
START_TEST(test_json_creation_basic)
{
    // Given & When
    cJSON *json = cJSON_CreateObject();
    ck_assert_ptr_nonnull(json);
    
    cJSON_AddStringToObject(json, "type", "test");
    cJSON_AddNumberToObject(json, "id", 42);
    
    char *json_string = cJSON_Print(json);
    ck_assert_ptr_nonnull(json_string);
    
    // Then
    ck_assert(strstr(json_string, "test") != NULL);
    ck_assert(strstr(json_string, "42") != NULL);
    
    // Cleanup
    free(json_string);
    cJSON_Delete(json);
}
END_TEST

/**
 * 메모리 할당 테스트
 */
START_TEST(test_memory_allocation)
{
    // Given & When
    void *ptr1 = malloc(100);
    void *ptr2 = malloc(1000);
    void *ptr3 = malloc(10000);
    
    // Then
    ck_assert_ptr_nonnull(ptr1);
    ck_assert_ptr_nonnull(ptr2);
    ck_assert_ptr_nonnull(ptr3);
    
    // 메모리 접근 테스트
    memset(ptr1, 0, 100);
    memset(ptr2, 1, 1000);
    memset(ptr3, 2, 10000);
    
    // Cleanup
    free(ptr1);
    free(ptr2);
    free(ptr3);
}
END_TEST

Suite *simple_unit_suite(void)
{
    Suite *s;
    TCase *tc_extsock, *tc_json, *tc_memory;

    s = suite_create("Simple Unit Tests");

    /* ExternalSocket 에러 처리 테스트 */
    tc_extsock = tcase_create("ExternalSocket Errors");
    tcase_add_test(tc_extsock, test_extsock_error_create);
    tcase_add_test(tc_extsock, test_extsock_error_destroy);
    tcase_add_test(tc_extsock, test_extsock_error_various_codes);
    tcase_add_test(tc_extsock, test_extsock_error_long_message);
    tcase_add_test(tc_extsock, test_extsock_error_memory_conditions);
    suite_add_tcase(s, tc_extsock);

    /* JSON 처리 테스트 */
    tc_json = tcase_create("JSON Processing");
    tcase_add_test(tc_json, test_json_parsing_basic);
    tcase_add_test(tc_json, test_json_creation_basic);
    suite_add_tcase(s, tc_json);

    /* 메모리 관리 테스트 */
    tc_memory = tcase_create("Memory Management");
    tcase_add_test(tc_memory, test_memory_allocation);
    suite_add_tcase(s, tc_memory);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = simple_unit_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 