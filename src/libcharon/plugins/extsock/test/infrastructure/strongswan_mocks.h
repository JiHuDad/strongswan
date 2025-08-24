/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * strongSwan API Mocking Infrastructure
 * 
 * This file provides mock implementations of strongSwan's complex API
 * to enable isolated unit testing of extsock plugin components.
 */

#ifndef STRONGSWAN_MOCKS_H
#define STRONGSWAN_MOCKS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Mock system initialization and cleanup
 */
void strongswan_mocks_init(void);
void strongswan_mocks_cleanup(void);
void strongswan_mocks_reset_state(void);

/*
 * ============================================================================
 * Mock Types - Simplified versions of strongSwan types
 * ============================================================================
 */

// Forward declarations
typedef struct mock_linked_list mock_linked_list_t;
typedef struct mock_enumerator mock_enumerator_t; 
typedef struct mock_ike_cfg mock_ike_cfg_t;
typedef struct mock_peer_cfg mock_peer_cfg_t;
typedef struct mock_child_cfg mock_child_cfg_t;
typedef struct mock_auth_cfg mock_auth_cfg_t;
typedef struct mock_identification mock_identification_t;
typedef struct mock_traffic_selector mock_traffic_selector_t;
typedef struct mock_proposal mock_proposal_t;

// Generic destroy function pointer
typedef void (*mock_destroy_t)(void *this);

/**
 * Mock linked_list_t implementation
 */
struct mock_linked_list {
    void (*destroy)(mock_linked_list_t *this);
    int (*get_count)(mock_linked_list_t *this);
    void (*insert_last)(mock_linked_list_t *this, void *item);
    void* (*get_first)(mock_linked_list_t *this);
    mock_enumerator_t* (*create_enumerator)(mock_linked_list_t *this);
    
    // Internal state
    void **items;
    int count;
    int capacity;
};

/**
 * Mock enumerator_t implementation
 */
struct mock_enumerator {
    void (*destroy)(mock_enumerator_t *this);
    bool (*enumerate)(mock_enumerator_t *this, void **item);
    
    // Internal state
    mock_linked_list_t *list;
    int current_index;
};

/**
 * Mock ike_cfg_t implementation
 */
struct mock_ike_cfg {
    void (*destroy)(mock_ike_cfg_t *this);
    char* (*get_name)(mock_ike_cfg_t *this);
    void (*add_proposal)(mock_ike_cfg_t *this, mock_proposal_t *proposal);
    
    // Internal state
    char *name;
    mock_linked_list_t *my_hosts;
    mock_linked_list_t *other_hosts;
    mock_linked_list_t *proposals;
    uint16_t my_port;
    uint16_t other_port;
    int ike_version;
};

/**
 * Mock peer_cfg_t implementation
 */
struct mock_peer_cfg {
    void (*destroy)(mock_peer_cfg_t *this);
    char* (*get_name)(mock_peer_cfg_t *this);
    mock_ike_cfg_t* (*get_ike_cfg)(mock_peer_cfg_t *this);
    void (*add_auth_cfg)(mock_peer_cfg_t *this, mock_auth_cfg_t *cfg, bool local);
    void (*add_child_cfg)(mock_peer_cfg_t *this, mock_child_cfg_t *cfg);
    mock_enumerator_t* (*create_child_cfg_enumerator)(mock_peer_cfg_t *this);
    
    // Internal state  
    char *name;
    mock_ike_cfg_t *ike_cfg;
    mock_linked_list_t *local_auth_cfgs;
    mock_linked_list_t *remote_auth_cfgs;
    mock_linked_list_t *child_cfgs;
};

/**
 * Mock child_cfg_t implementation
 */
struct mock_child_cfg {
    void (*destroy)(mock_child_cfg_t *this);
    char* (*get_name)(mock_child_cfg_t *this);
    void (*add_proposal)(mock_child_cfg_t *this, mock_proposal_t *proposal);
    void (*add_traffic_selector)(mock_child_cfg_t *this, bool local, mock_traffic_selector_t *ts);
    mock_enumerator_t* (*create_proposal_enumerator)(mock_child_cfg_t *this);
    
