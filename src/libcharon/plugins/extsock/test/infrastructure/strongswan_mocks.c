/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * strongSwan API Mocking Infrastructure Implementation
 */

#include "strongswan_mocks.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Global mock state
mock_call_state_t *g_mock_state = NULL;
mock_config_t *g_mock_config = NULL;

/*
 * ============================================================================
 * Mock System Lifecycle
 * ============================================================================
 */

void strongswan_mocks_init(void)
{
    if (g_mock_state) {
        strongswan_mocks_cleanup();
    }
    
    g_mock_state = malloc(sizeof(mock_call_state_t));
    g_mock_config = malloc(sizeof(mock_config_t));
    
    assert(g_mock_state != NULL);
    assert(g_mock_config != NULL);
    
    strongswan_mocks_reset_state();
    mock_reset_config();
}

void strongswan_mocks_cleanup(void)
{
    if (g_mock_state) {
        // Clean up captured strings
        for (int i = 0; i < g_mock_state->params.capture_count; i++) {
            if (g_mock_state->params.captured_strings[i]) {
                free(g_mock_state->params.captured_strings[i]);
            }
        }
        
        if (g_mock_state->last_ike_cfg_name) {
            free(g_mock_state->last_ike_cfg_name);
        }
        if (g_mock_state->last_peer_cfg_name) {
            free(g_mock_state->last_peer_cfg_name);
        }
        if (g_mock_state->last_child_cfg_name) {
            free(g_mock_state->last_child_cfg_name);
        }
        
        free(g_mock_state);
        g_mock_state = NULL;
    }
    
    if (g_mock_config) {
        free(g_mock_config);
        g_mock_config = NULL;
    }
}

void strongswan_mocks_reset_state(void)
{
    if (!g_mock_state) return;
    
    memset(g_mock_state, 0, sizeof(mock_call_state_t));
}

/*
 * ============================================================================
 * Mock linked_list_t Implementation
 * ============================================================================
 */

static void mock_linked_list_destroy(mock_linked_list_t *this)
{
    if (!this) return;
    
    if (this->items) {
        free(this->items);
    }
    
    g_mock_state->total_deallocations++;
    g_mock_state->current_allocations--;
    
    free(this);
}

static int mock_linked_list_get_count(mock_linked_list_t *this)
{
    return this ? this->count : 0;
}

static void mock_linked_list_insert_last(mock_linked_list_t *this, void *item)
{
    if (!this || !item) return;
    
    if (this->count >= this->capacity) {
        this->capacity = this->capacity > 0 ? this->capacity * 2 : 4;
        this->items = realloc(this->items, sizeof(void*) * this->capacity);
        assert(this->items != NULL);
    }
    
    this->items[this->count++] = item;
}

static void* mock_linked_list_get_first(mock_linked_list_t *this)
{
    return (this && this->count > 0) ? this->items[0] : NULL;
}

static mock_enumerator_t* mock_linked_list_create_enumerator(mock_linked_list_t *this);

mock_linked_list_t* mock_linked_list_create(void)
{
    if (g_mock_config && g_mock_config->should_fail_allocations) {
        return NULL;
    }
    
    mock_linked_list_t *list = malloc(sizeof(mock_linked_list_t));
    if (!list) return NULL;
    
    list->destroy = mock_linked_list_destroy;
    list->get_count = mock_linked_list_get_count;
    list->insert_last = mock_linked_list_insert_last;
    list->get_first = mock_linked_list_get_first;
    list->create_enumerator = mock_linked_list_create_enumerator;
    
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    
    g_mock_state->total_allocations++;
    g_mock_state->current_allocations++;
    
    return list;
}

/*
 * ============================================================================
 * Mock enumerator_t Implementation
 * ============================================================================
 */

static void mock_enumerator_destroy(mock_enumerator_t *this)
{
    if (!this) return;
    
    g_mock_state->total_deallocations++;
    g_mock_state->current_allocations--;
    
    free(this);
}

static bool mock_enumerator_enumerate(mock_enumerator_t *this, void **item)
{
    if (!this || !item || !this->list) {
        return false;
    }
    
    if (this->current_index >= this->list->count) {
        return false;
    }
    
    *item = this->list->items[this->current_index++];
    return true;
}

