/*
 * strongSwan Mock Factory Implementation
 * Copyright (C) 2024 strongSwan Project
 * 
 * Factory functions for creating mock strongSwan objects
 */

#include "strongswan_mocks.h"
#include <stdio.h>
#include <assert.h>

/* Global mock behavior control */
mock_behavior_t g_mock_behavior = {
    .simulate_failure = false,
    .error_message = {0},
    .call_count = 0,
    .last_remote_addr = {0}
};

/* =============================================================================
 * Mock Host Implementation
 * =============================================================================
 */

static const char* mock_host_get_address(mock_host_t *this) {
    return this ? this->address : NULL;
}

static uint16_t mock_host_get_port(mock_host_t *this) {
    return this ? this->port : 0;
}

static void mock_host_destroy(mock_host_t *this) {
    if (this) {
        free(this);
    }
}

mock_host_t* create_mock_host(const char *address, uint16_t port) {
    mock_host_t *host = malloc(sizeof(mock_host_t));
    if (!host) return NULL;
    
    strncpy(host->address, address ? address : "127.0.0.1", sizeof(host->address) - 1);
    host->address[sizeof(host->address) - 1] = '\0';
    host->port = port;
    
    /* Set up method pointers */
    host->get_address = mock_host_get_address;
    host->get_port = mock_host_get_port;
    host->destroy = mock_host_destroy;
    
    return host;
}

/* =============================================================================
 * Mock IKE Config Implementation
 * =============================================================================
 */

static char* mock_ike_cfg_get_other_addr(mock_ike_cfg_t *this) {
    if (!this) return NULL;
    return strdup(this->remote_addr);
}

static char* mock_ike_cfg_get_my_addr(mock_ike_cfg_t *this) {
    if (!this) return NULL;
    return strdup(this->local_addr);
}

static uint16_t mock_ike_cfg_get_my_port(mock_ike_cfg_t *this) {
    return this ? this->local_port : 0;
}

static uint16_t mock_ike_cfg_get_other_port(mock_ike_cfg_t *this) {
    return this ? this->remote_port : 0;
}

static ike_version_t mock_ike_cfg_get_version(mock_ike_cfg_t *this) {
    return this ? this->version : IKEV2;
}

static bool mock_ike_cfg_send_certreq(mock_ike_cfg_t *this) {
    return this ? this->certreq : true;
}

static bool mock_ike_cfg_force_encap(mock_ike_cfg_t *this) {
    return this ? this->force_encap : false;
}

static void mock_ike_cfg_destroy(mock_ike_cfg_t *this) {
    if (this) {
        free(this);
    }
}

mock_ike_cfg_t* create_mock_ike_cfg(const char *local_addr, const char *remote_addr) {
    mock_ike_cfg_t *ike_cfg = malloc(sizeof(mock_ike_cfg_t));
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
    
    /* Set up method pointers */
    ike_cfg->get_other_addr = mock_ike_cfg_get_other_addr;
    ike_cfg->get_my_addr = mock_ike_cfg_get_my_addr;
    ike_cfg->get_my_port = mock_ike_cfg_get_my_port;
    ike_cfg->get_other_port = mock_ike_cfg_get_other_port;
    ike_cfg->get_version = mock_ike_cfg_get_version;
    ike_cfg->send_certreq = mock_ike_cfg_send_certreq;
    ike_cfg->force_encap = mock_ike_cfg_force_encap;
    ike_cfg->destroy = mock_ike_cfg_destroy;
    
    return ike_cfg;
}

/* =============================================================================
 * Mock Linked List Implementation
 * =============================================================================
 */

static void mock_list_insert_last(mock_linked_list_t *this, void *item) {
    if (!this) return;
    
    if (this->count >= this->capacity) {
        /* Simple resize */
        this->capacity = this->capacity ? this->capacity * 2 : 4;
        this->items = realloc(this->items, this->capacity * sizeof(void*));
        assert(this->items);
    }
    
    this->items[this->count++] = item;
}

static void* mock_list_get(mock_linked_list_t *this, int index) {
    if (!this || index < 0 || index >= this->count) {
        return NULL;
    }
    return this->items[index];
}

static int mock_list_get_count(mock_linked_list_t *this) {
    return this ? this->count : 0;
}

static void mock_list_destroy(mock_linked_list_t *this) {
    if (this) {
        free(this->items);
        free(this);
    }
}

mock_linked_list_t* create_mock_linked_list() {
    mock_linked_list_t *list = malloc(sizeof(mock_linked_list_t));
    if (!list) return NULL;
    
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    
    /* Set up method pointers */
    list->insert_last = mock_list_insert_last;
    list->get = mock_list_get;
    list->get_count = mock_list_get_count;
    list->destroy = mock_list_destroy;
    
    return list;
}

/* =============================================================================
 * Mock Peer Config Implementation
 * =============================================================================
 */

static const char* mock_peer_cfg_get_name(mock_peer_cfg_t *this) {
    return this ? this->name : NULL;
}

static mock_ike_cfg_t* mock_peer_cfg_get_ike_cfg(mock_peer_cfg_t *this) {
    return this ? this->ike_cfg : NULL;
}

