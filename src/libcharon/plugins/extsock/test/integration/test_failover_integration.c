/*
 * Copyright (C) 2024 strongSwan Project
 * Integration tests for Failover Manager complete flow
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <library.h>
#include <daemon.h>
#include <collections/linked_list.h>

#include "../../interfaces/extsock_failover_manager.h"
#include "../../usecases/extsock_config_usecase.h"
#include "../../usecases/extsock_event_usecase.h"
#include "../../common/extsock_common.h"

// Test globals
static extsock_failover_manager_t *failover_manager;
static extsock_config_usecase_t *config_usecase;
static extsock_event_usecase_t *event_usecase;

// Test result tracking
typedef struct test_results_t {
    int config_creation_calls;
    int connection_attempts;
    char *last_created_config_name;
    char *last_target_address;
    bool success;
} test_results_t;

static test_results_t test_results;

/**
 * Mock Config Usecase for integration testing
 */
typedef struct mock_integration_config_usecase_t {
    extsock_config_usecase_t public;
    test_results_t *results;
} mock_integration_config_usecase_t;

static extsock_error_t mock_integration_add_peer_config_and_initiate(
    mock_integration_config_usecase_t *this, peer_cfg_t *peer_cfg)
{
    if (!peer_cfg || !this->results) {
        return EXTSOCK_ERROR_INVALID_PARAMETER;
    }
    
    // 결과 기록
    this->results->config_creation_calls++;
    this->results->connection_attempts++;
    
    // 설정 이름 기록
    const char *name = peer_cfg->get_name(peer_cfg);
    if (name) {
        free(this->results->last_created_config_name);
        this->results->last_created_config_name = strdup(name);
        
        // failover 설정에서 주소 추출 (간단한 파싱)
        if (strstr(name, "failover")) {
            char *addr_start = strrchr(name, '-');
            if (addr_start) {
                free(this->results->last_target_address);
                this->results->last_target_address = strdup(addr_start + 1);
            }
        }
    }
    
    // peer_cfg 정리 (mock에서는 실제로 사용하지 않음)
    peer_cfg->destroy(peer_cfg);
    
    this->results->success = TRUE;
    return EXTSOCK_SUCCESS;
}

static void mock_integration_config_usecase_destroy(mock_integration_config_usecase_t *this)
{
    free(this);
}

static extsock_config_usecase_t *create_mock_integration_config_usecase(test_results_t *results)
{
    mock_integration_config_usecase_t *mock;
    
    INIT(mock,
        .public = {
            .add_peer_config_and_initiate = (void*)mock_integration_add_peer_config_and_initiate,
            .destroy = (void*)mock_integration_config_usecase_destroy,
        },
        .results = results,
    );
    
    return &mock->public;
}

/**
 * Mock peer_cfg for integration testing
 */
typedef struct mock_integration_peer_cfg_t {
    peer_cfg_t public;
    char *name;
    ike_cfg_t *ike_cfg;
    linked_list_t *child_cfgs;
    linked_list_t *local_auth_cfgs;
    linked_list_t *remote_auth_cfgs;
} mock_integration_peer_cfg_t;

static const char* mock_integration_peer_cfg_get_name(mock_integration_peer_cfg_t *this)
{
    return this->name;
}

static ike_cfg_t* mock_integration_peer_cfg_get_ike_cfg(mock_integration_peer_cfg_t *this)
{
    return this->ike_cfg;
}

static enumerator_t* mock_integration_peer_cfg_create_child_cfg_enumerator(mock_integration_peer_cfg_t *this)
{
    return this->child_cfgs->create_enumerator(this->child_cfgs);
}

static enumerator_t* mock_integration_peer_cfg_create_auth_cfg_enumerator(mock_integration_peer_cfg_t *this, bool local)
{
    if (local) {
        return this->local_auth_cfgs->create_enumerator(this->local_auth_cfgs);
    } else {
        return this->remote_auth_cfgs->create_enumerator(this->remote_auth_cfgs);
    }
}

static void mock_integration_peer_cfg_destroy(mock_integration_peer_cfg_t *this)
{
    free(this->name);
    DESTROY_IF(this->ike_cfg);
    DESTROY_IF(this->child_cfgs);
    DESTROY_IF(this->local_auth_cfgs);
    DESTROY_IF(this->remote_auth_cfgs);
    free(this);
}

