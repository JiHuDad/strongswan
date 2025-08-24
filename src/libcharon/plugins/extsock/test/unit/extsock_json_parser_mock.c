/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Mock implementation of extsock_json_parser for adapter unit tests
 * TASK-007: JSON Parser 실제 테스트
 * 
 * This mock implementation focuses on testing the adapter layer behavior
 * without external dependencies like cJSON.
 */

#include "extsock_json_parser_mock.h"
#include "../infrastructure/strongswan_mocks.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Mock strongSwan constants and types
#define IKE_ANY 0
#define IKEV2_UDP_PORT 500
#define AUTH_CLASS_PSK 1
#define AUTH_CLASS_PUBKEY 2
#define AUTH_RULE_AUTH_CLASS 1
#define AUTH_RULE_IDENTITY 2
#define SHARED_IKE 1
#define ACTION_NONE 0
#define DSCP_COPY_OUT_ONLY 0

// Mock initialization macro (simplified)
#define INIT(obj, ...) do { \
    obj = malloc(sizeof(*(obj))); \
} while(0)

// METHOD macro for function definitions
#define METHOD(interface, name, ret, ...) \
    static ret name(__VA_ARGS__)

typedef struct private_extsock_json_parser_mock_t private_extsock_json_parser_mock_t;

/**
 * Mock JSON parser private structure
 */
struct private_extsock_json_parser_mock_t {
    /**
     * Public interface
     */
    extsock_json_parser_t public;
    
    /**
     * Mock call counter
     */
    int parse_calls;
};

/*
 * ============================================================================
 * Mock cJSON Implementation
 * ============================================================================
 */

/**
 * Simple mock cJSON parser that creates predictable test data
 */
mock_cJSON *mock_cJSON_Parse(const char *value)
{
    if (!value) return NULL;
    
    mock_cJSON *json = malloc(sizeof(mock_cJSON));
    if (!json) return NULL;
    
    memset(json, 0, sizeof(mock_cJSON));
    json->type = MOCK_cJSON_Object;
    
    // Store the original JSON string for context
    if (strlen(value) < sizeof(json->context_str) - 1) {
        strcpy(json->context_str, value);
    }
    
    // Create predictable test data based on input
    if (strstr(value, "\"local_addrs\"")) {
        json->type = MOCK_cJSON_Object;
        // Mock valid IKE config
    } else if (strstr(value, "\"auth\"")) {
        json->type = MOCK_cJSON_Object;
        // Mock valid auth config
    } else if (value[0] == '[') {
        json->type = MOCK_cJSON_Array;
        // Mock array
    } else if (strstr(value, "incomplete")) {
        // Invalid JSON
        free(json);
        return NULL;
    }
    
    return json;
}

mock_cJSON *mock_cJSON_CreateObject(void)
{
    mock_cJSON *json = malloc(sizeof(mock_cJSON));
    if (json) {
        memset(json, 0, sizeof(mock_cJSON));
        json->type = MOCK_cJSON_Object;
    }
    return json;
}

mock_cJSON *mock_cJSON_CreateArray(void)
{
    mock_cJSON *json = malloc(sizeof(mock_cJSON));
    if (json) {
        memset(json, 0, sizeof(mock_cJSON));
        json->type = MOCK_cJSON_Array;
    }
    return json;
}

mock_cJSON *mock_cJSON_CreateString(const char *string)
{
    mock_cJSON *json = malloc(sizeof(mock_cJSON));
    if (json && string) {
        memset(json, 0, sizeof(mock_cJSON));
        json->type = MOCK_cJSON_String;
        json->valuestring = strdup(string);
    }
    return json;
}

mock_cJSON *mock_cJSON_CreateNumber(double num)
{
    mock_cJSON *json = malloc(sizeof(mock_cJSON));
    if (json) {
        memset(json, 0, sizeof(mock_cJSON));
        json->type = MOCK_cJSON_Number;
        json->valueint = (int)num;
    }
    return json;
}

mock_cJSON *mock_cJSON_GetObjectItem(const mock_cJSON *object, const char *string)
{
    if (!object || !string) return NULL;
    
    // Return mock items based on key
    if (strcmp(string, "local_addrs") == 0) {
        return mock_cJSON_CreateString("192.168.1.100");
    } else if (strcmp(string, "remote_addrs") == 0) {
        return mock_cJSON_CreateString("203.0.113.5");
    } else if (strcmp(string, "version") == 0) {
        return mock_cJSON_CreateNumber(2);
    } else if (strcmp(string, "auth") == 0) {
        // Check if the parent object contains "invalid_type" in its context
        if (object && strstr(object->context_str, "invalid_type")) {
            return mock_cJSON_CreateString("invalid_type");
        } else {
            return mock_cJSON_CreateString("psk");
        }
    } else if (strcmp(string, "id") == 0) {
        return mock_cJSON_CreateString("client@strongswan.org");
    } else if (strcmp(string, "secret") == 0) {
        return mock_cJSON_CreateString("test-preshared-key-123");
    } else if (strcmp(string, "name") == 0) {
        return mock_cJSON_CreateString("test-child");
    }
    
    return NULL;
}

void mock_cJSON_Delete(mock_cJSON *c)
{
    if (c) {
        free(c->valuestring);
        if (c->child) mock_cJSON_Delete(c->child);
        if (c->next) mock_cJSON_Delete(c->next);
        free(c);
    }
}

bool mock_cJSON_IsArray(const mock_cJSON *item)
{
    return item && item->type == MOCK_cJSON_Array;
}

bool mock_cJSON_IsObject(const mock_cJSON *item)
{
    return item && item->type == MOCK_cJSON_Object;
}

