/*
 * Simplified strongSwan Mock Objects for testing
 * Copyright (C) 2024 strongSwan Project
 */

#ifndef SIMPLE_MOCKS_H_
#define SIMPLE_MOCKS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error types */
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_INVALID_PARAMETER = -1,
    EXTSOCK_ERROR_CONFIG_CREATION_FAILED = -2,
    EXTSOCK_ERROR_CONNECTION_FAILED = -3
} extsock_error_t;

/* Simple enum types */
typedef enum {
    IKEV1 = 1,
    IKEV2 = 2
} ike_version_t;

typedef enum {
    UNIQUE_NO = 0,
    UNIQUE_REPLACE = 1,
    UNIQUE_KEEP = 2
} unique_policy_t;

/* Simple mock structures */
typedef struct mock_host_t {
    char address[64];
    uint16_t port;
} mock_host_t;

typedef struct mock_ike_cfg_t {
    char local_addr[64];
    char remote_addr[256];
    uint16_t local_port;
    uint16_t remote_port;
    ike_version_t version;
    bool certreq;
    bool force_encap;
} mock_ike_cfg_t;

typedef struct mock_peer_cfg_t {
    char name[128];
    mock_ike_cfg_t *ike_cfg;
    unique_policy_t unique_policy;
    uint32_t keyingtries;
    uint32_t rekey_time;
    uint32_t reauth_time;
    uint32_t over_time;
    uint32_t dpd_timeout;
} mock_peer_cfg_t;

typedef struct mock_ike_sa_t {
    char name[128];
    mock_host_t *remote_host;
    mock_peer_cfg_t *peer_cfg;
    uint32_t state;
} mock_ike_sa_t;

/* Configuration creation structures */
typedef struct {
    ike_version_t version;
    const char *local;
    const char *remote;
    uint16_t local_port;
    uint16_t remote_port;
    bool no_certreq;
    bool force_encap;
} ike_cfg_create_t;

typedef struct {
    unique_policy_t unique;
    uint32_t keyingtries;
    uint32_t rekey_time;
    uint32_t reauth_time;
    uint32_t over_time;
    uint32_t dpd_timeout;
} peer_cfg_create_t;

/* Type aliases for compatibility */
typedef mock_ike_sa_t ike_sa_t;
typedef mock_peer_cfg_t peer_cfg_t;
typedef mock_ike_cfg_t ike_cfg_t;
typedef mock_host_t host_t;

/* Simple function interfaces */
const char* mock_ike_sa_get_name(ike_sa_t *sa);
peer_cfg_t* mock_ike_sa_get_peer_cfg(ike_sa_t *sa);
host_t* mock_ike_sa_get_other_host(ike_sa_t *sa);

const char* mock_peer_cfg_get_name(peer_cfg_t *cfg);
ike_cfg_t* mock_peer_cfg_get_ike_cfg(peer_cfg_t *cfg);
unique_policy_t mock_peer_cfg_get_unique_policy(peer_cfg_t *cfg);
uint32_t mock_peer_cfg_get_keyingtries(peer_cfg_t *cfg);
uint32_t mock_peer_cfg_get_rekey_time(peer_cfg_t *cfg);
uint32_t mock_peer_cfg_get_reauth_time(peer_cfg_t *cfg);
uint32_t mock_peer_cfg_get_over_time(peer_cfg_t *cfg);
uint32_t mock_peer_cfg_get_dpd_timeout(peer_cfg_t *cfg);

char* mock_ike_cfg_get_other_addr(ike_cfg_t *cfg);
char* mock_ike_cfg_get_my_addr(ike_cfg_t *cfg);
uint16_t mock_ike_cfg_get_my_port(ike_cfg_t *cfg);
uint16_t mock_ike_cfg_get_other_port(ike_cfg_t *cfg);
ike_version_t mock_ike_cfg_get_version(ike_cfg_t *cfg);
bool mock_ike_cfg_send_certreq(ike_cfg_t *cfg);
bool mock_ike_cfg_force_encap(ike_cfg_t *cfg);

const char* mock_host_get_address(host_t *host);

/* Creation functions */
mock_ike_cfg_t* create_mock_ike_cfg(const char *local_addr, const char *remote_addr);
mock_peer_cfg_t* create_mock_peer_cfg(const char *name, const char *segw_addresses);
mock_ike_sa_t* create_mock_ike_sa(const char *name, const char *remote_addr);
mock_host_t* create_mock_host(const char *address, uint16_t port);

ike_cfg_t* ike_cfg_create(ike_cfg_create_t *data);
peer_cfg_t* peer_cfg_create(const char *name, ike_cfg_t *ike_cfg, peer_cfg_create_t *data);

void mock_destroy_ike_cfg(ike_cfg_t *cfg);
void mock_destroy_peer_cfg(peer_cfg_t *cfg);
void mock_destroy_ike_sa(ike_sa_t *sa);
void mock_destroy_host(host_t *host);

/* Global mock control */
extern bool g_mock_simulate_failure;

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_MOCKS_H_ */