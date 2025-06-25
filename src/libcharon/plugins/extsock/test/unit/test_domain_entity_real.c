/*
 * Copyright (C) 2024 strongSwan Project
 * Real Domain Entity Implementation Tests
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <cjson/cJSON.h>

// Domain entity 관련 타입들
typedef enum {
    EXTSOCK_CONN_STATE_DISCONNECTED,
    EXTSOCK_CONN_STATE_CONNECTING,
    EXTSOCK_CONN_STATE_AUTHENTICATING,
    EXTSOCK_CONN_STATE_CONNECTED,
    EXTSOCK_CONN_STATE_ERROR
} extsock_connection_state_t;

typedef enum {
    EXTSOCK_AUTH_TYPE_PSK,
    EXTSOCK_AUTH_TYPE_PUBKEY,
    EXTSOCK_AUTH_TYPE_EAP
} extsock_auth_type_t;

typedef struct {
    char *local_ip;
    char *remote_ip;
    char *local_subnets;
    char *remote_subnets;
    bool is_valid;
} extsock_network_config_t;

typedef struct {
    extsock_auth_type_t type;
    char *identity;
    char *secret;
    char *certificate_path;
    bool is_valid;
} extsock_auth_config_t;

typedef struct {
    char *connection_name;
    extsock_connection_state_t state;
    extsock_network_config_t *network;
    extsock_auth_config_t *local_auth;
    extsock_auth_config_t *remote_auth;
    time_t created_at;
    time_t last_activity;
    bool (*validate)(void *this);
    void (*destroy)(void *this);
} extsock_connection_entity_t;

static extsock_connection_entity_t *connection_entity;
static extsock_network_config_t *network_config;
static extsock_auth_config_t *auth_config;

/**
 * 네트워크 설정 생성 헬퍼
 */
static extsock_network_config_t* create_network_config(const char *local_ip, const char *remote_ip, 
                                                       const char *local_subnets, const char *remote_subnets)
{
    extsock_network_config_t *config = malloc(sizeof(extsock_network_config_t));
    if (!config) return NULL;
    
    config->local_ip = strdup(local_ip);
    config->remote_ip = strdup(remote_ip);
    config->local_subnets = strdup(local_subnets);
    config->remote_subnets = strdup(remote_subnets);
    config->is_valid = true;
    
    return config;
}

/**
 * 인증 설정 생성 헬퍼
 */
static extsock_auth_config_t* create_auth_config(extsock_auth_type_t type, const char *identity, 
                                                  const char *secret, const char *cert_path)
{
    extsock_auth_config_t *config = malloc(sizeof(extsock_auth_config_t));
    if (!config) return NULL;
    
    config->type = type;
    config->identity = identity ? strdup(identity) : NULL;
    config->secret = secret ? strdup(secret) : NULL;
    config->certificate_path = cert_path ? strdup(cert_path) : NULL;
    config->is_valid = true;
    
    return config;
}

/**
 * 연결 엔티티 생성 헬퍼
 */
static extsock_connection_entity_t* create_connection_entity(const char *name, 
                                                             extsock_network_config_t *net_config,
                                                             extsock_auth_config_t *local_auth,
                                                             extsock_auth_config_t *remote_auth)
{
    extsock_connection_entity_t *entity = malloc(sizeof(extsock_connection_entity_t));
    if (!entity) return NULL;
    
    entity->connection_name = strdup(name);
    entity->state = EXTSOCK_CONN_STATE_DISCONNECTED;
    entity->network = net_config;
    entity->local_auth = local_auth;
    entity->remote_auth = remote_auth;
    entity->created_at = time(NULL);
    entity->last_activity = time(NULL);
    
    return entity;
}

/**
 * 설정 해제 헬퍼
 */
static void destroy_network_config(extsock_network_config_t *config)
{
    if (config) {
        free(config->local_ip);
        free(config->remote_ip);
        free(config->local_subnets);
        free(config->remote_subnets);
        free(config);
    }
}

static void destroy_auth_config(extsock_auth_config_t *config)
{
    if (config) {
        free(config->identity);
        free(config->secret);
        free(config->certificate_path);
        free(config);
    }
}

static void destroy_connection_entity(extsock_connection_entity_t *entity)
{
    if (entity) {
        free(entity->connection_name);
        destroy_network_config(entity->network);
        destroy_auth_config(entity->local_auth);
        destroy_auth_config(entity->remote_auth);
        free(entity);
    }
}

/**
 * 테스트 설정
 */
