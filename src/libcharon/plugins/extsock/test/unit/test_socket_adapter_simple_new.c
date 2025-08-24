/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Simple Level 2 (Adapter) Tests for Socket Adapter functionality
 * TASK-008: Socket Adapter 실제 테스트
 * 
 * This is a simplified test that focuses on testing Socket Adapter 
 * adapter functionality with minimal dependencies.
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test infrastructure
#include "../infrastructure/test_container.h"
#include "../infrastructure/strongswan_mocks.h"

// Mock Socket Adapter interface
#include "extsock_socket_adapter_mock.h"

// Test data
static const char *VALID_EVENT_JSON = 
"{\n"
"    \"type\": \"connection_established\",\n"
"    \"peer_addr\": \"203.0.113.5\",\n"
"    \"timestamp\": \"2024-08-23T10:30:00Z\"\n"
"}";

static const char *INVALID_EVENT_JSON = 
"{\n"
"    \"type\": \"invalid_event\",\n";

// Test container
static test_container_t *container = NULL;

/*
 * ============================================================================
 * Test Fixtures
 * ============================================================================
 */

void setup_socket_adapter_test(void)
{
    container = test_container_create_adapter();
    ck_assert_ptr_nonnull(container);
    
    // Reset mock state
    strongswan_mocks_reset_state();
}

void teardown_socket_adapter_test(void)
{
    if (container) {
        container->destroy(container);
        container = NULL;
    }
}

/*
 * ============================================================================
 * Basic Socket Adapter Tests
 * ============================================================================
 */

