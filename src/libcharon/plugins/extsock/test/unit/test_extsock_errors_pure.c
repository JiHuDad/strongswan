/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Pure Unit Tests for extsock_errors module
 * TASK-005: extsock_errors 실제 테스트
 * 
 * These are Level 1 (Pure) unit tests that test business logic
 * without any strongSwan dependencies.
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Pure unit test types (without strongSwan dependencies)
#include "extsock_types_pure.h"

// Test fixtures
void setup_extsock_errors_test(void)
{
    // No setup needed for pure unit tests
}

void teardown_extsock_errors_test(void)
{
    // No teardown needed for pure unit tests
}

/*
 * ============================================================================
 * Error Creation and Destruction Tests
 * ============================================================================
 */

START_TEST(test_extsock_error_create_valid_input)
{
    // Given: Valid error code and message
    extsock_error_t code = EXTSOCK_ERROR_JSON_PARSE;
    const char *message = "Test error message";
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    // Then: Error info should be created correctly
    ck_assert_ptr_nonnull(error_info);
    ck_assert_int_eq(error_info->code, code);
    ck_assert_int_eq(error_info->severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    ck_assert_ptr_nonnull(error_info->message);
    ck_assert_str_eq(error_info->message, message);
    ck_assert_ptr_null(error_info->context);
    ck_assert_int_gt(error_info->timestamp, 0);
    ck_assert_int_gt(error_info->thread_id, 0);
    ck_assert(!error_info->recoverable);
    ck_assert(!error_info->retry_recommended);
    
    // Cleanup
    extsock_error_destroy(error_info);
}
END_TEST

START_TEST(test_extsock_error_create_null_message)
{
    // Given: Valid error code but NULL message
    extsock_error_t code = EXTSOCK_ERROR_CONFIG_INVALID;
    const char *message = NULL;
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    // Then: Error info should be created with NULL message
    ck_assert_ptr_nonnull(error_info);
    ck_assert_int_eq(error_info->code, code);
    ck_assert_ptr_null(error_info->message);
    ck_assert_int_eq(error_info->severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    
    // Cleanup
    extsock_error_destroy(error_info);
}
END_TEST

START_TEST(test_extsock_error_create_empty_message)
{
    // Given: Valid error code and empty message
    extsock_error_t code = EXTSOCK_ERROR_SOCKET_FAILED;
    const char *message = "";
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    // Then: Error info should be created with empty message
    ck_assert_ptr_nonnull(error_info);
    ck_assert_int_eq(error_info->code, code);
    ck_assert_ptr_nonnull(error_info->message);
    ck_assert_str_eq(error_info->message, "");
    
    // Cleanup
    extsock_error_destroy(error_info);
}
END_TEST

START_TEST(test_extsock_error_create_long_message)
{
    // Given: Very long message (test string handling)
    extsock_error_t code = EXTSOCK_ERROR_MEMORY_ALLOCATION;
    char long_message[1024];
    memset(long_message, 'A', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = '\0';
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(code, long_message);
    
    // Then: Error info should handle long message correctly
    ck_assert_ptr_nonnull(error_info);
    ck_assert_int_eq(error_info->code, code);
    ck_assert_ptr_nonnull(error_info->message);
    ck_assert_str_eq(error_info->message, long_message);
    ck_assert_int_eq(strlen(error_info->message), strlen(long_message));
    
    // Cleanup
    extsock_error_destroy(error_info);
}
END_TEST

START_TEST(test_extsock_error_destroy_null_pointer)
{
    // Given: NULL error info pointer
    extsock_error_info_t *error_info = NULL;
    
    // When: Destroy NULL pointer
    // Then: Should not crash
    extsock_error_destroy(error_info);
    
    // Test passes if we reach here without crashing
    ck_assert(true);
}
END_TEST

START_TEST(test_extsock_error_destroy_valid_pointer)
{
    // Given: Valid error info
    extsock_error_info_t *error_info = extsock_error_create(EXTSOCK_ERROR_STRONGSWAN_API, "Test message");
    ck_assert_ptr_nonnull(error_info);
    
    // When: Destroy valid pointer
    extsock_error_destroy(error_info);
    
    // Then: Should not crash (memory properly freed)
    // Note: We can't verify memory is freed without valgrind, but test should not crash
    ck_assert(true);
}
END_TEST

/*
 * ============================================================================
 * Error Code to String Conversion Tests
 * ============================================================================
 */

START_TEST(test_extsock_error_to_string_all_codes)
{
    // Test all defined error codes
    struct {
        extsock_error_t code;
        const char *expected;
    } test_cases[] = {
        { EXTSOCK_SUCCESS, "Success" },
        { EXTSOCK_ERROR_JSON_PARSE, "JSON Parse Error" },
        { EXTSOCK_ERROR_CONFIG_INVALID, "Invalid Configuration" },
        { EXTSOCK_ERROR_SOCKET_FAILED, "Socket Operation Failed" },
        { EXTSOCK_ERROR_MEMORY_ALLOCATION, "Memory Allocation Error" },
        { EXTSOCK_ERROR_STRONGSWAN_API, "strongSwan API Error" }
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        // When: Convert error code to string
        const char *result = extsock_error_to_string(test_cases[i].code);
        
        // Then: Should return expected string
        ck_assert_ptr_nonnull(result);
        ck_assert_str_eq(result, test_cases[i].expected);
    }
}
END_TEST

START_TEST(test_extsock_error_to_string_unknown_code)
{
    // Given: Unknown error code
    extsock_error_t unknown_code = (extsock_error_t)999;
    
    // When: Convert unknown code to string
    const char *result = extsock_error_to_string(unknown_code);
    
    // Then: Should return "Unknown Error"
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "Unknown Error");
}
END_TEST

/*
 * ============================================================================
 * Error Info Field Validation Tests
 * ============================================================================
 */

START_TEST(test_extsock_error_timestamp_validity)
{
    // Given: Current time before creating error
    time_t before = time(NULL);
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(EXTSOCK_ERROR_JSON_PARSE, "Test");
    
    // Given: Current time after creating error
    time_t after = time(NULL);
    
    // Then: Timestamp should be within reasonable range
    ck_assert_ptr_nonnull(error_info);
    ck_assert_int_ge(error_info->timestamp, before);
    ck_assert_int_le(error_info->timestamp, after);
    
    // Cleanup
    extsock_error_destroy(error_info);
}
END_TEST

START_TEST(test_extsock_error_thread_id_consistency)
{
    // When: Create multiple error infos in same thread
    extsock_error_info_t *error1 = extsock_error_create(EXTSOCK_ERROR_CONFIG_INVALID, "Error 1");
    extsock_error_info_t *error2 = extsock_error_create(EXTSOCK_ERROR_SOCKET_FAILED, "Error 2");
    
    // Then: Thread IDs should be the same and non-zero
    ck_assert_ptr_nonnull(error1);
    ck_assert_ptr_nonnull(error2);
    ck_assert_int_gt(error1->thread_id, 0);
    ck_assert_int_gt(error2->thread_id, 0);
    ck_assert_int_eq(error1->thread_id, error2->thread_id);
    
    // Cleanup
    extsock_error_destroy(error1);
    extsock_error_destroy(error2);
}
END_TEST

START_TEST(test_extsock_error_default_field_values)
{
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(EXTSOCK_ERROR_MEMORY_ALLOCATION, "Test");
    
    // Then: Default field values should be correct
    ck_assert_ptr_nonnull(error_info);
    ck_assert_int_eq(error_info->severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    ck_assert_ptr_null(error_info->context);
    ck_assert(!error_info->recoverable);
    ck_assert(!error_info->retry_recommended);
    
    // Cleanup
    extsock_error_destroy(error_info);
}
END_TEST

/*
 * ============================================================================
 * Memory Management and Edge Cases Tests
 * ============================================================================
 */

START_TEST(test_extsock_error_memory_ownership)
{
    // Given: Original message string
    char original_message[] = "Original message";
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(EXTSOCK_SUCCESS, original_message);
    ck_assert_ptr_nonnull(error_info);
    
    // When: Modify original string
    strcpy(original_message, "Modified message");
    
    // Then: Error info message should be unchanged (independent copy)
    ck_assert_str_eq(error_info->message, "Original message");
    ck_assert_str_ne(error_info->message, original_message);
    
    // Cleanup
    extsock_error_destroy(error_info);
}
END_TEST

START_TEST(test_extsock_error_multiple_create_destroy)
{
    // Test creating and destroying multiple error infos
    const int count = 10;
    extsock_error_info_t *errors[count];
    
    // When: Create multiple error infos
    for (int i = 0; i < count; i++) {
        char message[64];
        snprintf(message, sizeof(message), "Error message %d", i);
        errors[i] = extsock_error_create((extsock_error_t)(i % 6), message);
        ck_assert_ptr_nonnull(errors[i]);
    }
    
    // Then: All should be created successfully
    for (int i = 0; i < count; i++) {
        ck_assert_ptr_nonnull(errors[i]);
        ck_assert_int_eq(errors[i]->code, (extsock_error_t)(i % 6));
    }
    
    // When: Destroy all error infos
    for (int i = 0; i < count; i++) {
        extsock_error_destroy(errors[i]);
    }
    
    // Test passes if no crashes occur
    ck_assert(true);
}
END_TEST

/*
 * ============================================================================
 * Test Suite Definition
 * ============================================================================
 */

Suite *extsock_errors_pure_suite(void)
{
    Suite *s;
    TCase *tc_creation, *tc_conversion, *tc_validation, *tc_memory;

    s = suite_create("extsock_errors Pure Unit Tests");

    /* Error Creation and Destruction Tests */
    tc_creation = tcase_create("Error Creation/Destruction");
    tcase_add_checked_fixture(tc_creation, setup_extsock_errors_test, teardown_extsock_errors_test);
    tcase_add_test(tc_creation, test_extsock_error_create_valid_input);
    tcase_add_test(tc_creation, test_extsock_error_create_null_message);
    tcase_add_test(tc_creation, test_extsock_error_create_empty_message);
    tcase_add_test(tc_creation, test_extsock_error_create_long_message);
    tcase_add_test(tc_creation, test_extsock_error_destroy_null_pointer);
    tcase_add_test(tc_creation, test_extsock_error_destroy_valid_pointer);
    suite_add_tcase(s, tc_creation);

    /* Error Code to String Conversion Tests */
    tc_conversion = tcase_create("Error Code Conversion");
    tcase_add_checked_fixture(tc_conversion, setup_extsock_errors_test, teardown_extsock_errors_test);
    tcase_add_test(tc_conversion, test_extsock_error_to_string_all_codes);
    tcase_add_test(tc_conversion, test_extsock_error_to_string_unknown_code);
    suite_add_tcase(s, tc_conversion);

    /* Error Info Field Validation Tests */
    tc_validation = tcase_create("Field Validation");
    tcase_add_checked_fixture(tc_validation, setup_extsock_errors_test, teardown_extsock_errors_test);
    tcase_add_test(tc_validation, test_extsock_error_timestamp_validity);
    tcase_add_test(tc_validation, test_extsock_error_thread_id_consistency);
    tcase_add_test(tc_validation, test_extsock_error_default_field_values);
    suite_add_tcase(s, tc_validation);

    /* Memory Management Tests */
    tc_memory = tcase_create("Memory Management");
    tcase_add_checked_fixture(tc_memory, setup_extsock_errors_test, teardown_extsock_errors_test);
    tcase_add_test(tc_memory, test_extsock_error_memory_ownership);
    tcase_add_test(tc_memory, test_extsock_error_multiple_create_destroy);
    suite_add_tcase(s, tc_memory);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = extsock_errors_pure_suite();
    sr = srunner_create(s);

    printf("Running extsock_errors Pure Unit Tests (Level 1)...\n");
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    if (number_failed == 0) {
        printf("✅ All extsock_errors pure unit tests passed!\n");
        printf("Level 1 tests for error handling module completed successfully.\n");
    } else {
        printf("❌ %d extsock_errors pure unit test(s) failed.\n", number_failed);
    }

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}