/**
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Test Unit Tests for extsock_types module
 * Migrated from test_extsock_types_pure.c
 * 
 * Level 1 (Pure) unit tests that test type definitions,
 * enums, and constants without any strongSwan dependencies.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <climits>
#include <cstring>

// Pure unit test types (without strongSwan dependencies)  
extern "C" {
    #include "extsock_types_pure.h"
}

/**
 * ============================================================================
 * Test Fixture Class
 * ============================================================================
 */
class ExtsockTypesTest : public ::testing::Test {
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
 * Error Code Enum Tests
 * ============================================================================
 */

TEST_F(ExtsockTypesTest, ErrorEnumValues) {
    // Test that error codes have expected values and ordering
    EXPECT_EQ(EXTSOCK_SUCCESS, 0);
    EXPECT_EQ(EXTSOCK_ERROR_JSON_PARSE, 1);
    EXPECT_EQ(EXTSOCK_ERROR_CONFIG_INVALID, 2);
    EXPECT_EQ(EXTSOCK_ERROR_SOCKET_FAILED, 3);
    EXPECT_EQ(EXTSOCK_ERROR_MEMORY_ALLOCATION, 4);
    EXPECT_EQ(EXTSOCK_ERROR_STRONGSWAN_API, 5);
    EXPECT_EQ(EXTSOCK_ERROR_INVALID_PARAMETER, 6);
    EXPECT_EQ(EXTSOCK_ERROR_CONFIG_CREATION_FAILED, 7);
}

TEST_F(ExtsockTypesTest, ErrorEnumUniqueness) {
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
            EXPECT_NE(errors[i], errors[j]) 
                << "Error codes at index " << i << " and " << j << " are not unique";
        }
    }
}

TEST_F(ExtsockTypesTest, ErrorEnumRange) {
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
        EXPECT_GE(errors[i], 0) << "Error code at index " << i << " is negative";
        EXPECT_LT(errors[i], 100) << "Error code at index " << i << " is too large";
    }
}

/**
 * ============================================================================
 * Error Severity Enum Tests
 * ============================================================================
 */

TEST_F(ExtsockTypesTest, ErrorSeverityValues) {
    // Test severity levels have expected values and ordering
    EXPECT_EQ(EXTSOCK_ERROR_SEVERITY_TRACE, 0);
    EXPECT_EQ(EXTSOCK_ERROR_SEVERITY_DEBUG, 1);
    EXPECT_EQ(EXTSOCK_ERROR_SEVERITY_INFO, 2);
    EXPECT_EQ(EXTSOCK_ERROR_SEVERITY_WARNING, 3);
    EXPECT_EQ(EXTSOCK_ERROR_SEVERITY_ERROR, 4);
    EXPECT_EQ(EXTSOCK_ERROR_SEVERITY_CRITICAL, 5);
}

TEST_F(ExtsockTypesTest, ErrorSeverityOrdering) {
    // Test that severity levels are in correct order (ascending severity)
    EXPECT_LT(EXTSOCK_ERROR_SEVERITY_TRACE, EXTSOCK_ERROR_SEVERITY_DEBUG);
    EXPECT_LT(EXTSOCK_ERROR_SEVERITY_DEBUG, EXTSOCK_ERROR_SEVERITY_INFO);
    EXPECT_LT(EXTSOCK_ERROR_SEVERITY_INFO, EXTSOCK_ERROR_SEVERITY_WARNING);
    EXPECT_LT(EXTSOCK_ERROR_SEVERITY_WARNING, EXTSOCK_ERROR_SEVERITY_ERROR);
    EXPECT_LT(EXTSOCK_ERROR_SEVERITY_ERROR, EXTSOCK_ERROR_SEVERITY_CRITICAL);
}

TEST_F(ExtsockTypesTest, ErrorSeverityUniqueness) {
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
            EXPECT_NE(severities[i], severities[j])
                << "Severity levels at index " << i << " and " << j << " are not unique";
        }
    }
}

