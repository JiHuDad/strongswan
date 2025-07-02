/*
 * Copyright (C) 2024 strongSwan Project  
 * Week 2: Core Functionality Tests - extsock Common Functionality Tests
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Test utilities and common includes
#include "test_utils.hpp"
#include "c_wrappers/extsock_errors.h"
#include "c_wrappers/extsock_types.h"

using namespace testing;
using namespace std;

/**
 * Week 2: extsock Common Functionality Test Suite
 */
class ExtSockCommonTest : public ::testing::Test {
protected:
    void SetUp() override {
        cout << "Setting up ExtSockCommonTest" << endl;
        
        // Initialize test environment
        memory_tracker = make_unique<MemoryTracker>();
        string_utils = make_unique<StringUtils>();
        json_helper = make_unique<JsonTestHelper>();
        file_helper = make_unique<FileSystemHelper>();
        
        cout << "Testing extsock common functionality" << endl;
    }
    
    void TearDown() override {
        cout << "Tearing down ExtSockCommonTest" << endl;
        
        // Clean up test environment
        memory_tracker.reset();
        string_utils.reset();
        json_helper.reset();
        file_helper.reset();
    }
    
    // Test utilities
    unique_ptr<MemoryTracker> memory_tracker;
    unique_ptr<StringUtils> string_utils;
    unique_ptr<JsonTestHelper> json_helper;
    unique_ptr<FileSystemHelper> file_helper;
};

/**
 * Test: Common Constants and Definitions
 */
TEST_F(ExtSockCommonTest, CommonConstantsAndDefinitions) {
    cout << "Testing common constants and definitions" << endl;
    
    // Test that all error codes are properly defined
    EXPECT_EQ(EXTSOCK_SUCCESS, 0);
    EXPECT_NE(EXTSOCK_ERROR_JSON_PARSE, EXTSOCK_SUCCESS);
    EXPECT_NE(EXTSOCK_ERROR_CONFIG_INVALID, EXTSOCK_SUCCESS);
    EXPECT_NE(EXTSOCK_ERROR_SOCKET_FAILED, EXTSOCK_SUCCESS);
    EXPECT_NE(EXTSOCK_ERROR_MEMORY_ALLOCATION, EXTSOCK_SUCCESS);
    EXPECT_NE(EXTSOCK_ERROR_STRONGSWAN_API, EXTSOCK_SUCCESS);
    
    // Test that all event types are properly defined
    EXPECT_NE(EXTSOCK_EVENT_CONFIG_CHANGED, EXTSOCK_EVENT_CONNECTION_ESTABLISHED);
    EXPECT_NE(EXTSOCK_EVENT_DATA_RECEIVED, EXTSOCK_EVENT_CONNECTION_ESTABLISHED);
    EXPECT_NE(EXTSOCK_EVENT_ERROR_OCCURRED, EXTSOCK_EVENT_CONNECTION_ESTABLISHED);
    
    // Test that all command types are properly defined
    EXPECT_NE(EXTSOCK_CMD_GET_CONFIG, EXTSOCK_CMD_SET_CONFIG);
    EXPECT_NE(EXTSOCK_CMD_LIST_CONNECTIONS, EXTSOCK_CMD_SET_CONFIG);
    EXPECT_NE(EXTSOCK_CMD_CLOSE_CONNECTION, EXTSOCK_CMD_SET_CONFIG);
    
    cout << "Common constants and definitions are correct" << endl;
}

/**
 * Test: Common Data Structure Sizes
 */
