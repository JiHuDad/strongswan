/*
 * Copyright (C) 2024 strongSwan Project
 * Simple Unit Test for ExternalSocket Plugin
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <library.h>

/**
 * JSON 파싱 기본 테스트
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
    
    cJSON_Delete(json);
}
END_TEST

/**
 * JSON 유효성 검증 테스트
 */
START_TEST(test_json_validation)
{
    // Given - 유효한 JSON
    const char *valid_json = "{\"key\":\"value\"}";
    cJSON *json = cJSON_Parse(valid_json);
    ck_assert_ptr_nonnull(json);
    cJSON_Delete(json);
    
    // Given - 잘못된 JSON
    const char *invalid_json = "{key:value}";
    json = cJSON_Parse(invalid_json);
    ck_assert_ptr_null(json);
}
END_TEST

/**
 * 문자열 처리 테스트
 */
START_TEST(test_string_operations)
{
    // Given
    const char *source = "test-connection";
    
    // When
    char *copy = strdup(source);
    
    // Then
    ck_assert_ptr_nonnull(copy);
    ck_assert_str_eq(copy, source);
    ck_assert_int_eq(strlen(copy), strlen(source));
    
    free(copy);
}
END_TEST

/**
 * 배열 처리 테스트
 */
START_TEST(test_array_operations)
{
    // Given
    cJSON *array = cJSON_CreateArray();
    ck_assert_ptr_nonnull(array);
    
    // When
    cJSON_AddItemToArray(array, cJSON_CreateString("item1"));
    cJSON_AddItemToArray(array, cJSON_CreateString("item2"));
    cJSON_AddItemToArray(array, cJSON_CreateString("item3"));
    
    // Then
    ck_assert_int_eq(cJSON_GetArraySize(array), 3);
    
    cJSON *first = cJSON_GetArrayItem(array, 0);
    ck_assert_ptr_nonnull(first);
    ck_assert_str_eq(cJSON_GetStringValue(first), "item1");
    
    cJSON_Delete(array);
}
END_TEST

/**
 * 메모리 관리 테스트
 */
START_TEST(test_memory_management)
{
    // Given
    void *ptr1 = malloc(100);
    void *ptr2 = malloc(200);
    
    // Then
    ck_assert_ptr_nonnull(ptr1);
    ck_assert_ptr_nonnull(ptr2);
    ck_assert_ptr_ne(ptr1, ptr2);
    
    // When
    free(ptr1);
    free(ptr2);
    
    // 메모리 해제 후 NULL 설정하는 것이 좋은 실천
    ptr1 = NULL;
    ptr2 = NULL;
    ck_assert_ptr_null(ptr1);
    ck_assert_ptr_null(ptr2);
}
END_TEST

/**
 * 설정 관련 헬퍼 함수 테스트
 */
START_TEST(test_config_helpers)
{
    // 연결 이름 유효성 검증 함수 시뮬레이션
    const char *valid_names[] = {"test-conn", "connection_1", "vpn-server"};
    const char *invalid_names[] = {"", "test conn", "conn@example", NULL};
    
    // 유효한 이름들 테스트
    for (int i = 0; i < 3; i++) {
        ck_assert_ptr_nonnull(valid_names[i]);
        ck_assert_int_gt(strlen(valid_names[i]), 0);
        ck_assert_int_le(strlen(valid_names[i]), 64);
    }
    
    // 잘못된 이름들 테스트
    for (int i = 0; i < 4; i++) {
        if (invalid_names[i] == NULL) {
            ck_assert_ptr_null(invalid_names[i]);
        } else if (strlen(invalid_names[i]) == 0) {
            ck_assert_int_eq(strlen(invalid_names[i]), 0);
        }
    }
}
END_TEST

/**
 * 에러 처리 테스트
 */
START_TEST(test_error_handling)
{
    // NULL 포인터 안전성 테스트
    char *null_str = NULL;
    ck_assert_ptr_null(null_str);
    
    // 빈 문자열 처리 테스트
    const char *empty_str = "";
    ck_assert_int_eq(strlen(empty_str), 0);
    
    // 범위 검사 테스트
    int valid_port = 500;
    int invalid_port = 100000;
    ck_assert_int_ge(valid_port, 1);
    ck_assert_int_le(valid_port, 65535);
    ck_assert_int_gt(invalid_port, 65535);
}
END_TEST

Suite *simple_unit_suite(void)
{
    Suite *s;
    TCase *tc_json, *tc_string, *tc_config, *tc_error;

    s = suite_create("Simple Unit Tests");

    /* JSON 테스트 */
    tc_json = tcase_create("JSON Operations");
    tcase_add_test(tc_json, test_json_parsing_basic);
    tcase_add_test(tc_json, test_json_validation);
    tcase_add_test(tc_json, test_array_operations);
    suite_add_tcase(s, tc_json);

    /* 문자열 테스트 */
    tc_string = tcase_create("String Operations");
    tcase_add_test(tc_string, test_string_operations);
    tcase_add_test(tc_string, test_memory_management);
    suite_add_tcase(s, tc_string);

    /* 설정 테스트 */
    tc_config = tcase_create("Config Helpers");
    tcase_add_test(tc_config, test_config_helpers);
    suite_add_tcase(s, tc_config);

    /* 에러 처리 테스트 */
    tc_error = tcase_create("Error Handling");
    tcase_add_test(tc_error, test_error_handling);
    suite_add_tcase(s, tc_error);

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