    // Internal state
    char *name;
    mock_linked_list_t *proposals;
    mock_linked_list_t *my_ts;
    mock_linked_list_t *other_ts;
};

/**
 * Mock auth_cfg_t implementation
 */
struct mock_auth_cfg {
    void (*destroy)(mock_auth_cfg_t *this);
    void (*add)(mock_auth_cfg_t *this, int type, void *value);
    void* (*get)(mock_auth_cfg_t *this, int type);
    
    // Internal state
    struct {
        int type;
        void *value;
    } *entries;
    int entry_count;
};

/**
 * Mock identification_t implementation
 */
struct mock_identification {
    void (*destroy)(mock_identification_t *this);
    char* (*get_string)(mock_identification_t *this);
    int (*get_type)(mock_identification_t *this);
    
    // Internal state
    char *id_str;
    int id_type;
};

/**
 * Mock traffic_selector_t implementation
 */
struct mock_traffic_selector {
    void (*destroy)(mock_traffic_selector_t *this);
    char* (*get_from_address)(mock_traffic_selector_t *this);
    char* (*get_to_address)(mock_traffic_selector_t *this);
    uint16_t (*get_from_port)(mock_traffic_selector_t *this);
    uint16_t (*get_to_port)(mock_traffic_selector_t *this);
    
    // Internal state
    char *from_addr;
    char *to_addr;
    uint16_t from_port;
    uint16_t to_port;
    int protocol;
};

/**
 * Mock proposal_t implementation
 */
struct mock_proposal {
    void (*destroy)(mock_proposal_t *this);
    char* (*get_string)(mock_proposal_t *this);
    int (*get_protocol)(mock_proposal_t *this);
    
    // Internal state
    char *proposal_str;
    int protocol_id;
};

/*
 * ============================================================================
 * Mock Factory Functions
 * ============================================================================
 */

/**
 * Create mock objects
 */
mock_linked_list_t* mock_linked_list_create(void);
mock_ike_cfg_t* mock_ike_cfg_create(const char *name);
mock_peer_cfg_t* mock_peer_cfg_create(const char *name, mock_ike_cfg_t *ike_cfg);
mock_child_cfg_t* mock_child_cfg_create(const char *name);
mock_auth_cfg_t* mock_auth_cfg_create(void);
mock_identification_t* mock_identification_create(const char *id_str, int type);
mock_traffic_selector_t* mock_traffic_selector_create(const char *from_addr, const char *to_addr, 
                                                     uint16_t from_port, uint16_t to_port);
mock_proposal_t* mock_proposal_create(const char *proposal_str, int protocol_id);

/*
 * ============================================================================
 * Mock State Tracking and Verification
 * ============================================================================
 */

/**
 * Call tracking for verification
 */
typedef struct {
    // ike_cfg_t related calls
    int ike_cfg_create_count;
    int ike_cfg_destroy_count;
    char *last_ike_cfg_name;
    
    // peer_cfg_t related calls
    int peer_cfg_create_count;
    int peer_cfg_destroy_count;
    char *last_peer_cfg_name;
    
    // child_cfg_t related calls
    int child_cfg_create_count;
    int child_cfg_destroy_count;
    char *last_child_cfg_name;
    
    // auth_cfg_t related calls
    int auth_cfg_create_count;
    int auth_cfg_destroy_count;
    
    // Memory tracking
    int total_allocations;
    int total_deallocations;
    int current_allocations;
    
    // Parameter capture
    struct {
        char *captured_strings[10];
        int captured_ints[10];
        void *captured_ptrs[10];
        int capture_count;
    } params;
    
} mock_call_state_t;

// Global state access
extern mock_call_state_t *g_mock_state;

/**
 * State verification functions
 */
bool mock_verify_ike_cfg_create_called(void);
bool mock_verify_peer_cfg_create_called(void);
bool mock_verify_child_cfg_create_called(void);
int mock_get_ike_cfg_create_count(void);
int mock_get_peer_cfg_create_count(void);
int mock_get_child_cfg_create_count(void);
const char* mock_get_last_ike_cfg_name(void);
const char* mock_get_last_peer_cfg_name(void);
const char* mock_get_last_child_cfg_name(void);

