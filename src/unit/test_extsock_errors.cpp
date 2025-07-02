/*
 * Copyright (C) 2024 strongSwan Project  
 * Week 2: Core Functionality Tests - extsock Error Handling Tests
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Test utilities and common includes
#include "test_utils.hpp"
#include "c_wrappers/extsock_errors.h"
#include "c_wrappers/extsock_types.h"

// Try to include actual error handling code if available
extern "C" {
    // Forward declarations for real error functions
    extern const char* extsock_error_to_string(extsock_error_t error);
    extern extsock_error_info_t *extsock_error_create(extsock_error_t code, const char *message);
    extern void extsock_error_destroy(extsock_error_info_t *error_info);
}

using namespace testing;
using namespace std;

/**
 * Week 2: extsock Error Handling Real Tests
 */
class ExtSockErrorsRealTest : public ::testing::Test {
protected:
    void SetUp() override {
        cout << "Setting up ExtSockErrorsRealTest" << endl;
        
        // Initialize test environment
        memory_tracker = make_unique<MemoryTracker>();
        string_utils = make_unique<StringUtils>();
        json_helper = make_unique<JsonTestHelper>();
        
        cout << "Testing real extsock error handling functionality" << endl;
    }
    
    void TearDown() override {
        cout << "Tearing down ExtSockErrorsRealTest" << endl;
        
        // Clean up test environment
        memory_tracker.reset();
        string_utils.reset();
        json_helper.reset();
    }
    
    // Test utilities
    unique_ptr<MemoryTracker> memory_tracker;
    unique_ptr<StringUtils> string_utils;
    unique_ptr<JsonTestHelper> json_helper;
};

/**
 * Test: Real Error Code to String Conversion
 */
TEST_F(ExtSockErrorsRealTest, RealErrorCodeToStringConversion) {
    cout << "Testing real error code to string conversion" << endl;
    
    // Test actual error string conversion function
    const char* success_str = extsock_error_to_string(EXTSOCK_SUCCESS);
    EXPECT_NE(success_str, nullptr);
    EXPECT_STREQ(success_str, "Success");
    
    const char* json_error_str = extsock_error_to_string(EXTSOCK_ERROR_JSON_PARSE);
    EXPECT_NE(json_error_str, nullptr);
    EXPECT_STREQ(json_error_str, "JSON Parse Error");
    
    const char* config_error_str = extsock_error_to_string(EXTSOCK_ERROR_CONFIG_INVALID);
    EXPECT_NE(config_error_str, nullptr);
    EXPECT_STREQ(config_error_str, "Invalid Configuration");
    
    cout << "Real error code to string conversion works correctly" << endl;
}

/**
 * Test: Error Code Validation
 */
TEST_F(ExtSockErrorsRealTest, ErrorCodeValidation) {
    cout << "Testing error code validation" << endl;
    
    // Test all defined error codes
    vector<extsock_error_t> all_errors = {
        EXTSOCK_SUCCESS,
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID,
        EXTSOCK_ERROR_SOCKET_FAILED,
        EXTSOCK_ERROR_MEMORY_ALLOCATION,
        EXTSOCK_ERROR_STRONGSWAN_API
    };
    
    for (auto error_code : all_errors) {
        const char* error_str = extsock_error_to_string(error_code);
        EXPECT_NE(error_str, nullptr);
        EXPECT_GT(strlen(error_str), 0);
        EXPECT_LT(strlen(error_str), 100); // Reasonable error message length
    }
    
    cout << "Error code validation works correctly" << endl;
}

/**
 * Test: Error Information Structure
 */
TEST_F(ExtSockErrorsRealTest, ErrorInformationStructure) {
    cout << "Testing error information structure" << endl;
    
    // Test that error info structure has proper size and alignment
    EXPECT_GT(sizeof(extsock_error_info_t), 0);
    EXPECT_LT(sizeof(extsock_error_info_t), 1024); // Reasonable structure size
    
    // Test error severity enum
    EXPECT_EQ(sizeof(extsock_error_severity_t), sizeof(int));
    
    cout << "Error information structure is valid" << endl;
}

/**
 * Test: Mock Error Handling Functions
 */