bool mock_cJSON_IsString(const mock_cJSON *item)
{
    return item && item->type == MOCK_cJSON_String;
}

bool mock_cJSON_IsNumber(const mock_cJSON *item)
{
    return item && item->type == MOCK_cJSON_Number;
}

int mock_cJSON_GetArraySize(const mock_cJSON *array)
{
    if (!mock_cJSON_IsArray(array)) return 0;
    return 3;  // Return predictable size for testing
}

/*
 * ============================================================================
 * Mock JSON Parser Implementation
 * ============================================================================
 */

METHOD(extsock_json_parser_t, parse_ike_config, ike_cfg_t *,
    private_extsock_json_parser_mock_t *this, mock_cJSON *ike_json)
{
    this->parse_calls++;
    
    if (!ike_json) {
        return NULL;
    }
    
    // Create mock IKE config using strongSwan mock system
    ike_cfg_t *ike_cfg = (ike_cfg_t*)mock_ike_cfg_create("mock-ike");
    
    return ike_cfg;
}

METHOD(extsock_json_parser_t, parse_auth_config, auth_cfg_t *,
    private_extsock_json_parser_mock_t *this, mock_cJSON *auth_json, bool is_local)
{
    this->parse_calls++;
    
    if (!auth_json) {
        return NULL;
    }
    
    auth_cfg_t *auth_cfg = (auth_cfg_t*)mock_auth_cfg_create();
    if (!auth_cfg) {
        return NULL;
    }
    
    // Mock authentication setup - validate JSON structure only
    mock_cJSON *auth_type = mock_cJSON_GetObjectItem(auth_json, "auth");
    if (auth_type && auth_type->valuestring) {
        if (strcmp(auth_type->valuestring, "psk") == 0) {
            // Valid PSK auth
        } else if (strcmp(auth_type->valuestring, "pubkey") == 0) {
            // Valid pubkey auth
        } else {
            // Unsupported auth type - free auth_cfg and return NULL
            free(auth_cfg);
            return NULL;
        }
    }
    
    return auth_cfg;
}

METHOD(extsock_json_parser_t, parse_proposals, linked_list_t *,
    private_extsock_json_parser_mock_t *this, mock_cJSON *json_array, 
    protocol_id_t proto, bool is_ike)
{
    this->parse_calls++;
    
    linked_list_t *proposals_list = (linked_list_t*)mock_linked_list_create();
    if (!proposals_list) return NULL;
    
    // Create mock proposals using strongSwan mock system
    if (json_array && mock_cJSON_IsArray(json_array)) {
        // Mock parsing array items
        for (int i = 0; i < 3; i++) {
            void *p = mock_proposal_create("aes256-sha256", proto);
            if (p) {
                // Mock insert - just ignore for now, testing logic only
            }
        }
    } else {
        // Create default proposals using mock system
        void *first = mock_proposal_create("aes256-sha256", proto);
        // Mock operations - just test logic, ignore API calls
        (void)first;
        void *second = mock_proposal_create("aes128-sha1", proto);  
        (void)second;
    }
    
    return proposals_list;
}

METHOD(extsock_json_parser_t, parse_traffic_selectors, linked_list_t *,
    private_extsock_json_parser_mock_t *this, mock_cJSON *json_array)
{
    this->parse_calls++;
    
    linked_list_t *ts_list = (linked_list_t*)mock_linked_list_create();
    if (!ts_list) return NULL;
    
    if (json_array && mock_cJSON_IsArray(json_array)) {
        // Mock parsing TS array
        for (int i = 0; i < 3; i++) {
            void *ts = mock_traffic_selector_create("192.168.1.0", "192.168.1.255", 0, 65535);
            if (ts) {
                // Mock insert - just ignore
            }
        }
    } else {
        // Create default dynamic TS
        void *ts = mock_traffic_selector_create("0.0.0.0", "255.255.255.255", 0, 65535);
        // Mock operation - just ignore
        (void)ts;
    }
    
    return ts_list;
}

METHOD(extsock_json_parser_t, parse_child_configs, bool,
    private_extsock_json_parser_mock_t *this, peer_cfg_t *peer_cfg, mock_cJSON *children_json_array)
{
    this->parse_calls++;
    
    if (!children_json_array || !mock_cJSON_IsArray(children_json_array)) {
        return true;  // No children to process
    }
    
    // Mock child config creation using strongSwan mock system
    child_cfg_t *child_cfg = (child_cfg_t*)mock_child_cfg_create("mock-child");
    if (child_cfg) {
        // Mock operation - just ignore
        (void)peer_cfg;
        (void)child_cfg;
        return true;
    }
    
    return false;
}

METHOD(extsock_json_parser_t, parse_config_entity, extsock_config_entity_t *,
    private_extsock_json_parser_mock_t *this, const char *config_json)
{
    this->parse_calls++;
    return NULL;  // Not implemented
}

METHOD(extsock_json_parser_t, destroy, void,
    private_extsock_json_parser_mock_t *this)
{
    free(this);
}

/**
 * Mock JSON parser creation
 */
extsock_json_parser_t *extsock_json_parser_create()
{
    private_extsock_json_parser_mock_t *this;

    INIT(this);
    if (this) {
        this->public.parse_ike_config = (void*)parse_ike_config;
        this->public.parse_auth_config = (void*)parse_auth_config;
        this->public.parse_proposals = (void*)parse_proposals;
        this->public.parse_traffic_selectors = (void*)parse_traffic_selectors;
        this->public.parse_child_configs = (void*)parse_child_configs;
        this->public.parse_config_entity = (void*)parse_config_entity;
        this->public.destroy = (void*)destroy;
        this->parse_calls = 0;
    }

    return &this->public;
}