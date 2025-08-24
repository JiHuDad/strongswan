/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Level 2 Adapter Unit Tests for extsock_strongswan_adapter
 * TASK-009: strongSwan Adapter 실제 테스트
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test infrastructure
#include "../infrastructure/test_container.h"
#include "../infrastructure/strongswan_mocks.h"

// Mock implementations
#include "extsock_strongswan_adapter_mock.h"

// Test counter
static int test_count = 0;
static int tests_passed = 0;

// Test helper macros
#define TEST_START() printf("Test %d: %s... ", ++test_count, __func__)
#define TEST_PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define TEST_FAIL(msg) do { printf("FAIL - %s\n", msg); return; } while(0)
#define TEST_ASSERT(condition, msg) do { if (!(condition)) TEST_FAIL(msg); } while(0)

/**
 * Test: strongSwan Adapter 생성 및 소멸
 */
static void test_strongswan_adapter_create_destroy()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Verify initial state
    TEST_ASSERT(adapter->add_peer_config != NULL, "add_peer_config method missing");
    TEST_ASSERT(adapter->remove_peer_config != NULL, "remove_peer_config method missing");
    TEST_ASSERT(adapter->initiate_child_sa != NULL, "initiate_child_sa method missing");
    TEST_ASSERT(adapter->get_managed_configs != NULL, "get_managed_configs method missing");
    TEST_ASSERT(adapter->get_credentials != NULL, "get_credentials method missing");
    TEST_ASSERT(adapter->destroy != NULL, "destroy method missing");
    
    // Verify config repository interface
    TEST_ASSERT(adapter->config_repository.apply_config != NULL, "apply_config method missing");
    TEST_ASSERT(adapter->config_repository.remove_config != NULL, "remove_config method missing");
    TEST_ASSERT(adapter->config_repository.start_dpd != NULL, "start_dpd method missing");
    TEST_ASSERT(adapter->config_repository.destroy != NULL, "destroy method missing");
    
    // Test basic functionality
    linked_list_t *configs = adapter->get_managed_configs(adapter);
    TEST_ASSERT(configs != NULL, "Failed to get managed configs");
    
    mem_cred_t *creds = adapter->get_credentials(adapter);
    TEST_ASSERT(creds != NULL, "Failed to get credentials");
    
    // Cleanup
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 피어 설정 추가
 */
static void test_add_peer_config()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Create test peer config
    peer_cfg_t *peer_cfg = (peer_cfg_t*)mock_peer_cfg_create("test_peer", NULL);
    TEST_ASSERT(peer_cfg, "Failed to create peer config");
    
    // Add peer config
    extsock_error_t result = adapter->add_peer_config(adapter, peer_cfg);
    TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to add peer config");
    
    // Verify mock state
    mock_strongswan_state_t *state = mock_strongswan_get_state();
    TEST_ASSERT(state->add_peer_config_calls == 1, "Expected 1 add_peer_config call");
    TEST_ASSERT(strcmp(state->last_peer_name, "test_peer") == 0, "Wrong peer name recorded");
    
    // Verify managed configs
    linked_list_t *managed = adapter->get_managed_configs(adapter);
    TEST_ASSERT(managed != NULL, "Managed configs is NULL");
    mock_linked_list_t *mock_managed = (mock_linked_list_t*)managed;
    TEST_ASSERT(mock_managed->count == 1, "Expected 1 managed config");
    
    // Cleanup
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 피어 설정 제거
 */
static void test_remove_peer_config()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Remove peer config
    extsock_error_t result = adapter->remove_peer_config(adapter, "test_peer");
    TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to remove peer config");
    
    // Verify mock state
    mock_strongswan_state_t *state = mock_strongswan_get_state();
    TEST_ASSERT(state->remove_peer_config_calls == 1, "Expected 1 remove_peer_config call");
    TEST_ASSERT(strcmp(state->last_removed_name, "test_peer") == 0, "Wrong removed name recorded");
    
    // Cleanup
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: Child SA 개시
 */