TEST_F(ExtSockCommonTest, CommonDataStructureSizes) {
    cout << "Testing common data structure sizes" << endl;
    
    // Test enum sizes are reasonable
    EXPECT_EQ(sizeof(extsock_error_t), sizeof(int));
    EXPECT_EQ(sizeof(extsock_event_type_t), sizeof(int));
    EXPECT_EQ(sizeof(extsock_command_type_t), sizeof(int));
    
    // Test that enums fit in expected ranges
    EXPECT_GE(static_cast<int>(EXTSOCK_SUCCESS), 0);
    EXPECT_LT(static_cast<int>(EXTSOCK_ERROR_STRONGSWAN_API), 100);
    
    EXPECT_GE(static_cast<int>(EXTSOCK_EVENT_CONFIG_CHANGED), 0);
    EXPECT_LT(static_cast<int>(EXTSOCK_EVENT_ERROR_OCCURRED), 100);
    
    EXPECT_GE(static_cast<int>(EXTSOCK_CMD_GET_CONFIG), 0);
    EXPECT_LT(static_cast<int>(EXTSOCK_CMD_CLOSE_CONNECTION), 100);
    
    cout << "Common data structure sizes are appropriate" << endl;
}

/**
 * Test: Common String Operations
 */
TEST_F(ExtSockCommonTest, CommonStringOperations) {
    cout << "Testing common string operations" << endl;
    
    // Test string validation utilities
    EXPECT_TRUE(string_utils->isValidPath("/tmp/extsock.sock"));
    EXPECT_TRUE(string_utils->isValidPath("/var/run/extsock/socket"));
    EXPECT_FALSE(string_utils->isValidPath(""));
    EXPECT_FALSE(string_utils->isValidPath("relative/path"));
    
    // Test string formatting utilities
    string formatted = string_utils->format("Error {}: {}", 123, "Test message");
    EXPECT_EQ(formatted, "Error 123: Test message");
    
    // Test string trimming
    EXPECT_EQ(string_utils->trim("  test  "), "test");
    EXPECT_EQ(string_utils->trim("\t\ntest\r\n"), "test");
    EXPECT_EQ(string_utils->trim(""), "");
    
    cout << "Common string operations work correctly" << endl;
}

/**
 * Test: Common JSON Processing
 */
TEST_F(ExtSockCommonTest, CommonJsonProcessing) {
    cout << "Testing common JSON processing" << endl;
    
    // Test JSON object creation and manipulation
    json_object* config_json = json_helper->createObject();
    EXPECT_NE(config_json, nullptr);
    
    if (config_json) {
        // Add configuration parameters
        json_helper->addString(config_json, "socket_path", "/tmp/extsock.sock");
        json_helper->addNumber(config_json, "max_connections", 100);
        json_helper->addNumber(config_json, "timeout_seconds", 30);
        json_helper->addBoolean(config_json, "debug_enabled", true);
        
        // Verify configuration
        EXPECT_TRUE(json_helper->hasKey(config_json, "socket_path"));
        EXPECT_TRUE(json_helper->hasKey(config_json, "max_connections"));
        EXPECT_TRUE(json_helper->hasKey(config_json, "timeout_seconds"));
        EXPECT_TRUE(json_helper->hasKey(config_json, "debug_enabled"));
        
        // Test JSON string conversion
        string json_str = json_helper->toString(config_json);
        EXPECT_FALSE(json_str.empty());
        EXPECT_NE(json_str.find("socket_path"), string::npos);
        EXPECT_NE(json_str.find("max_connections"), string::npos);
        
        json_helper->cleanup(config_json);
    }
    
    cout << "Common JSON processing works correctly" << endl;
}

/**
 * Test: Common File System Operations
 */
TEST_F(ExtSockCommonTest, CommonFileSystemOperations) {
    cout << "Testing common file system operations" << endl;
    
    // Test temporary file creation
    string temp_file = file_helper->createTempFile();
    EXPECT_FALSE(temp_file.empty());
    EXPECT_TRUE(file_helper->exists(temp_file));
    
    // Test file writing and reading
    string test_content = "Test configuration data\nLine 2\n";
    EXPECT_TRUE(file_helper->writeFile(temp_file, test_content));
    
    string read_content = file_helper->readFile(temp_file);
    EXPECT_EQ(read_content, test_content);
    
    // Test file deletion
    EXPECT_TRUE(file_helper->deleteFile(temp_file));
    EXPECT_FALSE(file_helper->exists(temp_file));
    
    cout << "Common file system operations work correctly" << endl;
}

