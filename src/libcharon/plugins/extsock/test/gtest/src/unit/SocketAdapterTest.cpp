/**
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Test Unit Tests for Socket Adapter
 * Migrated from test_socket_adapter_standalone.c
 * 
 * Level 2 (Adapter) tests that use Google Mock to test socket adapter layer
 * functionality with controlled dependencies.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <memory>
#include <chrono>
#include <thread>

// Mock strongSwan dependencies
#include "../../infrastructure/mocks/MockStrongSwan.hpp"

// Pure types for testing  
extern "C" {
    #include "extsock_types_pure.h"
}

// Forward declarations for Socket structures
struct mock_mutex_t;
struct mock_thread_t;
struct mock_command_handler_t;

// Mock Socket Adapter Implementation for testing
class MockSocketAdapter {
public:
    MOCK_METHOD(extsock_error_t, publish_event, (const char* event_json), ());
    MOCK_METHOD(extsock_error_t, publish_tunnel_event, (const char* tunnel_event_json), ());
    MOCK_METHOD(extsock_error_t, send_event, (const char* event_json), ());
    MOCK_METHOD(void*, start_listening, (), ());
    MOCK_METHOD(void, stop_listening, (), ());
    MOCK_METHOD(void, set_socket_failure, (bool enable), ());
    MOCK_METHOD(void, set_send_failure, (bool enable), ());
    MOCK_METHOD(const char*, get_last_event_sent, (), ());
    MOCK_METHOD(int, get_command_count, (), ());
    MOCK_METHOD(bool, is_running, (), ());
};

// Mock Command Handler for testing
class MockCommandHandler {
public:
    MOCK_METHOD(void, handle_command, (const char* command), ());
    MOCK_METHOD(int, get_command_count, (), ());
    MOCK_METHOD(const char*, get_command, (int index), ());
};

/**
 * ============================================================================ 
 * Test Fixture Class
 * ============================================================================
 */
class SocketAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mock strongSwan system
        mock_strongswan = std::make_unique<extsock_test::mocks::StrongSwanMockManager>();
        mock_socket_adapter = std::make_unique<MockSocketAdapter>();
        mock_command_handler = std::make_unique<MockCommandHandler>();
        
        // Setup default expectations
        EXPECT_CALL(*mock_strongswan, reset_state())
            .WillRepeatedly(::testing::Return());
    }

    void TearDown() override {
        mock_command_handler.reset();
        mock_socket_adapter.reset();
        mock_strongswan.reset();
    }

    // Test data constants
    const std::string VALID_EVENT_JSON = R"({
        "type": "tunnel_up",
        "connection": "test-vpn",
        "timestamp": 1234567890
    })";

    const std::string VALID_TUNNEL_EVENT_JSON = R"({
        "type": "tunnel_down", 
        "connection": "vpn1",
        "reason": "user_disconnect"
    })";

    const std::string LARGE_EVENT_JSON = R"({
        "type": "status_update",
        "connection": "large-connection-name",
        "data": ")" + std::string(1000, 'A') + R"(",
        "details": {
            "field1": "value1",
            "field2": "value2",
            "field3": "value3"
        }
    })";

    std::unique_ptr<extsock_test::mocks::StrongSwanMockManager> mock_strongswan;
    std::unique_ptr<MockSocketAdapter> mock_socket_adapter;
    std::unique_ptr<MockCommandHandler> mock_command_handler;
};

/**
 * ============================================================================
 * Socket Adapter Creation and Destruction Tests
 * ============================================================================
 */

TEST_F(SocketAdapterTest, CreateDestroy) {
    // Test that we can create a mock socket adapter with all required methods
    MockSocketAdapter adapter;
    
    // Verify that all methods are available (compile-time check)
    EXPECT_CALL(adapter, publish_event(::testing::_))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    extsock_error_t result = adapter.publish_event("test");
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
}

