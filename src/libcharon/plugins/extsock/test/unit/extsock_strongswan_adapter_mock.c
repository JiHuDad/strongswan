/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Mock implementation of extsock_strongswan_adapter for adapter unit tests
 * TASK-009: strongSwan Adapter 실제 테스트
 */

#include "extsock_strongswan_adapter_mock.h"
#include "../infrastructure/strongswan_mocks.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Simple mock state for strongSwan Adapter
static struct {
    int add_peer_config_calls;
    int remove_peer_config_calls;
    int initiate_child_sa_calls;
    int apply_config_calls;
    int remove_config_calls;
    int start_dpd_calls;
    char last_peer_name[256];
    char last_removed_name[256];
    char last_ike_sa_name[256];
    bool simulate_failure;
    extsock_error_t failure_error;
} mock_adapter_state = {0};

// Mock mem_cred methods
static void mock_mem_cred_destroy(mem_cred_t *this)
{
    if (this) {
        free(this);
    }
}

// Mock config entity methods
static peer_cfg_t* mock_config_entity_to_peer_cfg(extsock_config_entity_t *this)
{
    if (!this || !this->config_data) return NULL;
    
    // Simple mock: create peer_cfg based on config data using infrastructure
    char name[256];
    if (sscanf(this->config_data, "%255s", name) == 1) {
        return (peer_cfg_t*)mock_peer_cfg_create(name, NULL);
    }
    return NULL;
}

static void mock_config_entity_destroy(extsock_config_entity_t *this)
{
    if (this) {
        free(this->config_data);
        free(this);
    }
}

// strongSwan adapter private structure
typedef struct private_extsock_strongswan_adapter_t {
    extsock_strongswan_adapter_t public;
    linked_list_t *managed_peer_cfgs;
    mem_cred_t *creds;
} private_extsock_strongswan_adapter_t;

// Config repository methods
static extsock_error_t mock_apply_config(extsock_config_repository_t *this, 
                                        extsock_config_entity_t *config)
{
    (void)this; // Suppress unused parameter warning
    
    if (mock_adapter_state.simulate_failure) {
        return mock_adapter_state.failure_error;
    }
    
    if (!config) return EXTSOCK_ERROR_CONFIG_INVALID;
    
    // Record call
    mock_adapter_state.apply_config_calls++;
    
    // Mock conversion and application
    peer_cfg_t *peer_cfg = config->to_peer_cfg(config);
    if (!peer_cfg) return EXTSOCK_ERROR_CONFIG_INVALID;
    
    mock_peer_cfg_t *mock_peer = (mock_peer_cfg_t*)peer_cfg;
    if (mock_peer->name) {
        strncpy(mock_adapter_state.last_peer_name, mock_peer->name, 
                sizeof(mock_adapter_state.last_peer_name) - 1);
        mock_adapter_state.last_peer_name[sizeof(mock_adapter_state.last_peer_name) - 1] = '\0';
    }
    
    free(mock_peer);
    return EXTSOCK_SUCCESS;
}

static extsock_error_t mock_remove_config(extsock_config_repository_t *this, 
                                         const char *name)
{
    (void)this; // Suppress unused parameter warning
    
    if (mock_adapter_state.simulate_failure) {
        return mock_adapter_state.failure_error;
    }
    
    if (!name) return EXTSOCK_ERROR_CONFIG_INVALID;
    
    // Record call and name
    mock_adapter_state.remove_config_calls++;
    strncpy(mock_adapter_state.last_removed_name, name, sizeof(mock_adapter_state.last_removed_name) - 1);
    mock_adapter_state.last_removed_name[sizeof(mock_adapter_state.last_removed_name) - 1] = '\0';
    
    return EXTSOCK_SUCCESS;
}

