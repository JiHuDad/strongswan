/**
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Test Unit Tests for extsock_errors module
 * Migrated from test_extsock_errors_pure.c
 * 
 * Level 1 (Pure) unit tests that test business logic
 * without any strongSwan dependencies.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <ctime>
#include <unistd.h>

// Pure unit test types (without strongSwan dependencies)
extern "C" {
    #include "extsock_errors_pure.h"
    #include "extsock_types_pure.h"
}

/**
 * ============================================================================
 * Test Fixture Class
 * ============================================================================
 */
class ExtsockErrorsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // No setup needed for pure unit tests
    }

    void TearDown() override {
        // No teardown needed for pure unit tests  
    }
};

/**
 * ============================================================================
 * Error Creation and Destruction Tests
 * ============================================================================
 */

TEST_F(ExtsockErrorsTest, CreateValidInput) {
    // Given: Valid error code and message
    extsock_error_t code = EXTSOCK_ERROR_JSON_PARSE;
    const char *message = "Test error message";
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    // Then: Error info should be created correctly
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, code);
    EXPECT_EQ(error_info->severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    EXPECT_NE(error_info->message, nullptr);
    EXPECT_STREQ(error_info->message, message);
    EXPECT_EQ(error_info->context, nullptr);
    EXPECT_GT(error_info->timestamp, 0);
    EXPECT_GT(error_info->thread_id, 0);
    EXPECT_FALSE(error_info->recoverable);
    EXPECT_FALSE(error_info->retry_recommended);
    
    // Cleanup
    extsock_error_destroy(error_info);
}

TEST_F(ExtsockErrorsTest, CreateNullMessage) {
    // Given: Valid error code but NULL message
    extsock_error_t code = EXTSOCK_ERROR_CONFIG_INVALID;
    const char *message = nullptr;
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    // Then: Error info should be created with NULL message
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, code);
    EXPECT_EQ(error_info->message, nullptr);
    EXPECT_EQ(error_info->severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    
    // Cleanup
    extsock_error_destroy(error_info);
}

TEST_F(ExtsockErrorsTest, CreateEmptyMessage) {
    // Given: Valid error code and empty message
    extsock_error_t code = EXTSOCK_ERROR_SOCKET_FAILED;
    const char *message = "";
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    // Then: Error info should be created with empty message
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, code);
    EXPECT_NE(error_info->message, nullptr);
    EXPECT_STREQ(error_info->message, "");
    
    // Cleanup
    extsock_error_destroy(error_info);
}

TEST_F(ExtsockErrorsTest, CreateLongMessage) {
    // Given: Very long message (test string handling)
    extsock_error_t code = EXTSOCK_ERROR_MEMORY_ALLOCATION;
    char long_message[1024];
    std::memset(long_message, 'A', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = '\0';
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(code, long_message);
    
    // Then: Error info should handle long message correctly
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, code);
    EXPECT_NE(error_info->message, nullptr);
    EXPECT_STREQ(error_info->message, long_message);
    EXPECT_EQ(std::strlen(error_info->message), std::strlen(long_message));
    
    // Cleanup
    extsock_error_destroy(error_info);
}

TEST_F(ExtsockErrorsTest, DestroyNullPointer) {
    // Given: NULL error info pointer
    extsock_error_info_t *error_info = nullptr;
    
    // When & Then: Should not crash when destroying NULL pointer
    EXPECT_NO_THROW(extsock_error_destroy(error_info));
}

TEST_F(ExtsockErrorsTest, DestroyValidPointer) {
    // Given: Valid error info
    extsock_error_info_t *error_info = extsock_error_create(EXTSOCK_ERROR_STRONGSWAN_API, "Test message");
    EXPECT_NE(error_info, nullptr);
    
    // When & Then: Should not crash when destroying valid pointer
    EXPECT_NO_THROW(extsock_error_destroy(error_info));
}

/**
 * ============================================================================
 * Error Code to String Conversion Tests
 * ============================================================================
 */

TEST_F(ExtsockErrorsTest, ErrorToStringAllCodes) {
    // Test all defined error codes
    struct TestCase {
        extsock_error_t code;
        const char *expected;
    };
    
    TestCase test_cases[] = {
        { EXTSOCK_SUCCESS, "Success" },
        { EXTSOCK_ERROR_JSON_PARSE, "JSON Parse Error" },
        { EXTSOCK_ERROR_CONFIG_INVALID, "Invalid Configuration" },
        { EXTSOCK_ERROR_SOCKET_FAILED, "Socket Operation Failed" },
        { EXTSOCK_ERROR_MEMORY_ALLOCATION, "Memory Allocation Error" },
        { EXTSOCK_ERROR_STRONGSWAN_API, "strongSwan API Error" }
    };
    
    for (const auto& test_case : test_cases) {
        // When: Convert error code to string
        const char *result = extsock_error_to_string(test_case.code);
        
        // Then: Should return expected string
        EXPECT_NE(result, nullptr);
        EXPECT_STREQ(result, test_case.expected);
    }
}

TEST_F(ExtsockErrorsTest, ErrorToStringUnknownCode) {
    // Given: Unknown error code
    extsock_error_t unknown_code = static_cast<extsock_error_t>(999);
    
    // When: Convert unknown code to string
    const char *result = extsock_error_to_string(unknown_code);
    
    // Then: Should return "Unknown Error"
    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(result, "Unknown Error");
}

/**
 * ============================================================================
 * Error Info Field Validation Tests
 * ============================================================================
 */