static mock_enumerator_t* mock_linked_list_create_enumerator(mock_linked_list_t *this)
{
    if (!this) return NULL;
    
    mock_enumerator_t *enumerator = malloc(sizeof(mock_enumerator_t));
    if (!enumerator) return NULL;
    
    enumerator->destroy = mock_enumerator_destroy;
    enumerator->enumerate = mock_enumerator_enumerate;
    enumerator->list = this;
    enumerator->current_index = 0;
    
    g_mock_state->total_allocations++;
    g_mock_state->current_allocations++;
    
    return enumerator;
}

/*
 * ============================================================================
 * Mock ike_cfg_t Implementation  
 * ============================================================================
 */

static void mock_ike_cfg_destroy(mock_ike_cfg_t *this)
{
    if (!this) return;
    
    if (this->name) {
        free(this->name);
    }
    if (this->my_hosts) {
        this->my_hosts->destroy(this->my_hosts);
    }
    if (this->other_hosts) {
        this->other_hosts->destroy(this->other_hosts);
    }
    if (this->proposals) {
        this->proposals->destroy(this->proposals);
    }
    
    g_mock_state->ike_cfg_destroy_count++;
    g_mock_state->total_deallocations++;
    g_mock_state->current_allocations--;
    
    free(this);
}

static char* mock_ike_cfg_get_name(mock_ike_cfg_t *this)
{
    return this ? this->name : NULL;
}

static void mock_ike_cfg_add_proposal(mock_ike_cfg_t *this, mock_proposal_t *proposal)
{
    if (!this || !proposal) return;
    
    if (!this->proposals) {
        this->proposals = mock_linked_list_create();
    }
    
    this->proposals->insert_last(this->proposals, proposal);
}

mock_ike_cfg_t* mock_ike_cfg_create(const char *name)
{
    if (g_mock_config && (g_mock_config->should_fail_ike_cfg_create || g_mock_config->should_fail_allocations)) {
        return NULL;
    }
    
    mock_ike_cfg_t *cfg = malloc(sizeof(mock_ike_cfg_t));
    if (!cfg) return NULL;
    
    cfg->destroy = mock_ike_cfg_destroy;
    cfg->get_name = mock_ike_cfg_get_name;
    cfg->add_proposal = mock_ike_cfg_add_proposal;
    
    cfg->name = name ? strdup(name) : NULL;
    cfg->my_hosts = mock_linked_list_create();
    cfg->other_hosts = mock_linked_list_create();
    cfg->proposals = mock_linked_list_create();
    cfg->my_port = 500;
    cfg->other_port = 500;
    cfg->ike_version = 2;
    
    // Track call
    g_mock_state->ike_cfg_create_count++;
    if (g_mock_state->last_ike_cfg_name) {
        free(g_mock_state->last_ike_cfg_name);
    }
    g_mock_state->last_ike_cfg_name = name ? strdup(name) : NULL;
    
    g_mock_state->total_allocations++;
    g_mock_state->current_allocations++;
    
    return cfg;
}

/*
 * ============================================================================
 * Mock peer_cfg_t Implementation
 * ============================================================================
 */

static void mock_peer_cfg_destroy(mock_peer_cfg_t *this)
{
    if (!this) return;
    
    if (this->name) {
        free(this->name);
    }
    if (this->ike_cfg) {
        // Note: We don't destroy ike_cfg here as it may be shared
    }
    if (this->local_auth_cfgs) {
        this->local_auth_cfgs->destroy(this->local_auth_cfgs);
    }
    if (this->remote_auth_cfgs) {
        this->remote_auth_cfgs->destroy(this->remote_auth_cfgs);
    }
    if (this->child_cfgs) {
        this->child_cfgs->destroy(this->child_cfgs);
    }
    
    g_mock_state->peer_cfg_destroy_count++;
    g_mock_state->total_deallocations++;
    g_mock_state->current_allocations--;
    
    free(this);
}

static char* mock_peer_cfg_get_name(mock_peer_cfg_t *this)
{
    return this ? this->name : NULL;
}

