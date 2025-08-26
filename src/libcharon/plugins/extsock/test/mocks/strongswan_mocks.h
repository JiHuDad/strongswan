/*
 * strongSwan Mock Objects for extsock plugin testing
 * Copyright (C) 2024 strongSwan Project
 * 
 * This header provides mock implementations of strongSwan data structures
 * to enable unit testing without full strongSwan library dependencies.
 */

#ifndef STRONGSWAN_MOCKS_H_
#define STRONGSWAN_MOCKS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for circular dependencies */
typedef struct mock_ike_sa_t mock_ike_sa_t;
typedef struct mock_peer_cfg_t mock_peer_cfg_t;
typedef struct mock_ike_cfg_t mock_ike_cfg_t;
typedef struct mock_host_t mock_host_t;
typedef struct mock_linked_list_t mock_linked_list_t;

/* strongSwan enum types for mock compatibility */
typedef enum {
    IKEV1 = 1,
    IKEV2 = 2
} ike_version_t;

typedef enum {
    UNIQUE_NO = 0,
    UNIQUE_REPLACE = 1,
    UNIQUE_KEEP = 2
} unique_policy_t;

/* Mock error types matching extsock plugin */
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_INVALID_PARAMETER = -1,
    EXTSOCK_ERROR_CONFIG_CREATION_FAILED = -2,
    EXTSOCK_ERROR_CONNECTION_FAILED = -3
} extsock_error_t;

/**
 * Mock Host (network address) structure
 */
struct mock_host_t {
    char address[64];
    uint16_t port;
    
    /* Mock methods */
    const char* (*get_address)(mock_host_t *self);
    uint16_t (*get_port)(mock_host_t *self);
    void (*destroy)(mock_host_t *self);
};

/**
 * Mock IKE Configuration structure
 */
struct mock_ike_cfg_t {
    char local_addr[64];
    char remote_addr[256];  /* Can contain comma-separated addresses */
    uint16_t local_port;
    uint16_t remote_port;
    ike_version_t version;
    bool certreq;
    bool force_encap;
    
    /* Mock methods */
    char* (*get_other_addr)(mock_ike_cfg_t *self);
    char* (*get_my_addr)(mock_ike_cfg_t *self);
    uint16_t (*get_my_port)(mock_ike_cfg_t *self);
    uint16_t (*get_other_port)(mock_ike_cfg_t *self);
    ike_version_t (*get_version)(mock_ike_cfg_t *self);
    bool (*send_certreq)(mock_ike_cfg_t *self);
    bool (*force_encap_method)(mock_ike_cfg_t *self);  /* Renamed to avoid conflict */
    void (*destroy)(mock_ike_cfg_t *self);
};

/**
 * Mock Peer Configuration structure
 */
struct mock_peer_cfg_t {
    char name[128];
    mock_ike_cfg_t *ike_cfg;
    unique_policy_t unique_policy;
    uint32_t keyingtries;
    uint32_t rekey_time;
    uint32_t reauth_time;
    uint32_t over_time;
    uint32_t dpd_timeout;
    
    /* Auth and child configs (simplified) */
    mock_linked_list_t *auth_cfgs;
    mock_linked_list_t *child_cfgs;
    
    /* Mock methods */
    const char* (*get_name)(mock_peer_cfg_t *self);
    mock_ike_cfg_t* (*get_ike_cfg)(mock_peer_cfg_t *self);
    unique_policy_t (*get_unique_policy)(mock_peer_cfg_t *self);
    uint32_t (*get_keyingtries)(mock_peer_cfg_t *self);
    uint32_t (*get_rekey_time)(mock_peer_cfg_t *self, bool jitter);
    uint32_t (*get_reauth_time)(mock_peer_cfg_t *self, bool jitter);
    uint32_t (*get_over_time)(mock_peer_cfg_t *self);
    uint32_t (*get_dpd_timeout)(mock_peer_cfg_t *self);
    void (*destroy)(mock_peer_cfg_t *self);
};

/**
 * Mock IKE Security Association structure
 */
struct mock_ike_sa_t {
    char name[128];
    mock_host_t *remote_host;
    mock_peer_cfg_t *peer_cfg;
    uint32_t state;
    
    /* Mock methods */
    const char* (*get_name)(mock_ike_sa_t *self);
    mock_peer_cfg_t* (*get_peer_cfg)(mock_ike_sa_t *self);
    mock_host_t* (*get_other_host)(mock_ike_sa_t *self);
    uint32_t (*get_state)(mock_ike_sa_t *self);
    void (*destroy)(mock_ike_sa_t *self);
};

/**
 * Mock Linked List (simplified)
 */
struct mock_linked_list_t {
    void **items;
    int count;
    int capacity;
    
    /* Mock methods */
    void (*insert_last)(mock_linked_list_t *self, void *item);
    void* (*get)(mock_linked_list_t *self, int index);
    int (*get_count)(mock_linked_list_t *self);
    void (*destroy)(mock_linked_list_t *self);
};

/**
 * Mock configuration creation structures
 */
typedef struct {
    ike_version_t version;
    const char *local;
    const char *remote;
    uint16_t local_port;
    uint16_t remote_port;
    bool no_certreq;
    bool ocsp_certreq;
    bool force_encap;
    bool fragmentation;
    bool childless;
    uint8_t dscp;
} ike_cfg_create_t;

typedef struct {
    unique_policy_t unique;
    uint32_t keyingtries;
    uint32_t rekey_time;
    uint32_t reauth_time;
    uint32_t jitter_time;
    uint32_t over_time;
    uint32_t dpd;
    uint32_t dpd_timeout;
    /* Add other fields as needed */
} peer_cfg_create_t;

/**
 * Mock behavior control structure
 */
typedef struct {
    bool simulate_failure;
    char error_message[256];
    int call_count;
    char last_remote_addr[64];
} mock_behavior_t;

/* Global mock behavior control */
extern mock_behavior_t g_mock_behavior;

/* =============================================================================
 * Mock Factory Function Declarations
 * =============================================================================
 */

/* Basic mock object creation */
mock_host_t* create_mock_host(const char *address, uint16_t port);
mock_ike_cfg_t* create_mock_ike_cfg(const char *local_addr, const char *remote_addr);
mock_peer_cfg_t* create_mock_peer_cfg(const char *name, const char *segw_addresses);
mock_ike_sa_t* create_mock_ike_sa(const char *name, const char *remote_addr);
mock_linked_list_t* create_mock_linked_list(void);

/* strongSwan API compatible creation functions */
mock_ike_cfg_t* ike_cfg_create(ike_cfg_create_t *data);
mock_peer_cfg_t* peer_cfg_create(const char *name, mock_ike_cfg_t *ike_cfg, 
                                peer_cfg_create_t *data);

/* Mock behavior control */
void mock_reset_behavior(void);
void mock_set_failure_mode(bool enable, const char *error_msg);
int mock_get_call_count(void);
const char* mock_get_last_remote_addr(void);

/**
 * Type aliases for compatibility with real strongSwan types
 * (Used in tests to maintain API compatibility)
 */
typedef mock_ike_sa_t ike_sa_t;
typedef mock_peer_cfg_t peer_cfg_t;
typedef mock_ike_cfg_t ike_cfg_t;
typedef mock_host_t host_t;
typedef mock_linked_list_t linked_list_t;

#ifdef __cplusplus
}
#endif

#endif /** STRONGSWAN_MOCKS_H_ */