static void test_initiate_child_sa()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Create test configs
    peer_cfg_t *peer_cfg = (peer_cfg_t*)mock_peer_cfg_create("test_peer", NULL);
    child_cfg_t *child_cfg = (child_cfg_t*)mock_child_cfg_create("test_child");
    TEST_ASSERT(peer_cfg && child_cfg, "Failed to create configs");
    
    // Initiate child SA
    extsock_error_t result = adapter->initiate_child_sa(adapter, peer_cfg, child_cfg);
    TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to initiate child SA");
    
    // Verify mock state
    mock_strongswan_state_t *state = mock_strongswan_get_state();
    TEST_ASSERT(state->initiate_child_sa_calls == 1, "Expected 1 initiate_child_sa call");
    
    // Cleanup
    mock_peer_cfg_t *mock_peer = (mock_peer_cfg_t*)peer_cfg;
    mock_child_cfg_t *mock_child = (mock_child_cfg_t*)child_cfg;
    mock_peer->destroy(mock_peer);
    mock_child->destroy(mock_child);
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: Config Repository - Apply Config
 */
static void test_config_repository_apply_config()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Create config entity
    extsock_config_entity_t *config = mock_config_entity_create("test_config");
    TEST_ASSERT(config, "Failed to create config entity");
    
    // Apply config through repository interface
    extsock_error_t result = adapter->config_repository.apply_config(&adapter->config_repository, config);
    TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to apply config");
    
    // Verify mock state
    mock_strongswan_state_t *state = mock_strongswan_get_state();
    TEST_ASSERT(state->apply_config_calls == 1, "Expected 1 apply_config call");
    TEST_ASSERT(strcmp(state->last_peer_name, "test_config") == 0, "Wrong peer name from config");
    
    // Cleanup
    config->destroy(config);
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: Config Repository - Remove Config
 */
static void test_config_repository_remove_config()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Remove config through repository interface
    extsock_error_t result = adapter->config_repository.remove_config(&adapter->config_repository, "test_config");
    TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to remove config");
    
    // Verify mock state
    mock_strongswan_state_t *state = mock_strongswan_get_state();
    TEST_ASSERT(state->remove_config_calls == 1, "Expected 1 remove_config call");
    TEST_ASSERT(strcmp(state->last_removed_name, "test_config") == 0, "Wrong removed config name");
    
    // Cleanup
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: Config Repository - Start DPD
 */
static void test_config_repository_start_dpd()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Start DPD through repository interface
    extsock_error_t result = adapter->config_repository.start_dpd(&adapter->config_repository, "test_ike_sa");
    TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to start DPD");
    
    // Verify mock state
    mock_strongswan_state_t *state = mock_strongswan_get_state();
    TEST_ASSERT(state->start_dpd_calls == 1, "Expected 1 start_dpd call");
    TEST_ASSERT(strcmp(state->last_ike_sa_name, "test_ike_sa") == 0, "Wrong IKE SA name");
    
    // Cleanup
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: NULL 포인터 안전성
 */
static void test_null_pointer_safety()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Test NULL peer config
    extsock_error_t result = adapter->add_peer_config(adapter, NULL);
    TEST_ASSERT(result == EXTSOCK_ERROR_CONFIG_INVALID, "Should reject NULL peer config");
    
    // Test NULL name
    result = adapter->remove_peer_config(adapter, NULL);
    TEST_ASSERT(result == EXTSOCK_ERROR_CONFIG_INVALID, "Should reject NULL name");
    
    // Test NULL child SA params
    result = adapter->initiate_child_sa(adapter, NULL, NULL);
    TEST_ASSERT(result == EXTSOCK_ERROR_CONFIG_INVALID, "Should reject NULL child SA params");
    
    // Test NULL config entity
    result = adapter->config_repository.apply_config(&adapter->config_repository, NULL);
    TEST_ASSERT(result == EXTSOCK_ERROR_CONFIG_INVALID, "Should reject NULL config entity");
    
    // Test NULL config name
    result = adapter->config_repository.remove_config(&adapter->config_repository, NULL);
    TEST_ASSERT(result == EXTSOCK_ERROR_CONFIG_INVALID, "Should reject NULL config name");
    
    // Test NULL IKE SA name
    result = adapter->config_repository.start_dpd(&adapter->config_repository, NULL);
    TEST_ASSERT(result == EXTSOCK_ERROR_CONFIG_INVALID, "Should reject NULL IKE SA name");
    
    // Cleanup
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 오류 시뮬레이션
 */