static mock_ike_cfg_t* mock_peer_cfg_get_ike_cfg(mock_peer_cfg_t *this)
{
    return this ? this->ike_cfg : NULL;
}

static void mock_peer_cfg_add_auth_cfg(mock_peer_cfg_t *this, mock_auth_cfg_t *cfg, bool local)
{
    if (!this || !cfg) return;
    
    mock_linked_list_t *list = local ? this->local_auth_cfgs : this->remote_auth_cfgs;
    if (list) {
        list->insert_last(list, cfg);
    }
}

static void mock_peer_cfg_add_child_cfg(mock_peer_cfg_t *this, mock_child_cfg_t *cfg)
{
    if (!this || !cfg) return;
    
    if (this->child_cfgs) {
        this->child_cfgs->insert_last(this->child_cfgs, cfg);
    }
}

static mock_enumerator_t* mock_peer_cfg_create_child_cfg_enumerator(mock_peer_cfg_t *this)
{
    return this && this->child_cfgs ? this->child_cfgs->create_enumerator(this->child_cfgs) : NULL;
}

mock_peer_cfg_t* mock_peer_cfg_create(const char *name, mock_ike_cfg_t *ike_cfg)
{
    if (g_mock_config && (g_mock_config->should_fail_peer_cfg_create || g_mock_config->should_fail_allocations)) {
        return NULL;
    }
    
    mock_peer_cfg_t *cfg = malloc(sizeof(mock_peer_cfg_t));
    if (!cfg) return NULL;
    
    cfg->destroy = mock_peer_cfg_destroy;
    cfg->get_name = mock_peer_cfg_get_name;
    cfg->get_ike_cfg = mock_peer_cfg_get_ike_cfg;
    cfg->add_auth_cfg = mock_peer_cfg_add_auth_cfg;
    cfg->add_child_cfg = mock_peer_cfg_add_child_cfg;
    cfg->create_child_cfg_enumerator = mock_peer_cfg_create_child_cfg_enumerator;
    
    cfg->name = name ? strdup(name) : NULL;
    cfg->ike_cfg = ike_cfg;
    cfg->local_auth_cfgs = mock_linked_list_create();
    cfg->remote_auth_cfgs = mock_linked_list_create();
    cfg->child_cfgs = mock_linked_list_create();
    
    // Track call
    g_mock_state->peer_cfg_create_count++;
    if (g_mock_state->last_peer_cfg_name) {
        free(g_mock_state->last_peer_cfg_name);
    }
    g_mock_state->last_peer_cfg_name = name ? strdup(name) : NULL;
    
    g_mock_state->total_allocations++;
    g_mock_state->current_allocations++;
    
    return cfg;
}

/*
 * ============================================================================
 * Mock child_cfg_t Implementation
 * ============================================================================
 */

static void mock_child_cfg_destroy(mock_child_cfg_t *this)
{
    if (!this) return;
    
    if (this->name) {
        free(this->name);
    }
    if (this->proposals) {
        this->proposals->destroy(this->proposals);
    }
    if (this->my_ts) {
        this->my_ts->destroy(this->my_ts);
    }
    if (this->other_ts) {
        this->other_ts->destroy(this->other_ts);
    }
    
    g_mock_state->child_cfg_destroy_count++;
    g_mock_state->total_deallocations++;
    g_mock_state->current_allocations--;
    
    free(this);
}

static char* mock_child_cfg_get_name(mock_child_cfg_t *this)
{
    return this ? this->name : NULL;
}

static void mock_child_cfg_add_proposal(mock_child_cfg_t *this, mock_proposal_t *proposal)
{
    if (!this || !proposal) return;
    
    if (!this->proposals) {
        this->proposals = mock_linked_list_create();
    }
    
    this->proposals->insert_last(this->proposals, proposal);
}

static void mock_child_cfg_add_traffic_selector(mock_child_cfg_t *this, bool local, mock_traffic_selector_t *ts)
{
    if (!this || !ts) return;
    
    mock_linked_list_t *list = local ? this->my_ts : this->other_ts;
    if (!list) {
        list = mock_linked_list_create();
        if (local) {
            this->my_ts = list;
        } else {
            this->other_ts = list;
        }
    }
    
    list->insert_last(list, ts);
}

