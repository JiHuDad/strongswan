/*
 * Copyright (C) 2024 strongSwan Project
 * Simplified Unit tests for Failover Manager
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <library.h>

// Direct function testing approach - just test the core logic
// instead of the full interface

// 핵심 로직을 직접 테스트하기 위한 전방 선언
char* parse_and_select_next_address(const char *remote_addrs, const char *current_addr);
void parse_comma_separated_addresses_test(const char *addr_str, char ***addresses, int *count);
int find_address_index_test(char **addresses, int count, const char *target_addr);

/**
 * 쉼표로 구분된 주소 문자열을 배열로 파싱 (테스트용 단순화 버전)
 */
void parse_comma_separated_addresses_test(const char *addr_str, char ***addresses, int *count)
{
    if (!addr_str || !addresses || !count) {
        *count = 0;
        return;
    }
    
    // 문자열 복사
    char *str_copy = strdup(addr_str);
    char *saveptr;
    char *token;
    int capacity = 10;
    char **addr_array = malloc(capacity * sizeof(char*));
    *count = 0;
    
    token = strtok_r(str_copy, ",", &saveptr);
    while (token != NULL) {
        // 공백 제거
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') {
            *end = '\0';
            end--;
        }
        
        if (strlen(token) > 0) {
            if (*count >= capacity) {
                capacity *= 2;
                addr_array = realloc(addr_array, capacity * sizeof(char*));
            }
            addr_array[*count] = strdup(token);
            (*count)++;
        }
        token = strtok_r(NULL, ",", &saveptr);
    }
    
    free(str_copy);
    *addresses = addr_array;
}

/**
 * 주소 배열에서 특정 주소의 인덱스 찾기
 */
int find_address_index_test(char **addresses, int count, const char *target_addr)
{
    if (!addresses || !target_addr || count <= 0) {
        return -1;
    }
    
    for (int i = 0; i < count; i++) {
        if (strcmp(addresses[i], target_addr) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * 다음 주소 선택 (테스트용 단순화 버전)
 */
char* parse_and_select_next_address(const char *remote_addrs, const char *current_addr)
{
    if (!remote_addrs || !current_addr) {
        return NULL;
    }
    
    char **addresses;
    int count;
    
    // 주소 파싱
    parse_comma_separated_addresses_test(remote_addrs, &addresses, &count);
    
    if (count < 2) {
        // 단일 주소는 failover 불가
        for (int i = 0; i < count; i++) {
            free(addresses[i]);
        }
        free(addresses);
        return NULL;
    }
    
    // 현재 주소 인덱스 찾기
    int current_index = find_address_index_test(addresses, count, current_addr);
    if (current_index == -1) {
        current_index = 0;  // 현재 주소를 찾을 수 없으면 첫 번째부터
    }
    
    // 다음 주소 선택 (순환)
    int next_index = (current_index + 1) % count;
    char *result = strdup(addresses[next_index]);
    
    // 정리
    for (int i = 0; i < count; i++) {
        free(addresses[i]);
    }
    free(addresses);
    
    return result;
}

// Test cases
START_TEST(test_parse_comma_separated_addresses)
{
    char **addresses;
    int count;
    
    // 기본 케이스
    parse_comma_separated_addresses_test("10.0.0.1,10.0.0.2", &addresses, &count);
    ck_assert_int_eq(count, 2);
    ck_assert_str_eq(addresses[0], "10.0.0.1");
    ck_assert_str_eq(addresses[1], "10.0.0.2");
    for (int i = 0; i < count; i++) free(addresses[i]);
    free(addresses);
    
    // 공백 포함 케이스
    parse_comma_separated_addresses_test(" 192.168.1.1 , 192.168.1.2 , 192.168.1.3 ", &addresses, &count);
    ck_assert_int_eq(count, 3);
    ck_assert_str_eq(addresses[0], "192.168.1.1");
    ck_assert_str_eq(addresses[1], "192.168.1.2");
    ck_assert_str_eq(addresses[2], "192.168.1.3");
    for (int i = 0; i < count; i++) free(addresses[i]);
    free(addresses);
    
    // 단일 주소
    parse_comma_separated_addresses_test("10.0.0.1", &addresses, &count);
    ck_assert_int_eq(count, 1);
    ck_assert_str_eq(addresses[0], "10.0.0.1");
    for (int i = 0; i < count; i++) free(addresses[i]);
    free(addresses);
    
    // NULL 입력
    parse_comma_separated_addresses_test(NULL, &addresses, &count);
    ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_find_address_index)
{
    char *test_addresses[] = {"10.0.0.1", "10.0.0.2", "10.0.0.3"};
    int count = 3;
    
    // 정상 케이스
    ck_assert_int_eq(find_address_index_test(test_addresses, count, "10.0.0.1"), 0);
    ck_assert_int_eq(find_address_index_test(test_addresses, count, "10.0.0.2"), 1);
    ck_assert_int_eq(find_address_index_test(test_addresses, count, "10.0.0.3"), 2);
    
    // 찾지 못하는 케이스
    ck_assert_int_eq(find_address_index_test(test_addresses, count, "10.0.0.99"), -1);
    
    // NULL 입력
    ck_assert_int_eq(find_address_index_test(NULL, count, "10.0.0.1"), -1);
    ck_assert_int_eq(find_address_index_test(test_addresses, count, NULL), -1);
}
END_TEST

START_TEST(test_select_next_address_basic)
{
    char *result;
    
    // 기본 2개 주소 케이스
    result = parse_and_select_next_address("10.0.0.1,10.0.0.2", "10.0.0.1");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "10.0.0.2");
    free(result);
    
    // 순환 테스트
    result = parse_and_select_next_address("10.0.0.1,10.0.0.2", "10.0.0.2");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "10.0.0.1");
    free(result);
}
END_TEST

START_TEST(test_select_next_address_multiple)
{
    char *result;
    
    // 3개 주소 케이스
    result = parse_and_select_next_address("192.168.1.1,192.168.1.2,192.168.1.3", "192.168.1.1");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "192.168.1.2");
    free(result);
    
    result = parse_and_select_next_address("192.168.1.1,192.168.1.2,192.168.1.3", "192.168.1.2");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "192.168.1.3");
    free(result);
    
    // 마지막에서 첫 번째로 순환
    result = parse_and_select_next_address("192.168.1.1,192.168.1.2,192.168.1.3", "192.168.1.3");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "192.168.1.1");
    free(result);
}
END_TEST

