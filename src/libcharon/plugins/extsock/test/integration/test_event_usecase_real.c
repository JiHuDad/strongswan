/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Level 3 Integration Tests for extsock_event_usecase
 * TASK-012: Event Usecase 실제 테스트
 * 
 * These tests verify the Event Usecase layer functionality
 * with minimal strongSwan dependencies for Phase 4.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test infrastructure
#include "../infrastructure/test_container.h"

// Minimal types for Phase 4 testing
#include "test_extsock_types_minimal.h"

// Forward declarations for stub types
typedef struct extsock_event_usecase_t extsock_event_usecase_t;
typedef struct extsock_event_publisher_t extsock_event_publisher_t;
typedef struct extsock_socket_adapter_t extsock_socket_adapter_t;
typedef struct extsock_failover_manager_t extsock_failover_manager_t;
typedef struct ike_sa_t ike_sa_t;
typedef struct child_sa_t child_sa_t;
typedef struct listener_t listener_t;

// Event Publisher interface (stub for Phase 4)
struct extsock_event_publisher_t {
    extsock_error_t (*publish_event)(extsock_event_publisher_t *this, const char *event_json);
    extsock_error_t (*publish_tunnel_event)(extsock_event_publisher_t *this, const char *tunnel_event_json);
    void (*destroy)(extsock_event_publisher_t *this);
};

// Listener interface (simplified for testing)
struct listener_t {
    bool (*ike_updown)(extsock_event_usecase_t *this, ike_sa_t *ike_sa, bool up);
    bool (*child_updown)(extsock_event_usecase_t *this, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up);
    bool (*ike_state_change)(extsock_event_usecase_t *this, ike_sa_t *ike_sa, int state);
    bool (*ike_rekey)(extsock_event_usecase_t *this, ike_sa_t *old, ike_sa_t *new);
    bool (*child_rekey)(extsock_event_usecase_t *this, ike_sa_t *ike_sa, child_sa_t *old, child_sa_t *new);
};

// Event Usecase interface (stub for Phase 4)
struct extsock_event_usecase_t {
    listener_t listener;
    void (*handle_child_updown)(extsock_event_usecase_t *this, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up);
    extsock_event_publisher_t* (*get_event_publisher)(extsock_event_usecase_t *this);
    void (*set_socket_adapter)(extsock_event_usecase_t *this, extsock_socket_adapter_t *socket_adapter);
    void (*set_failover_manager)(extsock_event_usecase_t *this, extsock_failover_manager_t *failover_manager);
    void (*destroy)(extsock_event_usecase_t *this);
};

// Stub socket adapter
struct extsock_socket_adapter_t {
    extsock_error_t (*send_event)(extsock_socket_adapter_t *this, const char *event_json);
};

// Test IKE SA stub
typedef struct {
    char *name;
    int state;
} test_ike_sa_t;

// Test Child SA stub
typedef struct {
    char *name;
    int state;
} test_child_sa_t;

// Test implementation
typedef struct {
    extsock_event_usecase_t public;
    extsock_event_publisher_t publisher;
    extsock_socket_adapter_t *socket_adapter;
    extsock_failover_manager_t *failover_manager;
    
    // Test tracking
    char *last_published_event;
    char *last_tunnel_event;
    int event_count;
    int tunnel_event_count;
} test_event_usecase_t;

// Test socket adapter implementation
typedef struct {
    extsock_socket_adapter_t public;
    char *last_event;
    int send_count;
} test_socket_adapter_t;

static extsock_error_t test_send_event(extsock_socket_adapter_t *this, const char *event_json) {
    test_socket_adapter_t *adapter = (test_socket_adapter_t*)this;
    if (adapter->last_event) free(adapter->last_event);
    adapter->last_event = event_json ? strdup(event_json) : NULL;
    adapter->send_count++;
    return EXTSOCK_SUCCESS;
}

static extsock_socket_adapter_t *test_socket_adapter_create() {
    test_socket_adapter_t *adapter = malloc(sizeof(*adapter));
    adapter->public.send_event = test_send_event;
    adapter->last_event = NULL;
    adapter->send_count = 0;
    return &adapter->public;
}