static void test_error_simulation()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset and enable failure simulation
    mock_strongswan_reset_state();
    mock_strongswan_simulate_failure(true, EXTSOCK_ERROR_STRONGSWAN_API);
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Test failure simulation
    peer_cfg_t *peer_cfg = (peer_cfg_t*)mock_peer_cfg_create("test_peer", NULL);
    extsock_error_t result = adapter->add_peer_config(adapter, peer_cfg);
    TEST_ASSERT(result == EXTSOCK_ERROR_STRONGSWAN_API, "Expected simulated failure");
    
    result = adapter->remove_peer_config(adapter, "test");
    TEST_ASSERT(result == EXTSOCK_ERROR_STRONGSWAN_API, "Expected simulated failure");
    
    child_cfg_t *child_cfg = (child_cfg_t*)mock_child_cfg_create("test_child");
    result = adapter->initiate_child_sa(adapter, peer_cfg, child_cfg);
    TEST_ASSERT(result == EXTSOCK_ERROR_STRONGSWAN_API, "Expected simulated failure");
    
    // Disable failure simulation
    mock_strongswan_simulate_failure(false, EXTSOCK_SUCCESS);
    result = adapter->add_peer_config(adapter, peer_cfg);
    TEST_ASSERT(result == EXTSOCK_SUCCESS, "Should succeed after disabling failure simulation");
    
    // Cleanup
    mock_peer_cfg_t *mock_peer = (mock_peer_cfg_t*)peer_cfg;
    mock_child_cfg_t *mock_child = (mock_child_cfg_t*)child_cfg;
    mock_peer->destroy(mock_peer);
    mock_child->destroy(mock_child);
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 메모리 추적 및 누수 방지
 */
