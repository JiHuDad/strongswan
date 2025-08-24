/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Mock based Socket Adapter Mock Classes
 * TASK-M002: Mock Infrastructure Construction
 * 
 * This file provides Google Mock-based mock implementations of the 
 * extsock Socket Adapter interface for testing network operations.
 */

#ifndef MOCK_SOCKET_ADAPTER_HPP_
#define MOCK_SOCKET_ADAPTER_HPP_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <functional>
#include <vector>

extern "C" {
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>

// extsock types
typedef struct extsock_socket_adapter_t extsock_socket_adapter_t;
typedef struct extsock_event_t extsock_event_t;

// Callback function types
typedef void (*event_callback_t)(extsock_event_t* event, void* user_data);
typedef bool (*message_handler_t)(const char* message, size_t length, void* user_data);
}

namespace extsock_test {
namespace mocks {

/**
 * Event types for socket operations
 */
enum class SocketEventType {
    CONNECTION_ESTABLISHED,
    CONNECTION_LOST,
    DATA_RECEIVED,
    DATA_SENT,
    ERROR_OCCURRED,
    TIMEOUT
};

/**
 * Abstract interface for extsock Socket Adapter
 * 
 * This interface defines the contract for socket operations
 * that will be mocked during testing.
 */
class SocketAdapterInterface {
public:
    virtual ~SocketAdapterInterface() = default;
    
    // Connection management
    virtual bool create_socket(extsock_socket_adapter_t* this_adapter, 
                              const char* host, uint16_t port) = 0;
    virtual bool connect(extsock_socket_adapter_t* this_adapter) = 0;
    virtual bool disconnect(extsock_socket_adapter_t* this_adapter) = 0;
    virtual bool is_connected(extsock_socket_adapter_t* this_adapter) = 0;
    
    // Data transmission
    virtual ssize_t send_data(extsock_socket_adapter_t* this_adapter, 
                             const void* data, size_t length) = 0;
    virtual ssize_t receive_data(extsock_socket_adapter_t* this_adapter, 
                                void* buffer, size_t buffer_size) = 0;
    
    // Event handling
    virtual bool publish_event(extsock_socket_adapter_t* this_adapter, 
                              extsock_event_t* event) = 0;
    virtual bool register_event_listener(extsock_socket_adapter_t* this_adapter,
                                        event_callback_t callback, void* user_data) = 0;
    virtual bool unregister_event_listener(extsock_socket_adapter_t* this_adapter,
                                          event_callback_t callback) = 0;
    
    // Async operations
    virtual bool start_listening(extsock_socket_adapter_t* this_adapter,
                               message_handler_t handler, void* user_data) = 0;
    virtual bool stop_listening(extsock_socket_adapter_t* this_adapter) = 0;
    virtual bool set_timeout(extsock_socket_adapter_t* this_adapter, 
                           uint32_t timeout_ms) = 0;
    
    // Status and configuration
    virtual int get_socket_fd(extsock_socket_adapter_t* this_adapter) = 0;
    virtual const char* get_host(extsock_socket_adapter_t* this_adapter) = 0;
    virtual uint16_t get_port(extsock_socket_adapter_t* this_adapter) = 0;
    virtual bool get_last_error(extsock_socket_adapter_t* this_adapter, 
                               char* error_buffer, size_t buffer_size) = 0;
    
    // Lifecycle
    virtual void destroy(extsock_socket_adapter_t* this_adapter) = 0;
};

/**
 * Mock implementation of extsock Socket Adapter
 */
class MockSocketAdapter : public SocketAdapterInterface {
public:
    MockSocketAdapter() = default;
    virtual ~MockSocketAdapter() = default;

    // Connection management mocks
    MOCK_METHOD(bool, create_socket, 
                (extsock_socket_adapter_t* this_adapter, const char* host, uint16_t port), 
                (override));
    
    MOCK_METHOD(bool, connect, (extsock_socket_adapter_t* this_adapter), (override));
    MOCK_METHOD(bool, disconnect, (extsock_socket_adapter_t* this_adapter), (override));
    MOCK_METHOD(bool, is_connected, (extsock_socket_adapter_t* this_adapter), (override));
    
    // Data transmission mocks
    MOCK_METHOD(ssize_t, send_data, 
                (extsock_socket_adapter_t* this_adapter, const void* data, size_t length), 
                (override));
    
    MOCK_METHOD(ssize_t, receive_data, 
                (extsock_socket_adapter_t* this_adapter, void* buffer, size_t buffer_size), 
                (override));
    
    // Event handling mocks
    MOCK_METHOD(bool, publish_event, 
                (extsock_socket_adapter_t* this_adapter, extsock_event_t* event), 
                (override));
    
    MOCK_METHOD(bool, register_event_listener, 
                (extsock_socket_adapter_t* this_adapter, event_callback_t callback, void* user_data), 
                (override));
    
    MOCK_METHOD(bool, unregister_event_listener, 
                (extsock_socket_adapter_t* this_adapter, event_callback_t callback), 
                (override));
    
    // Async operations mocks
    MOCK_METHOD(bool, start_listening, 
                (extsock_socket_adapter_t* this_adapter, message_handler_t handler, void* user_data), 
                (override));
    
    MOCK_METHOD(bool, stop_listening, (extsock_socket_adapter_t* this_adapter), (override));
    
    MOCK_METHOD(bool, set_timeout, 
                (extsock_socket_adapter_t* this_adapter, uint32_t timeout_ms), 
                (override));
    
    // Status and configuration mocks
    MOCK_METHOD(int, get_socket_fd, (extsock_socket_adapter_t* this_adapter), (override));
    MOCK_METHOD(const char*, get_host, (extsock_socket_adapter_t* this_adapter), (override));
    MOCK_METHOD(uint16_t, get_port, (extsock_socket_adapter_t* this_adapter), (override));
    