/**
 * Test: Common Memory Management
 */
TEST_F(ExtSockCommonTest, CommonMemoryManagement) {
    cout << "Testing common memory management" << endl;
    
    // Track initial memory usage
    size_t initial_memory = memory_tracker->getCurrentUsage();
    
    // Allocate various sizes
    vector<size_t> allocation_sizes = {16, 64, 256, 1024, 4096};
    
    for (size_t size : allocation_sizes) {
        memory_tracker->allocate(size);
        
        size_t current_usage = memory_tracker->getCurrentUsage();
        EXPECT_GE(current_usage, initial_memory);
        
        memory_tracker->deallocate(size);
    }
    
    // Verify memory is back to initial state
    size_t final_memory = memory_tracker->getCurrentUsage();
    EXPECT_EQ(final_memory, initial_memory);
    
    cout << "Common memory management works correctly" << endl;
}

/**
 * Test: Common Configuration Validation
 */
TEST_F(ExtSockCommonTest, CommonConfigurationValidation) {
    cout << "Testing common configuration validation" << endl;
    
    // Test valid configuration parameters
    map<string, string> valid_config = {
        {"socket_path", "/tmp/extsock.sock"},
        {"max_connections", "100"},
        {"timeout_seconds", "30"},
        {"buffer_size", "8192"},
        {"debug_level", "1"}
    };
    
    for (const auto& [key, value] : valid_config) {
        EXPECT_FALSE(key.empty());
        EXPECT_FALSE(value.empty());
        EXPECT_GT(key.length(), 2);
        EXPECT_LT(key.length(), 50);
    }
    
    // Test invalid configuration parameters
    map<string, string> invalid_config = {
        {"", "value"},              // Empty key
        {"key", ""},                // Empty value
        {"socket_path", "relative"}, // Invalid path
        {"max_connections", "-1"},   // Negative number
        {"timeout_seconds", "abc"}   // Non-numeric
    };
    
    // Validate configuration format
    for (const auto& [key, value] : invalid_config) {
        bool is_valid = !key.empty() && !value.empty() && 
                       key.length() > 2 && key.length() < 50;
        
        if (key == "socket_path") {
            is_valid = is_valid && string_utils->isValidPath(value);
        }
        
        // These should be invalid
        EXPECT_FALSE(is_valid || key.empty() || value.empty());
    }
    
    cout << "Common configuration validation works correctly" << endl;
}

/**
 * Test: Common Event Processing
 */
TEST_F(ExtSockCommonTest, CommonEventProcessing) {
    cout << "Testing common event processing" << endl;
    
    // Test event structure creation
    struct TestEvent {
        extsock_event_type_t type;
        uint32_t timestamp;
        string data;
    };
    
    vector<TestEvent> test_events = {
        {EXTSOCK_EVENT_CONFIG_CHANGED, static_cast<uint32_t>(time(nullptr)), "config_data"},
        {EXTSOCK_EVENT_CONNECTION_ESTABLISHED, static_cast<uint32_t>(time(nullptr)), "conn_info"},
        {EXTSOCK_EVENT_DATA_RECEIVED, static_cast<uint32_t>(time(nullptr)), "received_data"},
        {EXTSOCK_EVENT_ERROR_OCCURRED, static_cast<uint32_t>(time(nullptr)), "error_info"}
    };
    
    for (const auto& event : test_events) {
        EXPECT_GE(static_cast<int>(event.type), 0);
        EXPECT_LT(static_cast<int>(event.type), 100);
        EXPECT_GT(event.timestamp, 0);
        EXPECT_FALSE(event.data.empty());
    }
    
    cout << "Common event processing works correctly" << endl;
}

/**
 * Test: Common Command Processing
 */
