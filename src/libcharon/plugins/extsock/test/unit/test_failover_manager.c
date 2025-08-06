/*
 * Copyright (C) 2024 strongSwan Project
 * Unit tests for Failover Manager
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <library.h>
#include "../../interfaces/extsock_failover_manager.h"
#include "../../usecases/extsock_config_usecase.h"
#include "../../common/extsock_common.h"

// Mock 객체들
static extsock_failover_manager_t *failover_manager;
static extsock_config_usecase_t *mock_config_usecase;

/**
 * Mock Config Usecase for testing
 */
typedef struct mock_config_usecase_t {
    extsock_config_usecase_t public;
    extsock_error_t add_peer_config_result;
    char *last_peer_name;
    int add_peer_config_call_count;
} mock_config_usecase_t;

static extsock_error_t mock_add_peer_config_and_initiate(mock_config_usecase_t *this, 
                                                        peer_cfg_t *peer_cfg)
{
    if (peer_cfg) {
        free(this->last_peer_name);
        this->last_peer_name = strdup(peer_cfg->get_name(peer_cfg));
        this->add_peer_config_call_count++;
        
        // Mock에서는 peer_cfg를 실제로 사용하지 않으므로 destroy
        peer_cfg->destroy(peer_cfg);
    }
    return this->add_peer_config_result;
}

static void mock_config_usecase_destroy(mock_config_usecase_t *this)
{
    free(this->last_peer_name);
    free(this);
}

static extsock_config_usecase_t *create_mock_config_usecase()
{
    mock_config_usecase_t *mock;
    
    INIT(mock,
        .public = {
            .add_peer_config_and_initiate = (void*)mock_add_peer_config_and_initiate,
            .destroy = (void*)mock_config_usecase_destroy,
        },
        .add_peer_config_result = EXTSOCK_SUCCESS,
        .last_peer_name = NULL,
        .add_peer_config_call_count = 0,
    );
    
    return &mock->public;
}

/**
 * Mock IKE SA for testing
 */
typedef struct mock_ike_sa_t {
    ike_sa_t public;
    char *name;
    peer_cfg_t *peer_cfg;
    host_t *other_host;
} mock_ike_sa_t;

static const char* mock_ike_sa_get_name(mock_ike_sa_t *this)
{
    return this->name;
}

static peer_cfg_t* mock_ike_sa_get_peer_cfg(mock_ike_sa_t *this)
{
    return this->peer_cfg;
}

static host_t* mock_ike_sa_get_other_host(mock_ike_sa_t *this)
{
    return this->other_host;
}

static void mock_ike_sa_destroy(mock_ike_sa_t *this)
{
    free(this->name);
    DESTROY_IF(this->peer_cfg);
    DESTROY_IF(this->other_host);
    free(this);
}

/**
 * Mock peer_cfg for testing
 */
typedef struct mock_peer_cfg_t {
    peer_cfg_t public;
    char *name;
    ike_cfg_t *ike_cfg;
    linked_list_t *child_cfgs;
    linked_list_t *local_auth_cfgs;
    linked_list_t *remote_auth_cfgs;
} mock_peer_cfg_t;

static const char* mock_peer_cfg_get_name(mock_peer_cfg_t *this)
{
    return this->name;
}

static ike_cfg_t* mock_peer_cfg_get_ike_cfg(mock_peer_cfg_t *this)
{
    return this->ike_cfg;
}

static enumerator_t* mock_peer_cfg_create_child_cfg_enumerator(mock_peer_cfg_t *this)
{
    return this->child_cfgs->create_enumerator(this->child_cfgs);
}

static void mock_peer_cfg_destroy(mock_peer_cfg_t *this)
{
    free(this->name);
    DESTROY_IF(this->ike_cfg);
    DESTROY_IF(this->child_cfgs);
    DESTROY_IF(this->local_auth_cfgs);
    DESTROY_IF(this->remote_auth_cfgs);
    free(this);
}

/**
 * Mock ike_cfg for testing
 */
typedef struct mock_ike_cfg_t {
    ike_cfg_t public;
    char *other_addr;
} mock_ike_cfg_t;

static char* mock_ike_cfg_get_other_addr(mock_ike_cfg_t *this)
{
    return this->other_addr;
}

static void mock_ike_cfg_destroy(mock_ike_cfg_t *this)
{
    free(this->other_addr);
    free(this);
}

/**
 * Mock host for testing
 */
typedef struct mock_host_t {
    host_t public;
    char *address;
} mock_host_t;

static void mock_host_destroy(mock_host_t *this)
{
    free(this->address);
    free(this);
}

// Helper functions for creating mock objects
static ike_cfg_t* create_mock_ike_cfg(const char *other_addr)
{
    mock_ike_cfg_t *mock;
    
    INIT(mock,
        .public = {
            .get_other_addr = (void*)mock_ike_cfg_get_other_addr,
            .destroy = (void*)mock_ike_cfg_destroy,
        },
        .other_addr = strdup(other_addr),
    );
    
    return &mock->public;
}

