/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Base Test Fixture for extsock Plugin Google Tests
 * TASK-M002: Mock Infrastructure Construction - Test Fixtures
 * 
 * This file provides the base test fixture class that all extsock
 * Google Tests should inherit from. It provides common setup/teardown
 * functionality and access to mock managers.
 */

#ifndef EXTSOCK_TEST_BASE_HPP_
#define EXTSOCK_TEST_BASE_HPP_

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "../mocks/MockStrongSwan.hpp"
#include "../mocks/MockJsonParser.hpp" 
#include "../mocks/MockSocketAdapter.hpp"

extern "C" {
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
}

namespace extsock_test {

/**
 * Base Test Fixture for extsock Plugin Tests
 * 
 * This class provides common functionality that all extsock tests need:
 * - Mock manager instances and lifecycle management
 * - Common test data and helper methods
 * - Memory management and cleanup
 * - Consistent test environment setup
 */
class ExtsockTestBase : public ::testing::Test {
protected:
    /**
     * Set up test environment before each test
     * - Creates all mock managers
     * - Initializes common test data
     * - Configures default mock behaviors
     */
    void SetUp() override;
    
    /**
     * Clean up test environment after each test
     * - Resets all mock expectations
     * - Cleans up allocated memory
     * - Verifies mock expectations were met
     */
    void TearDown() override;

    // Mock Manager Access Methods
    mocks::StrongSwanMockManager* getStrongSwanManager() { 
        return strongswan_manager_.get(); 
    }
    
    mocks::JsonParserMockManager* getJsonParserManager() { 
        return json_parser_manager_.get(); 
    }
    
    mocks::SocketAdapterMockManager* getSocketAdapterManager() { 
        return socket_adapter_manager_.get(); 
    }

    // Test Data Helper Methods
    const char* getTestConfigJson();
    const char* getTestIkeConfigJson();
    const char* getTestChildConfigJson();
    const char* getInvalidJson();
    
    // Common Test Addresses and Ports
    const char* getTestHost() { return "192.168.1.100"; }
    const char* getTestPeerHost() { return "192.168.1.200"; }
    uint16_t getTestPort() { return 8080; }
    uint16_t getIkePort() { return 500; }
    
    // Test Message Templates
    const char* getTestMessage() { return "Test extsock message"; }
    const char* getTestErrorMessage() { return "Test error occurred"; }

    // Memory Management Helpers
    void* allocateTestMemory(size_t size);
    void freeTestMemory(void* ptr);
    char* duplicateString(const char* str);
    
    // Mock Verification Helpers
    void verifyAllMocksClean();
    void resetAllMocks();
    
    // Common Test Scenarios Setup
    void setupBasicIkeScenario();
    void setupCompleteWorkflowScenario();
    void setupErrorScenario();
    
private:
    // Mock managers
    std::unique_ptr<mocks::StrongSwanMockManager> strongswan_manager_;
    std::unique_ptr<mocks::JsonParserMockManager> json_parser_manager_;
    std::unique_ptr<mocks::SocketAdapterMockManager> socket_adapter_manager_;
    
    // Memory management
    std::vector<void*> allocated_memory_;
    std::vector<std::unique_ptr<char[]>> allocated_strings_;
    
    // Helper methods
    void initializeMockManagers();
    void configureDefaultMockBehaviors();
    void cleanupAllocatedMemory();
    
    // Test data storage
    std::string test_config_json_;
    std::string test_ike_config_json_;
    std::string test_child_config_json_;
    std::string invalid_json_;
    
    void initializeTestData();
};

/**
 * Specialized fixture for unit tests
 * 
 * Provides additional functionality specific to unit testing:
 * - Isolated component testing setup
 * - Single component mock configuration
 * - Focused test data and scenarios
 */
class ExtsockUnitTestBase : public ExtsockTestBase {
protected:
    void SetUp() override;
    
    // Unit test specific helpers
    void setupIsolatedComponent(const std::string& component_name);
    void verifyNoUnexpectedInteractions();
    
private:
    std::string current_component_;
};

/**
 * Specialized fixture for performance tests
 * 
 * Provides functionality for performance and stress testing:
 * - Timing measurement utilities
 * - Resource usage monitoring
 * - Performance threshold validation
 */
class ExtsockPerformanceTestBase : public ExtsockTestBase {
protected:
    void SetUp() override;
    void TearDown() override;
    
    // Performance measurement
    void startPerformanceTimer();
    void stopPerformanceTimer();
    double getElapsedMilliseconds();
    
    // Resource monitoring
    size_t getMemoryUsage();
    void checkMemoryLeaks();
    
    // Performance assertions
    void assertPerformanceThreshold(double max_milliseconds);
    void assertMemoryUsageThreshold(size_t max_bytes);
    
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point end_time_;
    size_t initial_memory_usage_;
};

/**
 * Test parameter helper for parameterized tests
 */
struct ExtsockTestParams {
    std::string test_name;
    std::string config_json;
    bool should_succeed;
    std::string expected_error;
    
    ExtsockTestParams(const std::string& name, const std::string& config, 
                     bool success, const std::string& error = "")
        : test_name(name), config_json(config), should_succeed(success), expected_error(error) {}
};

/**
 * Custom test assertions for extsock
 */
#define ASSERT_EXTSOCK_SUCCESS(result) \
    ASSERT_TRUE(result) << "extsock operation should have succeeded"

#define EXPECT_EXTSOCK_SUCCESS(result) \
    EXPECT_TRUE(result) << "extsock operation should have succeeded"

#define ASSERT_EXTSOCK_FAILURE(result) \
    ASSERT_FALSE(result) << "extsock operation should have failed"

#define EXPECT_EXTSOCK_FAILURE(result) \
    EXPECT_FALSE(result) << "extsock operation should have failed"

#define ASSERT_VALID_POINTER(ptr) \
    ASSERT_NE(ptr, nullptr) << "Pointer should be valid (non-null)"

#define EXPECT_VALID_POINTER(ptr) \
    EXPECT_NE(ptr, nullptr) << "Pointer should be valid (non-null)"

#define ASSERT_MOCK_CALLED(mock, method) \
    do { \
        ::testing::Mock::VerifyAndClearExpectations(&mock); \
    } while(0)

} // namespace extsock_test

#endif // EXTSOCK_TEST_BASE_HPP_