TEST_F(ExtSockCommonTest, CommonCommandProcessing) {
    cout << "Testing common command processing" << endl;
    
    // Test command structure creation
    struct TestCommand {
        extsock_command_type_t type;
        uint32_t id;
        string payload;
    };
    
    vector<TestCommand> test_commands = {
        {EXTSOCK_CMD_GET_CONFIG, 1, "{}"},
        {EXTSOCK_CMD_SET_CONFIG, 2, "{\"socket_path\":\"/tmp/test.sock\"}"},
        {EXTSOCK_CMD_LIST_CONNECTIONS, 3, "{}"},
        {EXTSOCK_CMD_CLOSE_CONNECTION, 4, "{\"connection_id\":123}"}
    };
    
    for (const auto& cmd : test_commands) {
        EXPECT_GE(static_cast<int>(cmd.type), 0);
        EXPECT_LT(static_cast<int>(cmd.type), 100);
        EXPECT_GT(cmd.id, 0);
        EXPECT_FALSE(cmd.payload.empty());
        
        // Verify payload is valid JSON
        json_object* payload_json = json_helper->parse(cmd.payload);
        EXPECT_NE(payload_json, nullptr);
        if (payload_json) {
            json_helper->cleanup(payload_json);
        }
    }
    
    cout << "Common command processing works correctly" << endl;
}

/**
 * Test: Common Threading Support
 */
TEST_F(ExtSockCommonTest, CommonThreadingSupport) {
    cout << "Testing common threading support" << endl;
    
    // Test thread-safe operations
    const int num_threads = 4;
    const int operations_per_thread = 100;
    
    atomic<int> total_operations(0);
    atomic<int> successful_operations(0);
    
    vector<thread> threads;
    
    auto worker = [&]() {
        for (int i = 0; i < operations_per_thread; ++i) {
            total_operations++;
            
            // Simulate common operations
            memory_tracker->allocate(64);
            this_thread::sleep_for(chrono::microseconds(1));
            memory_tracker->deallocate(64);
            
            successful_operations++;
        }
    };
    
    // Start threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify results
    EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);
    EXPECT_EQ(successful_operations.load(), num_threads * operations_per_thread);
    
    cout << "Common threading support works correctly" << endl;
}

/**
 * Test: Common Error Integration
 */
TEST_F(ExtSockCommonTest, CommonErrorIntegration) {
    cout << "Testing common error integration" << endl;
    
    // Test error handling in common operations
    vector<pair<extsock_error_t, string>> error_scenarios = {
        {EXTSOCK_ERROR_JSON_PARSE, "Invalid JSON in configuration"},
        {EXTSOCK_ERROR_CONFIG_INVALID, "Missing required configuration parameter"},
        {EXTSOCK_ERROR_SOCKET_FAILED, "Failed to create socket"},
        {EXTSOCK_ERROR_MEMORY_ALLOCATION, "Out of memory"},
        {EXTSOCK_ERROR_STRONGSWAN_API, "strongSwan API call failed"}
    };
    
    for (const auto& [error_code, error_msg] : error_scenarios) {
        // Simulate error occurrence
        EXPECT_NE(error_code, EXTSOCK_SUCCESS);
        EXPECT_FALSE(error_msg.empty());
        
        // Test error information can be properly formatted
        string formatted_error = string_utils->format(
            "Error {}: {}", 
            static_cast<int>(error_code), 
            error_msg
        );
        
        EXPECT_FALSE(formatted_error.empty());
        EXPECT_NE(formatted_error.find(error_msg), string::npos);
    }
    
    cout << "Common error integration works correctly" << endl;
}

// Entry point for standalone execution
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    cout << "Starting Week 2 extsock Common Functionality Tests" << endl;
    
    int result = RUN_ALL_TESTS();
    
    if (result == 0) {
        cout << "All extsock common functionality tests passed!" << endl;
    } else {
        cout << "Some extsock common functionality tests failed!" << endl;
    }
    
    return result;
} 