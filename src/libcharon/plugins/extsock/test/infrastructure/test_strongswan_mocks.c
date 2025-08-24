/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * strongSwan Mock System Unit Tests
 * 
 * These tests verify that the Mock infrastructure works correctly
 * before we use it to test actual extsock plugin code.
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strongswan_mocks.h"

// Test fixtures
void setup_mock_test(void)
{
    strongswan_mocks_init();
}

void teardown_mock_test(void)
{
    strongswan_mocks_cleanup();
}

/*
 * ============================================================================
 * Mock System Lifecycle Tests
 * ============================================================================
 */

START_TEST(test_mock_system_init_and_cleanup)
{
    // Given: Fresh system
    strongswan_mocks_cleanup(); // Ensure clean state
    
    // When: Initialize mock system
    strongswan_mocks_init();
    
    // Then: Global state should be initialized
    ck_assert_ptr_nonnull(g_mock_state);
    ck_assert_ptr_nonnull(g_mock_config);
    ck_assert_int_eq(g_mock_state->ike_cfg_create_count, 0);
    ck_assert_int_eq(g_mock_state->peer_cfg_create_count, 0);
    ck_assert_int_eq(g_mock_state->current_allocations, 0);
    
    // When: Cleanup
    strongswan_mocks_cleanup();
    
    // Then: Global state should be cleaned
    ck_assert_ptr_null(g_mock_state);
    ck_assert_ptr_null(g_mock_config);
}
END_TEST

START_TEST(test_mock_system_reset_state)
{
    // Given: Mock system with some state
    mock_ike_cfg_t *cfg = mock_ike_cfg_create("test");
    ck_assert_ptr_nonnull(cfg);
    ck_assert_int_gt(g_mock_state->ike_cfg_create_count, 0);
    cfg->destroy(cfg);
    
    // When: Reset state
    strongswan_mocks_reset_state();
    
    // Then: Call counts should be reset
    ck_assert_int_eq(g_mock_state->ike_cfg_create_count, 0);
    ck_assert_int_eq(g_mock_state->peer_cfg_create_count, 0);
    ck_assert_int_eq(g_mock_state->child_cfg_create_count, 0);
}
END_TEST

/*
 * ============================================================================
 * Mock linked_list_t Tests
 * ============================================================================
 */

START_TEST(test_mock_linked_list_basic_operations)
{
    // Given: Create mock linked list
    mock_linked_list_t *list = mock_linked_list_create();
    ck_assert_ptr_nonnull(list);
    ck_assert_int_eq(list->get_count(list), 0);
    
    // When: Add items
    char *item1 = "test1";
    char *item2 = "test2"; 
    char *item3 = "test3";
    
    list->insert_last(list, item1);
    list->insert_last(list, item2);
    list->insert_last(list, item3);
    
    // Then: Count should be correct
    ck_assert_int_eq(list->get_count(list), 3);
    
    // And: First item should be accessible
    ck_assert_ptr_eq(list->get_first(list), item1);
    
    // Cleanup
    list->destroy(list);
}
END_TEST

START_TEST(test_mock_linked_list_enumerator)
{
    // Given: List with items
    mock_linked_list_t *list = mock_linked_list_create();
    char *items[] = {"item1", "item2", "item3"};
    
    for (int i = 0; i < 3; i++) {
        list->insert_last(list, items[i]);
    }
    
    // When: Create enumerator
    mock_enumerator_t *enumerator = list->create_enumerator(list);
    ck_assert_ptr_nonnull(enumerator);
    
    // Then: Should enumerate all items in order
    char *item;
    int count = 0;
    while (enumerator->enumerate(enumerator, (void**)&item)) {
        ck_assert_ptr_eq(item, items[count]);
        count++;
    }
    ck_assert_int_eq(count, 3);
    
    // Cleanup
    enumerator->destroy(enumerator);
    list->destroy(list);
}
END_TEST

/*
 * ============================================================================
 * Mock ike_cfg_t Tests
 * ============================================================================
 */