static peer_cfg_t* create_mock_peer_cfg(const char *name, const char *other_addr)
{
    mock_peer_cfg_t *mock;
    
    INIT(mock,
        .public = {
            .get_name = (void*)mock_peer_cfg_get_name,
            .get_ike_cfg = (void*)mock_peer_cfg_get_ike_cfg,
            .create_child_cfg_enumerator = (void*)mock_peer_cfg_create_child_cfg_enumerator,
            .destroy = (void*)mock_peer_cfg_destroy,
        },
        .name = strdup(name),
        .ike_cfg = create_mock_ike_cfg(other_addr),
        .child_cfgs = linked_list_create(),
        .local_auth_cfgs = linked_list_create(),
        .remote_auth_cfgs = linked_list_create(),
    );
    
    return &mock->public;
}

static host_t* create_mock_host(const char *address)
{
    mock_host_t *mock;
    
    INIT(mock,
        .public = {
            .destroy = (void*)mock_host_destroy,
        },
        .address = strdup(address),
    );
    
    return &mock->public;
}

static ike_sa_t* create_mock_ike_sa(const char *name, const char *other_addr, const char *current_addr)
{
    mock_ike_sa_t *mock;
    
    INIT(mock,
        .public = {
            .get_name = (void*)mock_ike_sa_get_name,
            .get_peer_cfg = (void*)mock_ike_sa_get_peer_cfg,
            .get_other_host = (void*)mock_ike_sa_get_other_host,
            .destroy = (void*)mock_ike_sa_destroy,
        },
        .name = strdup(name),
        .peer_cfg = create_mock_peer_cfg(name, other_addr),
        .other_host = create_mock_host(current_addr),
    );
    
    return &mock->public;
}

// Test setup and teardown
static void setup()
{
    // Mock config usecase 생성
    mock_config_usecase = create_mock_config_usecase();
    
    // Failover manager 생성
    failover_manager = extsock_failover_manager_create(mock_config_usecase);
    ck_assert_ptr_nonnull(failover_manager);
}

static void teardown()
{
    if (failover_manager) {
        failover_manager->destroy(failover_manager);
        failover_manager = NULL;
    }
    // mock_config_usecase는 failover_manager 소멸 시 함께 정리됨
    mock_config_usecase = NULL;
}

// 테스트 케이스들

START_TEST(test_select_next_segw_basic)
{
    char *result;
    
    // 기본 2개 주소 케이스
    result = failover_manager->select_next_segw(failover_manager, 
                                               "10.0.0.1,10.0.0.2", 
                                               "10.0.0.1");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "10.0.0.2");
    free(result);
    
    // 순환 테스트
    result = failover_manager->select_next_segw(failover_manager, 
                                               "10.0.0.1,10.0.0.2", 
                                               "10.0.0.2");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "10.0.0.1");
    free(result);
}
END_TEST

START_TEST(test_select_next_segw_multiple_addresses)
{
    char *result;
    
    // 3개 주소 케이스
    result = failover_manager->select_next_segw(failover_manager, 
                                               "192.168.1.1,192.168.1.2,192.168.1.3", 
                                               "192.168.1.1");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "192.168.1.2");
    free(result);
    
    result = failover_manager->select_next_segw(failover_manager, 
                                               "192.168.1.1,192.168.1.2,192.168.1.3", 
                                               "192.168.1.2");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "192.168.1.3");
    free(result);
    
    // 마지막에서 첫 번째로 순환
    result = failover_manager->select_next_segw(failover_manager, 
                                               "192.168.1.1,192.168.1.2,192.168.1.3", 
                                               "192.168.1.3");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "192.168.1.1");
    free(result);
}
END_TEST

START_TEST(test_select_next_segw_with_spaces)
{
    char *result;
    
    // 공백이 포함된 주소 목록
    result = failover_manager->select_next_segw(failover_manager, 
                                               " 10.0.0.1 , 10.0.0.2 , 10.0.0.3 ", 
                                               "10.0.0.1");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "10.0.0.2");
    free(result);
}
END_TEST

START_TEST(test_select_next_segw_edge_cases)
{
    char *result;
    
    // NULL 인자
    result = failover_manager->select_next_segw(failover_manager, NULL, "10.0.0.1");
    ck_assert_ptr_null(result);
    
    result = failover_manager->select_next_segw(failover_manager, "10.0.0.1,10.0.0.2", NULL);
    ck_assert_ptr_null(result);
    
    // 단일 주소 (failover 불가)
    result = failover_manager->select_next_segw(failover_manager, "10.0.0.1", "10.0.0.1");
    ck_assert_ptr_null(result);
    
    // 현재 주소가 목록에 없는 경우 (첫 번째 다음 주소 반환)
    result = failover_manager->select_next_segw(failover_manager, 
                                               "10.0.0.1,10.0.0.2", 
                                               "10.0.0.99");
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "10.0.0.2");
    free(result);
}
END_TEST