/**
 * Mock ike_cfg for integration testing
 */
typedef struct mock_integration_ike_cfg_t {
    ike_cfg_t public;
    char *other_addr;
    linked_list_t *proposals;
} mock_integration_ike_cfg_t;

static char* mock_integration_ike_cfg_get_other_addr(mock_integration_ike_cfg_t *this)
{
    return this->other_addr;
}

static linked_list_t* mock_integration_ike_cfg_get_proposals(mock_integration_ike_cfg_t *this)
{
    return this->proposals;
}

static void mock_integration_ike_cfg_destroy(mock_integration_ike_cfg_t *this)
{
    free(this->other_addr);
    DESTROY_IF(this->proposals);
    free(this);
}

/**
 * Mock host for integration testing
 */
typedef struct mock_integration_host_t {
    host_t public;
    char *address;
} mock_integration_host_t;

static void mock_integration_host_destroy(mock_integration_host_t *this)
{
    free(this->address);
    free(this);
}

/**
 * Mock IKE SA for integration testing
 */
typedef struct mock_integration_ike_sa_t {
    ike_sa_t public;
    char *name;
    peer_cfg_t *peer_cfg;
    host_t *other_host;
} mock_integration_ike_sa_t;

static const char* mock_integration_ike_sa_get_name(mock_integration_ike_sa_t *this)
{
    return this->name;
}

static peer_cfg_t* mock_integration_ike_sa_get_peer_cfg(mock_integration_ike_sa_t *this)
{
    return this->peer_cfg;
}

static host_t* mock_integration_ike_sa_get_other_host(mock_integration_ike_sa_t *this)
{
    return this->other_host;
}

static void mock_integration_ike_sa_destroy(mock_integration_ike_sa_t *this)
{
    free(this->name);
    DESTROY_IF(this->peer_cfg);
    DESTROY_IF(this->other_host);
    free(this);
}

// Helper functions for creating mock objects
static ike_cfg_t* create_mock_integration_ike_cfg(const char *other_addr)
{
    mock_integration_ike_cfg_t *mock;
    
    INIT(mock,
        .public = {
            .get_other_addr = (void*)mock_integration_ike_cfg_get_other_addr,
            .get_proposals = (void*)mock_integration_ike_cfg_get_proposals,
            .destroy = (void*)mock_integration_ike_cfg_destroy,
        },
        .other_addr = strdup(other_addr),
        .proposals = linked_list_create(),
    );
    
    return &mock->public;
}

static peer_cfg_t* create_mock_integration_peer_cfg(const char *name, const char *other_addr)
{
    mock_integration_peer_cfg_t *mock;
    
    INIT(mock,
        .public = {
            .get_name = (void*)mock_integration_peer_cfg_get_name,
            .get_ike_cfg = (void*)mock_integration_peer_cfg_get_ike_cfg,
            .create_child_cfg_enumerator = (void*)mock_integration_peer_cfg_create_child_cfg_enumerator,
            .create_auth_cfg_enumerator = (void*)mock_integration_peer_cfg_create_auth_cfg_enumerator,
            .destroy = (void*)mock_integration_peer_cfg_destroy,
        },
        .name = strdup(name),
        .ike_cfg = create_mock_integration_ike_cfg(other_addr),
        .child_cfgs = linked_list_create(),
        .local_auth_cfgs = linked_list_create(),
        .remote_auth_cfgs = linked_list_create(),
    );
    
    return &mock->public;
}

static host_t* create_mock_integration_host(const char *address)
{
    mock_integration_host_t *mock;
    
    INIT(mock,
        .public = {
            .destroy = (void*)mock_integration_host_destroy,
        },
        .address = strdup(address),
    );
    
    return &mock->public;
}

static ike_sa_t* create_mock_integration_ike_sa(const char *name, const char *other_addr, const char *current_addr)
{
    mock_integration_ike_sa_t *mock;
    
    INIT(mock,
        .public = {
            .get_name = (void*)mock_integration_ike_sa_get_name,
            .get_peer_cfg = (void*)mock_integration_ike_sa_get_peer_cfg,
            .get_other_host = (void*)mock_integration_ike_sa_get_other_host,
            .destroy = (void*)mock_integration_ike_sa_destroy,
        },
        .name = strdup(name),
        .peer_cfg = create_mock_integration_peer_cfg(name, other_addr),
        .other_host = create_mock_integration_host(current_addr),
    );
    
    return &mock->public;
}