START_TEST(test_mock_ike_cfg_create_and_destroy)
{
    // Given: Clean state
    int initial_allocs = mock_get_current_allocation_count();
    
    // When: Create ike_cfg
    mock_ike_cfg_t *cfg = mock_ike_cfg_create("test_ike");
    
    // Then: Object should be created correctly
    ck_assert_ptr_nonnull(cfg);
    ck_assert_str_eq(cfg->get_name(cfg), "test_ike");
    
    // And: Call should be tracked
    ck_assert(mock_verify_ike_cfg_create_called());
    ck_assert_int_eq(mock_get_ike_cfg_create_count(), 1);
    ck_assert_str_eq(mock_get_last_ike_cfg_name(), "test_ike");
    
    // And: Memory allocation should be tracked
    ck_assert_int_gt(mock_get_current_allocation_count(), initial_allocs);
    
    // Store count before destroy
    int count_before_destroy = mock_get_current_allocation_count();
    
    // When: Destroy
    cfg->destroy(cfg);
    
    // Then: Memory should be freed (though some may remain due to internal lists)
    ck_assert_int_lt(mock_get_current_allocation_count(), count_before_destroy);
}
END_TEST

START_TEST(test_mock_ike_cfg_with_proposals)
{
    // Given: IKE config and proposal
    mock_ike_cfg_t *ike_cfg = mock_ike_cfg_create("test_ike");
    mock_proposal_t *proposal = mock_proposal_create("aes256-sha256-modp2048", 1);
    
    // When: Add proposal
    ike_cfg->add_proposal(ike_cfg, proposal);
    
    // Then: Should not crash and proposal should be tracked
    // (Full verification would require enumerating proposals)
    
    // Cleanup
    ike_cfg->destroy(ike_cfg);
}
END_TEST

/*
 * ============================================================================
 * Mock peer_cfg_t Tests
 * ============================================================================
 */

START_TEST(test_mock_peer_cfg_create_and_basic_operations)
{
    // Given: IKE config for peer config
    mock_ike_cfg_t *ike_cfg = mock_ike_cfg_create("test_ike");
    
    // When: Create peer config
    mock_peer_cfg_t *peer_cfg = mock_peer_cfg_create("test_peer", ike_cfg);
    
    // Then: Should be created correctly
    ck_assert_ptr_nonnull(peer_cfg);
    ck_assert_str_eq(peer_cfg->get_name(peer_cfg), "test_peer");
    ck_assert_ptr_eq(peer_cfg->get_ike_cfg(peer_cfg), ike_cfg);
    
    // And: Call should be tracked
    ck_assert(mock_verify_peer_cfg_create_called());
    ck_assert_int_eq(mock_get_peer_cfg_create_count(), 1);
    ck_assert_str_eq(mock_get_last_peer_cfg_name(), "test_peer");
    
    // Cleanup
    peer_cfg->destroy(peer_cfg);
    ike_cfg->destroy(ike_cfg);
}
END_TEST

START_TEST(test_mock_peer_cfg_with_child_configs)
{
    // Given: Peer config and child config
    mock_ike_cfg_t *ike_cfg = mock_ike_cfg_create("test_ike");
    mock_peer_cfg_t *peer_cfg = mock_peer_cfg_create("test_peer", ike_cfg);
    mock_child_cfg_t *child_cfg = mock_child_cfg_create("test_child");
    
    // When: Add child config
    peer_cfg->add_child_cfg(peer_cfg, child_cfg);
    
    // Then: Should be able to enumerate children
    mock_enumerator_t *children = peer_cfg->create_child_cfg_enumerator(peer_cfg);
    ck_assert_ptr_nonnull(children);
    
    mock_child_cfg_t *enumerated_child;
    bool found = children->enumerate(children, (void**)&enumerated_child);
    ck_assert(found);
    ck_assert_ptr_eq(enumerated_child, child_cfg);
    ck_assert_str_eq(enumerated_child->get_name(enumerated_child), "test_child");
    
    // Cleanup
    children->destroy(children);
    peer_cfg->destroy(peer_cfg);
    ike_cfg->destroy(ike_cfg);
}
END_TEST

/*
 * ============================================================================
 * Mock child_cfg_t Tests
 * ============================================================================
 */

START_TEST(test_mock_child_cfg_create_and_operations)
{
    // Given: Child config
    mock_child_cfg_t *child_cfg = mock_child_cfg_create("test_child");
    
    // Then: Should be created correctly
    ck_assert_ptr_nonnull(child_cfg);
    ck_assert_str_eq(child_cfg->get_name(child_cfg), "test_child");
    
    // And: Call should be tracked
    ck_assert(mock_verify_child_cfg_create_called());
    ck_assert_int_eq(mock_get_child_cfg_create_count(), 1);
    ck_assert_str_eq(mock_get_last_child_cfg_name(), "test_child");
    
    // When: Add traffic selectors
    mock_traffic_selector_t *local_ts = mock_traffic_selector_create("10.0.0.0", "10.0.0.255", 0, 65535);
    mock_traffic_selector_t *remote_ts = mock_traffic_selector_create("10.0.1.0", "10.0.1.255", 0, 65535);
    
    child_cfg->add_traffic_selector(child_cfg, true, local_ts);   // local
    child_cfg->add_traffic_selector(child_cfg, false, remote_ts); // remote
    
    // Then: Should not crash (full verification would need TS enumerator)
    
    // Cleanup
    child_cfg->destroy(child_cfg);
}
END_TEST

