/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Mock based Socket Adapter Mock Implementation
 * TASK-M002: Mock Infrastructure Construction
 */

#include "MockSocketAdapter.hpp"
#include <cstring>
#include <errno.h>

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::NotNull;

namespace extsock_test {
namespace mocks {

SocketAdapterMockManager::SocketAdapterMockManager()
    : socket_adapter_mock_(std::make_unique<MockSocketAdapter>())
    , system_socket_mock_(std::make_unique<MockSystemSocket>())
{
    configureDefaultBehaviors();
}

SocketAdapterMockManager::~SocketAdapterMockManager() {
    // Unique pointers will clean up automatically
}

void SocketAdapterMockManager::configureDefaultBehaviors() {
    // Configure default behaviors for socket adapter mock
    ON_CALL(*socket_adapter_mock_, create_socket(_, _, _))
        .WillByDefault(Return(true));
    
    ON_CALL(*socket_adapter_mock_, connect(_))
        .WillByDefault(Return(true));
        
    ON_CALL(*socket_adapter_mock_, disconnect(_))
        .WillByDefault(Return(true));
        
    ON_CALL(*socket_adapter_mock_, is_connected(_))
        .WillByDefault(Return(false));
    
    ON_CALL(*socket_adapter_mock_, send_data(_, _, _))
        .WillByDefault(Return(0));
        
    ON_CALL(*socket_adapter_mock_, receive_data(_, _, _))
        .WillByDefault(Return(0));
    
    ON_CALL(*socket_adapter_mock_, publish_event(_, _))
        .WillByDefault(Return(true));
        
    ON_CALL(*socket_adapter_mock_, register_event_listener(_, _, _))
        .WillByDefault(Return(true));
        
    ON_CALL(*socket_adapter_mock_, unregister_event_listener(_, _))
        .WillByDefault(Return(true));
    
    ON_CALL(*socket_adapter_mock_, start_listening(_, _, _))
        .WillByDefault(Return(true));
        
    ON_CALL(*socket_adapter_mock_, stop_listening(_))
        .WillByDefault(Return(true));
        
    ON_CALL(*socket_adapter_mock_, set_timeout(_, _))
        .WillByDefault(Return(true));
    
    ON_CALL(*socket_adapter_mock_, get_socket_fd(_))
        .WillByDefault(Return(-1)); // Invalid fd by default
        
    ON_CALL(*socket_adapter_mock_, get_host(_))
        .WillByDefault(Return("127.0.0.1"));
        
    ON_CALL(*socket_adapter_mock_, get_port(_))
        .WillByDefault(Return(8080));
        
    ON_CALL(*socket_adapter_mock_, get_last_error(_, _, _))
        .WillByDefault(Return(false));
    
    // Configure default behaviors for system socket mock
    ON_CALL(*system_socket_mock_, socket(_, _, _))
        .WillByDefault(Return(-1)); // Fail by default
        
    ON_CALL(*system_socket_mock_, connect(_, _, _))
        .WillByDefault(Return(-1));
        
    ON_CALL(*system_socket_mock_, bind(_, _, _))
        .WillByDefault(Return(-1));
        
    ON_CALL(*system_socket_mock_, listen(_, _))
        .WillByDefault(Return(-1));
        
    ON_CALL(*system_socket_mock_, accept(_, _, _))
        .WillByDefault(Return(-1));
        
    ON_CALL(*system_socket_mock_, send(_, _, _, _))
        .WillByDefault(Return(-1));
        
    ON_CALL(*system_socket_mock_, recv(_, _, _, _))
        .WillByDefault(Return(-1));
        
    ON_CALL(*system_socket_mock_, close(_))
        .WillByDefault(Return(0));
        
    ON_CALL(*system_socket_mock_, setsockopt(_, _, _, _, _))
        .WillByDefault(Return(0));
        
    ON_CALL(*system_socket_mock_, getsockopt(_, _, _, _, _))
        .WillByDefault(Return(0));
}

std::unique_ptr<MockSocketAdapter> SocketAdapterMockManager::createSocketAdapterMock() {
    auto mock = std::make_unique<MockSocketAdapter>();
    
    // Set up default behaviors (same as above but for new instance)
    ON_CALL(*mock, create_socket(_, _, _))
        .WillByDefault(Return(true));
    ON_CALL(*mock, connect(_))
        .WillByDefault(Return(true));
    ON_CALL(*mock, is_connected(_))
        .WillByDefault(Return(false));
        
    return mock;
}

std::unique_ptr<MockSystemSocket> SocketAdapterMockManager::createSystemSocketMock() {
    auto mock = std::make_unique<MockSystemSocket>();
    
    // Set up default failure behaviors for system calls
    ON_CALL(*mock, socket(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*mock, connect(_, _, _))
        .WillByDefault(Return(-1));
        
    return mock;
}

void SocketAdapterMockManager::setupSuccessfulConnectionScenario() {
    // Configure for successful connection scenario
    int mock_socket_fd = 42;
    
    EXPECT_CALL(*socket_adapter_mock_, create_socket(_, StrEq(getTestHost()), getTestPort()))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*socket_adapter_mock_, connect(_))
        .WillOnce(Return(true));
        
    EXPECT_CALL(*socket_adapter_mock_, is_connected(_))
        .WillRepeatedly(Return(true));
        
    EXPECT_CALL(*socket_adapter_mock_, get_socket_fd(_))
        .WillRepeatedly(Return(mock_socket_fd));
        
    EXPECT_CALL(*socket_adapter_mock_, get_host(_))
        .WillRepeatedly(Return(getTestHost()));
        
    EXPECT_CALL(*socket_adapter_mock_, get_port(_))
        .WillRepeatedly(Return(getTestPort()));
    
    // System socket calls
    EXPECT_CALL(*system_socket_mock_, socket(AF_INET, SOCK_STREAM, 0))
        .WillOnce(Return(mock_socket_fd));
        
    EXPECT_CALL(*system_socket_mock_, connect(mock_socket_fd, _, _))
        .WillOnce(Return(0));
}

void SocketAdapterMockManager::setupConnectionFailureScenario() {
    // Configure for connection failure scenario
    
    EXPECT_CALL(*socket_adapter_mock_, create_socket(_, _, _))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*socket_adapter_mock_, connect(_))
        .WillOnce(Return(false));
        