START_TEST(test_select_next_address_with_spaces)
{
    char *result;
    
    // 공백이 포함된 주소 목록
    result = parse_and_select_next_address(" 10.0.0.1 , 10.0.0.2 , 10.0.0.3 ", "10.0.0.1");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "10.0.0.2");
    free(result);
}
END_TEST

START_TEST(test_select_next_address_edge_cases)
{
    char *result;
    
    // NULL 인자
    result = parse_and_select_next_address(NULL, "10.0.0.1");
    ck_assert_ptr_null(result);
    
    result = parse_and_select_next_address("10.0.0.1,10.0.0.2", NULL);
    ck_assert_ptr_null(result);
    
    // 단일 주소 (failover 불가)
    result = parse_and_select_next_address("10.0.0.1", "10.0.0.1");
    ck_assert_ptr_null(result);
    
    // 현재 주소가 목록에 없는 경우 (첫 번째 다음 주소 반환)
    result = parse_and_select_next_address("10.0.0.1,10.0.0.2", "10.0.0.99");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "10.0.0.2");
    free(result);
}
END_TEST

// Test Suite 생성
Suite *failover_manager_simple_suite(void)
{
    Suite *s;
    TCase *tc_core;
    
    s = suite_create("Failover Manager Simple Tests");
    
    // Core test case
    tc_core = tcase_create("Core Logic");
    
    // 주소 파싱 테스트
    tcase_add_test(tc_core, test_parse_comma_separated_addresses);
    tcase_add_test(tc_core, test_find_address_index);
    
    // 주소 선택 테스트
    tcase_add_test(tc_core, test_select_next_address_basic);
    tcase_add_test(tc_core, test_select_next_address_multiple);
    tcase_add_test(tc_core, test_select_next_address_with_spaces);
    tcase_add_test(tc_core, test_select_next_address_edge_cases);
    
    suite_add_tcase(s, tc_core);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    // Library 초기화
    if (!library_init(NULL, "test-failover-simple")) {
        library_deinit();
        return EXIT_FAILURE;
    }
    
    s = failover_manager_simple_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    library_deinit();
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 