static mock_enumerator_t* mock_child_cfg_create_proposal_enumerator(mock_child_cfg_t *this)
{
    return this && this->proposals ? this->proposals->create_enumerator(this->proposals) : NULL;
}

mock_child_cfg_t* mock_child_cfg_create(const char *name)
{
    if (g_mock_config && (g_mock_config->should_fail_child_cfg_create || g_mock_config->should_fail_allocations)) {
        return NULL;
    }
    
    mock_child_cfg_t *cfg = malloc(sizeof(mock_child_cfg_t));
    if (!cfg) return NULL;
    
    cfg->destroy = mock_child_cfg_destroy;
    cfg->get_name = mock_child_cfg_get_name;
    cfg->add_proposal = mock_child_cfg_add_proposal;
    cfg->add_traffic_selector = mock_child_cfg_add_traffic_selector;
    cfg->create_proposal_enumerator = mock_child_cfg_create_proposal_enumerator;
    
    cfg->name = name ? strdup(name) : NULL;
    cfg->proposals = mock_linked_list_create();
    cfg->my_ts = mock_linked_list_create();
    cfg->other_ts = mock_linked_list_create();
    
    // Track call
    g_mock_state->child_cfg_create_count++;
    if (g_mock_state->last_child_cfg_name) {
        free(g_mock_state->last_child_cfg_name);
    }
    g_mock_state->last_child_cfg_name = name ? strdup(name) : NULL;
    
    g_mock_state->total_allocations++;
    g_mock_state->current_allocations++;
    
    return cfg;
}

/*
 * ============================================================================
 * Additional Mock Object Implementations (Simplified)
 * ============================================================================
 */

mock_auth_cfg_t* mock_auth_cfg_create(void)
{
    mock_auth_cfg_t *cfg = malloc(sizeof(mock_auth_cfg_t));
    if (!cfg) return NULL;
    
    // Basic implementation - expand as needed
    cfg->entries = NULL;
    cfg->entry_count = 0;
    
    g_mock_state->auth_cfg_create_count++;
    g_mock_state->total_allocations++;
    g_mock_state->current_allocations++;
    
    return cfg;
}

mock_identification_t* mock_identification_create(const char *id_str, int type)
{
    mock_identification_t *id = malloc(sizeof(mock_identification_t));
    if (!id) return NULL;
    
    id->id_str = id_str ? strdup(id_str) : NULL;
    id->id_type = type;
    
    g_mock_state->total_allocations++;
    g_mock_state->current_allocations++;
    
    return id;
}

mock_traffic_selector_t* mock_traffic_selector_create(const char *from_addr, const char *to_addr, 
                                                     uint16_t from_port, uint16_t to_port)
{
    mock_traffic_selector_t *ts = malloc(sizeof(mock_traffic_selector_t));
    if (!ts) return NULL;
    
    ts->from_addr = from_addr ? strdup(from_addr) : NULL;
    ts->to_addr = to_addr ? strdup(to_addr) : NULL;
    ts->from_port = from_port;
    ts->to_port = to_port;
    ts->protocol = 0; // ANY
    
    g_mock_state->total_allocations++;
    g_mock_state->current_allocations++;
    
    return ts;
}

mock_proposal_t* mock_proposal_create(const char *proposal_str, int protocol_id)
{
    mock_proposal_t *proposal = malloc(sizeof(mock_proposal_t));
    if (!proposal) return NULL;
    
    proposal->proposal_str = proposal_str ? strdup(proposal_str) : NULL;
    proposal->protocol_id = protocol_id;
    
    g_mock_state->total_allocations++;
    g_mock_state->current_allocations++;
    
    return proposal;
}

/*
 * ============================================================================
 * Mock State Verification Functions
 * ============================================================================
 */

bool mock_verify_ike_cfg_create_called(void)
{
    return g_mock_state && g_mock_state->ike_cfg_create_count > 0;
}

bool mock_verify_peer_cfg_create_called(void)
{
    return g_mock_state && g_mock_state->peer_cfg_create_count > 0;
}

