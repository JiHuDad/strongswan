/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Pure Unit Tests for extsock_types module
 * TASK-006: extsock_types 실제 테스트
 * 
 * These are Level 1 (Pure) unit tests that test type definitions,
 * enums, and constants without any strongSwan dependencies.
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Pure unit test types (without strongSwan dependencies)
#include "extsock_types_pure.h"

// Test fixtures
void setup_extsock_types_test(void)
{
    // No setup needed for pure unit tests
}

void teardown_extsock_types_test(void)
{
    // No teardown needed for pure unit tests
}

/*
 * ============================================================================
 * Error Code Enum Tests
 * ============================================================================
 */

START_TEST(test_extsock_error_enum_values)
{
    // Test that error codes have expected values and ordering
    ck_assert_int_eq(EXTSOCK_SUCCESS, 0);
    ck_assert_int_eq(EXTSOCK_ERROR_JSON_PARSE, 1);
    ck_assert_int_eq(EXTSOCK_ERROR_CONFIG_INVALID, 2);
    ck_assert_int_eq(EXTSOCK_ERROR_SOCKET_FAILED, 3);
    ck_assert_int_eq(EXTSOCK_ERROR_MEMORY_ALLOCATION, 4);
    ck_assert_int_eq(EXTSOCK_ERROR_STRONGSWAN_API, 5);
    ck_assert_int_eq(EXTSOCK_ERROR_INVALID_PARAMETER, 6);
    ck_assert_int_eq(EXTSOCK_ERROR_CONFIG_CREATION_FAILED, 7);
}
END_TEST

START_TEST(test_extsock_error_enum_uniqueness)
{
    // Test that all error codes are unique
    extsock_error_t errors[] = {
        EXTSOCK_SUCCESS,
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID,
        EXTSOCK_ERROR_SOCKET_FAILED,
        EXTSOCK_ERROR_MEMORY_ALLOCATION,
        EXTSOCK_ERROR_STRONGSWAN_API,
        EXTSOCK_ERROR_INVALID_PARAMETER,
        EXTSOCK_ERROR_CONFIG_CREATION_FAILED
    };
    
    size_t count = sizeof(errors) / sizeof(errors[0]);
    
    // Check that each error code is unique
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            ck_assert_int_ne(errors[i], errors[j]);
        }
    }
}
END_TEST

START_TEST(test_extsock_error_enum_range)
{
    // Test that error codes are within reasonable range
    extsock_error_t errors[] = {
        EXTSOCK_SUCCESS,
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID,
        EXTSOCK_ERROR_SOCKET_FAILED,
        EXTSOCK_ERROR_MEMORY_ALLOCATION,
        EXTSOCK_ERROR_STRONGSWAN_API,
        EXTSOCK_ERROR_INVALID_PARAMETER,
        EXTSOCK_ERROR_CONFIG_CREATION_FAILED
    };
    
    size_t count = sizeof(errors) / sizeof(errors[0]);
    
    for (size_t i = 0; i < count; i++) {
        // Error codes should be non-negative and reasonable
        ck_assert_int_ge(errors[i], 0);
        ck_assert_int_lt(errors[i], 100); // Reasonable upper bound
    }
}
END_TEST

/*
 * ============================================================================
 * Error Severity Enum Tests
 * ============================================================================
 */

START_TEST(test_extsock_error_severity_values)
{
    // Test severity levels have expected values and ordering
    ck_assert_int_eq(EXTSOCK_ERROR_SEVERITY_TRACE, 0);
    ck_assert_int_eq(EXTSOCK_ERROR_SEVERITY_DEBUG, 1);
    ck_assert_int_eq(EXTSOCK_ERROR_SEVERITY_INFO, 2);
    ck_assert_int_eq(EXTSOCK_ERROR_SEVERITY_WARNING, 3);
    ck_assert_int_eq(EXTSOCK_ERROR_SEVERITY_ERROR, 4);
    ck_assert_int_eq(EXTSOCK_ERROR_SEVERITY_CRITICAL, 5);
}
END_TEST

START_TEST(test_extsock_error_severity_ordering)
{
    // Test that severity levels are in correct order (ascending severity)
    ck_assert_int_lt(EXTSOCK_ERROR_SEVERITY_TRACE, EXTSOCK_ERROR_SEVERITY_DEBUG);
    ck_assert_int_lt(EXTSOCK_ERROR_SEVERITY_DEBUG, EXTSOCK_ERROR_SEVERITY_INFO);
    ck_assert_int_lt(EXTSOCK_ERROR_SEVERITY_INFO, EXTSOCK_ERROR_SEVERITY_WARNING);
    ck_assert_int_lt(EXTSOCK_ERROR_SEVERITY_WARNING, EXTSOCK_ERROR_SEVERITY_ERROR);
    ck_assert_int_lt(EXTSOCK_ERROR_SEVERITY_ERROR, EXTSOCK_ERROR_SEVERITY_CRITICAL);
}
END_TEST