/**
 * ============================================================================
 * Error Info Structure Tests
 * ============================================================================
 */

TEST_F(ExtsockTypesTest, ErrorInfoStructSize) {
    // Test that the struct size is reasonable
    size_t struct_size = sizeof(extsock_error_info_t);
    
    // Should be large enough to hold all fields
    EXPECT_GT(struct_size, 0) << "Struct size should be positive";
    
    // Should not be excessively large (reasonable upper bound)
    EXPECT_LT(struct_size, 1024) << "Struct size should be less than 1KB";
    
    std::cout << "extsock_error_info_t size: " << struct_size << " bytes" << std::endl;
}

TEST_F(ExtsockTypesTest, ErrorInfoFieldOffsets) {
    // Test that all fields can be accessed (basic structural test)
    extsock_error_info_t info = {};
    std::memset(&info, 0, sizeof(info));
    
    // Set fields to known values
    info.code = EXTSOCK_ERROR_JSON_PARSE;
    info.severity = EXTSOCK_ERROR_SEVERITY_ERROR;
    info.message = nullptr;
    info.context = nullptr;
    info.timestamp = 12345;
    info.thread_id = 67890;
    info.recoverable = true;
    info.retry_recommended = false;
    
    // Verify fields can be read back correctly
    EXPECT_EQ(info.code, EXTSOCK_ERROR_JSON_PARSE);
    EXPECT_EQ(info.severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    EXPECT_EQ(info.message, nullptr);
    EXPECT_EQ(info.context, nullptr);
    EXPECT_EQ(info.timestamp, 12345);
    EXPECT_EQ(info.thread_id, 67890);
    EXPECT_TRUE(info.recoverable);
    EXPECT_FALSE(info.retry_recommended);
}

TEST_F(ExtsockTypesTest, ErrorInfoFieldTypes) {
    // Test field type compatibility and basic operations
    extsock_error_info_t info = {};
    std::memset(&info, 0, sizeof(info));
    
    // Test error code assignment and comparison
    info.code = EXTSOCK_ERROR_CONFIG_INVALID;
    EXPECT_EQ(info.code, EXTSOCK_ERROR_CONFIG_INVALID);
    EXPECT_NE(info.code, EXTSOCK_SUCCESS);
    
    // Test severity assignment and comparison
    info.severity = EXTSOCK_ERROR_SEVERITY_CRITICAL;
    EXPECT_EQ(info.severity, EXTSOCK_ERROR_SEVERITY_CRITICAL);
    EXPECT_NE(info.severity, EXTSOCK_ERROR_SEVERITY_TRACE);
    
    // Test boolean fields
    info.recoverable = true;
    info.retry_recommended = false;
    EXPECT_TRUE(info.recoverable);
    EXPECT_FALSE(info.retry_recommended);
    
    // Test pointer fields
    info.message = const_cast<char*>("test");
    info.context = const_cast<char*>("context");
    EXPECT_NE(info.message, nullptr);
    EXPECT_NE(info.context, nullptr);
    
    // Test numeric fields
    info.timestamp = 1234567890;
    info.thread_id = 42;
    EXPECT_EQ(info.timestamp, 1234567890);
    EXPECT_EQ(info.thread_id, 42);
}

/**
 * ============================================================================
 * Type Compatibility Tests
 * ============================================================================
 */

TEST_F(ExtsockTypesTest, ErrorTypeArithmetic) {
    // Test that error codes can be used in arithmetic operations
    extsock_error_t base = EXTSOCK_SUCCESS;
    extsock_error_t next = static_cast<extsock_error_t>(base + 1);
    
    EXPECT_EQ(next, EXTSOCK_ERROR_JSON_PARSE);
    
    // Test that we can compare error codes
    EXPECT_LT(EXTSOCK_SUCCESS, EXTSOCK_ERROR_JSON_PARSE);
    EXPECT_LT(EXTSOCK_ERROR_JSON_PARSE, EXTSOCK_ERROR_CONFIG_INVALID);
}

TEST_F(ExtsockTypesTest, ErrorSeverityArithmetic) {
    // Test that severity levels can be compared
    EXPECT_LT(EXTSOCK_ERROR_SEVERITY_TRACE, EXTSOCK_ERROR_SEVERITY_CRITICAL);
    EXPECT_GT(EXTSOCK_ERROR_SEVERITY_ERROR, EXTSOCK_ERROR_SEVERITY_WARNING);
    
    // Test arithmetic operations
    extsock_error_severity_t high = EXTSOCK_ERROR_SEVERITY_CRITICAL;
    extsock_error_severity_t low = EXTSOCK_ERROR_SEVERITY_TRACE;
    
    EXPECT_GT(high, low);
    EXPECT_GT((high - low), 0);
}

TEST_F(ExtsockTypesTest, TypeCastingSafety) {
    // Test safe casting between related types
    int error_as_int = static_cast<int>(EXTSOCK_ERROR_JSON_PARSE);
    extsock_error_t error_from_int = static_cast<extsock_error_t>(error_as_int);
    
    EXPECT_EQ(error_from_int, EXTSOCK_ERROR_JSON_PARSE);
    
    // Test severity casting
    int severity_as_int = static_cast<int>(EXTSOCK_ERROR_SEVERITY_ERROR);
    extsock_error_severity_t severity_from_int = static_cast<extsock_error_severity_t>(severity_as_int);
    
    EXPECT_EQ(severity_from_int, EXTSOCK_ERROR_SEVERITY_ERROR);
}

/**
 * ============================================================================
 * Constants and Boundaries Tests
 * ============================================================================
 */

TEST_F(ExtsockTypesTest, SuccessCodeProperties) {
    // Test properties of the success code
    EXPECT_EQ(EXTSOCK_SUCCESS, 0);
    
    // Success should be the smallest value
    EXPECT_LT(EXTSOCK_SUCCESS, EXTSOCK_ERROR_JSON_PARSE);
    EXPECT_LT(EXTSOCK_SUCCESS, EXTSOCK_ERROR_CONFIG_INVALID);
    EXPECT_LT(EXTSOCK_SUCCESS, EXTSOCK_ERROR_SOCKET_FAILED);
    
    // Test that success can be used in boolean context
    EXPECT_FALSE(EXTSOCK_SUCCESS) << "Success should be falsy";
}

TEST_F(ExtsockTypesTest, ErrorCodeProperties) {
    // All error codes should be non-zero (truthy in boolean context)
    EXPECT_TRUE(EXTSOCK_ERROR_JSON_PARSE);
    EXPECT_TRUE(EXTSOCK_ERROR_CONFIG_INVALID);
    EXPECT_TRUE(EXTSOCK_ERROR_SOCKET_FAILED);
    EXPECT_TRUE(EXTSOCK_ERROR_MEMORY_ALLOCATION);
    EXPECT_TRUE(EXTSOCK_ERROR_STRONGSWAN_API);
    EXPECT_TRUE(EXTSOCK_ERROR_INVALID_PARAMETER);
    EXPECT_TRUE(EXTSOCK_ERROR_CONFIG_CREATION_FAILED);
}

/**
 * ============================================================================
 * Parameterized Tests for Error Codes
 * ============================================================================
 */

class ExtsockTypesParameterizedTest : 
    public ::testing::TestWithParam<extsock_error_t> {};

TEST_P(ExtsockTypesParameterizedTest, ErrorCodeValidation) {
    extsock_error_t error_code = GetParam();
    
    // All error codes should be in valid range
    EXPECT_GE(error_code, 0) << "Error code should be non-negative";
    EXPECT_LT(error_code, 100) << "Error code should be reasonable";
    
    // All non-success codes should be truthy
    if (error_code != EXTSOCK_SUCCESS) {
        EXPECT_TRUE(error_code) << "Non-success error codes should be truthy";
    }
}

INSTANTIATE_TEST_SUITE_P(
    AllErrorCodes,
    ExtsockTypesParameterizedTest,
    ::testing::Values(
        EXTSOCK_SUCCESS,
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID,
        EXTSOCK_ERROR_SOCKET_FAILED,
        EXTSOCK_ERROR_MEMORY_ALLOCATION,
        EXTSOCK_ERROR_STRONGSWAN_API,
        EXTSOCK_ERROR_INVALID_PARAMETER,
        EXTSOCK_ERROR_CONFIG_CREATION_FAILED
    )
);

/**
 * ============================================================================
 * Parameterized Tests for Severity Levels
 * ============================================================================
 */

class ExtsockSeverityParameterizedTest : 
    public ::testing::TestWithParam<extsock_error_severity_t> {};

TEST_P(ExtsockSeverityParameterizedTest, SeverityLevelValidation) {
    extsock_error_severity_t severity = GetParam();
    
    // All severity levels should be in valid range
    EXPECT_GE(severity, EXTSOCK_ERROR_SEVERITY_TRACE);
    EXPECT_LE(severity, EXTSOCK_ERROR_SEVERITY_CRITICAL);
}

INSTANTIATE_TEST_SUITE_P(
    AllSeverityLevels,
    ExtsockSeverityParameterizedTest,
    ::testing::Values(
        EXTSOCK_ERROR_SEVERITY_TRACE,
        EXTSOCK_ERROR_SEVERITY_DEBUG,
        EXTSOCK_ERROR_SEVERITY_INFO,
        EXTSOCK_ERROR_SEVERITY_WARNING,
        EXTSOCK_ERROR_SEVERITY_ERROR,
        EXTSOCK_ERROR_SEVERITY_CRITICAL
    )
);

/**
 * ============================================================================
 * Google Mock Integration Examples
 * ============================================================================
 */

// Mock type validator for advanced type validation scenarios
class MockTypeValidator {
public:
    MOCK_METHOD(bool, is_valid_error_code, (extsock_error_t code), ());
    MOCK_METHOD(bool, is_valid_severity, (extsock_error_severity_t severity), ());
    MOCK_METHOD(const char*, get_error_category, (extsock_error_t code), ());
};

TEST_F(ExtsockTypesTest, MockTypeValidatorExample) {
    MockTypeValidator mock_validator;
    
    // Setup expectations
    EXPECT_CALL(mock_validator, is_valid_error_code(EXTSOCK_ERROR_JSON_PARSE))
        .WillOnce(::testing::Return(true));
    
    EXPECT_CALL(mock_validator, get_error_category(EXTSOCK_ERROR_JSON_PARSE))
        .WillOnce(::testing::Return("PARSE_ERROR"));
    
    // Test the mock
    bool is_valid = mock_validator.is_valid_error_code(EXTSOCK_ERROR_JSON_PARSE);
    const char* category = mock_validator.get_error_category(EXTSOCK_ERROR_JSON_PARSE);
    
    EXPECT_TRUE(is_valid);
    EXPECT_STREQ(category, "PARSE_ERROR");
}

/**
 * ============================================================================
 * Type Safety Tests
 * ============================================================================
 */

TEST_F(ExtsockTypesTest, EnumSizes) {
    // Test that enum types have reasonable sizes
    EXPECT_LE(sizeof(extsock_error_t), sizeof(int));
    EXPECT_LE(sizeof(extsock_error_severity_t), sizeof(int));
    
    std::cout << "extsock_error_t size: " << sizeof(extsock_error_t) << " bytes" << std::endl;
    std::cout << "extsock_error_severity_t size: " << sizeof(extsock_error_severity_t) << " bytes" << std::endl;
}

TEST_F(ExtsockTypesTest, BooleanFieldSizes) {
    // Test that boolean fields have correct size
    extsock_error_info_t info;
    
    // Boolean fields should be 1 byte typically
    EXPECT_EQ(sizeof(info.recoverable), sizeof(bool));
    EXPECT_EQ(sizeof(info.retry_recommended), sizeof(bool));
}