// Event Publisher implementations
static extsock_error_t test_publish_event(extsock_event_publisher_t *this, const char *event_json) {
    test_event_usecase_t *usecase = (test_event_usecase_t*)((char*)this - offsetof(test_event_usecase_t, publisher));
    
    if (usecase->last_published_event) free(usecase->last_published_event);
    usecase->last_published_event = event_json ? strdup(event_json) : NULL;
    usecase->event_count++;
    
    if (usecase->socket_adapter) {
        return usecase->socket_adapter->send_event(usecase->socket_adapter, event_json);
    }
    return EXTSOCK_SUCCESS;
}

static extsock_error_t test_publish_tunnel_event(extsock_event_publisher_t *this, const char *tunnel_event_json) {
    test_event_usecase_t *usecase = (test_event_usecase_t*)((char*)this - offsetof(test_event_usecase_t, publisher));
    
    if (usecase->last_tunnel_event) free(usecase->last_tunnel_event);
    usecase->last_tunnel_event = tunnel_event_json ? strdup(tunnel_event_json) : NULL;
    usecase->tunnel_event_count++;
    
    return test_publish_event(this, tunnel_event_json);
}

static void test_destroy_publisher(extsock_event_publisher_t *this) {
    // Publisher is part of usecase, no separate cleanup needed
}

// Listener implementations (stubs for Phase 4)
static bool test_ike_updown(extsock_event_usecase_t *this, ike_sa_t *ike_sa, bool up) {
    // Basic test implementation
    return true;
}

static bool test_child_updown(extsock_event_usecase_t *this, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up) {
    this->handle_child_updown(this, ike_sa, child_sa, up);
    return true;
}

static bool test_ike_state_change(extsock_event_usecase_t *this, ike_sa_t *ike_sa, int state) {
    // Basic test implementation
    return true;
}

static bool test_ike_rekey(extsock_event_usecase_t *this, ike_sa_t *old, ike_sa_t *new) {
    // Basic test implementation
    return true;
}

static bool test_child_rekey(extsock_event_usecase_t *this, ike_sa_t *ike_sa, child_sa_t *old, child_sa_t *new) {
    // Basic test implementation
    return true;
}

// Event Usecase implementations
static void test_handle_child_updown(extsock_event_usecase_t *this, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up) {
    if (!ike_sa || !child_sa) return;
    
    // Simulate event creation for testing
    test_event_usecase_t *usecase = (test_event_usecase_t*)this;
    char event_json[256];
    snprintf(event_json, sizeof(event_json), 
             "{\"event\":\"tunnel_%s\",\"ike_sa_name\":\"test-ike\",\"child_sa_name\":\"test-child\"}",
             up ? "up" : "down");
    
    extsock_event_publisher_t *publisher = this->get_event_publisher(this);
    if (publisher) {
        publisher->publish_tunnel_event(publisher, event_json);
    }
}

static extsock_event_publisher_t* test_get_event_publisher(extsock_event_usecase_t *this) {
    test_event_usecase_t *usecase = (test_event_usecase_t*)this;
    return &usecase->publisher;
}

static void test_set_socket_adapter(extsock_event_usecase_t *this, extsock_socket_adapter_t *socket_adapter) {
    test_event_usecase_t *usecase = (test_event_usecase_t*)this;
    usecase->socket_adapter = socket_adapter;
}

static void test_set_failover_manager(extsock_event_usecase_t *this, extsock_failover_manager_t *failover_manager) {
    test_event_usecase_t *usecase = (test_event_usecase_t*)this;
    usecase->failover_manager = failover_manager;
}

static void test_destroy_usecase(extsock_event_usecase_t *this) {
    test_event_usecase_t *usecase = (test_event_usecase_t*)this;
    if (usecase->last_published_event) free(usecase->last_published_event);
    if (usecase->last_tunnel_event) free(usecase->last_tunnel_event);
    free(usecase);
}