    EXPECT_CALL(*socket_adapter_mock_, is_connected(_))
        .WillRepeatedly(Return(false));
        
    EXPECT_CALL(*socket_adapter_mock_, get_last_error(_, NotNull(), _))
        .WillOnce(DoAll(
            testing::Invoke([](extsock_socket_adapter_t*, char* error_buffer, size_t) {
                strcpy(error_buffer, "Connection refused");
            }),
            Return(true)
        ));
    
    // System socket calls fail
    EXPECT_CALL(*system_socket_mock_, socket(_, _, _))
        .WillOnce(Return(5)); // Valid fd
        
    EXPECT_CALL(*system_socket_mock_, connect(5, _, _))
        .WillOnce(Return(-1)); // Connection fails
}

void SocketAdapterMockManager::setupDataTransmissionScenario() {
    // Configure for successful data transmission
    const char* test_data = getTestMessage();
    size_t test_data_len = strlen(test_data);
    
    // Sending data
    EXPECT_CALL(*socket_adapter_mock_, send_data(_, _, test_data_len))
        .WillOnce(Return(static_cast<ssize_t>(test_data_len)));
    
    // Receiving data
    EXPECT_CALL(*socket_adapter_mock_, receive_data(_, NotNull(), _))
        .WillOnce(DoAll(
            testing::Invoke([test_data](extsock_socket_adapter_t*, void* buffer, size_t) {
                strcpy(static_cast<char*>(buffer), test_data);
            }),
            Return(static_cast<ssize_t>(test_data_len))
        ));
    
    // System level
    EXPECT_CALL(*system_socket_mock_, send(_, _, test_data_len, _))
        .WillOnce(Return(static_cast<ssize_t>(test_data_len)));
        
    EXPECT_CALL(*system_socket_mock_, recv(_, NotNull(), _, _))
        .WillOnce(Return(static_cast<ssize_t>(test_data_len)));
}

void SocketAdapterMockManager::setupNetworkErrorScenario() {
    // Configure for network error scenario
    
    EXPECT_CALL(*socket_adapter_mock_, send_data(_, _, _))
        .WillOnce(Return(-1)); // Send fails
        
    EXPECT_CALL(*socket_adapter_mock_, receive_data(_, _, _))
        .WillOnce(Return(-1)); // Receive fails
        
    EXPECT_CALL(*socket_adapter_mock_, get_last_error(_, NotNull(), _))
        .WillRepeatedly(DoAll(
            testing::Invoke([](extsock_socket_adapter_t*, char* error_buffer, size_t) {
                strcpy(error_buffer, "Network unreachable");
            }),
            Return(true)
        ));
    
    // System calls fail with errno
    EXPECT_CALL(*system_socket_mock_, send(_, _, _, _))
        .WillOnce(Return(-1));
        
    EXPECT_CALL(*system_socket_mock_, recv(_, _, _, _))
        .WillOnce(Return(-1));
}

void SocketAdapterMockManager::setupTimeoutScenario() {
    // Configure for timeout scenario
    uint32_t timeout_ms = 5000;
    
    EXPECT_CALL(*socket_adapter_mock_, set_timeout(_, timeout_ms))
        .WillOnce(Return(true));
    
    // Simulate timeout on receive
    EXPECT_CALL(*socket_adapter_mock_, receive_data(_, _, _))
        .WillOnce(Return(0)); // Timeout (no data)
    
    // Will trigger timeout event
    EXPECT_CALL(*socket_adapter_mock_, publish_event(_, NotNull()))
        .WillOnce(Return(true));
}

void SocketAdapterMockManager::setupEventPublishingScenario() {
    // Configure for event publishing and listening
    
    event_callback_t dummy_callback = [](extsock_event_t*, void*) {};
    void* dummy_user_data = reinterpret_cast<void*>(0x5000);
    
    EXPECT_CALL(*socket_adapter_mock_, register_event_listener(_, dummy_callback, dummy_user_data))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*socket_adapter_mock_, publish_event(_, NotNull()))
        .WillRepeatedly(Return(true));
        
