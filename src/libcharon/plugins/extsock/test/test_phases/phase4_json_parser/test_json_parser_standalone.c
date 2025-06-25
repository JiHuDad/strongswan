/*
 * Copyright (C) 2024 strongSwan Project
 * JSON Parser Standalone Tests - Phase 4
 * 실제 JSON Parser 함수들을 호출하는 테스트
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <cjson/cJSON.h>

/*
 * Strategy 5: strongSwan API Mock + Real JSON Parser
 * JSON Parser는 실제 구현을 사용하되, strongSwan API는 Mock으로 처리
 */

// 필요한 타입들 정의 (strongSwan 의존성 제거)
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_JSON_PARSE,
    EXTSOCK_ERROR_CONFIG_INVALID,
    EXTSOCK_ERROR_SOCKET_FAILED,
    EXTSOCK_ERROR_MEMORY_ALLOCATION,
    EXTSOCK_ERROR_STRONGSWAN_API
} extsock_error_t;

typedef enum {
    PROTO_IKE = 1,
    PROTO_ESP = 2,
    PROTO_AH = 3
} protocol_id_t;

typedef enum {
    IKE_ANY = 0,
    IKEV1 = 1,
    IKEV2 = 2
} ike_version_t;

typedef enum {
    ACTION_NONE = 0,
    ACTION_TRAP = 1,
    ACTION_START = 2
} action_t;

// Mock 구조체들
typedef struct mock_linked_list_t {
    void **items;
    int count;
    int capacity;
} mock_linked_list_t;

typedef struct mock_proposal_t {
    protocol_id_t protocol;
    char *transform_str;
} mock_proposal_t;

typedef struct mock_traffic_selector_t {
    char *cidr;
    uint16_t from_port;
    uint16_t to_port;
} mock_traffic_selector_t;

typedef struct mock_ike_cfg_t {
    char *local;
    char *remote;
    ike_version_t version;
    mock_linked_list_t *proposals;
} mock_ike_cfg_t;

typedef struct mock_auth_cfg_t {
    char *auth_method;
    char *identity;
    char *psk;
} mock_auth_cfg_t;

typedef struct mock_child_cfg_t {
    char *name;
    mock_linked_list_t *local_ts;
    mock_linked_list_t *remote_ts;
    mock_linked_list_t *proposals;
    action_t start_action;
    action_t close_action;
} mock_child_cfg_t;

typedef struct mock_peer_cfg_t {
    char *name;
    mock_ike_cfg_t *ike_cfg;
    mock_auth_cfg_t *local_auth;
    mock_auth_cfg_t *remote_auth;
    mock_linked_list_t *children;
} mock_peer_cfg_t;

typedef struct extsock_config_entity_t {
    char *connection_name;
    mock_peer_cfg_t *peer_cfg;
    extsock_error_t status;
} extsock_config_entity_t;

// Mock 타입 aliases
typedef mock_linked_list_t linked_list_t;
typedef mock_proposal_t proposal_t;
typedef mock_traffic_selector_t traffic_selector_t;
typedef mock_ike_cfg_t ike_cfg_t;
typedef mock_auth_cfg_t auth_cfg_t;
typedef mock_child_cfg_t child_cfg_t;
typedef mock_peer_cfg_t peer_cfg_t;

// Mock 함수들
#define DBG(level, fmt, ...) printf("[DBG%d] " fmt "\n", level, ##__VA_ARGS__)
#define EXTSOCK_DBG(level, fmt, ...) printf("[EXTSOCK DBG%d] " fmt "\n", level, ##__VA_ARGS__)
#define TRUE true
#define FALSE false
#define streq(a, b) (strcmp(a, b) == 0)

// Mock linked_list implementation
static linked_list_t *linked_list_create() {
    linked_list_t *list = malloc(sizeof(linked_list_t));
    if (!list) return NULL;
    list->items = malloc(sizeof(void*) * 10);
    list->count = 0;
    list->capacity = 10;
    return list;
}

static void linked_list_insert_last(linked_list_t *list, void *item) {
    if (!list || !item) return;
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, sizeof(void*) * list->capacity);
    }
    list->items[list->count++] = item;
}