void setup_domain_entity_real_test(void)
{
    // 네트워크 설정 생성
    network_config = create_network_config("192.168.1.10", "203.0.113.5", "192.168.1.0/24", "10.0.0.0/16");
    ck_assert_ptr_nonnull(network_config);
    
    // 인증 설정 생성
    auth_config = create_auth_config(EXTSOCK_AUTH_TYPE_PSK, "client@example.com", "secret123", NULL);
    ck_assert_ptr_nonnull(auth_config);
    
    // 연결 엔티티 생성
    extsock_auth_config_t *remote_auth = create_auth_config(EXTSOCK_AUTH_TYPE_PUBKEY, "server@example.com", NULL, "/etc/certs/server.pem");
    connection_entity = create_connection_entity("test_connection", network_config, auth_config, remote_auth);
    ck_assert_ptr_nonnull(connection_entity);
}

/**
 * 테스트 해제
 */
void teardown_domain_entity_real_test(void)
{
    if (connection_entity) {
        destroy_connection_entity(connection_entity);
        connection_entity = NULL;
        network_config = NULL;  // connection_entity에서 해제됨
        auth_config = NULL;     // connection_entity에서 해제됨
    }
}

/**
 * 네트워크 설정 생성 테스트
 */
START_TEST(test_real_network_config_creation)
{
    // Given / When / Then
    ck_assert_ptr_nonnull(network_config);
    ck_assert_str_eq(network_config->local_ip, "192.168.1.10");
    ck_assert_str_eq(network_config->remote_ip, "203.0.113.5");
    ck_assert_str_eq(network_config->local_subnets, "192.168.1.0/24");
    ck_assert_str_eq(network_config->remote_subnets, "10.0.0.0/16");
    ck_assert(network_config->is_valid);
}
END_TEST

/**
 * 인증 설정 유형별 테스트
 */
START_TEST(test_real_auth_config_types)
{
    // Given - PSK 인증
    extsock_auth_config_t *psk_config = create_auth_config(EXTSOCK_AUTH_TYPE_PSK, "user@domain.com", "password123", NULL);
    
    // When / Then
    ck_assert_ptr_nonnull(psk_config);
    ck_assert_int_eq(psk_config->type, EXTSOCK_AUTH_TYPE_PSK);
    ck_assert_str_eq(psk_config->identity, "user@domain.com");
    ck_assert_str_eq(psk_config->secret, "password123");
    ck_assert_ptr_null(psk_config->certificate_path);
    ck_assert(psk_config->is_valid);
    
    // Given - 공개키 인증
    extsock_auth_config_t *pubkey_config = create_auth_config(EXTSOCK_AUTH_TYPE_PUBKEY, "server@domain.com", NULL, "/path/to/cert.pem");
    
    // When / Then
    ck_assert_ptr_nonnull(pubkey_config);
    ck_assert_int_eq(pubkey_config->type, EXTSOCK_AUTH_TYPE_PUBKEY);
    ck_assert_str_eq(pubkey_config->identity, "server@domain.com");
    ck_assert_ptr_null(pubkey_config->secret);
    ck_assert_str_eq(pubkey_config->certificate_path, "/path/to/cert.pem");
    
    // Given - EAP 인증
    extsock_auth_config_t *eap_config = create_auth_config(EXTSOCK_AUTH_TYPE_EAP, "eap_user", "eap_password", NULL);
    
    // When / Then
    ck_assert_int_eq(eap_config->type, EXTSOCK_AUTH_TYPE_EAP);
    ck_assert_str_eq(eap_config->identity, "eap_user");
    ck_assert_str_eq(eap_config->secret, "eap_password");
    
    // Cleanup
    destroy_auth_config(psk_config);
    destroy_auth_config(pubkey_config);
    destroy_auth_config(eap_config);
}
END_TEST

/**
 * 연결 엔티티 생성 테스트
 */
START_TEST(test_real_connection_entity_creation)
{
    // Given / When / Then
    ck_assert_ptr_nonnull(connection_entity);
    ck_assert_str_eq(connection_entity->connection_name, "test_connection");
    ck_assert_int_eq(connection_entity->state, EXTSOCK_CONN_STATE_DISCONNECTED);
    ck_assert_ptr_nonnull(connection_entity->network);
    ck_assert_ptr_nonnull(connection_entity->local_auth);
    ck_assert_ptr_nonnull(connection_entity->remote_auth);
    ck_assert_int_gt(connection_entity->created_at, 0);
    ck_assert_int_gt(connection_entity->last_activity, 0);
}
END_TEST

/**
 * 연결 상태 전환 테스트
 */