    EXPECT_CALL(*socket_adapter_mock_, unregister_event_listener(_, dummy_callback))
        .WillOnce(Return(true));
}

void SocketAdapterMockManager::setupAsyncListeningScenario() {
    // Configure for asynchronous listening
    
    message_handler_t dummy_handler = [](const char*, size_t, void*) { return true; };
    void* dummy_user_data = reinterpret_cast<void*>(0x5001);
    
    EXPECT_CALL(*socket_adapter_mock_, start_listening(_, dummy_handler, dummy_user_data))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*socket_adapter_mock_, stop_listening(_))
        .WillOnce(Return(true));
}

void SocketAdapterMockManager::simulateConnectionEstablished() {
    auto event = createMockEvent(SocketEventType::CONNECTION_ESTABLISHED);
    simulated_events_.push_back(event);
}

void SocketAdapterMockManager::simulateConnectionLost() {
    auto event = createMockEvent(SocketEventType::CONNECTION_LOST);
    simulated_events_.push_back(event);
}

void SocketAdapterMockManager::simulateDataReceived(const char* data, size_t length) {
    (void)data; (void)length; // Suppress unused parameter warnings
    auto event = createMockEvent(SocketEventType::DATA_RECEIVED);
    // In a real implementation, would store the data with the event
    simulated_events_.push_back(event);
}

void SocketAdapterMockManager::simulateNetworkError(const char* error_message) {
    (void)error_message; // Suppress unused parameter warning
    auto event = createMockEvent(SocketEventType::ERROR_OCCURRED);
    // In a real implementation, would store the error message with the event
    simulated_events_.push_back(event);
}

void SocketAdapterMockManager::simulateTimeout() {
    auto event = createMockEvent(SocketEventType::TIMEOUT);
    simulated_events_.push_back(event);
}

extsock_event_t* SocketAdapterMockManager::createMockEvent(SocketEventType type) {
    return createEventInternal(type);
}

extsock_event_t* SocketAdapterMockManager::createEventInternal(SocketEventType type, 
                                                              const char* message) {
    (void)message; // Suppress unused parameter warning
    // Simplified mock event creation
    // In real implementation, would allocate and initialize proper event structure
    return reinterpret_cast<extsock_event_t*>(0x6000 + static_cast<int>(type));
}

void SocketAdapterMockManager::resetAllMocks() {
    ::testing::Mock::VerifyAndClearExpectations(socket_adapter_mock_.get());
    ::testing::Mock::VerifyAndClearExpectations(system_socket_mock_.get());
    simulated_events_.clear();
}

} // namespace mocks
} // namespace extsock_test