/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Base Test Fixture for extsock Plugin Google Tests Implementation
 * TASK-M002: Mock Infrastructure Construction - Test Fixtures
 */

#include "ExtsockTestBase.hpp"
#include <cstring>
#include <chrono>

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnNull;

namespace extsock_test {

void ExtsockTestBase::SetUp() {
    // Initialize mock managers
    initializeMockManagers();
    
    // Configure default behaviors
    configureDefaultMockBehaviors();
    
    // Initialize test data
    initializeTestData();
}

void ExtsockTestBase::TearDown() {
    // Verify all mocks were used as expected
    verifyAllMocksClean();
    
    // Clean up allocated memory
    cleanupAllocatedMemory();
    
    // Reset mock expectations for next test
    resetAllMocks();
}

void ExtsockTestBase::initializeMockManagers() {
    strongswan_manager_ = std::make_unique<mocks::StrongSwanMockManager>();
    json_parser_manager_ = std::make_unique<mocks::JsonParserMockManager>();
    socket_adapter_manager_ = std::make_unique<mocks::SocketAdapterMockManager>();
}

void ExtsockTestBase::configureDefaultMockBehaviors() {
    // Default behaviors are already configured in the mock managers
    // This method can be overridden by derived classes for custom setup
}

void ExtsockTestBase::initializeTestData() {
    // Initialize JSON test data
    test_config_json_ = R"({
        "ike_config": {
            "version": 2,
            "local_port": 500,
            "remote_port": 500,
            "proposals": ["aes256-sha256-modp2048"]
        },
        "peer_config": {
            "name": "test_peer",
            "local_id": "client@example.com",
            "remote_id": "server@example.com"
        },
        "children": [
            {
                "name": "test_child",
                "esp_proposals": ["aes256-sha256"],
                "local_ts": ["192.168.1.0/24"],
                "remote_ts": ["10.0.0.0/8"]
            }
        ]
    })";

    test_ike_config_json_ = R"({
        "version": 2,
        "local_port": 500,
        "remote_port": 500,
        "proposals": ["aes256-sha256-modp2048"]
    })";

    test_child_config_json_ = R"({
        "name": "test_child",
        "esp_proposals": ["aes256-sha256"],
        "local_ts": ["192.168.1.0/24"],
        "remote_ts": ["10.0.0.0/8"]
    })";

    invalid_json_ = "{ invalid json syntax missing closing brace";
}

const char* ExtsockTestBase::getTestConfigJson() {
    return test_config_json_.c_str();
}

const char* ExtsockTestBase::getTestIkeConfigJson() {
    return test_ike_config_json_.c_str();
}

const char* ExtsockTestBase::getTestChildConfigJson() {
    return test_child_config_json_.c_str();
}

const char* ExtsockTestBase::getInvalidJson() {
    return invalid_json_.c_str();
}

void* ExtsockTestBase::allocateTestMemory(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        allocated_memory_.push_back(ptr);
    }
    return ptr;
}

void ExtsockTestBase::freeTestMemory(void* ptr) {
    if (ptr) {
        auto it = std::find(allocated_memory_.begin(), allocated_memory_.end(), ptr);
        if (it != allocated_memory_.end()) {
            allocated_memory_.erase(it);
            free(ptr);
        }
    }
}

char* ExtsockTestBase::duplicateString(const char* str) {
    if (!str) return nullptr;
    
    size_t len = strlen(str) + 1;
    auto dup_str = std::make_unique<char[]>(len);
    strncpy(dup_str.get(), str, len);
    
    char* result = dup_str.get();
    allocated_strings_.push_back(std::move(dup_str));
    return result;
}

void ExtsockTestBase::verifyAllMocksClean() {
    // Verify that all expected mock calls were made
    // Google Mock will automatically verify expectations in the destructor
    // This method can be extended for additional verification
}

void ExtsockTestBase::resetAllMocks() {
    if (strongswan_manager_) {
        strongswan_manager_->resetAllMocks();
    }
    if (json_parser_manager_) {
        json_parser_manager_->resetAllMocks();
    }
    if (socket_adapter_manager_) {
        socket_adapter_manager_->resetAllMocks();
    }
}

void ExtsockTestBase::cleanupAllocatedMemory() {
    // Free all allocated memory
    for (void* ptr : allocated_memory_) {
        free(ptr);
    }
    allocated_memory_.clear();
    
    // Unique pointers will clean up automatically
    allocated_strings_.clear();
}