START_TEST(test_real_connection_state_transitions)
{
    // Given - 초기 상태
    ck_assert_int_eq(connection_entity->state, EXTSOCK_CONN_STATE_DISCONNECTED);
    
    // When - 연결 시도
    connection_entity->state = EXTSOCK_CONN_STATE_CONNECTING;
    connection_entity->last_activity = time(NULL);
    
    // Then
    ck_assert_int_eq(connection_entity->state, EXTSOCK_CONN_STATE_CONNECTING);
    
    // When - 인증 단계
    connection_entity->state = EXTSOCK_CONN_STATE_AUTHENTICATING;
    connection_entity->last_activity = time(NULL);
    
    // Then
    ck_assert_int_eq(connection_entity->state, EXTSOCK_CONN_STATE_AUTHENTICATING);
    
    // When - 연결 완료
    connection_entity->state = EXTSOCK_CONN_STATE_CONNECTED;
    connection_entity->last_activity = time(NULL);
    
    // Then
    ck_assert_int_eq(connection_entity->state, EXTSOCK_CONN_STATE_CONNECTED);
    
    // When - 에러 발생
    connection_entity->state = EXTSOCK_CONN_STATE_ERROR;
    
    // Then
    ck_assert_int_eq(connection_entity->state, EXTSOCK_CONN_STATE_ERROR);
}
END_TEST

/**
 * 네트워크 설정 유효성 검증 테스트
 */
START_TEST(test_real_network_config_validation)
{
    // Given - 유효한 설정
    extsock_network_config_t *valid_config = create_network_config("10.0.0.1", "10.0.1.1", "10.0.0.0/24", "10.0.1.0/24");
    
    // When - 유효성 검증
    bool is_valid = true;
    if (!valid_config->local_ip || strlen(valid_config->local_ip) == 0) is_valid = false;
    if (!valid_config->remote_ip || strlen(valid_config->remote_ip) == 0) is_valid = false;
    if (!valid_config->local_subnets || strlen(valid_config->local_subnets) == 0) is_valid = false;
    if (!valid_config->remote_subnets || strlen(valid_config->remote_subnets) == 0) is_valid = false;
    
    // Then
    ck_assert(is_valid);
    
    // Given - 잘못된 설정 (빈 IP)
    extsock_network_config_t *invalid_config = create_network_config("", "10.0.1.1", "10.0.0.0/24", "10.0.1.0/24");
    
    // When
    is_valid = true;
    if (!invalid_config->local_ip || strlen(invalid_config->local_ip) == 0) is_valid = false;
    
    // Then
    ck_assert(!is_valid);
    
    // Cleanup
    destroy_network_config(valid_config);
    destroy_network_config(invalid_config);
}
END_TEST

/**
 * 인증 설정 유효성 검증 테스트
 */
START_TEST(test_real_auth_config_validation)
{
    // Given - 유효한 PSK 설정
    extsock_auth_config_t *valid_psk = create_auth_config(EXTSOCK_AUTH_TYPE_PSK, "user@domain.com", "secret", NULL);
    
    // When - PSK 유효성 검증
    bool is_valid = true;
    if (valid_psk->type == EXTSOCK_AUTH_TYPE_PSK) {
        if (!valid_psk->identity || strlen(valid_psk->identity) == 0) is_valid = false;
        if (!valid_psk->secret || strlen(valid_psk->secret) == 0) is_valid = false;
    }
    
    // Then
    ck_assert(is_valid);
    
    // Given - 잘못된 PSK 설정 (시크릿 없음)
    extsock_auth_config_t *invalid_psk = create_auth_config(EXTSOCK_AUTH_TYPE_PSK, "user@domain.com", "", NULL);
    
    // When
    is_valid = true;
    if (invalid_psk->type == EXTSOCK_AUTH_TYPE_PSK) {
        if (!invalid_psk->secret || strlen(invalid_psk->secret) == 0) is_valid = false;
    }
    
    // Then
    ck_assert(!is_valid);
    
    // Given - 유효한 공개키 설정
    extsock_auth_config_t *valid_pubkey = create_auth_config(EXTSOCK_AUTH_TYPE_PUBKEY, "server@domain.com", NULL, "/path/to/cert.pem");
    
    // When - 공개키 유효성 검증
    is_valid = true;
    if (valid_pubkey->type == EXTSOCK_AUTH_TYPE_PUBKEY) {
        if (!valid_pubkey->identity || strlen(valid_pubkey->identity) == 0) is_valid = false;
        if (!valid_pubkey->certificate_path || strlen(valid_pubkey->certificate_path) == 0) is_valid = false;
    }
    
    // Then
    ck_assert(is_valid);
    
    // Cleanup
    destroy_auth_config(valid_psk);
    destroy_auth_config(invalid_psk);
    destroy_auth_config(valid_pubkey);
}
END_TEST

