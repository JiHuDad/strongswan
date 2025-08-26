/*
 * Simplified strongSwan Mock Implementation
 * Copyright (C) 2024 strongSwan Project
 */

#include "simple_mocks.h"
#include <stdio.h>

/* Global mock control */
bool g_mock_simulate_failure = false;

/* =============================================================================
 * IKE SA Mock Functions
 * =============================================================================
 */

const char* mock_ike_sa_get_name(ike_sa_t *sa) {
    return sa ? sa->name : NULL;
}

peer_cfg_t* mock_ike_sa_get_peer_cfg(ike_sa_t *sa) {
    return sa ? sa->peer_cfg : NULL;
}

host_t* mock_ike_sa_get_other_host(ike_sa_t *sa) {
    return sa ? sa->remote_host : NULL;
}

/* =============================================================================
 * Peer Config Mock Functions
 * =============================================================================
 */

const char* mock_peer_cfg_get_name(peer_cfg_t *cfg) {
    return cfg ? cfg->name : NULL;
}

ike_cfg_t* mock_peer_cfg_get_ike_cfg(peer_cfg_t *cfg) {
    return cfg ? cfg->ike_cfg : NULL;
}

unique_policy_t mock_peer_cfg_get_unique_policy(peer_cfg_t *cfg) {
    return cfg ? cfg->unique_policy : UNIQUE_NO;
}

uint32_t mock_peer_cfg_get_keyingtries(peer_cfg_t *cfg) {
    return cfg ? cfg->keyingtries : 3;
}

uint32_t mock_peer_cfg_get_rekey_time(peer_cfg_t *cfg) {
    return cfg ? cfg->rekey_time : 3600;
}

uint32_t mock_peer_cfg_get_reauth_time(peer_cfg_t *cfg) {
    return cfg ? cfg->reauth_time : 14400;
}

uint32_t mock_peer_cfg_get_over_time(peer_cfg_t *cfg) {
    return cfg ? cfg->over_time : 300;
}

uint32_t mock_peer_cfg_get_dpd_timeout(peer_cfg_t *cfg) {
    return cfg ? cfg->dpd_timeout : 150;
}

/* =============================================================================
 * IKE Config Mock Functions
 * =============================================================================
 */

char* mock_ike_cfg_get_other_addr(ike_cfg_t *cfg) {
    if (!cfg) return NULL;
    return strdup(cfg->remote_addr);
}

char* mock_ike_cfg_get_my_addr(ike_cfg_t *cfg) {
    if (!cfg) return NULL;
    return strdup(cfg->local_addr);
}

uint16_t mock_ike_cfg_get_my_port(ike_cfg_t *cfg) {
    return cfg ? cfg->local_port : 0;
}

uint16_t mock_ike_cfg_get_other_port(ike_cfg_t *cfg) {
    return cfg ? cfg->remote_port : 0;
}

ike_version_t mock_ike_cfg_get_version(ike_cfg_t *cfg) {
    return cfg ? cfg->version : IKEV2;
}

bool mock_ike_cfg_send_certreq(ike_cfg_t *cfg) {
    return cfg ? cfg->certreq : true;
}

bool mock_ike_cfg_force_encap(ike_cfg_t *cfg) {
    return cfg ? cfg->force_encap : false;
}

/* =============================================================================
 * Host Mock Functions
 * =============================================================================
 */

const char* mock_host_get_address(host_t *host) {
    return host ? host->address : NULL;
}

/* =============================================================================
 * Creation Functions
 * =============================================================================
 */

mock_host_t* create_mock_host(const char *address, uint16_t port) {
    mock_host_t *host = (mock_host_t*)malloc(sizeof(mock_host_t));
    if (!host) return NULL;
    
    strncpy(host->address, address ? address : "127.0.0.1", sizeof(host->address) - 1);
    host->address[sizeof(host->address) - 1] = '\0';
    host->port = port;
    
    return host;
}

mock_ike_cfg_t* create_mock_ike_cfg(const char *local_addr, const char *remote_addr) {
    mock_ike_cfg_t *ike_cfg = (mock_ike_cfg_t*)malloc(sizeof(mock_ike_cfg_t));
    if (!ike_cfg) return NULL;
    
    strncpy(ike_cfg->local_addr, local_addr ? local_addr : "127.0.0.1", 
            sizeof(ike_cfg->local_addr) - 1);
    ike_cfg->local_addr[sizeof(ike_cfg->local_addr) - 1] = '\0';
    
    strncpy(ike_cfg->remote_addr, remote_addr ? remote_addr : "10.1.1.1", 
            sizeof(ike_cfg->remote_addr) - 1);
    ike_cfg->remote_addr[sizeof(ike_cfg->remote_addr) - 1] = '\0';
    
    ike_cfg->local_port = 500;
    ike_cfg->remote_port = 500;
    ike_cfg->version = IKEV2;
    ike_cfg->certreq = true;
    ike_cfg->force_encap = false;
    
    return ike_cfg;
}