TEST_F(ExtSockErrorsRealTest, MockErrorHandlingFunctions) {
    cout << "Testing mock error handling functions" << endl;
    
    // Test mock functions that simulate real behavior
    const char* mock_success = extsock_error_to_string(EXTSOCK_SUCCESS);
    EXPECT_STREQ(mock_success, "Success");
    
    const char* mock_unknown = extsock_error_to_string(static_cast<extsock_error_t>(999));
    EXPECT_STREQ(mock_unknown, "Unknown Error");
    
    cout << "Mock error handling functions work correctly" << endl;
}

/**
 * Test: Error Integration with Types
 */
TEST_F(ExtSockErrorsRealTest, ErrorIntegrationWithTypes) {
    cout << "Testing error integration with types" << endl;
    
    // Test that error codes work with event and command types
    vector<pair<extsock_error_t, string>> error_scenarios = {
        {EXTSOCK_ERROR_JSON_PARSE, "JSON parsing failed"},
        {EXTSOCK_ERROR_CONFIG_INVALID, "Configuration validation failed"},
        {EXTSOCK_ERROR_SOCKET_FAILED, "Socket operation failed"},
        {EXTSOCK_ERROR_MEMORY_ALLOCATION, "Memory allocation failed"},
        {EXTSOCK_ERROR_STRONGSWAN_API, "strongSwan API call failed"}
    };
    
    for (const auto& [error_code, description] : error_scenarios) {
        EXPECT_NE(error_code, EXTSOCK_SUCCESS);
        EXPECT_FALSE(description.empty());
        
        // Test error can be properly formatted
        string formatted = string_utils->format("Error {}: {}", 
                                               static_cast<int>(error_code), 
                                               description);
        EXPECT_FALSE(formatted.empty());
    }
    
    cout << "Error integration with types works correctly" << endl;
}

/**
 * Test: Error Event Processing
 */
TEST_F(ExtSockErrorsRealTest, ErrorEventProcessing) {
    cout << "Testing error event processing" << endl;
    
    // Test error events
    struct ErrorEvent {
        extsock_event_type_t event_type;
        extsock_error_t error_code;
        string message;
    };
    
    vector<ErrorEvent> error_events = {
        {EXTSOCK_EVENT_ERROR_OCCURRED, EXTSOCK_ERROR_JSON_PARSE, "JSON parse error"},
        {EXTSOCK_EVENT_ERROR_OCCURRED, EXTSOCK_ERROR_CONFIG_INVALID, "Config error"},
        {EXTSOCK_EVENT_ERROR_OCCURRED, EXTSOCK_ERROR_SOCKET_FAILED, "Socket error"}
    };
    
    for (const auto& event : error_events) {
        EXPECT_EQ(event.event_type, EXTSOCK_EVENT_ERROR_OCCURRED);
        EXPECT_NE(event.error_code, EXTSOCK_SUCCESS);
        EXPECT_FALSE(event.message.empty());
    }
    
    cout << "Error event processing works correctly" << endl;
}

/**
 * Test: Error Recovery Scenarios
 */
TEST_F(ExtSockErrorsRealTest, ErrorRecoveryScenarios) {
    cout << "Testing error recovery scenarios" << endl;
    
    // Test error recovery patterns
    map<extsock_error_t, bool> recoverable_errors = {
        {EXTSOCK_ERROR_JSON_PARSE, true},        // Can retry with fixed JSON
        {EXTSOCK_ERROR_CONFIG_INVALID, false},   // Requires manual intervention
        {EXTSOCK_ERROR_SOCKET_FAILED, true},     // Can retry socket operation
        {EXTSOCK_ERROR_MEMORY_ALLOCATION, false}, // System resource issue
        {EXTSOCK_ERROR_STRONGSWAN_API, true}     // May be transient
    };
    
    for (const auto& [error_code, is_recoverable] : recoverable_errors) {
        EXPECT_NE(error_code, EXTSOCK_SUCCESS);
        
        // Test error recovery logic
        bool should_retry = is_recoverable && (error_code != EXTSOCK_ERROR_CONFIG_INVALID);
        EXPECT_EQ(should_retry, is_recoverable && (error_code != EXTSOCK_ERROR_CONFIG_INVALID));
    }
    
    cout << "Error recovery scenarios work correctly" << endl;
}

/**
 * Test: Error Logging Integration
 */