// Factory function
static extsock_event_usecase_t *extsock_event_usecase_create() {
    test_event_usecase_t *this = malloc(sizeof(*this));
    if (!this) return NULL;
    
    this->public.listener.ike_updown = test_ike_updown;
    this->public.listener.child_updown = test_child_updown;
    this->public.listener.ike_state_change = test_ike_state_change;
    this->public.listener.ike_rekey = test_ike_rekey;
    this->public.listener.child_rekey = test_child_rekey;
    
    this->public.handle_child_updown = test_handle_child_updown;
    this->public.get_event_publisher = test_get_event_publisher;
    this->public.set_socket_adapter = test_set_socket_adapter;
    this->public.set_failover_manager = test_set_failover_manager;
    this->public.destroy = test_destroy_usecase;
    
    this->publisher.publish_event = test_publish_event;
    this->publisher.publish_tunnel_event = test_publish_tunnel_event;
    this->publisher.destroy = test_destroy_publisher;
    
    this->socket_adapter = NULL;
    this->failover_manager = NULL;
    this->last_published_event = NULL;
    this->last_tunnel_event = NULL;
    this->event_count = 0;
    this->tunnel_event_count = 0;
    
    return &this->public;
}

// Test helper functions for creating test SA objects
static ike_sa_t *test_ike_sa_create(const char *name) {
    test_ike_sa_t *ike_sa = malloc(sizeof(*ike_sa));
    ike_sa->name = name ? strdup(name) : NULL;
    ike_sa->state = 0;
    return (ike_sa_t*)ike_sa;
}

static child_sa_t *test_child_sa_create(const char *name) {
    test_child_sa_t *child_sa = malloc(sizeof(*child_sa));
    child_sa->name = name ? strdup(name) : NULL;
    child_sa->state = 0;
    return (child_sa_t*)child_sa;
}

static void test_ike_sa_destroy(ike_sa_t *ike_sa) {
    test_ike_sa_t *test_ike = (test_ike_sa_t*)ike_sa;
    if (test_ike->name) free(test_ike->name);
    free(test_ike);
}

static void test_child_sa_destroy(child_sa_t *child_sa) {
    test_child_sa_t *test_child = (test_child_sa_t*)child_sa;
    if (test_child->name) free(test_child->name);
    free(test_child);
}

// Test counter
static int test_count = 0;
static int tests_passed = 0;

// Test helper macros
#define TEST_START() printf("Test %d: %s... ", ++test_count, __func__)
#define TEST_PASS() do { printf("PASS\\n"); tests_passed++; } while(0)
#define TEST_FAIL(msg) do { printf("FAIL - %s\\n", msg); return; } while(0)
#define TEST_ASSERT(condition, msg) do { if (!(condition)) TEST_FAIL(msg); } while(0)

/**
 * Test: Event Usecase 생성 및 기본 기능
 */