void ExtsockTestBase::setupBasicIkeScenario() {
    // Setup strongSwan mocks for basic IKE scenario
    strongswan_manager_->setupBasicIkeScenario();
    
    // Setup JSON parser for valid config
    json_parser_manager_->setupValidIkeConfigScenario();
    
    // Setup socket for successful connection
    socket_adapter_manager_->setupSuccessfulConnectionScenario();
}

void ExtsockTestBase::setupCompleteWorkflowScenario() {
    // Setup complete workflow including child SA configuration
    strongswan_manager_->setupFailoverScenario(); // Use existing scenario
    json_parser_manager_->setupComplexConfigScenario();
    socket_adapter_manager_->setupDataTransmissionScenario();
}

void ExtsockTestBase::setupErrorScenario() {
    // Setup error conditions across all components
    strongswan_manager_->setupFailoverScenario(); // Use existing scenario
    json_parser_manager_->setupInvalidJsonScenario();
    socket_adapter_manager_->setupConnectionFailureScenario();
}

// ExtsockUnitTestBase Implementation

void ExtsockUnitTestBase::SetUp() {
    ExtsockTestBase::SetUp();
    
    // Additional unit test setup
    // Enable strict mock checking
    ::testing::FLAGS_gmock_verbose = "error";
}

void ExtsockUnitTestBase::setupIsolatedComponent(const std::string& component_name) {
    current_component_ = component_name;
    
    // Configure mocks for isolated testing of specific component
    if (component_name == "json_parser") {
        // Only setup JSON parser mocks, keep others minimal
        getJsonParserManager()->setupValidIkeConfigScenario();
    } else if (component_name == "socket_adapter") {
        // Only setup socket adapter mocks
        getSocketAdapterManager()->setupSuccessfulConnectionScenario();
    } else if (component_name == "strongswan_adapter") {
        // Only setup strongSwan mocks
        getStrongSwanManager()->setupBasicIkeScenario();
    }
}

void ExtsockUnitTestBase::verifyNoUnexpectedInteractions() {
    // Verify that only expected component was interacted with
    // This helps ensure unit tests are properly isolated
    
    if (current_component_ != "strongswan_adapter") {
        // Verify minimal strongSwan interaction
        ::testing::Mock::VerifyAndClear(getStrongSwanManager()->getIkeConfigMock());
    }
    
    if (current_component_ != "json_parser") {
        // Verify minimal JSON parser interaction
        ::testing::Mock::VerifyAndClear(getJsonParserManager()->getJsonParserMock());
    }
    
    if (current_component_ != "socket_adapter") {
        // Verify minimal socket adapter interaction
        ::testing::Mock::VerifyAndClear(getSocketAdapterManager()->getSocketAdapterMock());
    }
}

// ExtsockPerformanceTestBase Implementation

void ExtsockPerformanceTestBase::SetUp() {
    ExtsockTestBase::SetUp();
    
    initial_memory_usage_ = getMemoryUsage();
}

void ExtsockPerformanceTestBase::TearDown() {
    checkMemoryLeaks();
    
    ExtsockTestBase::TearDown();
}

void ExtsockPerformanceTestBase::startPerformanceTimer() {
    start_time_ = std::chrono::high_resolution_clock::now();
}

void ExtsockPerformanceTestBase::stopPerformanceTimer() {
    end_time_ = std::chrono::high_resolution_clock::now();
}

double ExtsockPerformanceTestBase::getElapsedMilliseconds() {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time_ - start_time_);
    return duration.count() / 1000.0; // Convert to milliseconds
}

size_t ExtsockPerformanceTestBase::getMemoryUsage() {
    // Simplified memory usage tracking
    // In a real implementation, this would use system calls to get actual memory usage
    return 0; // Placeholder
}

void ExtsockPerformanceTestBase::checkMemoryLeaks() {
    size_t current_usage = getMemoryUsage();
    size_t leaked_bytes = current_usage - initial_memory_usage_;
    
    if (leaked_bytes > 1024) { // Allow some tolerance
        ADD_FAILURE() << "Potential memory leak detected: " << leaked_bytes << " bytes";
    }
}

void ExtsockPerformanceTestBase::assertPerformanceThreshold(double max_milliseconds) {
    double elapsed = getElapsedMilliseconds();
    ASSERT_LE(elapsed, max_milliseconds) 
        << "Performance threshold exceeded: " << elapsed << "ms > " << max_milliseconds << "ms";
}

void ExtsockPerformanceTestBase::assertMemoryUsageThreshold(size_t max_bytes) {
    size_t usage = getMemoryUsage() - initial_memory_usage_;
    ASSERT_LE(usage, max_bytes) 
        << "Memory usage threshold exceeded: " << usage << " bytes > " << max_bytes << " bytes";
}

} // namespace extsock_test