// Test setup and teardown
static void integration_setup()
{
    // Test results 초기화
    memset(&test_results, 0, sizeof(test_results));
    
    // Mock config usecase 생성
    config_usecase = create_mock_integration_config_usecase(&test_results);
    ck_assert_ptr_nonnull(config_usecase);
    
    // Failover manager 생성
    failover_manager = extsock_failover_manager_create(config_usecase);
    ck_assert_ptr_nonnull(failover_manager);
    
    printf("Integration test setup completed\n");
}

static void integration_teardown()
{
    if (failover_manager) {
        failover_manager->destroy(failover_manager);
        failover_manager = NULL;
    }
    
    // config_usecase는 failover_manager 소멸 시 함께 정리됨
    config_usecase = NULL;
    
    // Test results 정리
    free(test_results.last_created_config_name);
    free(test_results.last_target_address);
    memset(&test_results, 0, sizeof(test_results));
    
    printf("Integration test teardown completed\n");
}

// 통합 테스트 케이스들

START_TEST(test_complete_failover_flow_basic)
{
    printf("\n=== Testing Complete Failover Flow (Basic) ===\n");
    
    // 1. Mock IKE SA 생성 (2개 주소 설정)
    ike_sa_t *mock_ike_sa = create_mock_integration_ike_sa(
        "test-connection", 
        "10.0.0.1,10.0.0.2", 
        "10.0.0.1"
    );
    
    printf("Created mock IKE SA with addresses: 10.0.0.1,10.0.0.2\n");
    printf("Current address: 10.0.0.1\n");
    
    // 2. Failover Manager를 통한 연결 실패 처리
    failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
    
    // 3. 결과 검증
    printf("Config creation calls: %d\n", test_results.config_creation_calls);
    printf("Connection attempts: %d\n", test_results.connection_attempts);
    printf("Last created config: %s\n", test_results.last_created_config_name ?: "NULL");
    printf("Target address: %s\n", test_results.last_target_address ?: "NULL");
    
    ck_assert_int_eq(test_results.config_creation_calls, 1);
    ck_assert_int_eq(test_results.connection_attempts, 1);
    ck_assert_ptr_nonnull(test_results.last_created_config_name);
    ck_assert_ptr_nonnull(test_results.last_target_address);
    ck_assert_str_eq(test_results.last_target_address, "10.0.0.2");
    ck_assert(test_results.success);
    
    // 정리
    mock_ike_sa->destroy(mock_ike_sa);
    
    printf("=== Basic Failover Flow Test PASSED ===\n");
}
END_TEST

START_TEST(test_complete_failover_flow_multiple_addresses)
{
    printf("\n=== Testing Complete Failover Flow (Multiple Addresses) ===\n");
    
    // 1. Mock IKE SA 생성 (3개 주소 설정)
    ike_sa_t *mock_ike_sa = create_mock_integration_ike_sa(
        "multi-segw-connection", 
        "192.168.1.1,192.168.1.2,192.168.1.3", 
        "192.168.1.2"
    );
    
    printf("Created mock IKE SA with addresses: 192.168.1.1,192.168.1.2,192.168.1.3\n");
    printf("Current address: 192.168.1.2\n");
    
    // 2. Failover Manager를 통한 연결 실패 처리
    failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
    
    // 3. 결과 검증 (192.168.1.2 -> 192.168.1.3로 전환되어야 함)
    printf("Target address: %s\n", test_results.last_target_address ?: "NULL");
    
    ck_assert_int_eq(test_results.config_creation_calls, 1);
    ck_assert_str_eq(test_results.last_target_address, "192.168.1.3");
    
    // 정리
    mock_ike_sa->destroy(mock_ike_sa);
    
    printf("=== Multiple Addresses Failover Test PASSED ===\n");
}
END_TEST