static unique_policy_t mock_peer_cfg_get_unique_policy(mock_peer_cfg_t *this) {
    return this ? this->unique_policy : UNIQUE_NO;
}

static uint32_t mock_peer_cfg_get_keyingtries(mock_peer_cfg_t *this) {
    return this ? this->keyingtries : 3;
}

static uint32_t mock_peer_cfg_get_rekey_time(mock_peer_cfg_t *this, bool jitter) {
    (void)jitter; /* Unused */
    return this ? this->rekey_time : 3600;
}

static uint32_t mock_peer_cfg_get_reauth_time(mock_peer_cfg_t *this, bool jitter) {
    (void)jitter; /* Unused */
    return this ? this->reauth_time : 14400;
}

static uint32_t mock_peer_cfg_get_over_time(mock_peer_cfg_t *this) {
    return this ? this->over_time : 300;
}

static uint32_t mock_peer_cfg_get_dpd_timeout(mock_peer_cfg_t *this) {
    return this ? this->dpd_timeout : 150;
}

static void mock_peer_cfg_destroy(mock_peer_cfg_t *this) {
    if (this) {
        if (this->ike_cfg) {
            this->ike_cfg->destroy(this->ike_cfg);
        }
        if (this->auth_cfgs) {
            this->auth_cfgs->destroy(this->auth_cfgs);
        }
        if (this->child_cfgs) {
            this->child_cfgs->destroy(this->child_cfgs);
        }
        free(this);
    }
}

mock_peer_cfg_t* create_mock_peer_cfg(const char *name, const char *segw_addresses) {
    mock_peer_cfg_t *peer_cfg = malloc(sizeof(mock_peer_cfg_t));
    if (!peer_cfg) return NULL;
    
    strncpy(peer_cfg->name, name ? name : "test-conn", sizeof(peer_cfg->name) - 1);
    peer_cfg->name[sizeof(peer_cfg->name) - 1] = '\0';
    
    /* Create IKE config with SEGW addresses */
    peer_cfg->ike_cfg = create_mock_ike_cfg("127.0.0.1", segw_addresses);
    if (!peer_cfg->ike_cfg) {
        free(peer_cfg);
        return NULL;
    }
    
    /* Set default values */
    peer_cfg->unique_policy = UNIQUE_REPLACE;
    peer_cfg->keyingtries = 3;
    peer_cfg->rekey_time = 3600;
    peer_cfg->reauth_time = 14400;
    peer_cfg->over_time = 300;
    peer_cfg->dpd_timeout = 150;
    
    /* Create empty lists for auth and child configs */
    peer_cfg->auth_cfgs = create_mock_linked_list();
    peer_cfg->child_cfgs = create_mock_linked_list();
    
    /* Set up method pointers */
    peer_cfg->get_name = mock_peer_cfg_get_name;
    peer_cfg->get_ike_cfg = mock_peer_cfg_get_ike_cfg;
    peer_cfg->get_unique_policy = mock_peer_cfg_get_unique_policy;
    peer_cfg->get_keyingtries = mock_peer_cfg_get_keyingtries;
    peer_cfg->get_rekey_time = mock_peer_cfg_get_rekey_time;
    peer_cfg->get_reauth_time = mock_peer_cfg_get_reauth_time;
    peer_cfg->get_over_time = mock_peer_cfg_get_over_time;
    peer_cfg->get_dpd_timeout = mock_peer_cfg_get_dpd_timeout;
    peer_cfg->destroy = mock_peer_cfg_destroy;
    
    return peer_cfg;
}

/* =============================================================================
 * Mock IKE SA Implementation
 * =============================================================================
 */

static const char* mock_ike_sa_get_name(mock_ike_sa_t *this) {
    return this ? this->name : NULL;
}

static mock_peer_cfg_t* mock_ike_sa_get_peer_cfg(mock_ike_sa_t *this) {
    return this ? this->peer_cfg : NULL;
}

static mock_host_t* mock_ike_sa_get_other_host(mock_ike_sa_t *this) {
    return this ? this->remote_host : NULL;
}

static uint32_t mock_ike_sa_get_state(mock_ike_sa_t *this) {
    return this ? this->state : 0;
}

static void mock_ike_sa_destroy(mock_ike_sa_t *this) {
    if (this) {
        if (this->remote_host) {
            this->remote_host->destroy(this->remote_host);
        }
        /* Note: peer_cfg is not owned by ike_sa in real strongSwan */
        free(this);
    }
}

