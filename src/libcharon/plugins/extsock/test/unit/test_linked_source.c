/*
 * Copyright (C) 2024 strongSwan Project
 * Linked Source Tests - Phase 3
 * 실제 소스 파일을 컴파일 시간에 직접 링크하여 테스트
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/syscall.h>

/*
 * Strategy 4: 외부 링크 방식
 * 실제 object 파일을 링크하여 원본 함수 직접 호출
 */

// 필요한 타입들을 다시 정의 (실제 헤더 의존성 우회)
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_JSON_PARSE,
    EXTSOCK_ERROR_CONFIG_INVALID,
    EXTSOCK_ERROR_SOCKET_FAILED,
    EXTSOCK_ERROR_MEMORY_ALLOCATION,
    EXTSOCK_ERROR_STRONGSWAN_API
} extsock_error_t;

typedef enum {
    EXTSOCK_ERROR_SEVERITY_TRACE = 0,
    EXTSOCK_ERROR_SEVERITY_DEBUG,
    EXTSOCK_ERROR_SEVERITY_INFO,
    EXTSOCK_ERROR_SEVERITY_WARNING,
    EXTSOCK_ERROR_SEVERITY_ERROR,
    EXTSOCK_ERROR_SEVERITY_CRITICAL
} extsock_error_severity_t;

typedef struct extsock_error_info_t {
    extsock_error_t code;
    extsock_error_severity_t severity;
    char *message;
    char *context;
    time_t timestamp;
    uint32_t thread_id;
    bool recoverable;
    bool retry_recommended;
} extsock_error_info_t;

/*
 * 외부 함수 선언 (실제 extsock_errors.c에서 링크됨)
 * 이 함수들은 별도로 컴파일된 object 파일에서 가져옴
 */
extern extsock_error_info_t *extsock_error_create(extsock_error_t code, const char *message);
extern void extsock_error_destroy(extsock_error_info_t *error_info);

/**
 * 테스트 설정
 */
void setup_linked_source_test(void)
{
    printf("Starting linked source tests...\n");
}

/**
 * 테스트 해제
 */
void teardown_linked_source_test(void)
{
    printf("Linked source tests completed.\n");
}

/**
 * 실제 링크된 함수 기본 테스트
 */