mock_peer_cfg_t* create_mock_peer_cfg(const char *name, const char *segw_addresses) {
    mock_peer_cfg_t *peer_cfg = (mock_peer_cfg_t*)malloc(sizeof(mock_peer_cfg_t));
    if (!peer_cfg) return NULL;
    
    strncpy(peer_cfg->name, name ? name : "test-conn", sizeof(peer_cfg->name) - 1);
    peer_cfg->name[sizeof(peer_cfg->name) - 1] = '\0';
    
    peer_cfg->ike_cfg = create_mock_ike_cfg("127.0.0.1", segw_addresses);
    if (!peer_cfg->ike_cfg) {
        free(peer_cfg);
        return NULL;
    }
    
    peer_cfg->unique_policy = UNIQUE_REPLACE;
    peer_cfg->keyingtries = 3;
    peer_cfg->rekey_time = 3600;
    peer_cfg->reauth_time = 14400;
    peer_cfg->over_time = 300;
    peer_cfg->dpd_timeout = 150;
    
    return peer_cfg;
}

mock_ike_sa_t* create_mock_ike_sa(const char *name, const char *remote_addr) {
    mock_ike_sa_t *ike_sa = (mock_ike_sa_t*)malloc(sizeof(mock_ike_sa_t));
    if (!ike_sa) return NULL;
    
    strncpy(ike_sa->name, name ? name : "test-ike-sa", sizeof(ike_sa->name) - 1);
    ike_sa->name[sizeof(ike_sa->name) - 1] = '\0';
    
    ike_sa->remote_host = create_mock_host(remote_addr ? remote_addr : "10.1.1.1", 500);
    if (!ike_sa->remote_host) {
        free(ike_sa);
        return NULL;
    }
    
    ike_sa->peer_cfg = NULL;
    ike_sa->state = 1;
    
    return ike_sa;
}

/* =============================================================================
 * strongSwan API Compatible Functions
 * =============================================================================
 */

ike_cfg_t* ike_cfg_create(ike_cfg_create_t *data) {
    if (!data || g_mock_simulate_failure) {
        return NULL;
    }
    
    return create_mock_ike_cfg(data->local, data->remote);
}

peer_cfg_t* peer_cfg_create(const char *name, ike_cfg_t *ike_cfg, peer_cfg_create_t *data) {
    if (!name || !ike_cfg || g_mock_simulate_failure) {
        return NULL;
    }
    
    mock_peer_cfg_t *peer_cfg = (mock_peer_cfg_t*)malloc(sizeof(mock_peer_cfg_t));
    if (!peer_cfg) return NULL;
    
    strncpy(peer_cfg->name, name, sizeof(peer_cfg->name) - 1);
    peer_cfg->name[sizeof(peer_cfg->name) - 1] = '\0';
    
    peer_cfg->ike_cfg = ike_cfg;  /* Transfer ownership */
    
    if (data) {
        peer_cfg->unique_policy = data->unique;
        peer_cfg->keyingtries = data->keyingtries;
        peer_cfg->rekey_time = data->rekey_time;
        peer_cfg->reauth_time = data->reauth_time;
        peer_cfg->over_time = data->over_time;
        peer_cfg->dpd_timeout = data->dpd_timeout;
    } else {
        peer_cfg->unique_policy = UNIQUE_REPLACE;
        peer_cfg->keyingtries = 3;
        peer_cfg->rekey_time = 3600;
        peer_cfg->reauth_time = 14400;
        peer_cfg->over_time = 300;
        peer_cfg->dpd_timeout = 150;
    }
    
    return peer_cfg;
}

/* =============================================================================
 * Cleanup Functions
 * =============================================================================
 */

void mock_destroy_host(host_t *host) {
    if (host) {
        free(host);
    }
}

void mock_destroy_ike_cfg(ike_cfg_t *cfg) {
    if (cfg) {
        free(cfg);
    }
}

void mock_destroy_peer_cfg(peer_cfg_t *cfg) {
    if (cfg) {
        if (cfg->ike_cfg) {
            mock_destroy_ike_cfg(cfg->ike_cfg);
        }
        free(cfg);
    }
}

void mock_destroy_ike_sa(ike_sa_t *sa) {
    if (sa) {
        if (sa->remote_host) {
            mock_destroy_host(sa->remote_host);
        }
        free(sa);
    }
}