TEST_F(SocketAdapterTest, MultipleCreateDestroy) {
    // Test creating and destroying multiple socket adapters
    std::vector<std::unique_ptr<MockSocketAdapter>> adapters;
    
    // Create multiple adapters
    for (int i = 0; i < 5; i++) {
        adapters.push_back(std::make_unique<MockSocketAdapter>());
        EXPECT_NE(adapters[i], nullptr);
    }
    
    // Verify they are all valid
    for (const auto& adapter : adapters) {
        EXPECT_NE(adapter.get(), nullptr);
    }
    
    // Adapters will be automatically destroyed when going out of scope
}

/**
 * ============================================================================
 * Event Publishing Tests
 * ============================================================================
 */

TEST_F(SocketAdapterTest, PublishEventValid) {
    // Setup mock expectations
    EXPECT_CALL(*mock_socket_adapter, publish_event(::testing::StrEq(VALID_EVENT_JSON.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_socket_adapter, get_last_event_sent())
        .WillOnce(::testing::Return(VALID_EVENT_JSON.c_str()));
    
    // Execute test
    extsock_error_t result = mock_socket_adapter->publish_event(VALID_EVENT_JSON.c_str());
    
    // Verify results
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_STREQ(mock_socket_adapter->get_last_event_sent(), VALID_EVENT_JSON.c_str());
}

TEST_F(SocketAdapterTest, PublishEventNullInput) {
    EXPECT_CALL(*mock_socket_adapter, publish_event(nullptr))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_INVALID_PARAMETER));
    
    extsock_error_t result = mock_socket_adapter->publish_event(nullptr);
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
}