START_TEST(test_linked_error_create_basic)
{
    // When - 실제 링크된 함수 호출
    extsock_error_info_t *error = extsock_error_create(EXTSOCK_ERROR_CONFIG_INVALID, "linked test");
    
    // Then
    ck_assert_ptr_nonnull(error);
    ck_assert_int_eq(error->code, EXTSOCK_ERROR_CONFIG_INVALID);
    ck_assert_str_eq(error->message, "linked test");
    ck_assert_int_eq(error->severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    ck_assert(error->timestamp > 0);
    ck_assert(error->thread_id > 0);
    ck_assert_int_eq(error->recoverable, false);
    ck_assert_int_eq(error->retry_recommended, false);
    
    // Cleanup
    extsock_error_destroy(error);
}
END_TEST

/**
 * 실제 함수 호출로 NULL 메시지 테스트
 */
START_TEST(test_linked_error_null_message)
{
    // When
    extsock_error_info_t *error = extsock_error_create(EXTSOCK_ERROR_JSON_PARSE, NULL);
    
    // Then
    ck_assert_ptr_nonnull(error);
    ck_assert_int_eq(error->code, EXTSOCK_ERROR_JSON_PARSE);
    ck_assert_ptr_null(error->message);
    ck_assert_ptr_null(error->context);
    
    // Cleanup
    extsock_error_destroy(error);
}
END_TEST

/**
 * 실제 함수 안전성 테스트
 */
START_TEST(test_linked_error_safety)
{
    // Given
    extsock_error_info_t *error = extsock_error_create(EXTSOCK_ERROR_MEMORY_ALLOCATION, "safety test");
    ck_assert_ptr_nonnull(error);
    
    // When - 정상 소멸
    extsock_error_destroy(error);
    
    // When - NULL 포인터 안전성 테스트
    extsock_error_destroy(NULL); // 실제 함수가 안전하게 처리해야 함
    
    // Then - 정상적으로 완료되면 성공
    ck_assert_int_eq(1, 1);
}
END_TEST

/**
 * 모든 에러 코드 실제 함수 테스트
 */
START_TEST(test_linked_error_all_codes)
{
    extsock_error_t codes[] = {
        EXTSOCK_SUCCESS,
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID,
        EXTSOCK_ERROR_SOCKET_FAILED,
        EXTSOCK_ERROR_MEMORY_ALLOCATION,
        EXTSOCK_ERROR_STRONGSWAN_API
    };
    
    const char *messages[] = {
        "success test",
        "json parse error",
        "config invalid error", 
        "socket failed error",
        "memory allocation error",
        "strongswan api error"
    };
    
    for (int i = 0; i < sizeof(codes)/sizeof(codes[0]); i++) {
        extsock_error_info_t *error = extsock_error_create(codes[i], messages[i]);
        ck_assert_ptr_nonnull(error);
        ck_assert_int_eq(error->code, codes[i]);
        ck_assert_str_eq(error->message, messages[i]);
        extsock_error_destroy(error);
    }
}
END_TEST

/**
 * 실제 함수 성능 테스트
 */
START_TEST(test_linked_error_performance)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // 1000개 에러 객체 생성/소멸로 성능 측정
    for (int i = 0; i < 1000; i++) {
        char msg[100];
        snprintf(msg, sizeof(msg), "performance test %d", i);
        
        extsock_error_info_t *error = extsock_error_create(EXTSOCK_ERROR_SOCKET_FAILED, msg);
        ck_assert_ptr_nonnull(error);
        extsock_error_destroy(error);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long duration_ms = (end.tv_sec - start.tv_sec) * 1000 + 
                       (end.tv_nsec - start.tv_nsec) / 1000000;
    
    printf("Performance test: 1000 operations completed in %ld ms\n", duration_ms);
    
    // 성능이 너무 느리지 않은지 확인 (10초 이내)
    ck_assert(duration_ms < 10000);
}
END_TEST

/**
 * 실제 함수 메모리 정확성 테스트
 */
START_TEST(test_linked_error_memory_correctness)
{
    // 메모리 정렬 확인
    extsock_error_info_t *error = extsock_error_create(EXTSOCK_ERROR_CONFIG_INVALID, "alignment test");
    ck_assert_ptr_nonnull(error);
    
    // 구조체 포인터가 올바르게 정렬되었는지 확인
    ck_assert_int_eq((uintptr_t)error % sizeof(void*), 0);
    
    // 멤버 접근이 모두 정상적인지 확인
    ck_assert_int_eq(error->code, EXTSOCK_ERROR_CONFIG_INVALID);
    ck_assert_int_eq(error->severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    ck_assert_ptr_nonnull(error->message);
    ck_assert_ptr_null(error->context);
    ck_assert(error->timestamp > 0);
    ck_assert(error->thread_id > 0);
    ck_assert_int_eq(error->recoverable, false);
    ck_assert_int_eq(error->retry_recommended, false);
    
    extsock_error_destroy(error);
}
END_TEST

Suite *linked_source_suite(void)
{
    Suite *s;
    TCase *tc_basic, *tc_advanced, *tc_performance;
    
    s = suite_create("Linked Source Tests");
    
    /* 기본 링크 테스트 */
    tc_basic = tcase_create("Basic Linked Functions");
    tcase_add_checked_fixture(tc_basic, setup_linked_source_test, teardown_linked_source_test);
    tcase_add_test(tc_basic, test_linked_error_create_basic);
    tcase_add_test(tc_basic, test_linked_error_null_message);
    tcase_add_test(tc_basic, test_linked_error_safety);
    suite_add_tcase(s, tc_basic);
    
    /* 고급 테스트 */
    tc_advanced = tcase_create("Advanced Linked Tests");
    tcase_add_test(tc_advanced, test_linked_error_all_codes);
    tcase_add_test(tc_advanced, test_linked_error_memory_correctness);
    suite_add_tcase(s, tc_advanced);
    
    /* 성능 테스트 */
    tc_performance = tcase_create("Performance Tests");
    tcase_add_test(tc_performance, test_linked_error_performance);
    suite_add_tcase(s, tc_performance);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    printf("=== Linked Source Tests ===\n");
    printf("Testing actual object file linking\n\n");
    
    s = linked_source_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    printf("\n=== Test Results ===\n");
    printf("Failed tests: %d\n", number_failed);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 