mock_ike_sa_t* create_mock_ike_sa(const char *name, const char *remote_addr) {
    mock_ike_sa_t *ike_sa = malloc(sizeof(mock_ike_sa_t));
    if (!ike_sa) return NULL;
    
    strncpy(ike_sa->name, name ? name : "test-ike-sa", sizeof(ike_sa->name) - 1);
    ike_sa->name[sizeof(ike_sa->name) - 1] = '\0';
    
    /* Create remote host */
    ike_sa->remote_host = create_mock_host(remote_addr ? remote_addr : "10.1.1.1", 500);
    if (!ike_sa->remote_host) {
        free(ike_sa);
        return NULL;
    }
    
    ike_sa->peer_cfg = NULL; /* Set externally if needed */
    ike_sa->state = 1; /* Connected state */
    
    /* Set up method pointers */
    ike_sa->get_name = mock_ike_sa_get_name;
    ike_sa->get_peer_cfg = mock_ike_sa_get_peer_cfg;
    ike_sa->get_other_host = mock_ike_sa_get_other_host;
    ike_sa->get_state = mock_ike_sa_get_state;
    ike_sa->destroy = mock_ike_sa_destroy;
    
    return ike_sa;
}

/* =============================================================================
 * Mock Configuration Creation Functions
 * =============================================================================
 */

mock_ike_cfg_t* ike_cfg_create(ike_cfg_create_t *data) {
    if (!data) return NULL;
    
    /* Update global mock behavior */
    g_mock_behavior.call_count++;
    if (data->remote) {
        strncpy(g_mock_behavior.last_remote_addr, data->remote, 
                sizeof(g_mock_behavior.last_remote_addr) - 1);
        g_mock_behavior.last_remote_addr[sizeof(g_mock_behavior.last_remote_addr) - 1] = '\0';
    }
    
    if (g_mock_behavior.simulate_failure) {
        return NULL;
    }
    
    mock_ike_cfg_t *ike_cfg = create_mock_ike_cfg(data->local, data->remote);
    if (ike_cfg) {
        ike_cfg->local_port = data->local_port;
        ike_cfg->remote_port = data->remote_port;
        ike_cfg->version = data->version;
        ike_cfg->certreq = !data->no_certreq;
        ike_cfg->force_encap = data->force_encap;
    }
    
    return ike_cfg;
}

mock_peer_cfg_t* peer_cfg_create(const char *name, mock_ike_cfg_t *ike_cfg, 
                                peer_cfg_create_t *data) {
    if (!name || !ike_cfg) return NULL;
    
    if (g_mock_behavior.simulate_failure) {
        return NULL;
    }
    
    mock_peer_cfg_t *peer_cfg = malloc(sizeof(mock_peer_cfg_t));
    if (!peer_cfg) return NULL;
    
    strncpy(peer_cfg->name, name, sizeof(peer_cfg->name) - 1);
    peer_cfg->name[sizeof(peer_cfg->name) - 1] = '\0';
    
    peer_cfg->ike_cfg = ike_cfg; /* Transfer ownership */
    
    if (data) {
        peer_cfg->unique_policy = data->unique;
        peer_cfg->keyingtries = data->keyingtries;
        peer_cfg->rekey_time = data->rekey_time;
        peer_cfg->reauth_time = data->reauth_time;
        peer_cfg->over_time = data->over_time;
        peer_cfg->dpd_timeout = data->dpd_timeout;
    } else {
        /* Default values */
        peer_cfg->unique_policy = UNIQUE_REPLACE;
        peer_cfg->keyingtries = 3;
        peer_cfg->rekey_time = 3600;
        peer_cfg->reauth_time = 14400;
        peer_cfg->over_time = 300;
        peer_cfg->dpd_timeout = 150;
    }
    
    /* Create empty lists */
    peer_cfg->auth_cfgs = create_mock_linked_list();
    peer_cfg->child_cfgs = create_mock_linked_list();
    
    /* Set up method pointers */
    peer_cfg->get_name = mock_peer_cfg_get_name;
    peer_cfg->get_ike_cfg = mock_peer_cfg_get_ike_cfg;
    peer_cfg->get_unique_policy = mock_peer_cfg_get_unique_policy;
    peer_cfg->get_keyingtries = mock_peer_cfg_get_keyingtries;
    peer_cfg->get_rekey_time = mock_peer_cfg_get_rekey_time;
    peer_cfg->get_reauth_time = mock_peer_cfg_get_reauth_time;
    peer_cfg->get_over_time = mock_peer_cfg_get_over_time;
    peer_cfg->get_dpd_timeout = mock_peer_cfg_get_dpd_timeout;
    peer_cfg->destroy = mock_peer_cfg_destroy;
    
    return peer_cfg;
}

/* =============================================================================
 * Mock Behavior Control Functions
 * =============================================================================
 */

void mock_reset_behavior() {
    g_mock_behavior.simulate_failure = false;
    g_mock_behavior.error_message[0] = '\0';
    g_mock_behavior.call_count = 0;
    g_mock_behavior.last_remote_addr[0] = '\0';
}

void mock_set_failure_mode(bool enable, const char *error_msg) {
    g_mock_behavior.simulate_failure = enable;
    if (error_msg) {
        strncpy(g_mock_behavior.error_message, error_msg, 
                sizeof(g_mock_behavior.error_message) - 1);
        g_mock_behavior.error_message[sizeof(g_mock_behavior.error_message) - 1] = '\0';
    }
}

int mock_get_call_count() {
    return g_mock_behavior.call_count;
}

const char* mock_get_last_remote_addr() {
    return g_mock_behavior.last_remote_addr;
}