START_TEST(test_retry_count_management)
{
    // 재시도 횟수 관리 테스트
    const char *conn_name = "test-connection";
    
    // 초기 상태: 재시도 횟수 초과 안 함
    ck_assert(!failover_manager->is_max_retry_exceeded(failover_manager, conn_name));
    
    // 5번의 실패한 연결 시뮬레이션
    for (int i = 0; i < 5; i++) {
        ike_sa_t *mock_ike_sa = create_mock_ike_sa(conn_name, "10.0.0.1,10.0.0.2", "10.0.0.1");
        failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
        mock_ike_sa->destroy(mock_ike_sa);
    }
    
    // 최대 재시도 횟수 초과 확인
    ck_assert(failover_manager->is_max_retry_exceeded(failover_manager, conn_name));
    
    // 재시도 횟수 초기화
    failover_manager->reset_retry_count(failover_manager, conn_name);
    ck_assert(!failover_manager->is_max_retry_exceeded(failover_manager, conn_name));
}
END_TEST

START_TEST(test_handle_connection_failure_basic)
{
    mock_config_usecase_t *mock = (mock_config_usecase_t*)mock_config_usecase;
    mock->add_peer_config_result = EXTSOCK_SUCCESS;
    
    // 연결 실패 시뮬레이션
    ike_sa_t *mock_ike_sa = create_mock_ike_sa("test-conn", "10.0.0.1,10.0.0.2", "10.0.0.1");
    
    failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
    
    // Config usecase 호출 확인
    ck_assert_int_eq(mock->add_peer_config_call_count, 1);
    ck_assert_ptr_nonnull(mock->last_peer_name);
    ck_assert(strstr(mock->last_peer_name, "test-conn-failover") != NULL);
    
    mock_ike_sa->destroy(mock_ike_sa);
}
END_TEST

START_TEST(test_handle_connection_failure_edge_cases)
{
    mock_config_usecase_t *mock = (mock_config_usecase_t*)mock_config_usecase;
    
    // NULL IKE SA
    failover_manager->handle_connection_failure(failover_manager, NULL);
    ck_assert_int_eq(mock->add_peer_config_call_count, 0);
    
    // 단일 주소 (failover 불가)
    ike_sa_t *mock_ike_sa = create_mock_ike_sa("single-addr", "10.0.0.1", "10.0.0.1");
    failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
    ck_assert_int_eq(mock->add_peer_config_call_count, 0);
    mock_ike_sa->destroy(mock_ike_sa);
    
    // 쉼표가 없는 주소 (단일 주소로 판단)
    mock_ike_sa = create_mock_ike_sa("no-comma", "10.0.0.1", "10.0.0.1");
    failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
    ck_assert_int_eq(mock->add_peer_config_call_count, 0);
    mock_ike_sa->destroy(mock_ike_sa);
}
END_TEST

START_TEST(test_handle_connection_failure_max_retry)
{
    mock_config_usecase_t *mock = (mock_config_usecase_t*)mock_config_usecase;
    const char *conn_name = "max-retry-test";
    
    // 최대 재시도 횟수까지 실패 시뮬레이션
    for (int i = 0; i < 5; i++) {
        ike_sa_t *mock_ike_sa = create_mock_ike_sa(conn_name, "10.0.0.1,10.0.0.2", "10.0.0.1");
        failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
        mock_ike_sa->destroy(mock_ike_sa);
    }
    
    int call_count_before_limit = mock->add_peer_config_call_count;
    
    // 한 번 더 시도 (최대 재시도 초과)
    ike_sa_t *mock_ike_sa = create_mock_ike_sa(conn_name, "10.0.0.1,10.0.0.2", "10.0.0.1");
    failover_manager->handle_connection_failure(failover_manager, mock_ike_sa);
    mock_ike_sa->destroy(mock_ike_sa);
    
    // 재시도 횟수 초과로 인해 추가 호출이 없어야 함
    ck_assert_int_eq(mock->add_peer_config_call_count, call_count_before_limit);
}
END_TEST

// Test Suite 생성
Suite *failover_manager_suite(void)
{
    Suite *s;
    TCase *tc_core;
    
    s = suite_create("Failover Manager");
    
    // Core test case
    tc_core = tcase_create("Core");
    
    tcase_add_checked_fixture(tc_core, setup, teardown);
    
    // 주소 선택 테스트
    tcase_add_test(tc_core, test_select_next_segw_basic);
    tcase_add_test(tc_core, test_select_next_segw_multiple_addresses);
    tcase_add_test(tc_core, test_select_next_segw_with_spaces);
    tcase_add_test(tc_core, test_select_next_segw_edge_cases);
    
    // 재시도 관리 테스트
    tcase_add_test(tc_core, test_retry_count_management);
    
    // 연결 실패 처리 테스트
    tcase_add_test(tc_core, test_handle_connection_failure_basic);
    tcase_add_test(tc_core, test_handle_connection_failure_edge_cases);
    tcase_add_test(tc_core, test_handle_connection_failure_max_retry);
    
    suite_add_tcase(s, tc_core);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    // Library 초기화
    if (!library_init(NULL, "test-failover")) {
        library_deinit();
        return EXIT_FAILURE;
    }
    
    s = failover_manager_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    library_deinit();
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 