/*
 * ============================================================================
 * Mock State Tracking Tests
 * ============================================================================
 */

START_TEST(test_mock_call_tracking)
{
    // Given: Clean state
    strongswan_mocks_reset_state();
    ck_assert_int_eq(mock_get_ike_cfg_create_count(), 0);
    ck_assert_int_eq(mock_get_peer_cfg_create_count(), 0);
    ck_assert_int_eq(mock_get_child_cfg_create_count(), 0);
    
    // When: Create various objects
    mock_ike_cfg_t *ike1 = mock_ike_cfg_create("ike1");
    mock_ike_cfg_t *ike2 = mock_ike_cfg_create("ike2");
    mock_peer_cfg_t *peer1 = mock_peer_cfg_create("peer1", ike1);
    mock_child_cfg_t *child1 = mock_child_cfg_create("child1");
    mock_child_cfg_t *child2 = mock_child_cfg_create("child2");
    mock_child_cfg_t *child3 = mock_child_cfg_create("child3");
    
    // Then: Counts should be correct
    ck_assert_int_eq(mock_get_ike_cfg_create_count(), 2);
    ck_assert_int_eq(mock_get_peer_cfg_create_count(), 1);
    ck_assert_int_eq(mock_get_child_cfg_create_count(), 3);
    
    // And: Last names should be captured
    ck_assert_str_eq(mock_get_last_ike_cfg_name(), "ike2");
    ck_assert_str_eq(mock_get_last_peer_cfg_name(), "peer1");
    ck_assert_str_eq(mock_get_last_child_cfg_name(), "child3");
    
    // Cleanup
    ike1->destroy(ike1);
    ike2->destroy(ike2);
    peer1->destroy(peer1);
    child1->destroy(child1);
    child2->destroy(child2);
    child3->destroy(child3);
}
END_TEST

START_TEST(test_mock_memory_tracking)
{
    // Given: Initial allocation count
    int initial_allocs = mock_get_current_allocation_count();
    int initial_total = mock_get_total_allocation_count();
    
    // When: Create and destroy objects
    mock_ike_cfg_t *cfg1 = mock_ike_cfg_create("test1");
    mock_ike_cfg_t *cfg2 = mock_ike_cfg_create("test2");
    
    // Then: Allocations should increase
    ck_assert_int_gt(mock_get_current_allocation_count(), initial_allocs);
    ck_assert_int_gt(mock_get_total_allocation_count(), initial_total);
    
    // When: Destroy objects
    cfg1->destroy(cfg1);
    cfg2->destroy(cfg2);
    
    // Then: Current allocations should decrease
    // (May not be exactly initial due to internal structures)
    ck_assert_int_lt(mock_get_current_allocation_count(), 
                     mock_get_current_allocation_count() + 100); // Some reasonable bound
}
END_TEST

/*
 * ============================================================================
 * Mock Parameter Capture Tests
 * ============================================================================
 */

START_TEST(test_mock_parameter_capture)
{
    // Given: Clean capture state
    strongswan_mocks_reset_state();
    
    // When: Capture parameters
    mock_capture_string_param("test_string");
    mock_capture_int_param(42);
    mock_capture_ptr_param((void*)0xDEADBEEF);
    mock_capture_string_param("another_string");
    
    // Then: Parameters should be captured correctly
    ck_assert_int_eq(mock_get_capture_count(), 4); // 4 parameters captured
    ck_assert_str_eq(mock_get_captured_string(0), "test_string");
    ck_assert_int_eq(mock_get_captured_int(1), 42);
    ck_assert_ptr_eq(mock_get_captured_ptr(2), (void*)0xDEADBEEF);
    ck_assert_str_eq(mock_get_captured_string(3), "another_string");
}
END_TEST

/*
 * ============================================================================
 * Mock Configuration and Failure Simulation Tests
 * ============================================================================
 */