bool mock_verify_child_cfg_create_called(void)
{
    return g_mock_state && g_mock_state->child_cfg_create_count > 0;
}

int mock_get_ike_cfg_create_count(void)
{
    return g_mock_state ? g_mock_state->ike_cfg_create_count : 0;
}

int mock_get_peer_cfg_create_count(void)
{
    return g_mock_state ? g_mock_state->peer_cfg_create_count : 0;
}

int mock_get_child_cfg_create_count(void)
{
    return g_mock_state ? g_mock_state->child_cfg_create_count : 0;
}

const char* mock_get_last_ike_cfg_name(void)
{
    return g_mock_state ? g_mock_state->last_ike_cfg_name : NULL;
}

const char* mock_get_last_peer_cfg_name(void)
{
    return g_mock_state ? g_mock_state->last_peer_cfg_name : NULL;
}

const char* mock_get_last_child_cfg_name(void)
{
    return g_mock_state ? g_mock_state->last_child_cfg_name : NULL;
}

/*
 * ============================================================================
 * Memory Tracking Functions
 * ============================================================================
 */

bool mock_verify_no_memory_leaks(void)
{
    return g_mock_state && g_mock_state->current_allocations == 0;
}

int mock_get_current_allocation_count(void)
{
    return g_mock_state ? g_mock_state->current_allocations : 0;
}

int mock_get_total_allocation_count(void)
{
    return g_mock_state ? g_mock_state->total_allocations : 0;
}

int mock_get_total_deallocation_count(void)
{
    return g_mock_state ? g_mock_state->total_deallocations : 0;
}

/*
 * ============================================================================
 * Parameter Capture Functions
 * ============================================================================
 */

void mock_capture_string_param(const char *str)
{
    if (!g_mock_state || g_mock_state->params.capture_count >= 10) return;
    
    int index = g_mock_state->params.capture_count++;
    g_mock_state->params.captured_strings[index] = str ? strdup(str) : NULL;
}

void mock_capture_int_param(int value)
{
    if (!g_mock_state || g_mock_state->params.capture_count >= 10) return;
    
    int index = g_mock_state->params.capture_count++;
    g_mock_state->params.captured_ints[index] = value;
}

void mock_capture_ptr_param(void *ptr)
{
    if (!g_mock_state || g_mock_state->params.capture_count >= 10) return;
    
    int index = g_mock_state->params.capture_count++;
    g_mock_state->params.captured_ptrs[index] = ptr;
}

const char* mock_get_captured_string(int index)
{
    if (!g_mock_state || index >= g_mock_state->params.capture_count) return NULL;
    return g_mock_state->params.captured_strings[index];
}

int mock_get_captured_int(int index)
{
    if (!g_mock_state || index >= g_mock_state->params.capture_count) return 0;
    return g_mock_state->params.captured_ints[index];
}

void* mock_get_captured_ptr(int index)
{
    if (!g_mock_state || index >= g_mock_state->params.capture_count) return NULL;
    return g_mock_state->params.captured_ptrs[index];
}

int mock_get_capture_count(void)
{
    return g_mock_state ? g_mock_state->params.capture_count : 0;
}

/*
 * ============================================================================
 * Mock Configuration Functions
 * ============================================================================
 */

void mock_set_allocation_failure(bool should_fail)
{
    if (g_mock_config) {
        g_mock_config->should_fail_allocations = should_fail;
    }
}

void mock_set_max_allocations(int max_allocs)
{
    if (g_mock_config) {
        g_mock_config->max_allocations = max_allocs;
    }
}

void mock_set_allocation_failure_at(int failure_point)
{
    if (g_mock_config) {
        g_mock_config->allocation_failure_at = failure_point;
    }
}

void mock_enable_slow_operations(bool enable, int delay_ms)
{
    if (g_mock_config) {
        g_mock_config->simulate_slow_operations = enable;
        g_mock_config->operation_delay_ms = delay_ms;
    }
}

void mock_reset_config(void)
{
    if (!g_mock_config) return;
    
    memset(g_mock_config, 0, sizeof(mock_config_t));
    g_mock_config->max_allocations = -1; // unlimited
}