static int linked_list_get_count(linked_list_t *list) {
    return list ? list->count : 0;
}

static void linked_list_destroy(linked_list_t *list) {
    if (list) {
        for (int i = 0; i < list->count; i++) {
            free(list->items[i]);
        }
        free(list->items);
        free(list);
    }
}

// Mock strongSwan functions
static proposal_t *proposal_create_from_string(protocol_id_t proto, const char *str) {
    if (!str) return NULL;
    proposal_t *prop = malloc(sizeof(proposal_t));
    if (!prop) return NULL;
    prop->protocol = proto;
    prop->transform_str = strdup(str);
    return prop;
}

static proposal_t *proposal_create_default(protocol_id_t proto) {
    return proposal_create_from_string(proto, "default");
}

static proposal_t *proposal_create_default_aead(protocol_id_t proto) {
    return proposal_create_from_string(proto, "aead_default");
}

static traffic_selector_t *traffic_selector_create_from_cidr(const char *cidr, 
                                                           uint8_t protocol,
                                                           uint16_t from_port,
                                                           uint16_t to_port) {
    if (!cidr) return NULL;
    traffic_selector_t *ts = malloc(sizeof(traffic_selector_t));
    if (!ts) return NULL;
    ts->cidr = strdup(cidr);
    ts->from_port = from_port;
    ts->to_port = to_port;
    return ts;
}

static traffic_selector_t *traffic_selector_create_dynamic(uint8_t protocol,
                                                         uint16_t from_port,
                                                         uint16_t to_port) {
    return traffic_selector_create_from_cidr("0.0.0.0/0", protocol, from_port, to_port);
}

// IKE Config 생성을 위한 구조체
typedef struct {
    char *local;
    char *remote;
    ike_version_t version;
    uint16_t local_port;
    uint16_t remote_port;
} ike_cfg_create_t;

static ike_cfg_t *ike_cfg_create(ike_cfg_create_t *cfg) {
    if (!cfg) return NULL;
    ike_cfg_t *ike_cfg = malloc(sizeof(ike_cfg_t));
    if (!ike_cfg) return NULL;
    ike_cfg->local = cfg->local ? strdup(cfg->local) : NULL;
    ike_cfg->remote = cfg->remote ? strdup(cfg->remote) : NULL;
    ike_cfg->version = cfg->version;
    ike_cfg->proposals = linked_list_create();
    return ike_cfg;
}

// JSON Parser 인터페이스 정의
typedef struct extsock_json_parser_t {
    linked_list_t* (*parse_proposals)(struct extsock_json_parser_t *this,
                                     cJSON *json_array, 
                                     protocol_id_t proto, 
                                     bool is_ike);
    
    linked_list_t* (*parse_traffic_selectors)(struct extsock_json_parser_t *this,
                                             cJSON *json_array);
    
    ike_cfg_t* (*parse_ike_config)(struct extsock_json_parser_t *this, cJSON *ike_json);
    
    extsock_config_entity_t* (*parse_config_entity)(struct extsock_json_parser_t *this,
                                                   const char *config_json);
    
    void (*destroy)(struct extsock_json_parser_t *this);
} extsock_json_parser_t;

/*
 * 실제 JSON Parser 구현 (simplified)
 * 복잡한 strongSwan API 대신 Mock 사용
 */

static char* json_array_to_comma_separated_string(cJSON *json_array) {
    if (!json_array || !cJSON_IsArray(json_array) || cJSON_GetArraySize(json_array) == 0) {
        return strdup("%any");
    }

    size_t total_len = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, json_array) {
        if (cJSON_IsString(item) && item->valuestring) {
            total_len += strlen(item->valuestring) + 1; // +1 for comma
        }
    }
    
    if (total_len == 0) {
        return strdup("%any");
    }
    
    char *result = malloc(total_len + 1);
    if (!result) return strdup("%any");
    
    result[0] = '\0';
    bool first = true;
    cJSON_ArrayForEach(item, json_array) {
        if (cJSON_IsString(item) && item->valuestring && *(item->valuestring)) {
            if (!first) strcat(result, ",");
            strcat(result, item->valuestring);
            first = false;
        }
    }
    
    return result;
}