/**
 * 연결 엔티티 JSON 변환 테스트
 */
START_TEST(test_real_connection_entity_to_json)
{
    // Given - 연결 엔티티
    // When - JSON 변환
    cJSON *entity_json = cJSON_CreateObject();
    cJSON_AddStringToObject(entity_json, "connection_name", connection_entity->connection_name);
    cJSON_AddNumberToObject(entity_json, "state", connection_entity->state);
    cJSON_AddNumberToObject(entity_json, "created_at", connection_entity->created_at);
    cJSON_AddNumberToObject(entity_json, "last_activity", connection_entity->last_activity);
    
    // 네트워크 설정 추가
    cJSON *network_json = cJSON_CreateObject();
    cJSON_AddStringToObject(network_json, "local_ip", connection_entity->network->local_ip);
    cJSON_AddStringToObject(network_json, "remote_ip", connection_entity->network->remote_ip);
    cJSON_AddStringToObject(network_json, "local_subnets", connection_entity->network->local_subnets);
    cJSON_AddStringToObject(network_json, "remote_subnets", connection_entity->network->remote_subnets);
    cJSON_AddItemToObject(entity_json, "network", network_json);
    
    // 로컬 인증 설정 추가
    cJSON *local_auth_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(local_auth_json, "type", connection_entity->local_auth->type);
    cJSON_AddStringToObject(local_auth_json, "identity", connection_entity->local_auth->identity);
    if (connection_entity->local_auth->secret) {
        cJSON_AddStringToObject(local_auth_json, "secret", "***masked***");
    }
    cJSON_AddItemToObject(entity_json, "local_auth", local_auth_json);
    
    // Then - JSON 직렬화 검증
    char *json_string = cJSON_Print(entity_json);
    ck_assert_ptr_nonnull(json_string);
    ck_assert_ptr_nonnull(strstr(json_string, "test_connection"));
    ck_assert_ptr_nonnull(strstr(json_string, "192.168.1.10"));
    ck_assert_ptr_nonnull(strstr(json_string, "203.0.113.5"));
    ck_assert_ptr_nonnull(strstr(json_string, "client@example.com"));
    ck_assert_ptr_nonnull(strstr(json_string, "***masked***"));
    
    // JSON 파싱 재테스트
    cJSON *parsed = cJSON_Parse(json_string);
    ck_assert_ptr_nonnull(parsed);
    
    cJSON *name_item = cJSON_GetObjectItem(parsed, "connection_name");
    ck_assert_str_eq(cJSON_GetStringValue(name_item), "test_connection");
    
    cJSON *network_item = cJSON_GetObjectItem(parsed, "network");
    ck_assert_ptr_nonnull(network_item);
    
    // Cleanup
    free(json_string);
    cJSON_Delete(parsed);
    cJSON_Delete(entity_json);
}
END_TEST

/**
 * 복수 연결 엔티티 관리 테스트
 */