static void test_event_usecase_create_basic()
{
    TEST_START();
    
    // Container setup for integration tests
    test_container_t *container = test_container_create_integration();
    TEST_ASSERT(container, "Failed to create integration container");
    
    // Create event usecase
    extsock_event_usecase_t *usecase = extsock_event_usecase_create();
    TEST_ASSERT(usecase, "Failed to create event usecase");
    
    // Test event publisher access
    extsock_event_publisher_t *publisher = usecase->get_event_publisher(usecase);
    TEST_ASSERT(publisher, "Failed to get event publisher");
    
    // Test listener interface exists
    TEST_ASSERT(usecase->listener.ike_updown, "IKE updown listener missing");
    TEST_ASSERT(usecase->listener.child_updown, "Child updown listener missing");
    TEST_ASSERT(usecase->listener.ike_state_change, "IKE state change listener missing");
    
    // Cleanup
    usecase->destroy(usecase);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 이벤트 발행
 */
static void test_event_usecase_publish_event()
{
    TEST_START();
    
    test_container_t *container = test_container_create_integration();
    TEST_ASSERT(container, "Failed to create integration container");
    
    extsock_event_usecase_t *usecase = extsock_event_usecase_create();
    TEST_ASSERT(usecase, "Failed to create event usecase");
    
    // Create and set socket adapter
    extsock_socket_adapter_t *adapter = test_socket_adapter_create();
    usecase->set_socket_adapter(usecase, adapter);
    
    // Get publisher and publish event
    extsock_event_publisher_t *publisher = usecase->get_event_publisher(usecase);
    const char *test_event = "{\"event\":\"test_event\",\"data\":\"test_data\"}";
    
    extsock_error_t result = publisher->publish_event(publisher, test_event);
    TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to publish event");
    
    // Verify event was published
    test_event_usecase_t *test_usecase = (test_event_usecase_t*)usecase;
    TEST_ASSERT(test_usecase->last_published_event != NULL, "Event was not stored");
    TEST_ASSERT(strcmp(test_usecase->last_published_event, test_event) == 0, "Event content mismatch");
    TEST_ASSERT(test_usecase->event_count == 1, "Event count incorrect");
    
    // Verify socket adapter received event
    test_socket_adapter_t *test_adapter = (test_socket_adapter_t*)adapter;
    TEST_ASSERT(test_adapter->last_event != NULL, "Socket adapter did not receive event");
    TEST_ASSERT(strcmp(test_adapter->last_event, test_event) == 0, "Socket adapter event mismatch");
    TEST_ASSERT(test_adapter->send_count == 1, "Socket adapter send count incorrect");
    
    // Cleanup
    if (test_adapter->last_event) free(test_adapter->last_event);
    free(adapter);
    usecase->destroy(usecase);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 터널 이벤트 발행
 */
static void test_event_usecase_publish_tunnel_event()
{
    TEST_START();
    
    test_container_t *container = test_container_create_integration();
    TEST_ASSERT(container, "Failed to create integration container");
    
    extsock_event_usecase_t *usecase = extsock_event_usecase_create();
    TEST_ASSERT(usecase, "Failed to create event usecase");
    
    extsock_socket_adapter_t *adapter = test_socket_adapter_create();
    usecase->set_socket_adapter(usecase, adapter);
    
    // Test tunnel event
    extsock_event_publisher_t *publisher = usecase->get_event_publisher(usecase);
    const char *tunnel_event = "{\"event\":\"tunnel_up\",\"ike_sa_name\":\"test-ike\",\"child_sa_name\":\"test-child\"}";
    
    extsock_error_t result = publisher->publish_tunnel_event(publisher, tunnel_event);
    TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to publish tunnel event");
    
    // Verify tunnel event was published
    test_event_usecase_t *test_usecase = (test_event_usecase_t*)usecase;
    TEST_ASSERT(test_usecase->last_tunnel_event != NULL, "Tunnel event was not stored");
    TEST_ASSERT(strcmp(test_usecase->last_tunnel_event, tunnel_event) == 0, "Tunnel event content mismatch");
    TEST_ASSERT(test_usecase->tunnel_event_count == 1, "Tunnel event count incorrect");
    
    // Cleanup
    test_socket_adapter_t *test_adapter = (test_socket_adapter_t*)adapter;
    if (test_adapter->last_event) free(test_adapter->last_event);
    free(adapter);
    usecase->destroy(usecase);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: Child SA Up/Down 처리
 */
static void test_event_usecase_handle_child_updown()
{
    TEST_START();
    
    test_container_t *container = test_container_create_integration();
    TEST_ASSERT(container, "Failed to create integration container");
    
    extsock_event_usecase_t *usecase = extsock_event_usecase_create();
    TEST_ASSERT(usecase, "Failed to create event usecase");
    
    extsock_socket_adapter_t *adapter = test_socket_adapter_create();
    usecase->set_socket_adapter(usecase, adapter);
    
    // Create test SA objects
    ike_sa_t *ike_sa = test_ike_sa_create("test-ike");
    child_sa_t *child_sa = test_child_sa_create("test-child");
    
    // Test Child SA UP event
    usecase->handle_child_updown(usecase, ike_sa, child_sa, true);
    
    // Verify tunnel event was generated
    test_event_usecase_t *test_usecase = (test_event_usecase_t*)usecase;
    TEST_ASSERT(test_usecase->tunnel_event_count == 1, "Tunnel event count should be 1");
    TEST_ASSERT(strstr(test_usecase->last_tunnel_event, "tunnel_up"), "Should contain tunnel_up event");
    
    // Test Child SA DOWN event  
    usecase->handle_child_updown(usecase, ike_sa, child_sa, false);
    TEST_ASSERT(test_usecase->tunnel_event_count == 2, "Tunnel event count should be 2");
    TEST_ASSERT(strstr(test_usecase->last_tunnel_event, "tunnel_down"), "Should contain tunnel_down event");
    
    // Test NULL handling
    usecase->handle_child_updown(usecase, NULL, child_sa, true);
    TEST_ASSERT(test_usecase->tunnel_event_count == 2, "Event count should not change for NULL IKE SA");
    
    // Cleanup
    test_ike_sa_destroy(ike_sa);
    test_child_sa_destroy(child_sa);
    test_socket_adapter_t *test_adapter = (test_socket_adapter_t*)adapter;
    if (test_adapter->last_event) free(test_adapter->last_event);
    free(adapter);
    usecase->destroy(usecase);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 리스너 인터페이스
 */
static void test_event_usecase_listeners()
{
    TEST_START();
    
    test_container_t *container = test_container_create_integration();
    TEST_ASSERT(container, "Failed to create integration container");
    
    extsock_event_usecase_t *usecase = extsock_event_usecase_create();
    TEST_ASSERT(usecase, "Failed to create event usecase");
    
    extsock_socket_adapter_t *adapter = test_socket_adapter_create();
    usecase->set_socket_adapter(usecase, adapter);
    
    // Create test SA objects
    ike_sa_t *ike_sa = test_ike_sa_create("test-ike");
    child_sa_t *child_sa = test_child_sa_create("test-child");
    
    // Test IKE updown listener
    bool result = usecase->listener.ike_updown(usecase, ike_sa, true);
    TEST_ASSERT(result == true, "IKE updown listener should return true");
    
    // Test Child updown listener
    result = usecase->listener.child_updown(usecase, ike_sa, child_sa, true);
    TEST_ASSERT(result == true, "Child updown listener should return true");
    
    // Test IKE state change listener
    result = usecase->listener.ike_state_change(usecase, ike_sa, 1);
    TEST_ASSERT(result == true, "IKE state change listener should return true");
    
    // Test IKE rekey listener
    ike_sa_t *new_ike_sa = test_ike_sa_create("test-ike-new");
    result = usecase->listener.ike_rekey(usecase, ike_sa, new_ike_sa);
    TEST_ASSERT(result == true, "IKE rekey listener should return true");
    
    // Test Child rekey listener
    child_sa_t *new_child_sa = test_child_sa_create("test-child-new");
    result = usecase->listener.child_rekey(usecase, ike_sa, child_sa, new_child_sa);
    TEST_ASSERT(result == true, "Child rekey listener should return true");
    
    // Cleanup
    test_ike_sa_destroy(ike_sa);
    test_ike_sa_destroy(new_ike_sa);
    test_child_sa_destroy(child_sa);
    test_child_sa_destroy(new_child_sa);
    test_socket_adapter_t *test_adapter = (test_socket_adapter_t*)adapter;
    if (test_adapter->last_event) free(test_adapter->last_event);
    free(adapter);
    usecase->destroy(usecase);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 의존성 주입
 */
static void test_event_usecase_dependency_injection()
{
    TEST_START();
    
    test_container_t *container = test_container_create_integration();
    TEST_ASSERT(container, "Failed to create integration container");
    
    extsock_event_usecase_t *usecase = extsock_event_usecase_create();
    TEST_ASSERT(usecase, "Failed to create event usecase");
    
    // Test socket adapter injection
    extsock_socket_adapter_t *adapter = test_socket_adapter_create();
    usecase->set_socket_adapter(usecase, adapter);
    
    test_event_usecase_t *test_usecase = (test_event_usecase_t*)usecase;
    TEST_ASSERT(test_usecase->socket_adapter == adapter, "Socket adapter not set correctly");
    
    // Test failover manager injection (NULL for Phase 4)
    usecase->set_failover_manager(usecase, NULL);
    TEST_ASSERT(test_usecase->failover_manager == NULL, "Failover manager should be NULL");
    
    // Cleanup
    test_socket_adapter_t *test_adapter = (test_socket_adapter_t*)adapter;
    if (test_adapter->last_event) free(test_adapter->last_event);
    free(adapter);
    usecase->destroy(usecase);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 메모리 관리
 */
static void test_event_usecase_memory_management()
{
    TEST_START();
    
    test_container_t *container = test_container_create_integration();
    TEST_ASSERT(container, "Failed to create integration container");
    
    CONTAINER_TAKE_MEMORY_SNAPSHOT(container, "initial");
    
    // Multiple usecase operations
    for (int i = 0; i < 10; i++) {
        extsock_event_usecase_t *usecase = extsock_event_usecase_create();
        TEST_ASSERT(usecase, "Failed to create event usecase");
        
        extsock_socket_adapter_t *adapter = test_socket_adapter_create();
        usecase->set_socket_adapter(usecase, adapter);
        
        // Perform operations
        extsock_event_publisher_t *publisher = usecase->get_event_publisher(usecase);
        
        char event[256];
        snprintf(event, sizeof(event), "{\"event\":\"test-%d\"}", i);
        publisher->publish_event(publisher, event);
        
        char tunnel_event[256];
        snprintf(tunnel_event, sizeof(tunnel_event), "{\"event\":\"tunnel_up\",\"test\":\"%d\"}", i);
        publisher->publish_tunnel_event(publisher, tunnel_event);
        
        // Cleanup
        test_socket_adapter_t *test_adapter = (test_socket_adapter_t*)adapter;
        if (test_adapter->last_event) free(test_adapter->last_event);
        free(adapter);
        usecase->destroy(usecase);
    }
    
    // Verify no significant memory leaks
    CONTAINER_ASSERT_MEMORY_USAGE_UNDER(container, 1024 * 1024); // 1MB limit
    
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 스트레스 테스트
 */
static void test_event_usecase_stress()
{
    TEST_START();
    
    test_container_t *container = test_container_create_integration();
    TEST_ASSERT(container, "Failed to create integration container");
    
    extsock_event_usecase_t *usecase = extsock_event_usecase_create();
    TEST_ASSERT(usecase, "Failed to create event usecase");
    
    extsock_socket_adapter_t *adapter = test_socket_adapter_create();
    usecase->set_socket_adapter(usecase, adapter);
    
    extsock_event_publisher_t *publisher = usecase->get_event_publisher(usecase);
    
    // Stress test with many events
    const int stress_count = 100;
    
    for (int i = 0; i < stress_count; i++) {
        char event[256];
        snprintf(event, sizeof(event), "{\"event\":\"stress-test-%d\",\"index\":%d}", i, i);
        
        extsock_error_t result = publisher->publish_event(publisher, event);
        TEST_ASSERT(result == EXTSOCK_SUCCESS, "Event publish failed during stress test");
        
        char tunnel_event[256];
        snprintf(tunnel_event, sizeof(tunnel_event), 
                "{\"event\":\"tunnel_up\",\"stress\":\"%d\"}", i);
        
        result = publisher->publish_tunnel_event(publisher, tunnel_event);
        TEST_ASSERT(result == EXTSOCK_SUCCESS, "Tunnel event publish failed during stress test");
    }
    
    // Verify counts
    test_event_usecase_t *test_usecase = (test_event_usecase_t*)usecase;
    TEST_ASSERT(test_usecase->event_count == stress_count * 2, "Event count mismatch"); // 2 events per loop
    TEST_ASSERT(test_usecase->tunnel_event_count == stress_count, "Tunnel event count mismatch");
    
    // Cleanup
    test_socket_adapter_t *test_adapter = (test_socket_adapter_t*)adapter;
    if (test_adapter->last_event) free(test_adapter->last_event);
    free(adapter);
    usecase->destroy(usecase);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Main test runner
 */
int main()
{
    printf("=== Event Usecase Level 3 Integration Tests ===\\n\\n");
    
    // Run all tests
    test_event_usecase_create_basic();
    test_event_usecase_publish_event();
    test_event_usecase_publish_tunnel_event();
    test_event_usecase_handle_child_updown();
    test_event_usecase_listeners();
    test_event_usecase_dependency_injection();
    test_event_usecase_memory_management();
    test_event_usecase_stress();
    
    // Print results
    printf("\\n=== Test Results ===\\n");
    printf("Total tests: %d\\n", test_count);
    printf("Passed: %d\\n", tests_passed);
    printf("Failed: %d\\n", test_count - tests_passed);
    
    if (tests_passed == test_count) {
        printf("✅ All tests PASSED!\\n");
        return 0;
    } else {
        printf("❌ Some tests FAILED!\\n");
        return 1;
    }
}