START_TEST(test_complete_failover_flow_circular)
{
    printf("\n=== Testing Complete Failover Flow (Circular) ===\n");
    
    // 1. Mock IKE SA 생성 (마지막 주소에서 첫 번째로 순환)
    ike_sa_t *mock_ike_sa = create_mock_integration_ike_sa(
        "circular-test", 
        "10.1.1.1,10.1.1.2", 
        "10.1.1.2"  // 마지막 주소
    );
    
    printf("Created mock IKE SA with addresses: 10.1.1.1,10.1.1.2\n");
    printf("Current address: 10.1.1.2 (last address)\n");
    
    // 2. Failover Manager를 통한 연결 실패 처리
    failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
    
    // 3. 결과 검증 (10.1.1.2 -> 10.1.1.1로 순환되어야 함)
    printf("Target address: %s\n", test_results.last_target_address ?: "NULL");
    
    ck_assert_str_eq(test_results.last_target_address, "10.1.1.1");
    
    // 정리
    mock_ike_sa->destroy(mock_ike_sa);
    
    printf("=== Circular Failover Test PASSED ===\n");
}
END_TEST

START_TEST(test_complete_failover_flow_single_address)
{
    printf("\n=== Testing Complete Failover Flow (Single Address - No Failover) ===\n");
    
    // 1. Mock IKE SA 생성 (단일 주소 - failover 불가)
    ike_sa_t *mock_ike_sa = create_mock_integration_ike_sa(
        "single-addr-test", 
        "10.2.2.1",  // 단일 주소
        "10.2.2.1"
    );
    
    printf("Created mock IKE SA with single address: 10.2.2.1\n");
    
    // 2. Failover Manager를 통한 연결 실패 처리
    failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
    
    // 3. 결과 검증 (failover가 시도되지 않아야 함)
    printf("Config creation calls: %d (should be 0)\n", test_results.config_creation_calls);
    
    ck_assert_int_eq(test_results.config_creation_calls, 0);
    ck_assert_int_eq(test_results.connection_attempts, 0);
    
    // 정리
    mock_ike_sa->destroy(mock_ike_sa);
    
    printf("=== Single Address Test PASSED ===\n");
}
END_TEST

START_TEST(test_retry_limit_behavior)
{
    printf("\n=== Testing Retry Limit Behavior ===\n");
    
    const char *conn_name = "retry-limit-test";
    
    // 1. 최대 재시도 횟수까지 반복
    for (int i = 0; i < 5; i++) {
        printf("Retry attempt %d/5\n", i + 1);
        
        ike_sa_t *mock_ike_sa = create_mock_integration_ike_sa(
            conn_name, 
            "10.3.3.1,10.3.3.2", 
            "10.3.3.1"
        );
        
        failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
        mock_ike_sa->destroy(mock_ike_sa);
    }
    
    int attempts_before_limit = test_results.config_creation_calls;
    printf("Attempts before limit: %d\n", attempts_before_limit);
    
    // 2. 한 번 더 시도 (최대 재시도 초과)
    printf("Attempting after retry limit exceeded\n");
    ike_sa_t *mock_ike_sa = create_mock_integration_ike_sa(
        conn_name, 
        "10.3.3.1,10.3.3.2", 
        "10.3.3.1"
    );
    
    failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
    mock_ike_sa->destroy(mock_ike_sa);
    
    // 3. 결과 검증 (재시도 횟수 초과로 인해 추가 시도가 없어야 함)
    printf("Total attempts: %d (should be same as before limit)\n", test_results.config_creation_calls);
    
    ck_assert_int_eq(test_results.config_creation_calls, attempts_before_limit);
    
    printf("=== Retry Limit Test PASSED ===\n");
}
END_TEST

// Test Suite 생성
Suite *failover_integration_suite(void)
{
    Suite *s;
    TCase *tc_integration;
    
    s = suite_create("Failover Manager Integration Tests");
    
    // Integration test case
    tc_integration = tcase_create("Integration Flow");
    
    tcase_add_checked_fixture(tc_integration, integration_setup, integration_teardown);
    
    // 통합 테스트들
    tcase_add_test(tc_integration, test_complete_failover_flow_basic);
    tcase_add_test(tc_integration, test_complete_failover_flow_multiple_addresses);
    tcase_add_test(tc_integration, test_complete_failover_flow_circular);
    tcase_add_test(tc_integration, test_complete_failover_flow_single_address);
    tcase_add_test(tc_integration, test_retry_limit_behavior);
    
    suite_add_tcase(s, tc_integration);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    // Library 초기화
    if (!library_init(NULL, "test-failover-integration")) {
        library_deinit();
        return EXIT_FAILURE;
    }
    
    printf("Starting Failover Manager Integration Tests...\n");
    
    s = failover_integration_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    library_deinit();
    
    printf("Integration tests completed. Failed: %d\n", number_failed);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 