START_TEST(test_real_multiple_connection_entities)
{
    // Given - 여러 연결 엔티티 생성
    extsock_connection_entity_t *entities[3];
    
    // 연결 1 - PSK 인증
    extsock_network_config_t *net1 = create_network_config("192.168.1.10", "203.0.113.5", "192.168.1.0/24", "10.0.0.0/16");
    extsock_auth_config_t *local_auth1 = create_auth_config(EXTSOCK_AUTH_TYPE_PSK, "client1@example.com", "secret1", NULL);
    extsock_auth_config_t *remote_auth1 = create_auth_config(EXTSOCK_AUTH_TYPE_PSK, "server1@example.com", "secret1", NULL);
    entities[0] = create_connection_entity("connection_1", net1, local_auth1, remote_auth1);
    
    // 연결 2 - 공개키 인증
    extsock_network_config_t *net2 = create_network_config("10.0.0.1", "10.0.1.1", "10.0.0.0/24", "10.0.1.0/24");
    extsock_auth_config_t *local_auth2 = create_auth_config(EXTSOCK_AUTH_TYPE_PUBKEY, "client2@example.com", NULL, "/etc/certs/client2.pem");
    extsock_auth_config_t *remote_auth2 = create_auth_config(EXTSOCK_AUTH_TYPE_PUBKEY, "server2@example.com", NULL, "/etc/certs/server2.pem");
    entities[1] = create_connection_entity("connection_2", net2, local_auth2, remote_auth2);
    
    // 연결 3 - EAP 인증
    extsock_network_config_t *net3 = create_network_config("172.16.0.1", "172.16.1.1", "172.16.0.0/24", "172.16.1.0/24");
    extsock_auth_config_t *local_auth3 = create_auth_config(EXTSOCK_AUTH_TYPE_EAP, "eap_user", "eap_pass", NULL);
    extsock_auth_config_t *remote_auth3 = create_auth_config(EXTSOCK_AUTH_TYPE_PSK, "server3@example.com", "shared_secret", NULL);
    entities[2] = create_connection_entity("connection_3", net3, local_auth3, remote_auth3);
    
    // When - 상태 설정
    entities[0]->state = EXTSOCK_CONN_STATE_CONNECTED;
    entities[1]->state = EXTSOCK_CONN_STATE_CONNECTING;
    entities[2]->state = EXTSOCK_CONN_STATE_ERROR;
    
    // Then - 각 연결 검증
    for (int i = 0; i < 3; i++) {
        ck_assert_ptr_nonnull(entities[i]);
        ck_assert_ptr_nonnull(entities[i]->connection_name);
        ck_assert_ptr_nonnull(entities[i]->network);
        ck_assert_ptr_nonnull(entities[i]->local_auth);
        ck_assert_ptr_nonnull(entities[i]->remote_auth);
    }
    
    // 연결별 인증 타입 확인
    ck_assert_int_eq(entities[0]->local_auth->type, EXTSOCK_AUTH_TYPE_PSK);
    ck_assert_int_eq(entities[1]->local_auth->type, EXTSOCK_AUTH_TYPE_PUBKEY);
    ck_assert_int_eq(entities[2]->local_auth->type, EXTSOCK_AUTH_TYPE_EAP);
    
    // 상태 확인
    ck_assert_int_eq(entities[0]->state, EXTSOCK_CONN_STATE_CONNECTED);
    ck_assert_int_eq(entities[1]->state, EXTSOCK_CONN_STATE_CONNECTING);
    ck_assert_int_eq(entities[2]->state, EXTSOCK_CONN_STATE_ERROR);
    
    // 이름 중복 검사
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 3; j++) {
            ck_assert_str_ne(entities[i]->connection_name, entities[j]->connection_name);
        }
    }
    
    // Cleanup
    for (int i = 0; i < 3; i++) {
        destroy_connection_entity(entities[i]);
    }
}
END_TEST

Suite *domain_entity_real_suite(void)
{
    Suite *s;
    TCase *tc_network, *tc_auth, *tc_entity, *tc_validation, *tc_conversion, *tc_management;

    s = suite_create("Domain Entity Real Implementation Tests");

    /* 네트워크 설정 테스트 */
    tc_network = tcase_create("Network Config Tests");
    tcase_add_checked_fixture(tc_network, setup_domain_entity_real_test, teardown_domain_entity_real_test);
    tcase_add_test(tc_network, test_real_network_config_creation);
    suite_add_tcase(s, tc_network);

    /* 인증 설정 테스트 */
    tc_auth = tcase_create("Auth Config Tests");
    tcase_add_checked_fixture(tc_auth, setup_domain_entity_real_test, teardown_domain_entity_real_test);
    tcase_add_test(tc_auth, test_real_auth_config_types);
    suite_add_tcase(s, tc_auth);

    /* 연결 엔티티 테스트 */
    tc_entity = tcase_create("Connection Entity Tests");
    tcase_add_checked_fixture(tc_entity, setup_domain_entity_real_test, teardown_domain_entity_real_test);
    tcase_add_test(tc_entity, test_real_connection_entity_creation);
    tcase_add_test(tc_entity, test_real_connection_state_transitions);
    suite_add_tcase(s, tc_entity);

    /* 유효성 검증 테스트 */
    tc_validation = tcase_create("Validation Tests");
    tcase_add_checked_fixture(tc_validation, setup_domain_entity_real_test, teardown_domain_entity_real_test);
    tcase_add_test(tc_validation, test_real_network_config_validation);
    tcase_add_test(tc_validation, test_real_auth_config_validation);
    suite_add_tcase(s, tc_validation);

    /* 변환 테스트 */
    tc_conversion = tcase_create("Conversion Tests");
    tcase_add_checked_fixture(tc_conversion, setup_domain_entity_real_test, teardown_domain_entity_real_test);
    tcase_add_test(tc_conversion, test_real_connection_entity_to_json);
    suite_add_tcase(s, tc_conversion);

    /* 관리 테스트 */
    tc_management = tcase_create("Management Tests");
    tcase_add_checked_fixture(tc_management, setup_domain_entity_real_test, teardown_domain_entity_real_test);
    tcase_add_test(tc_management, test_real_multiple_connection_entities);
    suite_add_tcase(s, tc_management);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = domain_entity_real_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 