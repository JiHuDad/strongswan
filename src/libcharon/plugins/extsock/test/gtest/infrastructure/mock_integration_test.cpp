/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Mock Infrastructure Integration Test
 * TASK-M002: Mock Infrastructure Construction - Integration Testing
 * 
 * This file tests that all Mock classes can be instantiated,
 * work together, and provide expected behaviors.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mocks/MockStrongSwan.hpp"
#include "mocks/MockJsonParser.hpp"
#include "mocks/MockSocketAdapter.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::NotNull;

namespace extsock_test {

/**
 * Test fixture for Mock Infrastructure Integration Testing
 */
class MockInfrastructureIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize all mock managers
        strongswan_manager_ = std::make_unique<mocks::StrongSwanMockManager>();
        json_parser_manager_ = std::make_unique<mocks::JsonParserMockManager>();
        socket_adapter_manager_ = std::make_unique<mocks::SocketAdapterMockManager>();
    }
    
    void TearDown() override {
        // Reset all mocks to clean state
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
    
    std::unique_ptr<mocks::StrongSwanMockManager> strongswan_manager_;
    std::unique_ptr<mocks::JsonParserMockManager> json_parser_manager_;
    std::unique_ptr<mocks::SocketAdapterMockManager> socket_adapter_manager_;
};

// Test that all mock managers can be created and destroyed
TEST_F(MockInfrastructureIntegrationTest, MockManagersCanBeCreated) {
    EXPECT_NE(strongswan_manager_, nullptr);
    EXPECT_NE(json_parser_manager_, nullptr);
    EXPECT_NE(socket_adapter_manager_, nullptr);
}

// Test strongSwan mock creation and basic operations
TEST_F(MockInfrastructureIntegrationTest, StrongSwanMocksWork) {
    // Create individual mocks
    auto ike_config_mock = strongswan_manager_->createIkeConfigMock();
    auto peer_config_mock = strongswan_manager_->createPeerConfigMock();
    auto ike_sa_mock = strongswan_manager_->createIkeSAMock();
    
    EXPECT_NE(ike_config_mock, nullptr);
    EXPECT_NE(peer_config_mock, nullptr);
    EXPECT_NE(ike_sa_mock, nullptr);
    
    // Test basic mock behavior
    EXPECT_CALL(*ike_config_mock, get_version(_))
        .WillOnce(Return(2));
    
    // Simulate calling the mock
    int version = ike_config_mock->get_version(nullptr);
    EXPECT_EQ(version, 2);
}

// Test JSON parser mock creation and scenarios
TEST_F(MockInfrastructureIntegrationTest, JsonParserMocksWork) {
    // Create JSON parser mocks
    auto json_parser_mock = json_parser_manager_->createJsonParserMock();
    auto cjson_mock = json_parser_manager_->createCJsonMock();
    
    EXPECT_NE(json_parser_mock, nullptr);
    EXPECT_NE(cjson_mock, nullptr);
    
    // Test valid IKE config scenario
    json_parser_manager_->setupValidIkeConfigScenario();
    
    // The expectations should be set up correctly
    // (Actual verification happens when the mocks are used)
}

// Test socket adapter mock creation and scenarios
TEST_F(MockInfrastructureIntegrationTest, SocketAdapterMocksWork) {
    // Create socket adapter mocks
    auto socket_adapter_mock = socket_adapter_manager_->createSocketAdapterMock();
    auto system_socket_mock = socket_adapter_manager_->createSystemSocketMock();
    
    EXPECT_NE(socket_adapter_mock, nullptr);
    EXPECT_NE(system_socket_mock, nullptr);
    
    // Test successful connection scenario
    socket_adapter_manager_->setupSuccessfulConnectionScenario();
    
    // The expectations should be set up correctly
    // (Actual verification happens when the mocks are used)
}

// Test mock managers can access their mock instances
TEST_F(MockInfrastructureIntegrationTest, MockManagersProvideAccess) {
    // Test strongSwan manager access
    EXPECT_NE(strongswan_manager_->getIkeConfigMock(), nullptr);
    EXPECT_NE(strongswan_manager_->getPeerConfigMock(), nullptr);
    EXPECT_NE(strongswan_manager_->getIkeSAMock(), nullptr);
    
    // Test JSON parser manager access
    EXPECT_NE(json_parser_manager_->getJsonParserMock(), nullptr);
    EXPECT_NE(json_parser_manager_->getCJsonMock(), nullptr);
    
    // Test socket adapter manager access
    EXPECT_NE(socket_adapter_manager_->getSocketAdapterMock(), nullptr);
    EXPECT_NE(socket_adapter_manager_->getSystemSocketMock(), nullptr);
}