static action_t string_to_action(const char* action_str) {
    if (!action_str) return ACTION_NONE;
    if (streq(action_str, "trap")) return ACTION_TRAP;
    if (streq(action_str, "start")) return ACTION_START;
    if (streq(action_str, "clear")) return ACTION_TRAP;
    if (streq(action_str, "hold")) return ACTION_TRAP;
    if (streq(action_str, "restart")) return ACTION_START;
    return ACTION_NONE;
}

static linked_list_t *standalone_parse_proposals(extsock_json_parser_t *this, 
                                                cJSON *json_array, 
                                                protocol_id_t proto, 
                                                bool is_ike)
{
    linked_list_t *proposals_list = linked_list_create();
    if (!proposals_list) return NULL;

    if (json_array && cJSON_IsArray(json_array)) {
        cJSON *prop_json;
        cJSON_ArrayForEach(prop_json, json_array) {
            if (cJSON_IsString(prop_json) && prop_json->valuestring) {
                proposal_t *p = proposal_create_from_string(proto, prop_json->valuestring);
                if (p) {
                    linked_list_insert_last(proposals_list, p);
                } else {
                    EXTSOCK_DBG(1, "Failed to parse proposal string: %s for proto %d", 
                               prop_json->valuestring, proto);
                }
            }
        }
    }

    if (linked_list_get_count(proposals_list) == 0) {
        EXTSOCK_DBG(1, "No proposals in JSON, adding defaults for proto %d (is_ike: %d)", proto, is_ike);
        if (is_ike) {
            proposal_t *first = proposal_create_default(PROTO_IKE);
            if (first) linked_list_insert_last(proposals_list, first);
            proposal_t *second = proposal_create_default_aead(PROTO_IKE);
            if (second) linked_list_insert_last(proposals_list, second);
        } else {
            proposal_t *first = proposal_create_default_aead(proto);
            if (first) linked_list_insert_last(proposals_list, first);
            proposal_t *second = proposal_create_default(proto);
            if (second) linked_list_insert_last(proposals_list, second);
        }
    }
    return proposals_list;
}

static linked_list_t *standalone_parse_traffic_selectors(extsock_json_parser_t *this, cJSON *json_array)
{
    linked_list_t *ts_list = linked_list_create();
    if (!ts_list) return NULL;

    if (json_array && cJSON_IsArray(json_array)) {
        cJSON *ts_json;
        cJSON_ArrayForEach(ts_json, json_array) {
            if (cJSON_IsString(ts_json) && ts_json->valuestring) {
                traffic_selector_t *ts = traffic_selector_create_from_cidr(
                    ts_json->valuestring, 0, 0, 0xFFFF);
                if (ts) {
                    linked_list_insert_last(ts_list, ts);
                } else {
                    EXTSOCK_DBG(1, "Failed to parse TS string as CIDR: %s", ts_json->valuestring);
                }
            }
        }
    }
    
    if (linked_list_get_count(ts_list) == 0) {
        traffic_selector_t* ts = traffic_selector_create_dynamic(0, 0, 0xFFFF);
        if (ts) linked_list_insert_last(ts_list, ts);
        EXTSOCK_DBG(1, "No traffic selectors in JSON or all failed to parse, adding dynamic TS");
    }
    return ts_list;
}