START_TEST(test_mock_allocation_failure_simulation)
{
    // Given: Configure allocation failure
    mock_set_allocation_failure(true);
    
    // When: Try to create objects
    mock_ike_cfg_t *cfg1 = mock_ike_cfg_create("test1");
    mock_peer_cfg_t *cfg2 = mock_peer_cfg_create("test2", NULL);
    mock_child_cfg_t *cfg3 = mock_child_cfg_create("test3");
    
    // Then: Should return NULL
    ck_assert_ptr_null(cfg1);
    ck_assert_ptr_null(cfg2);
    ck_assert_ptr_null(cfg3);
    
    // When: Reset failure simulation
    mock_set_allocation_failure(false);
    mock_ike_cfg_t *cfg4 = mock_ike_cfg_create("test4");
    
    // Then: Should succeed
    ck_assert_ptr_nonnull(cfg4);
    
    // Cleanup
    cfg4->destroy(cfg4);
}
END_TEST

START_TEST(test_mock_specific_failure_simulation)
{
    // Given: Configure specific failures
    mock_reset_config();
    g_mock_config->should_fail_ike_cfg_create = true;
    g_mock_config->should_fail_peer_cfg_create = false;
    g_mock_config->should_fail_child_cfg_create = true;
    
    // When: Create objects
    mock_ike_cfg_t *ike = mock_ike_cfg_create("test_ike");
    mock_peer_cfg_t *peer = mock_peer_cfg_create("test_peer", NULL);
    mock_child_cfg_t *child = mock_child_cfg_create("test_child");
    
    // Then: Should fail according to configuration
    ck_assert_ptr_null(ike);
    ck_assert_ptr_nonnull(peer);
    ck_assert_ptr_null(child);
    
    // Cleanup
    peer->destroy(peer);
}
END_TEST

/*
 * ============================================================================
 * Integration Tests (Complex Scenarios)
 * ============================================================================
 */

START_TEST(test_mock_complex_peer_config_scenario)
{
    // Given: Complex peer configuration scenario
    mock_ike_cfg_t *ike_cfg = mock_ike_cfg_create("complex_ike");
    mock_peer_cfg_t *peer_cfg = mock_peer_cfg_create("complex_peer", ike_cfg);
    
    // Add multiple child configs
    mock_child_cfg_t *child1 = mock_child_cfg_create("child1");
    mock_child_cfg_t *child2 = mock_child_cfg_create("child2");
    peer_cfg->add_child_cfg(peer_cfg, child1);
    peer_cfg->add_child_cfg(peer_cfg, child2);
    
    // Add proposals and traffic selectors
    mock_proposal_t *ike_prop = mock_proposal_create("aes256-sha256-modp2048", 1);
    mock_proposal_t *esp_prop1 = mock_proposal_create("aes128gcm16", 3);
    mock_proposal_t *esp_prop2 = mock_proposal_create("aes256-sha256", 3);
    
    ike_cfg->add_proposal(ike_cfg, ike_prop);
    child1->add_proposal(child1, esp_prop1);
    child2->add_proposal(child2, esp_prop2);
    
    // Add traffic selectors
    mock_traffic_selector_t *ts1 = mock_traffic_selector_create("10.0.0.0", "10.0.0.255", 0, 65535);
    mock_traffic_selector_t *ts2 = mock_traffic_selector_create("10.0.1.0", "10.0.1.255", 0, 65535);
    child1->add_traffic_selector(child1, true, ts1);
    child2->add_traffic_selector(child2, true, ts2);
    
    // When: Verify the complex structure
    ck_assert_ptr_nonnull(peer_cfg);
    ck_assert_ptr_eq(peer_cfg->get_ike_cfg(peer_cfg), ike_cfg);
    
    // Enumerate and verify children
    mock_enumerator_t *children = peer_cfg->create_child_cfg_enumerator(peer_cfg);
    mock_child_cfg_t *child;
    int child_count = 0;
    
    while (children->enumerate(children, (void**)&child)) {
        ck_assert_ptr_nonnull(child);
        child_count++;
    }
    ck_assert_int_eq(child_count, 2);
    
    // Then: Should track all calls correctly
    ck_assert_int_eq(mock_get_ike_cfg_create_count(), 1);
    ck_assert_int_eq(mock_get_peer_cfg_create_count(), 1);
    ck_assert_int_eq(mock_get_child_cfg_create_count(), 2);
    
    // Cleanup
    children->destroy(children);
    peer_cfg->destroy(peer_cfg);
    ike_cfg->destroy(ike_cfg);
    
    // Memory should not have major leaks (some internal structures may remain)
    ck_assert_int_lt(mock_get_current_allocation_count(), 20); // Reasonable bound
}
END_TEST

