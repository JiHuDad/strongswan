/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Mock version of extsock_json_parser for adapter unit tests
 * TASK-007: JSON Parser 실제 테스트
 * 
 * This is a simplified mock implementation that doesn't require cJSON
 * but provides the same interface for testing adapter layer functionality.
 */

#ifndef EXTSOCK_JSON_PARSER_MOCK_H_
#define EXTSOCK_JSON_PARSER_MOCK_H_

#include <stdbool.h>

// Mock types for testing (avoiding strongSwan dependencies)
typedef struct ike_cfg_t ike_cfg_t;
typedef struct auth_cfg_t auth_cfg_t;  
typedef struct linked_list_t linked_list_t;
typedef struct peer_cfg_t peer_cfg_t;
typedef struct extsock_config_entity_t extsock_config_entity_t;
typedef struct proposal_t proposal_t;
typedef struct traffic_selector_t traffic_selector_t;
typedef struct child_cfg_t child_cfg_t;

// Mock protocol IDs
typedef enum {
    PROTO_IKE = 1,
    PROTO_AH = 51,
    PROTO_ESP = 50
} protocol_id_t;

// Forward declarations for mock cJSON types
typedef struct mock_cJSON mock_cJSON;

/**
 * JSON 파싱 어댑터 (Mock version)
 * JSON 형태의 설정을 strongSwan 객체로 변환
 */
typedef struct extsock_json_parser_t {
    
    /**
     * IKE 설정 파싱
     */
    ike_cfg_t* (*parse_ike_config)(struct extsock_json_parser_t *this, mock_cJSON *ike_json);
    
    /**
     * 인증 설정 파싱
     */
    auth_cfg_t* (*parse_auth_config)(struct extsock_json_parser_t *this, 
                                    mock_cJSON *auth_json, bool is_local);
    
    /**
     * 제안(Proposals) 파싱
     */
    linked_list_t* (*parse_proposals)(struct extsock_json_parser_t *this,
                                     mock_cJSON *json_array, 
                                     protocol_id_t proto, 
                                     bool is_ike);
    
    /**
     * 트래픽 셀렉터 파싱
     */
    linked_list_t* (*parse_traffic_selectors)(struct extsock_json_parser_t *this,
                                             mock_cJSON *json_array);
    
    /**
     * Child SA 설정 파싱
     */
    bool (*parse_child_configs)(struct extsock_json_parser_t *this,
                               peer_cfg_t *peer_cfg,
                               mock_cJSON *children_json);
    
    /**
     * 전체 설정 엔티티 파싱
     */
    extsock_config_entity_t* (*parse_config_entity)(struct extsock_json_parser_t *this,
                                                   const char *config_json);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(struct extsock_json_parser_t *this);
} extsock_json_parser_t;

/**
 * Mock cJSON structure for testing
 */
struct mock_cJSON {
    char *valuestring;
    int valueint;
    int type;
    struct mock_cJSON *next;
    struct mock_cJSON *child;
    char context_str[256]; // Store original JSON for context
};

// Mock cJSON types
#define MOCK_cJSON_Invalid 0
#define MOCK_cJSON_False   1
#define MOCK_cJSON_True    2
#define MOCK_cJSON_NULL    3
#define MOCK_cJSON_Number  4
#define MOCK_cJSON_String  5
#define MOCK_cJSON_Array   6
#define MOCK_cJSON_Object  7

/**
 * Mock JSON creation functions
 */
mock_cJSON *mock_cJSON_Parse(const char *value);
mock_cJSON *mock_cJSON_CreateObject(void);
mock_cJSON *mock_cJSON_CreateArray(void);
mock_cJSON *mock_cJSON_CreateString(const char *string);
mock_cJSON *mock_cJSON_CreateNumber(double num);
mock_cJSON *mock_cJSON_GetObjectItem(const mock_cJSON *object, const char *string);
void mock_cJSON_Delete(mock_cJSON *c);
bool mock_cJSON_IsArray(const mock_cJSON *item);
bool mock_cJSON_IsObject(const mock_cJSON *item);
bool mock_cJSON_IsString(const mock_cJSON *item);
bool mock_cJSON_IsNumber(const mock_cJSON *item);
int mock_cJSON_GetArraySize(const mock_cJSON *array);

/**
 * JSON 파싱 어댑터 생성 (Mock version)
 */
extsock_json_parser_t *extsock_json_parser_create();

#endif /** EXTSOCK_JSON_PARSER_MOCK_H_ @}*/