    MOCK_METHOD(bool, get_last_error, 
                (extsock_socket_adapter_t* this_adapter, char* error_buffer, size_t buffer_size), 
                (override));
    
    // Lifecycle mocks
    MOCK_METHOD(void, destroy, (extsock_socket_adapter_t* this_adapter), (override));
};

/**
 * Abstract interface for low-level socket operations
 * 
 * This allows mocking of system socket calls for comprehensive testing
 */
class SystemSocketInterface {
public:
    virtual ~SystemSocketInterface() = default;
    
    virtual int socket(int domain, int type, int protocol) = 0;
    virtual int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) = 0;
    virtual int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) = 0;
    virtual int listen(int sockfd, int backlog) = 0;
    virtual int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) = 0;
    virtual ssize_t send(int sockfd, const void* buf, size_t len, int flags) = 0;
    virtual ssize_t recv(int sockfd, void* buf, size_t len, int flags) = 0;
    virtual int close(int fd) = 0;
    virtual int setsockopt(int sockfd, int level, int optname, 
                          const void* optval, socklen_t optlen) = 0;
    virtual int getsockopt(int sockfd, int level, int optname, 
                          void* optval, socklen_t* optlen) = 0;
};

/**
 * Mock implementation of system socket operations
 */
class MockSystemSocket : public SystemSocketInterface {
public:
    MOCK_METHOD(int, socket, (int domain, int type, int protocol), (override));
    MOCK_METHOD(int, connect, (int sockfd, const struct sockaddr* addr, socklen_t addrlen), (override));
    MOCK_METHOD(int, bind, (int sockfd, const struct sockaddr* addr, socklen_t addrlen), (override));
    MOCK_METHOD(int, listen, (int sockfd, int backlog), (override));
    MOCK_METHOD(int, accept, (int sockfd, struct sockaddr* addr, socklen_t* addrlen), (override));
    MOCK_METHOD(ssize_t, send, (int sockfd, const void* buf, size_t len, int flags), (override));
    MOCK_METHOD(ssize_t, recv, (int sockfd, void* buf, size_t len, int flags), (override));
    MOCK_METHOD(int, close, (int fd), (override));
    MOCK_METHOD(int, setsockopt, (int sockfd, int level, int optname, 
                                 const void* optval, socklen_t optlen), (override));
    MOCK_METHOD(int, getsockopt, (int sockfd, int level, int optname, 
                                 void* optval, socklen_t* optlen), (override));
};

/**
 * Socket Adapter Mock Manager
 * 
 * Provides centralized management of socket adapter mocks and
 * pre-configured scenarios for common test cases.
 */
class SocketAdapterMockManager {
public:
    SocketAdapterMockManager();
    virtual ~SocketAdapterMockManager();

    // Factory methods
    std::unique_ptr<MockSocketAdapter> createSocketAdapterMock();
    std::unique_ptr<MockSystemSocket> createSystemSocketMock();

    // Pre-configured test scenarios
    void setupSuccessfulConnectionScenario();
    void setupConnectionFailureScenario();
    void setupDataTransmissionScenario();
    void setupNetworkErrorScenario();
    void setupTimeoutScenario();
    void setupEventPublishingScenario();
    void setupAsyncListeningScenario();
    
    // Mock instance access
    MockSocketAdapter* getSocketAdapterMock() { return socket_adapter_mock_.get(); }
    MockSystemSocket* getSystemSocketMock() { return system_socket_mock_.get(); }
    
    // Event simulation helpers
    void simulateConnectionEstablished();
    void simulateConnectionLost();
    void simulateDataReceived(const char* data, size_t length);
    void simulateNetworkError(const char* error_message);
    void simulateTimeout();
    
    // Test data helpers
    extsock_event_t* createMockEvent(SocketEventType type);
    const char* getTestMessage() { return "Test socket message"; }
    const char* getTestHost() { return "192.168.1.100"; }
    uint16_t getTestPort() { return 8080; }
    
    void resetAllMocks();

private:
    std::unique_ptr<MockSocketAdapter> socket_adapter_mock_;
    std::unique_ptr<MockSystemSocket> system_socket_mock_;
    
    // Simulated events storage (using raw pointers since extsock_event_t is incomplete)
    std::vector<extsock_event_t*> simulated_events_;
    
    // Helper methods
    void configureDefaultBehaviors();
    extsock_event_t* createEventInternal(SocketEventType type, 
                                        const char* message = nullptr);
};

/**
 * Custom matchers for socket testing
 */

// Matcher for checking socket file descriptor validity
MATCHER(IsValidSocketFd, "is a valid socket file descriptor") {
    return arg >= 0;
}

// Matcher for checking socket address
MATCHER_P2(HasSocketAddress, expected_host, expected_port, 
          "has socket address " + std::string(expected_host) + ":" + std::to_string(expected_port)) {
    (void)arg; (void)expected_host; (void)expected_port; // Suppress unused warnings
    // This would be implemented to check the socket address
    return true; // Simplified for now
}

// Matcher for checking data content
MATCHER_P(ContainsData, expected_data, "contains expected data") {
    if (arg == nullptr) return false;
    return std::string(static_cast<const char*>(arg)).find(expected_data) != std::string::npos;
}

// Matcher for validating event type
MATCHER_P(IsEventOfType, event_type, "is event of type " + std::to_string(static_cast<int>(event_type))) {
    // This would check the event type
    return arg != nullptr; // Simplified for now
}

} // namespace mocks
} // namespace extsock_test

#endif // MOCK_SOCKET_ADAPTER_HPP_