static extsock_error_t mock_start_dpd(extsock_config_repository_t *this, 
                                     const char *ike_sa_name)
{
    (void)this; // Suppress unused parameter warning
    
    if (mock_adapter_state.simulate_failure) {
        return mock_adapter_state.failure_error;
    }
    
    if (!ike_sa_name) return EXTSOCK_ERROR_CONFIG_INVALID;
    
    // Record call and IKE SA name
    mock_adapter_state.start_dpd_calls++;
    strncpy(mock_adapter_state.last_ike_sa_name, ike_sa_name, sizeof(mock_adapter_state.last_ike_sa_name) - 1);
    mock_adapter_state.last_ike_sa_name[sizeof(mock_adapter_state.last_ike_sa_name) - 1] = '\0';
    
    return EXTSOCK_SUCCESS;
}

static void mock_config_repository_destroy(extsock_config_repository_t *this)
{
    // Repository is part of adapter, no separate cleanup needed
}

// strongSwan adapter methods
static extsock_error_t mock_add_peer_config(extsock_strongswan_adapter_t *this,
                                           peer_cfg_t *peer_cfg)
{
    private_extsock_strongswan_adapter_t *adapter = 
        (private_extsock_strongswan_adapter_t*)this;
    
    if (mock_adapter_state.simulate_failure) {
        return mock_adapter_state.failure_error;
    }
    
    if (!peer_cfg) return EXTSOCK_ERROR_CONFIG_INVALID;
    
    // Record call
    mock_adapter_state.add_peer_config_calls++;
    
    mock_peer_cfg_t *mock_peer = (mock_peer_cfg_t*)peer_cfg;
    if (mock_peer->name) {
        strncpy(mock_adapter_state.last_peer_name, mock_peer->name, 
                sizeof(mock_adapter_state.last_peer_name) - 1);
        mock_adapter_state.last_peer_name[sizeof(mock_adapter_state.last_peer_name) - 1] = '\0';
    }
    
    // Add to managed list
    if (adapter->managed_peer_cfgs) {
        mock_linked_list_t *mock_list = (mock_linked_list_t*)adapter->managed_peer_cfgs;
        mock_list->insert_last(mock_list, peer_cfg);
    }
    
    return EXTSOCK_SUCCESS;
}

static extsock_error_t mock_remove_peer_config(extsock_strongswan_adapter_t *this,
                                              const char *name)
{
    (void)this; // Suppress unused parameter warning
    
    if (mock_adapter_state.simulate_failure) {
        return mock_adapter_state.failure_error;
    }
    
    if (!name) return EXTSOCK_ERROR_CONFIG_INVALID;
    
    // Record call and name
    mock_adapter_state.remove_peer_config_calls++;
    strncpy(mock_adapter_state.last_removed_name, name, sizeof(mock_adapter_state.last_removed_name) - 1);
    mock_adapter_state.last_removed_name[sizeof(mock_adapter_state.last_removed_name) - 1] = '\0';
    
    return EXTSOCK_SUCCESS;
}

static extsock_error_t mock_initiate_child_sa(extsock_strongswan_adapter_t *this,
                                             peer_cfg_t *peer_cfg,
                                             child_cfg_t *child_cfg)
{
    (void)this; // Suppress unused parameter warning
    
    if (mock_adapter_state.simulate_failure) {
        return mock_adapter_state.failure_error;
    }
    
    if (!peer_cfg || !child_cfg) return EXTSOCK_ERROR_CONFIG_INVALID;
    
    // Record call
    mock_adapter_state.initiate_child_sa_calls++;
    
    return EXTSOCK_SUCCESS;
}

static linked_list_t* mock_get_managed_configs(extsock_strongswan_adapter_t *this)
{
    private_extsock_strongswan_adapter_t *adapter = 
        (private_extsock_strongswan_adapter_t*)this;
    return adapter->managed_peer_cfgs;
}

static mem_cred_t* mock_get_credentials(extsock_strongswan_adapter_t *this)
{
    private_extsock_strongswan_adapter_t *adapter = 
        (private_extsock_strongswan_adapter_t*)this;
    return adapter->creds;
}