START_TEST(test_socket_adapter_create_destroy)
{
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    ck_assert_ptr_nonnull(cfg_usecase);
    
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    ck_assert_ptr_nonnull(adapter);
    ck_assert_ptr_nonnull(adapter->send_event);
    ck_assert_ptr_nonnull(adapter->start_listening);
    ck_assert_ptr_nonnull(adapter->stop_listening);
    ck_assert_ptr_nonnull(adapter->destroy);
    
    adapter->destroy(adapter);
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

START_TEST(test_socket_adapter_create_null_usecase)
{
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(NULL);
    ck_assert_ptr_null(adapter);
}
END_TEST

START_TEST(test_socket_adapter_multiple_create_destroy)
{
    // Test creating and destroying multiple adapters
    extsock_socket_adapter_t *adapters[3];
    extsock_config_usecase_t *usecases[3];
    
    // Create multiple adapters
    for (int i = 0; i < 3; i++) {
        usecases[i] = mock_config_usecase_create();
        ck_assert_ptr_nonnull(usecases[i]);
        
        adapters[i] = extsock_socket_adapter_create(usecases[i]);
        ck_assert_ptr_nonnull(adapters[i]);
    }
    
    // Destroy them in reverse order
    for (int i = 2; i >= 0; i--) {
        adapters[i]->destroy(adapters[i]);
        free(usecases[i]->config_data);
        free(usecases[i]);
    }
}
END_TEST

/*
 * ============================================================================
 * Event Sending Tests
 * ============================================================================
 */

START_TEST(test_send_event_valid)
{
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    ck_assert_ptr_nonnull(adapter);
    
    extsock_error_t result = adapter->send_event(adapter, VALID_EVENT_JSON);
    ck_assert_int_eq(result, EXTSOCK_SUCCESS);
    
    adapter->destroy(adapter);
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

START_TEST(test_send_event_null_input)
{
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    ck_assert_ptr_nonnull(adapter);
    
    extsock_error_t result = adapter->send_event(adapter, NULL);
    ck_assert_int_eq(result, EXTSOCK_ERROR_INVALID_PARAM);
    
    adapter->destroy(adapter);
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

START_TEST(test_send_event_null_adapter)
{
    // Test calling send_event with NULL adapter
    // This test validates that the system can handle NULL pointers gracefully
    // In real implementation, this would be handled by the mock system
    
    // For mock testing, we simply verify that NULL input detection works
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    
    // Test with NULL adapter pointer should be handled by mock implementation  
    extsock_error_t result = adapter->send_event(NULL, VALID_EVENT_JSON);
    ck_assert_int_eq(result, EXTSOCK_ERROR_INVALID_PARAM);
    
    adapter->destroy(adapter);
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

START_TEST(test_send_event_multiple_calls)
{
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    ck_assert_ptr_nonnull(adapter);
    
    // Send multiple events
    for (int i = 0; i < 5; i++) {
        extsock_error_t result = adapter->send_event(adapter, VALID_EVENT_JSON);
        ck_assert_int_eq(result, EXTSOCK_SUCCESS);
    }
    
    adapter->destroy(adapter);
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

/*
 * ============================================================================
 * Socket Listening Tests
 * ============================================================================
 */

START_TEST(test_start_listening_valid)
{
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    ck_assert_ptr_nonnull(adapter);
    
    thread_t *thread = adapter->start_listening(adapter);
    ck_assert_ptr_nonnull(thread);
    ck_assert_int_gt(thread->thread_id, 0);
    ck_assert(thread->is_running);
    
    // Verify mock interactions
    ck_assert_int_gt(g_mock_state->ike_cfg_create_count, 0);
    
    adapter->stop_listening(adapter);
    adapter->destroy(adapter);
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

START_TEST(test_start_listening_null_adapter)
{
    // Test calling start_listening with NULL adapter  
    // For mock testing, we create a valid adapter and test NULL handling
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    
    // Test with NULL parameter should be handled by the mock implementation
    thread_t *thread = adapter->start_listening(NULL);  // Pass NULL as 'this'
    ck_assert_ptr_null(thread);
    
    adapter->destroy(adapter);
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

START_TEST(test_start_stop_listening_cycle)
{
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    ck_assert_ptr_nonnull(adapter);
    
    // Test multiple start/stop cycles
    for (int i = 0; i < 3; i++) {
        thread_t *thread = adapter->start_listening(adapter);
        ck_assert_ptr_nonnull(thread);
        
        adapter->stop_listening(adapter);
        
        // Verify mock interactions
        ck_assert_int_gt(g_mock_state->peer_cfg_create_count, i);
    }
    
    adapter->destroy(adapter);
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

START_TEST(test_stop_listening_without_start)
{
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    ck_assert_ptr_nonnull(adapter);
    
    // Should not crash even if stop is called without start
    adapter->stop_listening(adapter);
    
    adapter->destroy(adapter);
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

/*
 * ============================================================================
 * Integration and Error Handling Tests
 * ============================================================================
 */

START_TEST(test_socket_adapter_integration_workflow)
{
    extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
    extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
    ck_assert_ptr_nonnull(adapter);
    
    // Complete workflow: create -> start -> send event -> stop -> destroy
    thread_t *thread = adapter->start_listening(adapter);
    ck_assert_ptr_nonnull(thread);
    
    extsock_error_t result = adapter->send_event(adapter, VALID_EVENT_JSON);
    ck_assert_int_eq(result, EXTSOCK_SUCCESS);
    
    adapter->stop_listening(adapter);
    adapter->destroy(adapter);
    
    free(cfg_usecase->config_data);
    free(cfg_usecase);
}
END_TEST

START_TEST(test_socket_adapter_memory_stress_test)
{
    // Create and destroy many adapters with operations
    for (int i = 0; i < 10; i++) {
        extsock_config_usecase_t *cfg_usecase = mock_config_usecase_create();
        extsock_socket_adapter_t *adapter = extsock_socket_adapter_create(cfg_usecase);
        ck_assert_ptr_nonnull(adapter);
        
        thread_t *thread = adapter->start_listening(adapter);
        if (thread) {
            adapter->send_event(adapter, VALID_EVENT_JSON);
            adapter->stop_listening(adapter);
        }
        
        adapter->destroy(adapter);
        free(cfg_usecase->config_data);
        free(cfg_usecase);
    }
}
END_TEST

/*
 * ============================================================================
 * Test Suite Definition
 * ============================================================================
 */

Suite *socket_adapter_simple_suite(void)
{
    Suite *s;
    TCase *tc_basic, *tc_events, *tc_listening, *tc_integration;

    s = suite_create("Socket Adapter Simple Tests");

    /* Basic Tests */
    tc_basic = tcase_create("Basic Adapter Tests");
    tcase_add_checked_fixture(tc_basic, setup_socket_adapter_test, teardown_socket_adapter_test);
    tcase_add_test(tc_basic, test_socket_adapter_create_destroy);
    tcase_add_test(tc_basic, test_socket_adapter_create_null_usecase);
    tcase_add_test(tc_basic, test_socket_adapter_multiple_create_destroy);
    suite_add_tcase(s, tc_basic);

    /* Event Sending Tests */
    tc_events = tcase_create("Event Sending");
    tcase_add_checked_fixture(tc_events, setup_socket_adapter_test, teardown_socket_adapter_test);
    tcase_add_test(tc_events, test_send_event_valid);
    tcase_add_test(tc_events, test_send_event_null_input);
    tcase_add_test(tc_events, test_send_event_null_adapter);
    tcase_add_test(tc_events, test_send_event_multiple_calls);
    suite_add_tcase(s, tc_events);

    /* Socket Listening Tests */
    tc_listening = tcase_create("Socket Listening");
    tcase_add_checked_fixture(tc_listening, setup_socket_adapter_test, teardown_socket_adapter_test);
    tcase_add_test(tc_listening, test_start_listening_valid);
    tcase_add_test(tc_listening, test_start_listening_null_adapter);
    tcase_add_test(tc_listening, test_start_stop_listening_cycle);
    tcase_add_test(tc_listening, test_stop_listening_without_start);
    suite_add_tcase(s, tc_listening);

    /* Integration Tests */
    tc_integration = tcase_create("Integration and Error Handling");
    tcase_add_checked_fixture(tc_integration, setup_socket_adapter_test, teardown_socket_adapter_test);
    tcase_add_test(tc_integration, test_socket_adapter_integration_workflow);
    tcase_add_test(tc_integration, test_socket_adapter_memory_stress_test);
    suite_add_tcase(s, tc_integration);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = socket_adapter_simple_suite();
    sr = srunner_create(s);

    printf("Running Socket Adapter Simple Tests (Level 2)...\n");
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    if (number_failed == 0) {
        printf("✅ All Socket Adapter simple tests passed!\n");
        printf("Level 2 adapter tests completed successfully.\n");
    } else {
        printf("❌ %d Socket Adapter simple test(s) failed.\n", number_failed);
    }

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}