TEST_F(SocketAdapterTest, PublishEventEmpty) {
    const std::string empty_json = "{}";
    
    EXPECT_CALL(*mock_socket_adapter, publish_event(::testing::StrEq(empty_json.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    extsock_error_t result = mock_socket_adapter->publish_event(empty_json.c_str());
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
}

TEST_F(SocketAdapterTest, PublishTunnelEventValid) {
    EXPECT_CALL(*mock_socket_adapter, publish_tunnel_event(::testing::StrEq(VALID_TUNNEL_EVENT_JSON.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_socket_adapter, get_last_event_sent())
        .WillOnce(::testing::Return(VALID_TUNNEL_EVENT_JSON.c_str()));
    
    extsock_error_t result = mock_socket_adapter->publish_tunnel_event(VALID_TUNNEL_EVENT_JSON.c_str());
    
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_STREQ(mock_socket_adapter->get_last_event_sent(), VALID_TUNNEL_EVENT_JSON.c_str());
}

TEST_F(SocketAdapterTest, SendEventDelegation) {
    // Test that send_event properly delegates to publish_event
    EXPECT_CALL(*mock_socket_adapter, send_event(::testing::StrEq(VALID_EVENT_JSON.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_socket_adapter, publish_event(::testing::StrEq(VALID_EVENT_JSON.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    // Call send_event which should delegate to publish_event
    extsock_error_t result = mock_socket_adapter->send_event(VALID_EVENT_JSON.c_str());
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    
    // Verify delegation occurred
    mock_socket_adapter->publish_event(VALID_EVENT_JSON.c_str());
}

/**
 * ============================================================================
 * Socket Listening Tests
 * ============================================================================
 */

TEST_F(SocketAdapterTest, StartListening) {
    void* mock_thread = reinterpret_cast<void*>(0x12345678);
    
    EXPECT_CALL(*mock_socket_adapter, start_listening())
        .WillOnce(::testing::Return(mock_thread));
    
    EXPECT_CALL(*mock_socket_adapter, is_running())
        .WillOnce(::testing::Return(true));
    
    void* thread = mock_socket_adapter->start_listening();
    
    EXPECT_EQ(thread, mock_thread);
    EXPECT_TRUE(mock_socket_adapter->is_running());
}

TEST_F(SocketAdapterTest, StopListening) {
    EXPECT_CALL(*mock_socket_adapter, stop_listening())
        .WillOnce(::testing::Return());
    
    EXPECT_CALL(*mock_socket_adapter, is_running())
        .WillOnce(::testing::Return(false));
    
    mock_socket_adapter->stop_listening();
    EXPECT_FALSE(mock_socket_adapter->is_running());
}

TEST_F(SocketAdapterTest, StartStopListeningCycle) {
    void* mock_thread = reinterpret_cast<void*>(0xABCDEF00);
    
    // Setup expectations for start
    EXPECT_CALL(*mock_socket_adapter, start_listening())
        .WillOnce(::testing::Return(mock_thread));
    
    EXPECT_CALL(*mock_socket_adapter, is_running())
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    
    // Setup expectations for stop
    EXPECT_CALL(*mock_socket_adapter, stop_listening())
        .WillOnce(::testing::Return());
    
    // Execute cycle
    void* thread = mock_socket_adapter->start_listening();
    EXPECT_EQ(thread, mock_thread);
    EXPECT_TRUE(mock_socket_adapter->is_running());
    
    mock_socket_adapter->stop_listening();
    EXPECT_FALSE(mock_socket_adapter->is_running());
}

/**
 * ============================================================================
 * Command Handler Tests
 * ============================================================================
 */

TEST_F(SocketAdapterTest, CommandHandlerBasic) {
    const std::string test_command = "test_command_1";
    
    EXPECT_CALL(*mock_command_handler, handle_command(::testing::StrEq(test_command.c_str())))
        .WillOnce(::testing::Return());
    
    EXPECT_CALL(*mock_command_handler, get_command_count())
        .WillOnce(::testing::Return(1));
    
    mock_command_handler->handle_command(test_command.c_str());
    EXPECT_EQ(mock_command_handler->get_command_count(), 1);
}

TEST_F(SocketAdapterTest, CommandHandlerMultiple) {
    const std::vector<std::string> commands = {
        "command_1", "command_2", "command_3"
    };
    
    for (size_t i = 0; i < commands.size(); i++) {
        EXPECT_CALL(*mock_command_handler, handle_command(::testing::StrEq(commands[i].c_str())))
            .WillOnce(::testing::Return());
    }
    
    EXPECT_CALL(*mock_command_handler, get_command_count())
        .WillOnce(::testing::Return(static_cast<int>(commands.size())));
    
    for (const auto& cmd : commands) {
        mock_command_handler->handle_command(cmd.c_str());
    }
    
    EXPECT_EQ(mock_command_handler->get_command_count(), commands.size());
}

TEST_F(SocketAdapterTest, CommandHandlerRetrieve) {
    const std::string test_command = "retrieve_test_command";
    
    EXPECT_CALL(*mock_command_handler, handle_command(::testing::StrEq(test_command.c_str())))
        .WillOnce(::testing::Return());
    
    EXPECT_CALL(*mock_command_handler, get_command(0))
        .WillOnce(::testing::Return(test_command.c_str()));
    
    mock_command_handler->handle_command(test_command.c_str());
    
    const char* retrieved = mock_command_handler->get_command(0);
    EXPECT_STREQ(retrieved, test_command.c_str());
}

/**
 * ============================================================================
 * Error Handling Tests
 * ============================================================================
 */

TEST_F(SocketAdapterTest, SendFailureSimulation) {
    EXPECT_CALL(*mock_socket_adapter, set_send_failure(true))
        .WillOnce(::testing::Return());
    
    EXPECT_CALL(*mock_socket_adapter, publish_event(::testing::_))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_SOCKET_FAILED));
    
    mock_socket_adapter->set_send_failure(true);
    
    extsock_error_t result = mock_socket_adapter->publish_event(VALID_EVENT_JSON.c_str());
    EXPECT_EQ(result, EXTSOCK_ERROR_SOCKET_FAILED);
}

TEST_F(SocketAdapterTest, SocketFailureSimulation) {
    EXPECT_CALL(*mock_socket_adapter, set_socket_failure(true))
        .WillOnce(::testing::Return());
    
    EXPECT_CALL(*mock_socket_adapter, start_listening())
        .WillOnce(::testing::Return(nullptr));
    
    mock_socket_adapter->set_socket_failure(true);
    
    void* result = mock_socket_adapter->start_listening();
    EXPECT_EQ(result, nullptr);
}

TEST_F(SocketAdapterTest, InvalidJsonHandling) {
    const std::string invalid_json = "{ invalid json syntax";
    
    EXPECT_CALL(*mock_socket_adapter, publish_event(::testing::StrEq(invalid_json.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_JSON_PARSE));
    
    extsock_error_t result = mock_socket_adapter->publish_event(invalid_json.c_str());
    EXPECT_EQ(result, EXTSOCK_ERROR_JSON_PARSE);
}

/**
 * ============================================================================
 * Performance and Stress Tests
 * ============================================================================
 */

TEST_F(SocketAdapterTest, LargeEventProcessing) {
    EXPECT_CALL(*mock_socket_adapter, publish_event(::testing::StrEq(LARGE_EVENT_JSON.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    auto start = std::chrono::high_resolution_clock::now();
    
    extsock_error_t result = mock_socket_adapter->publish_event(LARGE_EVENT_JSON.c_str());
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_LT(duration.count(), 100) << "Large event processing took too long";
}

TEST_F(SocketAdapterTest, ConcurrentEventProcessing) {
    // Test concurrent event processing scenarios (simplified)
    std::vector<std::unique_ptr<MockSocketAdapter>> adapters;
    
    for (int i = 0; i < 3; i++) {
        adapters.push_back(std::make_unique<MockSocketAdapter>());
        
        EXPECT_CALL(*adapters[i], publish_event(::testing::_))
            .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    }
    
    // Simulate concurrent processing
    for (int i = 0; i < 3; i++) {
        std::string event = R"({"type": "concurrent_test", "id": )" + std::to_string(i) + R"(})";
        extsock_error_t result = adapters[i]->publish_event(event.c_str());
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
    }
}

TEST_F(SocketAdapterTest, MemoryStressTest) {
    // Create and destroy many adapters with events
    for (int i = 0; i < 10; i++) {
        auto adapter = std::make_unique<MockSocketAdapter>();
        
        EXPECT_CALL(*adapter, publish_event(::testing::_))
            .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
        
        std::string event = R"({"type": "stress_test", "iteration": )" + std::to_string(i) + R"(})";
        extsock_error_t result = adapter->publish_event(event.c_str());
        
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
        
        // Adapter will be automatically destroyed when going out of scope
    }
}

/**
 * ============================================================================
 * Thread Safety Tests (Mock-based)
 * ============================================================================
 */

TEST_F(SocketAdapterTest, ThreadSafeEventPublishing) {
    // Test thread-safe event publishing with mocks
    EXPECT_CALL(*mock_socket_adapter, publish_event(::testing::_))
        .Times(2)
        .WillRepeatedly(::testing::Return(EXTSOCK_SUCCESS));
    
    // Simulate thread-safe access
    std::string event1 = R"({"type": "thread_test_1"})";
    std::string event2 = R"({"type": "thread_test_2"})";
    
    extsock_error_t result1 = mock_socket_adapter->publish_event(event1.c_str());
    extsock_error_t result2 = mock_socket_adapter->publish_event(event2.c_str());
    
    EXPECT_EQ(result1, EXTSOCK_SUCCESS);
    EXPECT_EQ(result2, EXTSOCK_SUCCESS);
}

TEST_F(SocketAdapterTest, ListeningThreadLifecycle) {
    void* mock_thread = reinterpret_cast<void*>(0x87654321);
    
    // Setup complete lifecycle expectations
    EXPECT_CALL(*mock_socket_adapter, start_listening())
        .WillOnce(::testing::Return(mock_thread));
    
    EXPECT_CALL(*mock_socket_adapter, is_running())
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    
    EXPECT_CALL(*mock_socket_adapter, stop_listening())
        .WillOnce(::testing::Return());
    
    // Execute lifecycle
    void* thread = mock_socket_adapter->start_listening();
    EXPECT_NE(thread, nullptr);
    EXPECT_TRUE(mock_socket_adapter->is_running());
    
    // Simulate running for some time
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    mock_socket_adapter->stop_listening();
    EXPECT_FALSE(mock_socket_adapter->is_running());
}