static void mock_strongswan_adapter_destroy(extsock_strongswan_adapter_t *this)
{
    private_extsock_strongswan_adapter_t *adapter = 
        (private_extsock_strongswan_adapter_t*)this;
    
    if (adapter) {
        if (adapter->managed_peer_cfgs) {
            mock_linked_list_t *mock_list = (mock_linked_list_t*)adapter->managed_peer_cfgs;
            mock_list->destroy(mock_list);
        }
        if (adapter->creds) {
            adapter->creds->destroy(adapter->creds);
        }
        free(adapter);
    }
}

// Factory functions

mem_cred_t *mock_mem_cred_create()
{
    mem_cred_t *cred = malloc(sizeof(mem_cred_t));
    if (!cred) return NULL;
    
    cred->initialized = 1;
    cred->destroy = mock_mem_cred_destroy;
    
    return cred;
}

extsock_config_entity_t *mock_config_entity_create(const char *config_data)
{
    extsock_config_entity_t *entity = malloc(sizeof(extsock_config_entity_t));
    if (!entity) return NULL;
    
    entity->config_data = config_data ? strdup(config_data) : NULL;
    entity->to_peer_cfg = mock_config_entity_to_peer_cfg;
    entity->destroy = mock_config_entity_destroy;
    
    return entity;
}

extsock_strongswan_adapter_t *extsock_strongswan_adapter_create()
{
    private_extsock_strongswan_adapter_t *adapter = 
        malloc(sizeof(private_extsock_strongswan_adapter_t));
    if (!adapter) return NULL;
    
    // Initialize config repository interface
    adapter->public.config_repository.apply_config = mock_apply_config;
    adapter->public.config_repository.remove_config = mock_remove_config;
    adapter->public.config_repository.start_dpd = mock_start_dpd;
    adapter->public.config_repository.destroy = mock_config_repository_destroy;
    
    // Initialize adapter interface
    adapter->public.add_peer_config = mock_add_peer_config;
    adapter->public.remove_peer_config = mock_remove_peer_config;
    adapter->public.initiate_child_sa = mock_initiate_child_sa;
    adapter->public.get_managed_configs = mock_get_managed_configs;
    adapter->public.get_credentials = mock_get_credentials;
    adapter->public.destroy = mock_strongswan_adapter_destroy;
    
    // Initialize private data
    adapter->managed_peer_cfgs = (linked_list_t*)mock_linked_list_create();
    adapter->creds = mock_mem_cred_create();
    
    return &adapter->public;
}

// Mock state management
void mock_strongswan_reset_state()
{
    memset(&mock_adapter_state, 0, sizeof(mock_adapter_state));
}

mock_strongswan_state_t* mock_strongswan_get_state()
{
    static mock_strongswan_state_t state;
    state.add_peer_config_calls = mock_adapter_state.add_peer_config_calls;
    state.remove_peer_config_calls = mock_adapter_state.remove_peer_config_calls;
    state.initiate_child_sa_calls = mock_adapter_state.initiate_child_sa_calls;
    state.apply_config_calls = mock_adapter_state.apply_config_calls;
    state.remove_config_calls = mock_adapter_state.remove_config_calls;
    state.start_dpd_calls = mock_adapter_state.start_dpd_calls;
    strncpy(state.last_peer_name, mock_adapter_state.last_peer_name, sizeof(state.last_peer_name) - 1);
    state.last_peer_name[sizeof(state.last_peer_name) - 1] = '\0';
    strncpy(state.last_removed_name, mock_adapter_state.last_removed_name, sizeof(state.last_removed_name) - 1);
    state.last_removed_name[sizeof(state.last_removed_name) - 1] = '\0';
    strncpy(state.last_ike_sa_name, mock_adapter_state.last_ike_sa_name, sizeof(state.last_ike_sa_name) - 1);
    state.last_ike_sa_name[sizeof(state.last_ike_sa_name) - 1] = '\0';
    state.simulate_failure = mock_adapter_state.simulate_failure;
    state.failure_error = mock_adapter_state.failure_error;
    return &state;
}

void mock_strongswan_simulate_failure(bool enable, extsock_error_t error)
{
    mock_adapter_state.simulate_failure = enable;
    mock_adapter_state.failure_error = error;
}