// Test pre-configured scenarios work correctly
TEST_F(MockInfrastructureIntegrationTest, PreConfiguredScenariosWork) {
    // strongSwan basic IKE scenario
    strongswan_manager_->setupBasicIkeScenario();
    
    auto peer_config = strongswan_manager_->getPeerConfigMock();
    const char* peer_name = peer_config->get_name(nullptr);
    EXPECT_STREQ(peer_name, "basic_peer");
    
    // JSON parser valid config scenario
    json_parser_manager_->setupValidIkeConfigScenario();
    
    // Socket adapter successful connection scenario
    socket_adapter_manager_->setupSuccessfulConnectionScenario();
}

// Test that mocks can be reset properly
TEST_F(MockInfrastructureIntegrationTest, MocksCanBeReset) {
    // Set up some expectations
    auto ike_config = strongswan_manager_->getIkeConfigMock();
    EXPECT_CALL(*ike_config, get_version(_))
        .WillOnce(Return(2));
    
    // Use the mock
    int version = ike_config->get_version(nullptr);
    EXPECT_EQ(version, 2);
    
    // Reset should not throw
    EXPECT_NO_THROW(strongswan_manager_->resetAllMocks());
}

// Test mock factory methods create unique instances
TEST_F(MockInfrastructureIntegrationTest, FactoryMethodsCreateUniqueInstances) {
    auto mock1 = strongswan_manager_->createIkeConfigMock();
    auto mock2 = strongswan_manager_->createIkeConfigMock();
    
    EXPECT_NE(mock1.get(), mock2.get());
    EXPECT_NE(mock1, nullptr);
    EXPECT_NE(mock2, nullptr);
}

// Test test data helpers work
TEST_F(MockInfrastructureIntegrationTest, TestDataHelpersWork) {
    // JSON test data
    const char* valid_json = json_parser_manager_->getValidIkeConfigJsonString();
    const char* invalid_json = json_parser_manager_->getInvalidJsonString();
    
    EXPECT_NE(valid_json, nullptr);
    EXPECT_NE(invalid_json, nullptr);
    EXPECT_STRNE(valid_json, invalid_json);
    
    // Socket test data
    const char* test_host = socket_adapter_manager_->getTestHost();
    uint16_t test_port = socket_adapter_manager_->getTestPort();
    const char* test_message = socket_adapter_manager_->getTestMessage();
    
    EXPECT_STREQ(test_host, "192.168.1.100");
    EXPECT_EQ(test_port, 8080);
    EXPECT_STREQ(test_message, "Test socket message");
}

// Test event simulation helpers
TEST_F(MockInfrastructureIntegrationTest, EventSimulationWorks) {
    // Test socket event simulation
    EXPECT_NO_THROW(socket_adapter_manager_->simulateConnectionEstablished());
    EXPECT_NO_THROW(socket_adapter_manager_->simulateConnectionLost());
    EXPECT_NO_THROW(socket_adapter_manager_->simulateDataReceived("test", 4));
    EXPECT_NO_THROW(socket_adapter_manager_->simulateNetworkError("Test error"));
    EXPECT_NO_THROW(socket_adapter_manager_->simulateTimeout());
}

// Integration test: Full workflow simulation
TEST_F(MockInfrastructureIntegrationTest, FullWorkflowSimulation) {
    // This test demonstrates how all mocks work together
    // in a realistic scenario
    
    // 1. Setup JSON parsing for configuration
    json_parser_manager_->setupValidIkeConfigScenario();
    
    // 2. Setup strongSwan IKE scenario
    strongswan_manager_->setupBasicIkeScenario();
    
    // 3. Setup socket connection
    socket_adapter_manager_->setupSuccessfulConnectionScenario();
    
    // 4. Simulate the workflow
    // Parse JSON config
    auto json_parser = json_parser_manager_->getJsonParserMock();
    EXPECT_NE(json_parser, nullptr);
    
    // Get strongSwan components
    auto peer_config = strongswan_manager_->getPeerConfigMock();
    const char* peer_name = peer_config->get_name(nullptr);
    EXPECT_STREQ(peer_name, "basic_peer");
    
    // Connect via socket
    auto socket_adapter = socket_adapter_manager_->getSocketAdapterMock();
    bool connected = socket_adapter->is_connected(nullptr);
    EXPECT_TRUE(connected);
    
    // All components work together successfully
}

// Performance test: Mock creation and destruction
TEST_F(MockInfrastructureIntegrationTest, MockPerformance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create many mocks quickly
    for (int i = 0; i < 100; ++i) {
        auto ike_mock = strongswan_manager_->createIkeConfigMock();
        auto json_mock = json_parser_manager_->createJsonParserMock();
        auto socket_mock = socket_adapter_manager_->createSocketAdapterMock();
        
        // Mocks should be created successfully
        EXPECT_NE(ike_mock, nullptr);
        EXPECT_NE(json_mock, nullptr);
        EXPECT_NE(socket_mock, nullptr);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    
    // Should be reasonably fast (less than 1 second for 100 mocks)
    EXPECT_LT(duration.count(), 1000) 
        << "Mock creation took too long: " << duration.count() << "ms";
}

} // namespace extsock_test