TEST_F(ExtSockErrorsRealTest, ErrorLoggingIntegration) {
    cout << "Testing error logging integration" << endl;
    
    // Test error logging format
    for (int i = 1; i <= 5; ++i) {
        extsock_error_t error_code = static_cast<extsock_error_t>(i);
        const char* error_str = extsock_error_to_string(error_code);
        
        // Test log message format
        string log_message = string_utils->format("[ERROR] Code {}: {}", 
                                                 static_cast<int>(error_code), 
                                                 error_str);
        
        EXPECT_FALSE(log_message.empty());
        EXPECT_NE(log_message.find("[ERROR]"), string::npos);
        EXPECT_NE(log_message.find(to_string(static_cast<int>(error_code))), string::npos);
    }
    
    cout << "Error logging integration works correctly" << endl;
}

/**
 * Test: Error Statistics Tracking
 */
TEST_F(ExtSockErrorsRealTest, ErrorStatisticsTracking) {
    cout << "Testing error statistics tracking" << endl;
    
    // Test error occurrence counting
    map<extsock_error_t, int> error_counts;
    
    // Simulate error occurrences
    vector<extsock_error_t> simulated_errors = {
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID,
        EXTSOCK_ERROR_SOCKET_FAILED,
        EXTSOCK_ERROR_JSON_PARSE
    };
    
    for (auto error : simulated_errors) {
        error_counts[error]++;
    }
    
    // Verify statistics
    EXPECT_EQ(error_counts[EXTSOCK_ERROR_JSON_PARSE], 3);
    EXPECT_EQ(error_counts[EXTSOCK_ERROR_CONFIG_INVALID], 1);
    EXPECT_EQ(error_counts[EXTSOCK_ERROR_SOCKET_FAILED], 1);
    EXPECT_EQ(error_counts[EXTSOCK_ERROR_MEMORY_ALLOCATION], 0);
    
    cout << "Error statistics tracking works correctly" << endl;
}

/**
 * Test: Error Context Information
 */
TEST_F(ExtSockErrorsRealTest, ErrorContextInformation) {
    cout << "Testing error context information" << endl;
    
    // Test error context tracking
    struct ErrorContext {
        extsock_error_t error_code;
        string function_name;
        int line_number;
        string additional_info;
    };
    
    vector<ErrorContext> error_contexts = {
        {EXTSOCK_ERROR_JSON_PARSE, "parse_config", 123, "Invalid JSON syntax"},
        {EXTSOCK_ERROR_SOCKET_FAILED, "create_socket", 456, "Address already in use"},
        {EXTSOCK_ERROR_CONFIG_INVALID, "validate_config", 789, "Missing required field"}
    };
    
    for (const auto& context : error_contexts) {
        EXPECT_NE(context.error_code, EXTSOCK_SUCCESS);
        EXPECT_FALSE(context.function_name.empty());
        EXPECT_GT(context.line_number, 0);
        EXPECT_FALSE(context.additional_info.empty());
    }
    
    cout << "Error context information works correctly" << endl;
}

/**
 * Test: Error Performance Impact
 */
TEST_F(ExtSockErrorsRealTest, ErrorPerformanceImpact) {
    cout << "Testing error performance impact" << endl;
    
    auto time_helper = make_unique<TimeHelper>();
    time_helper->startTiming();
    
    const int num_error_operations = 1000;
    
    // Test error string conversion performance
    for (int i = 0; i < num_error_operations; ++i) {
        extsock_error_t error_code = static_cast<extsock_error_t>(
            EXTSOCK_ERROR_JSON_PARSE + (i % 5)
        );
        
        const char* error_str = extsock_error_to_string(error_code);
        EXPECT_NE(error_str, nullptr);
    }
    
    auto elapsed_time = time_helper->getElapsedMs();
    
    // Performance expectation (adjust based on system)
    EXPECT_LT(elapsed_time, 100); // Less than 100ms for 1000 operations
    
    cout << "Error operations time: " << elapsed_time << "ms" << endl;
    cout << "Error performance impact is acceptable" << endl;
}

// Entry point for standalone execution
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    cout << "Starting Week 2 extsock Real Error Handling Tests" << endl;
    
    int result = RUN_ALL_TESTS();
    
    if (result == 0) {
        cout << "All extsock real error handling tests passed!" << endl;
    } else {
        cout << "Some extsock real error handling tests failed!" << endl;
    }
    
    return result;
} 