START_TEST(test_extsock_error_severity_uniqueness)
{
    // Test that all severity levels are unique
    extsock_error_severity_t severities[] = {
        EXTSOCK_ERROR_SEVERITY_TRACE,
        EXTSOCK_ERROR_SEVERITY_DEBUG,
        EXTSOCK_ERROR_SEVERITY_INFO,
        EXTSOCK_ERROR_SEVERITY_WARNING,
        EXTSOCK_ERROR_SEVERITY_ERROR,
        EXTSOCK_ERROR_SEVERITY_CRITICAL
    };
    
    size_t count = sizeof(severities) / sizeof(severities[0]);
    
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            ck_assert_int_ne(severities[i], severities[j]);
        }
    }
}
END_TEST

/*
 * ============================================================================
 * Error Info Structure Tests
 * ============================================================================
 */

START_TEST(test_extsock_error_info_struct_size)
{
    // Test that the struct size is reasonable
    size_t struct_size = sizeof(extsock_error_info_t);
    
    // Should be large enough to hold all fields
    ck_assert_int_gt(struct_size, 0);
    
    // Should not be excessively large (reasonable upper bound)
    ck_assert_int_lt(struct_size, 1024); // 1KB is more than enough
    
    printf("extsock_error_info_t size: %zu bytes\n", struct_size);
}
END_TEST