TEST_F(ExtsockErrorsTest, TimestampValidity) {
    // Given: Current time before creating error
    time_t before = time(nullptr);
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(EXTSOCK_ERROR_JSON_PARSE, "Test");
    
    // Given: Current time after creating error
    time_t after = time(nullptr);
    
    // Then: Timestamp should be within reasonable range
    EXPECT_NE(error_info, nullptr);
    EXPECT_GE(error_info->timestamp, before);
    EXPECT_LE(error_info->timestamp, after);
    
    // Cleanup
    extsock_error_destroy(error_info);
}

TEST_F(ExtsockErrorsTest, ThreadIdConsistency) {
    // When: Create multiple error infos in same thread
    extsock_error_info_t *error1 = extsock_error_create(EXTSOCK_ERROR_CONFIG_INVALID, "Error 1");
    extsock_error_info_t *error2 = extsock_error_create(EXTSOCK_ERROR_SOCKET_FAILED, "Error 2");
    
    // Then: Thread IDs should be the same and non-zero
    EXPECT_NE(error1, nullptr);
    EXPECT_NE(error2, nullptr);
    EXPECT_GT(error1->thread_id, 0);
    EXPECT_GT(error2->thread_id, 0);
    EXPECT_EQ(error1->thread_id, error2->thread_id);
    
    // Cleanup
    extsock_error_destroy(error1);
    extsock_error_destroy(error2);
}

TEST_F(ExtsockErrorsTest, DefaultFieldValues) {
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(EXTSOCK_ERROR_MEMORY_ALLOCATION, "Test");
    
    // Then: Default field values should be correct
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    EXPECT_EQ(error_info->context, nullptr);
    EXPECT_FALSE(error_info->recoverable);
    EXPECT_FALSE(error_info->retry_recommended);
    
    // Cleanup
    extsock_error_destroy(error_info);
}

/**
 * ============================================================================
 * Memory Management and Edge Cases Tests
 * ============================================================================
 */

TEST_F(ExtsockErrorsTest, MemoryOwnership) {
    // Given: Original message string
    char original_message[] = "Original message";
    
    // When: Create error info
    extsock_error_info_t *error_info = extsock_error_create(EXTSOCK_SUCCESS, original_message);
    EXPECT_NE(error_info, nullptr);
    
    // When: Modify original string
    std::strcpy(original_message, "Modified message");
    
    // Then: Error info message should be unchanged (independent copy)
    EXPECT_STREQ(error_info->message, "Original message");
    EXPECT_STRNE(error_info->message, original_message);
    
    // Cleanup
    extsock_error_destroy(error_info);
}

TEST_F(ExtsockErrorsTest, MultipleCreateDestroy) {
    // Test creating and destroying multiple error infos
    const int count = 10;
    extsock_error_info_t *errors[count];
    
    // When: Create multiple error infos
    for (int i = 0; i < count; i++) {
        char message[64];
        snprintf(message, sizeof(message), "Error message %d", i);
        errors[i] = extsock_error_create(static_cast<extsock_error_t>(i % 6), message);
        EXPECT_NE(errors[i], nullptr);
    }
    
    // Then: All should be created successfully
    for (int i = 0; i < count; i++) {
        EXPECT_NE(errors[i], nullptr);
        EXPECT_EQ(errors[i]->code, static_cast<extsock_error_t>(i % 6));
    }
    
    // When: Destroy all error infos
    for (int i = 0; i < count; i++) {
        EXPECT_NO_THROW(extsock_error_destroy(errors[i]));
    }
}

/**
 * ============================================================================
 * Parameterized Tests for Different Error Codes
 * ============================================================================
 */

class ExtsockErrorsParameterizedTest : 
    public ::testing::TestWithParam<std::pair<extsock_error_t, const char*>> {};

TEST_P(ExtsockErrorsParameterizedTest, ErrorCreationWithDifferentCodes) {
    auto [error_code, error_message] = GetParam();
    
    extsock_error_info_t *error_info = extsock_error_create(error_code, error_message);
    
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, error_code);
    EXPECT_STREQ(error_info->message, error_message);
    
    extsock_error_destroy(error_info);
}

INSTANTIATE_TEST_SUITE_P(
    ErrorCodes,
    ExtsockErrorsParameterizedTest,
    ::testing::Values(
        std::make_pair(EXTSOCK_SUCCESS, "Success message"),
        std::make_pair(EXTSOCK_ERROR_JSON_PARSE, "JSON parse failed"),
        std::make_pair(EXTSOCK_ERROR_CONFIG_INVALID, "Invalid config"),
        std::make_pair(EXTSOCK_ERROR_SOCKET_FAILED, "Connection failed"),
        std::make_pair(EXTSOCK_ERROR_MEMORY_ALLOCATION, "Out of memory"),
        std::make_pair(EXTSOCK_ERROR_STRONGSWAN_API, "API error")
    )
);

/**
 * ============================================================================
 * Google Mock Integration Example
 * ============================================================================
 */

// Mock error reporter for testing error propagation
class MockErrorReporter {
public:
    MOCK_METHOD(void, report_error, (const extsock_error_info_t* error), ());
    MOCK_METHOD(bool, should_retry, (extsock_error_t code), ());
};

TEST_F(ExtsockErrorsTest, MockIntegrationExample) {
    MockErrorReporter mock_reporter;
    
    // Setup expectations
    EXPECT_CALL(mock_reporter, should_retry(EXTSOCK_ERROR_JSON_PARSE))
        .WillOnce(::testing::Return(true));
    
    // Test the mock
    bool should_retry = mock_reporter.should_retry(EXTSOCK_ERROR_JSON_PARSE);
    EXPECT_TRUE(should_retry);
}