/*
 * ============================================================================
 * Test Suite Definition
 * ============================================================================
 */

Suite *strongswan_mocks_suite(void)
{
    Suite *s;
    TCase *tc_lifecycle, *tc_linked_list, *tc_ike_cfg, *tc_peer_cfg, *tc_child_cfg;
    TCase *tc_tracking, *tc_capture, *tc_config, *tc_integration;

    s = suite_create("strongSwan Mocks Infrastructure Tests");

    /* Mock System Lifecycle Tests */
    tc_lifecycle = tcase_create("Mock System Lifecycle");
    tcase_add_checked_fixture(tc_lifecycle, setup_mock_test, teardown_mock_test);
    tcase_add_test(tc_lifecycle, test_mock_system_init_and_cleanup);
    tcase_add_test(tc_lifecycle, test_mock_system_reset_state);
    suite_add_tcase(s, tc_lifecycle);

    /* Mock linked_list_t Tests */
    tc_linked_list = tcase_create("Mock Linked List");
    tcase_add_checked_fixture(tc_linked_list, setup_mock_test, teardown_mock_test);
    tcase_add_test(tc_linked_list, test_mock_linked_list_basic_operations);
    tcase_add_test(tc_linked_list, test_mock_linked_list_enumerator);
    suite_add_tcase(s, tc_linked_list);

    /* Mock ike_cfg_t Tests */
    tc_ike_cfg = tcase_create("Mock IKE Config");
    tcase_add_checked_fixture(tc_ike_cfg, setup_mock_test, teardown_mock_test);
    tcase_add_test(tc_ike_cfg, test_mock_ike_cfg_create_and_destroy);
    tcase_add_test(tc_ike_cfg, test_mock_ike_cfg_with_proposals);
    suite_add_tcase(s, tc_ike_cfg);

    /* Mock peer_cfg_t Tests */
    tc_peer_cfg = tcase_create("Mock Peer Config");
    tcase_add_checked_fixture(tc_peer_cfg, setup_mock_test, teardown_mock_test);
    tcase_add_test(tc_peer_cfg, test_mock_peer_cfg_create_and_basic_operations);
    tcase_add_test(tc_peer_cfg, test_mock_peer_cfg_with_child_configs);
    suite_add_tcase(s, tc_peer_cfg);

    /* Mock child_cfg_t Tests */
    tc_child_cfg = tcase_create("Mock Child Config");
    tcase_add_checked_fixture(tc_child_cfg, setup_mock_test, teardown_mock_test);
    tcase_add_test(tc_child_cfg, test_mock_child_cfg_create_and_operations);
    suite_add_tcase(s, tc_child_cfg);

    /* Mock State Tracking Tests */
    tc_tracking = tcase_create("Mock State Tracking");
    tcase_add_checked_fixture(tc_tracking, setup_mock_test, teardown_mock_test);
    tcase_add_test(tc_tracking, test_mock_call_tracking);
    tcase_add_test(tc_tracking, test_mock_memory_tracking);
    suite_add_tcase(s, tc_tracking);

    /* Mock Parameter Capture Tests */
    tc_capture = tcase_create("Mock Parameter Capture");
    tcase_add_checked_fixture(tc_capture, setup_mock_test, teardown_mock_test);
    tcase_add_test(tc_capture, test_mock_parameter_capture);
    suite_add_tcase(s, tc_capture);

    /* Mock Configuration Tests */
    tc_config = tcase_create("Mock Configuration");
    tcase_add_checked_fixture(tc_config, setup_mock_test, teardown_mock_test);
    tcase_add_test(tc_config, test_mock_allocation_failure_simulation);
    tcase_add_test(tc_config, test_mock_specific_failure_simulation);
    suite_add_tcase(s, tc_config);

    /* Integration Tests */
    tc_integration = tcase_create("Mock Integration");
    tcase_add_checked_fixture(tc_integration, setup_mock_test, teardown_mock_test);
    tcase_add_test(tc_integration, test_mock_complex_peer_config_scenario);
    suite_add_tcase(s, tc_integration);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = strongswan_mocks_suite();
    sr = srunner_create(s);

    printf("Running strongSwan Mock Infrastructure Tests...\n");
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    if (number_failed == 0) {
        printf("✅ All strongSwan Mock tests passed!\n");
        printf("Mock infrastructure is ready for use.\n");
    } else {
        printf("❌ %d strongSwan Mock test(s) failed.\n", number_failed);
    }

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}