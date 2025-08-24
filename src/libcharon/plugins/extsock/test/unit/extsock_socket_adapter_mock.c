/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Mock implementation of extsock_socket_adapter for adapter unit tests
 * TASK-008: Socket Adapter 실제 테스트
 * 
 * This mock implementation focuses on testing the adapter layer behavior
 * without actual socket operations.
 */

#include "extsock_socket_adapter_mock.h"
#include "../infrastructure/strongswan_mocks.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Mock socket constants
#define MOCK_SERVER_SOCKET_FD 100
#define MOCK_CLIENT_SOCKET_FD 101
#define MOCK_THREAD_ID 1001

// METHOD macro for function definitions
#define METHOD(interface, name, ret, ...) \
    static ret name(__VA_ARGS__)

typedef struct private_extsock_socket_adapter_mock_t private_extsock_socket_adapter_mock_t;

/**
 * Mock Socket Adapter private structure
 */
struct private_extsock_socket_adapter_mock_t {
    /**
     * Public interface
     */
    extsock_socket_adapter_t public;
    
    /**
     * Mock Config Usecase reference
     */
    extsock_config_usecase_t *cfg_usecase;
    
    /**
     * Mock Command Handler
     */
    extsock_command_handler_t *command_handler;
    
    /**
     * Mock socket file descriptors
     */
    int server_socket_fd;
    int client_socket_fd;
    
    /**
     * Mock thread reference
     */
    thread_t *listener_thread;
    
    /**
     * Mock state tracking
     */
    bool is_listening;
    int send_event_calls;
    char *last_event_json;
};

/*
 * ============================================================================
 * Mock Helper Functions
 * ============================================================================
 */

/**
 * Mock thread creation
 */
thread_t *mock_thread_create(const char *name)
{
    thread_t *thread = malloc(sizeof(thread_t));
    if (!thread) return NULL;
    
    thread->thread_id = MOCK_THREAD_ID;
    thread->is_running = true;
    thread->name = name ? strdup(name) : strdup("mock-thread");
    
    return thread;
}

void mock_thread_destroy(thread_t *thread)
{
    if (thread) {
        free(thread->name);
        free(thread);
    }
}

/**
 * Mock event publisher implementation
 */
static void mock_event_publisher_publish(extsock_event_publisher_t *this, const char *event)
{
    // Mock implementation - just track the call
    if (g_mock_state) {
        g_mock_state->auth_cfg_create_count++; // Reuse counter for tracking
    }
}

static void mock_event_publisher_destroy(extsock_event_publisher_t *this)
{
    // Mock cleanup
    if (this) {
        free(this);
    }
}

extsock_event_publisher_t *mock_event_publisher_create(void)
{
    extsock_event_publisher_t *publisher = malloc(sizeof(extsock_event_publisher_t));
    if (publisher) {
        publisher->publish_event = mock_event_publisher_publish;
        publisher->destroy = mock_event_publisher_destroy;
    }
    return publisher;
}

/**
 * Mock config usecase implementation
 */
extsock_config_usecase_t *mock_config_usecase_create(void)
{
    extsock_config_usecase_t *usecase = malloc(sizeof(extsock_config_usecase_t));
    if (usecase) {
        usecase->config_data = strdup("{\"test\": \"config\"}");
        usecase->is_valid = true;
    }
    return usecase;
}

/**
 * Mock command handler implementation
 */
static void mock_command_handler_handle(extsock_command_handler_t *this, const char *cmd)
{
    // Mock implementation - just track the call
    if (g_mock_state) {
        g_mock_state->child_cfg_create_count++; // Reuse counter for tracking
    }
}

static void mock_command_handler_destroy(extsock_command_handler_t *this)
{
    if (this) {
        free(this);
    }
}

extsock_command_handler_t *mock_command_handler_create(void)
{
    extsock_command_handler_t *handler = malloc(sizeof(extsock_command_handler_t));
    if (handler) {
        handler->handle_command = mock_command_handler_handle;
        handler->destroy = mock_command_handler_destroy;
    }
    return handler;
}

/*
 * ============================================================================
 * Mock Socket Adapter Implementation
 * ============================================================================
 */

METHOD(extsock_socket_adapter_t, send_event, extsock_error_t,
    private_extsock_socket_adapter_mock_t *this, const char *event_json)
{
    // Handle NULL 'this' parameter - this happens when test passes NULL as first param
    if (!this) {
        return EXTSOCK_ERROR_INVALID_PARAM;
    }
    
    if (!event_json) {
        return EXTSOCK_ERROR_INVALID_PARAM;
    }
    
    this->send_event_calls++;
    
    // Store last event for testing
    free(this->last_event_json);
    this->last_event_json = strdup(event_json);
    
    // Simulate successful event send
    return EXTSOCK_SUCCESS;
}

METHOD(extsock_socket_adapter_t, start_listening, thread_t *,
    private_extsock_socket_adapter_mock_t *this)
{
    // Handle NULL 'this' parameter - this happens when test passes NULL as first param
    if (!this) {
        return NULL;
    }
    
    if (this->is_listening) {
        // Already listening
        return this->listener_thread;
    }
    
    // Create mock socket file descriptors
    this->server_socket_fd = MOCK_SERVER_SOCKET_FD;
    
    // Create mock listener thread
    this->listener_thread = mock_thread_create("socket-listener");
    if (!this->listener_thread) {
        return NULL;
    }
    
    this->is_listening = true;
    
    // Track mock system call
    if (g_mock_state) {
        g_mock_state->ike_cfg_create_count++; // Reuse counter for tracking
    }
    
    return this->listener_thread;
}

METHOD(extsock_socket_adapter_t, stop_listening, void,
    private_extsock_socket_adapter_mock_t *this)
{
    if (!this) {
        return;
    }
    
    if (this->is_listening && this->listener_thread) {
        this->listener_thread->is_running = false;
        mock_thread_destroy(this->listener_thread);
        this->listener_thread = NULL;
    }
    
    // Close mock sockets
    this->server_socket_fd = -1;
    this->client_socket_fd = -1;
    this->is_listening = false;
    
    // Track mock system call
    if (g_mock_state) {
        g_mock_state->peer_cfg_create_count++; // Reuse counter for tracking
    }
}

METHOD(extsock_socket_adapter_t, destroy, void,
    private_extsock_socket_adapter_mock_t *this)
{
    if (this) {
        // Stop listening if still active
        if (this->is_listening) {
            stop_listening(this);
        }
        
        // Clean up mock resources
        free(this->last_event_json);
        free(this);
    }
}

/**
 * Mock Socket Adapter creation
 */
extsock_socket_adapter_t *extsock_socket_adapter_create(
    extsock_config_usecase_t *cfg_usecase)
{
    private_extsock_socket_adapter_mock_t *this;
    
    if (!cfg_usecase) {
        return NULL;
    }

    this = malloc(sizeof(*this));
    if (this) {
        memset(this, 0, sizeof(*this));
        
        // Initialize public interface
        this->public.send_event = (void*)send_event;
        this->public.start_listening = (void*)start_listening;
        this->public.stop_listening = (void*)stop_listening;
        this->public.destroy = (void*)destroy;
        
        // Initialize mock state
        this->cfg_usecase = cfg_usecase;
        this->command_handler = mock_command_handler_create();
        this->server_socket_fd = -1;
        this->client_socket_fd = -1;
        this->listener_thread = NULL;
        this->is_listening = false;
        this->send_event_calls = 0;
        this->last_event_json = NULL;
        
        // Create mock event publisher
        this->public.event_publisher = *mock_event_publisher_create();
    }

    return &this->public;
}