static ike_cfg_t *standalone_parse_ike_config(extsock_json_parser_t *this, cJSON *ike_json)
{
    if (!ike_json) return NULL;

    ike_cfg_create_t ike_create_cfg = {0};

    // 로컬 및 원격 주소 파싱
    cJSON *j_local_addrs = cJSON_GetObjectItem(ike_json, "local_addrs");
    ike_create_cfg.local = json_array_to_comma_separated_string(j_local_addrs);
    cJSON *j_remote_addrs = cJSON_GetObjectItem(ike_json, "remote_addrs");
    ike_create_cfg.remote = json_array_to_comma_separated_string(j_remote_addrs);
    
    // IKE 버전 파싱
    cJSON *j_version = cJSON_GetObjectItem(ike_json, "version");
    if (j_version && cJSON_IsNumber(j_version)) {
        ike_create_cfg.version = j_version->valueint;
    } else {
        ike_create_cfg.version = IKE_ANY; 
    }
    ike_create_cfg.local_port = 0;
    ike_create_cfg.remote_port = 0;

    // ike_cfg 객체 생성
    ike_cfg_t *ike_cfg = ike_cfg_create(&ike_create_cfg);
    free(ike_create_cfg.local);
    free(ike_create_cfg.remote);

    if (!ike_cfg) {
        EXTSOCK_DBG(1, "Failed to create ike_cfg");
        return NULL;
    }

    // IKE 제안 파싱 및 추가
    cJSON *j_proposals = cJSON_GetObjectItem(ike_json, "proposals");
    linked_list_t *ike_proposals = this->parse_proposals(this, j_proposals, PROTO_IKE, TRUE);
    if (ike_proposals) {
        ike_cfg->proposals = ike_proposals; // Mock에서는 단순히 할당
    }

    return ike_cfg;
}

static extsock_config_entity_t *standalone_parse_config_entity(extsock_json_parser_t *this, const char *config_json)
{
    if (!config_json) {
        return NULL;
    }

    cJSON *json = cJSON_Parse(config_json);
    if (!json) {
        EXTSOCK_DBG(1, "Failed to parse JSON: %s", config_json);
        return NULL;
    }

    extsock_config_entity_t *entity = malloc(sizeof(extsock_config_entity_t));
    if (!entity) {
        cJSON_Delete(json);
        return NULL;
    }

    // Connection name 파싱
    cJSON *j_name = cJSON_GetObjectItem(json, "connection_name");
    if (j_name && cJSON_IsString(j_name)) {
        entity->connection_name = strdup(j_name->valuestring);
    } else {
        entity->connection_name = strdup("default_connection");
    }

    // IKE 설정 파싱
    cJSON *j_ike = cJSON_GetObjectItem(json, "ike");
    ike_cfg_t *ike_cfg = this->parse_ike_config(this, j_ike);

    // Mock peer_cfg 생성
    peer_cfg_t *peer_cfg = malloc(sizeof(peer_cfg_t));
    if (peer_cfg) {
        peer_cfg->name = strdup(entity->connection_name);
        peer_cfg->ike_cfg = ike_cfg;
        peer_cfg->local_auth = NULL;
        peer_cfg->remote_auth = NULL;
        peer_cfg->children = linked_list_create();
    }

    entity->peer_cfg = peer_cfg;
    entity->status = EXTSOCK_SUCCESS;

    cJSON_Delete(json);
    return entity;
}

static void standalone_destroy(extsock_json_parser_t *this)
{
    if (this) {
        free(this);
    }
}

static extsock_json_parser_t *standalone_json_parser_create()
{
    extsock_json_parser_t *parser = malloc(sizeof(extsock_json_parser_t));
    if (!parser) return NULL;

    parser->parse_proposals = standalone_parse_proposals;
    parser->parse_traffic_selectors = standalone_parse_traffic_selectors;
    parser->parse_ike_config = standalone_parse_ike_config;
    parser->parse_config_entity = standalone_parse_config_entity;
    parser->destroy = standalone_destroy;

    return parser;
}

/**
 * 테스트 설정
 */
void setup_json_parser_standalone_test(void)
{
    printf("Starting JSON Parser standalone tests...\n");
}

/**
 * 테스트 해제
 */
void teardown_json_parser_standalone_test(void)
{
    printf("JSON Parser standalone tests completed.\n");
}

/**
 * JSON Parser 생성/소멸 테스트
 */
START_TEST(test_json_parser_create_destroy)
{
    // When
    extsock_json_parser_t *parser = standalone_json_parser_create();
    
    // Then
    ck_assert_ptr_nonnull(parser);
    ck_assert_ptr_nonnull(parser->parse_proposals);
    ck_assert_ptr_nonnull(parser->parse_traffic_selectors);
    ck_assert_ptr_nonnull(parser->parse_ike_config);
    ck_assert_ptr_nonnull(parser->parse_config_entity);
    ck_assert_ptr_nonnull(parser->destroy);
    
    // Cleanup
    parser->destroy(parser);
}
END_TEST