static void test_memory_tracking()
{
    TEST_START();
    
    // Container setup with memory tracking
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Take initial memory snapshot
    CONTAINER_TAKE_MEMORY_SNAPSHOT(container, "initial");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create and use strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Perform operations that allocate memory
    peer_cfg_t *peer_cfg = (peer_cfg_t*)mock_peer_cfg_create("test_peer", NULL);
    adapter->add_peer_config(adapter, peer_cfg);
    
    child_cfg_t *child_cfg = (child_cfg_t*)mock_child_cfg_create("test_child");
    adapter->initiate_child_sa(adapter, peer_cfg, child_cfg);
    
    extsock_config_entity_t *config = mock_config_entity_create("test_config");
    adapter->config_repository.apply_config(&adapter->config_repository, config);
    
    // Cleanup all resources
    config->destroy(config);
    mock_child_cfg_t *mock_child2 = (mock_child_cfg_t*)child_cfg;
    mock_child2->destroy(mock_child2);
    adapter->destroy(adapter);
    
    // Verify no memory leaks
    CONTAINER_ASSERT_MEMORY_UNCHANGED_SINCE_SNAPSHOT(container, "initial");
    
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 복잡한 워크플로우
 */
static void test_complex_workflow()
{
    TEST_START();
    
    // Container setup
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Complex workflow: multiple operations
    for (int i = 0; i < 3; i++) {
        char name[256];
        snprintf(name, sizeof(name), "peer_%d", i);
        
        // Create and add peer config
        peer_cfg_t *peer_cfg = (peer_cfg_t*)mock_peer_cfg_create(name, NULL);
        extsock_error_t result = adapter->add_peer_config(adapter, peer_cfg);
        TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to add peer config in workflow");
        
        // Create and initiate child SA
        child_cfg_t *child_cfg = (child_cfg_t*)mock_child_cfg_create("child");
        result = adapter->initiate_child_sa(adapter, peer_cfg, child_cfg);
        TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to initiate child SA in workflow");
        
        mock_child_cfg_t *mock_child = (mock_child_cfg_t*)child_cfg;
        mock_child->destroy(mock_child);
    }
    
    // Verify all operations were called
    mock_strongswan_state_t *state = mock_strongswan_get_state();
    TEST_ASSERT(state->add_peer_config_calls == 3, "Expected 3 add_peer_config calls");
    TEST_ASSERT(state->initiate_child_sa_calls == 3, "Expected 3 initiate_child_sa calls");
    
    // Verify managed configs count
    linked_list_t *managed = adapter->get_managed_configs(adapter);
    mock_linked_list_t *mock_managed = (mock_linked_list_t*)managed;
    TEST_ASSERT(mock_managed->count == 3, "Expected 3 managed configs");
    
    // Remove configs
    for (int i = 0; i < 3; i++) {
        char name[256];
        snprintf(name, sizeof(name), "peer_%d", i);
        extsock_error_t result = adapter->remove_peer_config(adapter, name);
        TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed to remove peer config in workflow");
    }
    
    // Re-fetch state after operations
    state = mock_strongswan_get_state();
    TEST_ASSERT(state->remove_peer_config_calls == 3, "Expected 3 remove_peer_config calls");
    
    // Cleanup
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Test: 스트레스 테스트
 */
static void test_stress_operations()
{
    TEST_START();
    
    // Container setup with memory monitoring
    test_container_t *container = test_container_create_adapter();
    TEST_ASSERT(container, "Failed to create adapter container");
    
    CONTAINER_SET_MEMORY_WARNING_THRESHOLD(container, 1024 * 1024); // 1MB threshold
    
    // Mock reset
    mock_strongswan_reset_state();
    
    // Create strongSwan adapter
    extsock_strongswan_adapter_t *adapter = extsock_strongswan_adapter_create();
    TEST_ASSERT(adapter, "Failed to create strongSwan adapter");
    
    // Stress test: many operations
    const int stress_count = 100;
    
    // Add many peer configs
    for (int i = 0; i < stress_count; i++) {
        char name[256];
        snprintf(name, sizeof(name), "stress_peer_%d", i);
        
        peer_cfg_t *peer_cfg = (peer_cfg_t*)mock_peer_cfg_create(name, NULL);
        extsock_error_t result = adapter->add_peer_config(adapter, peer_cfg);
        TEST_ASSERT(result == EXTSOCK_SUCCESS, "Failed in stress test add");
    }
    
    // Verify all operations
    mock_strongswan_state_t *state = mock_strongswan_get_state();
    TEST_ASSERT(state->add_peer_config_calls == stress_count, "Stress test call count mismatch");
    
    // Verify managed configs
    linked_list_t *managed = adapter->get_managed_configs(adapter);
    mock_linked_list_t *mock_managed = (mock_linked_list_t*)managed;
    TEST_ASSERT(mock_managed->count == stress_count, "Stress test managed count mismatch");
    
    // Memory usage should be under threshold
    CONTAINER_ASSERT_MEMORY_USAGE_UNDER(container, 1024 * 1024);
    
    // Cleanup
    adapter->destroy(adapter);
    container->destroy(container);
    
    TEST_PASS();
}

/**
 * Main test runner
 */
int main()
{
    printf("=== strongSwan Adapter Level 2 Tests ===\n\n");
    
    // Run all tests
    test_strongswan_adapter_create_destroy();
    test_add_peer_config();
    test_remove_peer_config();
    test_initiate_child_sa();
    test_config_repository_apply_config();
    test_config_repository_remove_config();
    test_config_repository_start_dpd();
    test_null_pointer_safety();
    test_error_simulation();
    test_memory_tracking();
    test_complex_workflow();
    test_stress_operations();
    
    // Print results
    printf("\n=== Test Results ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", test_count - tests_passed);
    
    if (tests_passed == test_count) {
        printf("✅ All tests PASSED!\n");
        return 0;
    } else {
        printf("❌ Some tests FAILED!\n");
        return 1;
    }
}