/**
 * Memory tracking verification
 */
bool mock_verify_no_memory_leaks(void);
int mock_get_current_allocation_count(void);
int mock_get_total_allocation_count(void);
int mock_get_total_deallocation_count(void);

/**
 * Parameter capture functions
 */
void mock_capture_string_param(const char *str);
void mock_capture_int_param(int value);
void mock_capture_ptr_param(void *ptr);
const char* mock_get_captured_string(int index);
int mock_get_captured_int(int index);
void* mock_get_captured_ptr(int index);
int mock_get_capture_count(void);

/*
 * ============================================================================
 * Mock Configuration and Behavior Control
 * ============================================================================
 */

/**
 * Mock behavior configuration
 */
typedef struct {
    // Return value overrides
    bool should_fail_allocations;
    bool should_fail_ike_cfg_create;
    bool should_fail_peer_cfg_create;
    bool should_fail_child_cfg_create;
    
    // Memory allocation limits
    int max_allocations;
    int allocation_failure_at;
    
    // Timing simulation
    bool simulate_slow_operations;
    int operation_delay_ms;
    
} mock_config_t;

extern mock_config_t *g_mock_config;

/**
 * Mock configuration functions
 */
void mock_set_allocation_failure(bool should_fail);
void mock_set_max_allocations(int max_allocs);
void mock_set_allocation_failure_at(int failure_point);
void mock_enable_slow_operations(bool enable, int delay_ms);
void mock_reset_config(void);

/*
 * ============================================================================
 * Type Mapping for strongSwan Compatibility
 * ============================================================================
 */

#ifdef UNIT_TEST_ADAPTER
// Map strongSwan types to mock types when building adapter tests
#define linked_list_t mock_linked_list_t
#define enumerator_t mock_enumerator_t
#define ike_cfg_t mock_ike_cfg_t
#define peer_cfg_t mock_peer_cfg_t
#define child_cfg_t mock_child_cfg_t
#define auth_cfg_t mock_auth_cfg_t
#define identification_t mock_identification_t
#define traffic_selector_t mock_traffic_selector_t
#define proposal_t mock_proposal_t

// Map factory functions
#define ike_cfg_create mock_ike_cfg_create
#define peer_cfg_create mock_peer_cfg_create
#define child_cfg_create mock_child_cfg_create
#define auth_cfg_create mock_auth_cfg_create
#define identification_create mock_identification_create
#define traffic_selector_create mock_traffic_selector_create
#define proposal_create mock_proposal_create
#define linked_list_create mock_linked_list_create

#endif // UNIT_TEST_ADAPTER

/*
 * ============================================================================
 * Mock Assertion Macros
 * ============================================================================
 */

#define MOCK_ASSERT_CALLED(func) \
    do { \
        if (!mock_verify_##func##_called()) { \
            fprintf(stderr, "MOCK_ASSERT_FAILED: %s was not called\n", #func); \
            abort(); \
        } \
    } while(0)

#define MOCK_ASSERT_CALL_COUNT(func, expected) \
    do { \
        int actual = mock_get_##func##_count(); \
        if (actual != expected) { \
            fprintf(stderr, "MOCK_ASSERT_FAILED: %s call count expected %d, got %d\n", \
                    #func, expected, actual); \
            abort(); \
        } \
    } while(0)

#define MOCK_ASSERT_NO_LEAKS() \
    do { \
        if (!mock_verify_no_memory_leaks()) { \
            fprintf(stderr, "MOCK_ASSERT_FAILED: Memory leaks detected\n"); \
            abort(); \
        } \
    } while(0)

#define MOCK_ASSERT_PARAM_STRING(index, expected) \
    do { \
        const char *actual = mock_get_captured_string(index); \
        if (!actual || strcmp(actual, expected) != 0) { \
            fprintf(stderr, "MOCK_ASSERT_FAILED: Parameter %d expected '%s', got '%s'\n", \
                    index, expected, actual ? actual : "NULL"); \
            abort(); \
        } \
    } while(0)

#endif // STRONGSWAN_MOCKS_H