START_TEST(test_extsock_error_info_field_offsets)
{
    // Test that all fields can be accessed (basic structural test)
    extsock_error_info_t info = {0};
    
    // Set fields to known values
    info.code = EXTSOCK_ERROR_JSON_PARSE;
    info.severity = EXTSOCK_ERROR_SEVERITY_ERROR;
    info.message = NULL;
    info.context = NULL;
    info.timestamp = 12345;
    info.thread_id = 67890;
    info.recoverable = true;
    info.retry_recommended = false;
    
    // Verify fields can be read back correctly
    ck_assert_int_eq(info.code, EXTSOCK_ERROR_JSON_PARSE);
    ck_assert_int_eq(info.severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    ck_assert_ptr_null(info.message);
    ck_assert_ptr_null(info.context);
    ck_assert_int_eq(info.timestamp, 12345);
    ck_assert_int_eq(info.thread_id, 67890);
    ck_assert(info.recoverable);
    ck_assert(!info.retry_recommended);
}
END_TEST

START_TEST(test_extsock_error_info_field_types)
{
    // Test field type compatibility and basic operations
    extsock_error_info_t info = {0};
    
    // Test error code assignment and comparison
    info.code = EXTSOCK_ERROR_CONFIG_INVALID;
    ck_assert(info.code == EXTSOCK_ERROR_CONFIG_INVALID);
    ck_assert(info.code != EXTSOCK_SUCCESS);
    
    // Test severity assignment and comparison
    info.severity = EXTSOCK_ERROR_SEVERITY_CRITICAL;
    ck_assert(info.severity == EXTSOCK_ERROR_SEVERITY_CRITICAL);
    ck_assert(info.severity != EXTSOCK_ERROR_SEVERITY_TRACE);
    
    // Test boolean fields
    info.recoverable = true;
    info.retry_recommended = false;
    ck_assert(info.recoverable == true);
    ck_assert(info.retry_recommended == false);
    
    // Test pointer fields
    info.message = (char*)"test";
    info.context = (char*)"context";
    ck_assert_ptr_nonnull(info.message);
    ck_assert_ptr_nonnull(info.context);
    
    // Test numeric fields
    info.timestamp = 1234567890;
    info.thread_id = 42;
    ck_assert_int_eq(info.timestamp, 1234567890);
    ck_assert_int_eq(info.thread_id, 42);
}
END_TEST

/*
 * ============================================================================
 * Type Compatibility Tests
 * ============================================================================
 */

START_TEST(test_extsock_error_type_arithmetic)
{
    // Test that error codes can be used in arithmetic operations
    extsock_error_t base = EXTSOCK_SUCCESS;
    extsock_error_t next = base + 1;
    
    ck_assert_int_eq(next, EXTSOCK_ERROR_JSON_PARSE);
    
    // Test that we can compare error codes
    ck_assert(EXTSOCK_SUCCESS < EXTSOCK_ERROR_JSON_PARSE);
    ck_assert(EXTSOCK_ERROR_JSON_PARSE < EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

START_TEST(test_extsock_error_severity_arithmetic)
{
    // Test that severity levels can be compared
    ck_assert(EXTSOCK_ERROR_SEVERITY_TRACE < EXTSOCK_ERROR_SEVERITY_CRITICAL);
    ck_assert(EXTSOCK_ERROR_SEVERITY_ERROR > EXTSOCK_ERROR_SEVERITY_WARNING);
    
    // Test arithmetic operations
    extsock_error_severity_t high = EXTSOCK_ERROR_SEVERITY_CRITICAL;
    extsock_error_severity_t low = EXTSOCK_ERROR_SEVERITY_TRACE;
    
    ck_assert(high > low);
    ck_assert((high - low) > 0);
}
END_TEST

START_TEST(test_type_casting_safety)
{
    // Test safe casting between related types
    int error_as_int = (int)EXTSOCK_ERROR_JSON_PARSE;
    extsock_error_t error_from_int = (extsock_error_t)error_as_int;
    
    ck_assert_int_eq(error_from_int, EXTSOCK_ERROR_JSON_PARSE);
    
    // Test severity casting
    int severity_as_int = (int)EXTSOCK_ERROR_SEVERITY_ERROR;
    extsock_error_severity_t severity_from_int = (extsock_error_severity_t)severity_as_int;
    
    ck_assert_int_eq(severity_from_int, EXTSOCK_ERROR_SEVERITY_ERROR);
}
END_TEST

/*
 * ============================================================================
 * Constants and Boundaries Tests
 * ============================================================================
 */

START_TEST(test_success_code_properties)
{
    // Test properties of the success code
    ck_assert_int_eq(EXTSOCK_SUCCESS, 0);
    
    // Success should be the smallest value
    ck_assert(EXTSOCK_SUCCESS < EXTSOCK_ERROR_JSON_PARSE);
    ck_assert(EXTSOCK_SUCCESS < EXTSOCK_ERROR_CONFIG_INVALID);
    ck_assert(EXTSOCK_SUCCESS < EXTSOCK_ERROR_SOCKET_FAILED);
    
    // Test that success can be used in boolean context
    ck_assert(!EXTSOCK_SUCCESS); // Should be falsy
}
END_TEST

START_TEST(test_error_code_properties)
{
    // All error codes should be non-zero (truthy in boolean context)
    ck_assert(EXTSOCK_ERROR_JSON_PARSE);
    ck_assert(EXTSOCK_ERROR_CONFIG_INVALID);
    ck_assert(EXTSOCK_ERROR_SOCKET_FAILED);
    ck_assert(EXTSOCK_ERROR_MEMORY_ALLOCATION);
    ck_assert(EXTSOCK_ERROR_STRONGSWAN_API);
    ck_assert(EXTSOCK_ERROR_INVALID_PARAMETER);
    ck_assert(EXTSOCK_ERROR_CONFIG_CREATION_FAILED);
}
END_TEST

/*
 * ============================================================================
 * Test Suite Definition
 * ============================================================================
 */

Suite *extsock_types_pure_suite(void)
{
    Suite *s;
    TCase *tc_error_enum, *tc_severity_enum, *tc_struct, *tc_compatibility, *tc_constants;

    s = suite_create("extsock_types Pure Unit Tests");

    /* Error Code Enum Tests */
    tc_error_enum = tcase_create("Error Code Enum");
    tcase_add_checked_fixture(tc_error_enum, setup_extsock_types_test, teardown_extsock_types_test);
    tcase_add_test(tc_error_enum, test_extsock_error_enum_values);
    tcase_add_test(tc_error_enum, test_extsock_error_enum_uniqueness);
    tcase_add_test(tc_error_enum, test_extsock_error_enum_range);
    suite_add_tcase(s, tc_error_enum);

    /* Error Severity Enum Tests */
    tc_severity_enum = tcase_create("Error Severity Enum");
    tcase_add_checked_fixture(tc_severity_enum, setup_extsock_types_test, teardown_extsock_types_test);
    tcase_add_test(tc_severity_enum, test_extsock_error_severity_values);
    tcase_add_test(tc_severity_enum, test_extsock_error_severity_ordering);
    tcase_add_test(tc_severity_enum, test_extsock_error_severity_uniqueness);
    suite_add_tcase(s, tc_severity_enum);

    /* Error Info Structure Tests */
    tc_struct = tcase_create("Error Info Structure");
    tcase_add_checked_fixture(tc_struct, setup_extsock_types_test, teardown_extsock_types_test);
    tcase_add_test(tc_struct, test_extsock_error_info_struct_size);
    tcase_add_test(tc_struct, test_extsock_error_info_field_offsets);
    tcase_add_test(tc_struct, test_extsock_error_info_field_types);
    suite_add_tcase(s, tc_struct);

    /* Type Compatibility Tests */
    tc_compatibility = tcase_create("Type Compatibility");
    tcase_add_checked_fixture(tc_compatibility, setup_extsock_types_test, teardown_extsock_types_test);
    tcase_add_test(tc_compatibility, test_extsock_error_type_arithmetic);
    tcase_add_test(tc_compatibility, test_extsock_error_severity_arithmetic);
    tcase_add_test(tc_compatibility, test_type_casting_safety);
    suite_add_tcase(s, tc_compatibility);

    /* Constants and Boundaries Tests */
    tc_constants = tcase_create("Constants and Boundaries");
    tcase_add_checked_fixture(tc_constants, setup_extsock_types_test, teardown_extsock_types_test);
    tcase_add_test(tc_constants, test_success_code_properties);
    tcase_add_test(tc_constants, test_error_code_properties);
    suite_add_tcase(s, tc_constants);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = extsock_types_pure_suite();
    sr = srunner_create(s);

    printf("Running extsock_types Pure Unit Tests (Level 1)...\n");
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    if (number_failed == 0) {
        printf("✅ All extsock_types pure unit tests passed!\n");
        printf("Level 1 tests for type definitions completed successfully.\n");
    } else {
        printf("❌ %d extsock_types pure unit test(s) failed.\n", number_failed);
    }

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}