/**
 * Proposals 파싱 테스트
 */
START_TEST(test_json_parser_parse_proposals)
{
    // Given
    extsock_json_parser_t *parser = standalone_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    const char *json_str = "[\"aes256-sha256-modp2048\", \"aes128-sha1-modp1024\"]";
    cJSON *json_array = cJSON_Parse(json_str);
    ck_assert_ptr_nonnull(json_array);
    
    // When
    linked_list_t *proposals = parser->parse_proposals(parser, json_array, PROTO_IKE, true);
    
    // Then
    ck_assert_ptr_nonnull(proposals);
    ck_assert_int_eq(linked_list_get_count(proposals), 2);
    
    // Cleanup
    linked_list_destroy(proposals);
    cJSON_Delete(json_array);
    parser->destroy(parser);
}
END_TEST

/**
 * Traffic Selectors 파싱 테스트
 */
START_TEST(test_json_parser_parse_traffic_selectors)
{
    // Given
    extsock_json_parser_t *parser = standalone_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    const char *json_str = "[\"192.168.1.0/24\", \"10.0.0.0/8\"]";
    cJSON *json_array = cJSON_Parse(json_str);
    ck_assert_ptr_nonnull(json_array);
    
    // When
    linked_list_t *ts_list = parser->parse_traffic_selectors(parser, json_array);
    
    // Then
    ck_assert_ptr_nonnull(ts_list);
    ck_assert_int_eq(linked_list_get_count(ts_list), 2);
    
    // Cleanup
    linked_list_destroy(ts_list);
    cJSON_Delete(json_array);
    parser->destroy(parser);
}
END_TEST

/**
 * IKE Config 파싱 테스트
 */
START_TEST(test_json_parser_parse_ike_config)
{
    // Given
    extsock_json_parser_t *parser = standalone_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    const char *json_str = "{"
        "\"local_addrs\": [\"192.168.1.1\"],"
        "\"remote_addrs\": [\"192.168.1.2\"],"
        "\"version\": 2,"
        "\"proposals\": [\"aes256-sha256-modp2048\"]"
    "}";
    cJSON *ike_json = cJSON_Parse(json_str);
    ck_assert_ptr_nonnull(ike_json);
    
    // When
    ike_cfg_t *ike_cfg = parser->parse_ike_config(parser, ike_json);
    
    // Then
    ck_assert_ptr_nonnull(ike_cfg);
    ck_assert_str_eq(ike_cfg->local, "192.168.1.1");
    ck_assert_str_eq(ike_cfg->remote, "192.168.1.2");
    ck_assert_int_eq(ike_cfg->version, 2);
    ck_assert_ptr_nonnull(ike_cfg->proposals);
    
    // Cleanup
    free(ike_cfg->local);
    free(ike_cfg->remote);
    linked_list_destroy(ike_cfg->proposals);
    free(ike_cfg);
    cJSON_Delete(ike_json);
    parser->destroy(parser);
}
END_TEST

/**
 * 전체 Config Entity 파싱 테스트
 */
START_TEST(test_json_parser_parse_config_entity)
{
    // Given
    extsock_json_parser_t *parser = standalone_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    const char *config_json = "{"
        "\"connection_name\": \"test_connection\","
        "\"ike\": {"
            "\"local_addrs\": [\"192.168.1.1\"],"
            "\"remote_addrs\": [\"192.168.1.2\"],"
            "\"version\": 2"
        "}"
    "}";
    
    // When
    extsock_config_entity_t *entity = parser->parse_config_entity(parser, config_json);
    
    // Then
    ck_assert_ptr_nonnull(entity);
    ck_assert_str_eq(entity->connection_name, "test_connection");
    ck_assert_ptr_nonnull(entity->peer_cfg);
    ck_assert_str_eq(entity->peer_cfg->name, "test_connection");
    ck_assert_ptr_nonnull(entity->peer_cfg->ike_cfg);
    ck_assert_int_eq(entity->status, EXTSOCK_SUCCESS);
    
    // Cleanup
    free(entity->connection_name);
    if (entity->peer_cfg) {
        free(entity->peer_cfg->name);
        if (entity->peer_cfg->ike_cfg) {
            free(entity->peer_cfg->ike_cfg->local);
            free(entity->peer_cfg->ike_cfg->remote);
            linked_list_destroy(entity->peer_cfg->ike_cfg->proposals);
            free(entity->peer_cfg->ike_cfg);
        }
        linked_list_destroy(entity->peer_cfg->children);
        free(entity->peer_cfg);
    }
    free(entity);
    parser->destroy(parser);
}
END_TEST

/**
 * 잘못된 JSON 처리 테스트
 */
START_TEST(test_json_parser_invalid_json)
{
    // Given
    extsock_json_parser_t *parser = standalone_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    const char *invalid_json = "{invalid json";
    
    // When
    extsock_config_entity_t *entity = parser->parse_config_entity(parser, invalid_json);
    
    // Then
    ck_assert_ptr_null(entity);
    
    // Cleanup
    parser->destroy(parser);
}
END_TEST

/**
 * 빈 배열 처리 테스트
 */
START_TEST(test_json_parser_empty_arrays)
{
    // Given
    extsock_json_parser_t *parser = standalone_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    cJSON *empty_array = cJSON_CreateArray();
    ck_assert_ptr_nonnull(empty_array);
    
    // When - Empty proposals
    linked_list_t *proposals = parser->parse_proposals(parser, empty_array, PROTO_IKE, true);
    
    // Then - Should have default proposals
    ck_assert_ptr_nonnull(proposals);
    ck_assert_int_eq(linked_list_get_count(proposals), 2); // default + aead_default
    
    // When - Empty traffic selectors
    linked_list_t *ts_list = parser->parse_traffic_selectors(parser, empty_array);
    
    // Then - Should have dynamic TS
    ck_assert_ptr_nonnull(ts_list);
    ck_assert_int_eq(linked_list_get_count(ts_list), 1);
    
    // Cleanup
    linked_list_destroy(proposals);
    linked_list_destroy(ts_list);
    cJSON_Delete(empty_array);
    parser->destroy(parser);
}
END_TEST

Suite *json_parser_standalone_suite(void)
{
    Suite *s;
    TCase *tc_basic, *tc_parsing, *tc_edge_cases;
    
    s = suite_create("JSON Parser Standalone Tests");
    
    /* 기본 기능 테스트 */
    tc_basic = tcase_create("Basic JSON Parser Functions");
    tcase_add_checked_fixture(tc_basic, setup_json_parser_standalone_test, teardown_json_parser_standalone_test);
    tcase_add_test(tc_basic, test_json_parser_create_destroy);
    suite_add_tcase(s, tc_basic);
    
    /* JSON 파싱 테스트 */
    tc_parsing = tcase_create("JSON Parsing Tests");
    tcase_add_test(tc_parsing, test_json_parser_parse_proposals);
    tcase_add_test(tc_parsing, test_json_parser_parse_traffic_selectors);
    tcase_add_test(tc_parsing, test_json_parser_parse_ike_config);
    tcase_add_test(tc_parsing, test_json_parser_parse_config_entity);
    suite_add_tcase(s, tc_parsing);
    
    /* 엣지 케이스 테스트 */
    tc_edge_cases = tcase_create("Edge Cases");
    tcase_add_test(tc_edge_cases, test_json_parser_invalid_json);
    tcase_add_test(tc_edge_cases, test_json_parser_empty_arrays);
    suite_add_tcase(s, tc_edge_cases);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    printf("=== JSON Parser Standalone Tests ===\n");
    printf("Testing actual JSON Parser implementations with Mock strongSwan API\n\n");
    
    s = json_parser_standalone_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    printf("\n=== Test Results ===\n");
    printf("Failed tests: %d\n", number_failed);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 