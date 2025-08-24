/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Level 2 (Adapter) Tests for extsock_json_parser module
 * TASK-007: JSON Parser 실제 테스트
 * 
 * These are Level 2 tests that use the mock strongSwan system
 * to test adapter layer functionality with controlled dependencies.
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Test infrastructure
#include "../infrastructure/test_container.h"
#include "../infrastructure/strongswan_mocks.h"

// Module under test (use mock implementation for Level 2 testing)
#include "extsock_json_parser_mock.h"

// Common types not needed for mock tests

// Test data and container
static test_container_t *container = NULL;

// Sample JSON configurations for testing
static const char *VALID_IKE_JSON = 
"{\n"
"    \"local_addrs\": [\"192.168.1.100\"],\n"
"    \"remote_addrs\": [\"203.0.113.5\"],\n"
"    \"version\": 2,\n"
"    \"dscp\": \"101000\",\n"
"    \"proposals\": [\n"
"        \"aes256-sha256-modp2048\",\n"
"        \"aes128-sha1-modp1024\"\n"
"    ]\n"
"}";

static const char *VALID_AUTH_PSK_JSON =
"{\n"
"    \"auth\": \"psk\",\n"
"    \"id\": \"client@strongswan.org\",\n"
"    \"secret\": \"test-preshared-key-123\"\n"
"}";

static const char *VALID_AUTH_PUBKEY_JSON =
"{\n"
"    \"auth\": \"pubkey\",\n"
"    \"id\": \"C=US,O=strongSwan,CN=client\"\n"
"}";

static const char *VALID_PROPOSALS_JSON = 
"[\"aes256-sha256-modp2048\", \"aes128-sha1-modp1024\", \"3des-md5-modp768\"]";

static const char *VALID_TRAFFIC_SELECTORS_JSON =
"[\"192.168.1.0/24\", \"10.0.0.0/8\", \"172.16.0.0/12\"]";

static const char *VALID_CHILD_CONFIG_JSON =
"{\n"
"    \"name\": \"test-child\",\n"
"    \"start_action\": \"trap\",\n"
"    \"dpd_action\": \"clear\",\n"
"    \"copy_dscp\": \"out\",\n"
"    \"local_ts\": [\"192.168.1.0/24\"],\n"
"    \"remote_ts\": [\"10.0.0.0/8\"],\n"
"    \"esp_proposals\": [\"aes256-sha256\", \"aes128-sha1\"],\n"
"    \"lifetime\": {\n"
"        \"rekey_time\": 3600\n"
"    }\n"
"}";

static const char *INVALID_JSON = "{ \"incomplete\": ";
static const char *EMPTY_JSON = "{}";

/*
 * ============================================================================
 * Test Fixtures
 * ============================================================================
 */

void setup_json_parser_adapter_test(void)
{
    // Create Level 2 test container (with Mock strongSwan components)
    container = test_container_create_adapter();
    ck_assert_ptr_nonnull(container);
    
    CONTAINER_ASSERT_NO_MEMORY_LEAKS(container);
    CONTAINER_TAKE_MEMORY_SNAPSHOT(container, "json_parser_test");
}

void teardown_json_parser_adapter_test(void)
{
    if (container) {
        CONTAINER_ASSERT_MEMORY_UNCHANGED_SINCE_SNAPSHOT(container, "json_parser_test");
        container->destroy(container);
        container = NULL;
    }
}

/*
 * ============================================================================
 * JSON Parser Creation and Destruction Tests
 * ============================================================================
 */

START_TEST(test_json_parser_create_destroy)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    ck_assert_ptr_nonnull(parser->parse_ike_config);
    ck_assert_ptr_nonnull(parser->parse_auth_config);
    ck_assert_ptr_nonnull(parser->parse_proposals);
    ck_assert_ptr_nonnull(parser->parse_traffic_selectors);
    ck_assert_ptr_nonnull(parser->parse_child_configs);
    ck_assert_ptr_nonnull(parser->parse_config_entity);
    ck_assert_ptr_nonnull(parser->destroy);
    
    parser->destroy(parser);
}
END_TEST

START_TEST(test_json_parser_multiple_create_destroy)
{
    // Test creating and destroying multiple parsers
    extsock_json_parser_t *parsers[5];
    
    // Create multiple parsers
    for (int i = 0; i < 5; i++) {
        parsers[i] = extsock_json_parser_create();
        ck_assert_ptr_nonnull(parsers[i]);
    }
    
    // Destroy them in reverse order
    for (int i = 4; i >= 0; i--) {
        parsers[i]->destroy(parsers[i]);
    }
}
END_TEST

/*
 * ============================================================================
 * IKE Configuration Parsing Tests
 * ============================================================================
 */

START_TEST(test_parse_ike_config_valid)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_mock_cJSON *ike_json = mock_mock_cJSON_Parse(VALID_IKE_JSON);
    ck_assert_ptr_nonnull(ike_json);
    
    ike_cfg_t *ike_cfg = parser->parse_ike_config(parser, ike_json);
    ck_assert_ptr_nonnull(ike_cfg);
    
    // Test that mock functions were called appropriately
    ck_assert_int_gt(g_mock_state->ike_cfg_create_count, 0);
    
    // Clean up - Mock objects don't need cleanup
    (void)ike_cfg; // Mock object
    mock_cJSON_Delete(ike_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_ike_config_null_input)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    ike_cfg_t *ike_cfg = parser->parse_ike_config(parser, NULL);
    ck_assert_ptr_null(ike_cfg);
    
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_ike_config_minimal)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_mock_cJSON *ike_json = mock_mock_cJSON_Parse(EMPTY_JSON);
    ck_assert_ptr_nonnull(ike_json);
    
    ike_cfg_t *ike_cfg = parser->parse_ike_config(parser, ike_json);
    ck_assert_ptr_nonnull(ike_cfg);
    
    // Should create with defaults
    strongswan_mock_state_t *mock_state = container->get_strongswan_mock_state(container);
    ck_assert_int_gt(mock_state->ike_cfg_create_called, 0);
    
    // Clean up - Mock objects don't need cleanup
    (void)ike_cfg; // Mock object
    mock_cJSON_Delete(ike_json);
    parser->destroy(parser);
}
END_TEST

/*
 * ============================================================================
 * Authentication Configuration Parsing Tests  
 * ============================================================================
 */

START_TEST(test_parse_auth_config_psk_valid)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *auth_json = mock_cJSON_Parse(VALID_AUTH_PSK_JSON);
    ck_assert_ptr_nonnull(auth_json);
    
    auth_cfg_t *auth_cfg = parser->parse_auth_config(parser, auth_json, true);
    ck_assert_ptr_nonnull(auth_cfg);
    
    // Verify mock interactions
    strongswan_mock_state_t *mock_state = container->get_strongswan_mock_state(container);
    ck_assert_int_gt(mock_state->auth_cfg_create_called, 0);
    ck_assert_int_gt(mock_state->identification_create_called, 0);
    ck_assert_int_gt(mock_state->shared_key_create_called, 0);
    
    // Clean up
    auth_cfg->destroy(auth_cfg);
    mock_cJSON_Delete(auth_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_auth_config_pubkey_valid)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *auth_json = mock_cJSON_Parse(VALID_AUTH_PUBKEY_JSON);
    ck_assert_ptr_nonnull(auth_json);
    
    auth_cfg_t *auth_cfg = parser->parse_auth_config(parser, auth_json, false);
    ck_assert_ptr_nonnull(auth_cfg);
    
    // Verify mock interactions
    strongswan_mock_state_t *mock_state = container->get_strongswan_mock_state(container);
    ck_assert_int_gt(mock_state->auth_cfg_create_called, 0);
    ck_assert_int_gt(mock_state->identification_create_called, 0);
    
    // Clean up
    auth_cfg->destroy(auth_cfg);
    mock_cJSON_Delete(auth_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_auth_config_null_input)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    auth_cfg_t *auth_cfg = parser->parse_auth_config(parser, NULL, true);
    ck_assert_ptr_null(auth_cfg);
    
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_auth_config_invalid_auth_type)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *auth_json = mock_cJSON_Parse("{ \"auth\": \"invalid_type\" }");
    ck_assert_ptr_nonnull(auth_json);
    
    auth_cfg_t *auth_cfg = parser->parse_auth_config(parser, auth_json, true);
    ck_assert_ptr_null(auth_cfg);
    
    mock_cJSON_Delete(auth_json);
    parser->destroy(parser);
}
END_TEST

/*
 * ============================================================================
 * Proposals Parsing Tests
 * ============================================================================
 */

START_TEST(test_parse_proposals_valid_ike)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *proposals_json = mock_cJSON_Parse(VALID_PROPOSALS_JSON);
    ck_assert_ptr_nonnull(proposals_json);
    
    linked_list_t *proposals = parser->parse_proposals(parser, proposals_json, PROTO_IKE, true);
    ck_assert_ptr_nonnull(proposals);
    
    // Should have at least the proposals from JSON (mock might add defaults)
    int count = proposals->get_count(proposals);
    ck_assert_int_ge(count, 3);
    
    // Verify mock interactions
    strongswan_mock_state_t *mock_state = container->get_strongswan_mock_state(container);
    ck_assert_int_gt(mock_state->proposal_create_called, 0);
    
    // Clean up
    proposals->destroy_offset(proposals, offsetof(proposal_t, destroy));
    mock_cJSON_Delete(proposals_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_proposals_valid_esp)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *proposals_json = mock_cJSON_Parse(VALID_PROPOSALS_JSON);
    ck_assert_ptr_nonnull(proposals_json);
    
    linked_list_t *proposals = parser->parse_proposals(parser, proposals_json, PROTO_ESP, false);
    ck_assert_ptr_nonnull(proposals);
    
    int count = proposals->get_count(proposals);
    ck_assert_int_ge(count, 3);
    
    // Clean up
    proposals->destroy_offset(proposals, offsetof(proposal_t, destroy));
    mock_cJSON_Delete(proposals_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_proposals_null_input)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    linked_list_t *proposals = parser->parse_proposals(parser, NULL, PROTO_IKE, true);
    ck_assert_ptr_nonnull(proposals);
    
    // Should create default proposals
    int count = proposals->get_count(proposals);
    ck_assert_int_gt(count, 0);
    
    // Clean up
    proposals->destroy_offset(proposals, offsetof(proposal_t, destroy));
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_proposals_empty_array)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *proposals_json = mock_cJSON_Parse("[]");
    ck_assert_ptr_nonnull(proposals_json);
    
    linked_list_t *proposals = parser->parse_proposals(parser, proposals_json, PROTO_ESP, false);
    ck_assert_ptr_nonnull(proposals);
    
    // Should create default proposals when empty
    int count = proposals->get_count(proposals);
    ck_assert_int_gt(count, 0);
    
    // Clean up
    proposals->destroy_offset(proposals, offsetof(proposal_t, destroy));
    mock_cJSON_Delete(proposals_json);
    parser->destroy(parser);
}
END_TEST

/*
 * ============================================================================
 * Traffic Selectors Parsing Tests
 * ============================================================================
 */

START_TEST(test_parse_traffic_selectors_valid)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *ts_json = mock_cJSON_Parse(VALID_TRAFFIC_SELECTORS_JSON);
    ck_assert_ptr_nonnull(ts_json);
    
    linked_list_t *ts_list = parser->parse_traffic_selectors(parser, ts_json);
    ck_assert_ptr_nonnull(ts_list);
    
    int count = ts_list->get_count(ts_list);
    ck_assert_int_ge(count, 3);
    
    // Verify mock interactions
    strongswan_mock_state_t *mock_state = container->get_strongswan_mock_state(container);
    ck_assert_int_gt(mock_state->traffic_selector_create_called, 0);
    
    // Clean up
    ts_list->destroy_offset(ts_list, offsetof(traffic_selector_t, destroy));
    mock_cJSON_Delete(ts_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_traffic_selectors_null_input)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    linked_list_t *ts_list = parser->parse_traffic_selectors(parser, NULL);
    ck_assert_ptr_nonnull(ts_list);
    
    // Should create dynamic TS as default
    int count = ts_list->get_count(ts_list);
    ck_assert_int_gt(count, 0);
    
    // Clean up
    ts_list->destroy_offset(ts_list, offsetof(traffic_selector_t, destroy));
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_traffic_selectors_empty_array)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *ts_json = mock_cJSON_Parse("[]");
    ck_assert_ptr_nonnull(ts_json);
    
    linked_list_t *ts_list = parser->parse_traffic_selectors(parser, ts_json);
    ck_assert_ptr_nonnull(ts_list);
    
    // Should create dynamic TS when empty
    int count = ts_list->get_count(ts_list);
    ck_assert_int_gt(count, 0);
    
    // Clean up
    ts_list->destroy_offset(ts_list, offsetof(traffic_selector_t, destroy));
    mock_cJSON_Delete(ts_json);
    parser->destroy(parser);
}
END_TEST

/*
 * ============================================================================
 * Child Configuration Parsing Tests
 * ============================================================================
 */

START_TEST(test_parse_child_configs_valid)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    // Create mock peer_cfg
    peer_cfg_t *peer_cfg = container->create_mock_peer_cfg(container, "test-peer");
    ck_assert_ptr_nonnull(peer_cfg);
    
    mock_cJSON *children_json = mock_cJSON_Parse("[ " VALID_CHILD_CONFIG_JSON " ]");
    ck_assert_ptr_nonnull(children_json);
    
    bool result = parser->parse_child_configs(parser, peer_cfg, children_json);
    ck_assert(result);
    
    // Verify mock interactions
    strongswan_mock_state_t *mock_state = container->get_strongswan_mock_state(container);
    ck_assert_int_gt(mock_state->child_cfg_create_called, 0);
    
    // Clean up
    peer_cfg->destroy(peer_cfg);
    mock_cJSON_Delete(children_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_child_configs_null_input)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    peer_cfg_t *peer_cfg = container->create_mock_peer_cfg(container, "test-peer");
    ck_assert_ptr_nonnull(peer_cfg);
    
    bool result = parser->parse_child_configs(parser, peer_cfg, NULL);
    ck_assert(result);  // Should succeed with no children
    
    // Clean up
    peer_cfg->destroy(peer_cfg);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_child_configs_empty_array)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    peer_cfg_t *peer_cfg = container->create_mock_peer_cfg(container, "test-peer");
    ck_assert_ptr_nonnull(peer_cfg);
    
    mock_cJSON *children_json = mock_cJSON_Parse("[]");
    ck_assert_ptr_nonnull(children_json);
    
    bool result = parser->parse_child_configs(parser, peer_cfg, children_json);
    ck_assert(result);
    
    // Clean up
    peer_cfg->destroy(peer_cfg);
    mock_cJSON_Delete(children_json);
    parser->destroy(parser);
}
END_TEST

/*
 * ============================================================================
 * Configuration Entity Parsing Tests
 * ============================================================================
 */

START_TEST(test_parse_config_entity_not_implemented)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    extsock_config_entity_t *entity = parser->parse_config_entity(parser, "{ \"test\": \"data\" }");
    ck_assert_ptr_null(entity);  // Not implemented yet
    
    parser->destroy(parser);
}
END_TEST

/*
 * ============================================================================
 * Error Handling and Edge Cases Tests
 * ============================================================================
 */

START_TEST(test_json_parser_invalid_json_handling)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *invalid_json = mock_cJSON_Parse(INVALID_JSON);
    ck_assert_ptr_null(invalid_json);  // Should fail to parse
    
    // Test with NULL should handle gracefully
    ike_cfg_t *ike_cfg = parser->parse_ike_config(parser, invalid_json);
    ck_assert_ptr_null(ike_cfg);
    
    parser->destroy(parser);
}
END_TEST

START_TEST(test_json_parser_mock_failure_simulation)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    // Simulate strongSwan API failures
    strongswan_mock_state_t *mock_state = container->get_strongswan_mock_state(container);
    container->simulate_failure(container, "ike_cfg_create");
    
    mock_cJSON *ike_json = mock_cJSON_Parse(VALID_IKE_JSON);
    ck_assert_ptr_nonnull(ike_json);
    
    ike_cfg_t *ike_cfg = parser->parse_ike_config(parser, ike_json);
    ck_assert_ptr_null(ike_cfg);  // Should fail due to simulated failure
    
    // Verify failure was triggered
    ck_assert_int_gt(mock_state->ike_cfg_create_called, 0);
    
    // Reset failure simulation
    container->reset_failures(container);
    
    mock_mock_cJSON_Delete(ike_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_json_parser_memory_stress_test)
{
    // Create and destroy many parsers with configurations
    for (int i = 0; i < 10; i++) {
        extsock_json_parser_t *parser = extsock_json_parser_create();
        ck_assert_ptr_nonnull(parser);
        
        mock_cJSON *ike_json = mock_cJSON_Parse(VALID_IKE_JSON);
        ck_assert_ptr_nonnull(ike_json);
        
        ike_cfg_t *ike_cfg = parser->parse_ike_config(parser, ike_json);
        if (ike_cfg) {
            ike_cfg->destroy(ike_cfg);
        }
        
        mock_mock_cJSON_Delete(ike_json);
        parser->destroy(parser);
    }
    
    // Check for memory leaks
    CONTAINER_ASSERT_NO_MEMORY_LEAKS(container);
}
END_TEST

/*
 * ============================================================================
 * Test Suite Definition
 * ============================================================================
 */

Suite *extsock_json_parser_adapter_suite(void)
{
    Suite *s;
    TCase *tc_creation, *tc_ike, *tc_auth, *tc_proposals, *tc_ts, *tc_child, *tc_entity, *tc_errors;

    s = suite_create("extsock_json_parser Adapter Tests");

    /* Parser Creation/Destruction Tests */
    tc_creation = tcase_create("Parser Creation/Destruction");
    tcase_add_checked_fixture(tc_creation, setup_json_parser_adapter_test, teardown_json_parser_adapter_test);
    tcase_add_test(tc_creation, test_json_parser_create_destroy);
    tcase_add_test(tc_creation, test_json_parser_multiple_create_destroy);
    suite_add_tcase(s, tc_creation);

    /* IKE Configuration Tests */
    tc_ike = tcase_create("IKE Configuration Parsing");
    tcase_add_checked_fixture(tc_ike, setup_json_parser_adapter_test, teardown_json_parser_adapter_test);
    tcase_add_test(tc_ike, test_parse_ike_config_valid);
    tcase_add_test(tc_ike, test_parse_ike_config_null_input);
    tcase_add_test(tc_ike, test_parse_ike_config_minimal);
    suite_add_tcase(s, tc_ike);

    /* Authentication Configuration Tests */
    tc_auth = tcase_create("Authentication Configuration Parsing");
    tcase_add_checked_fixture(tc_auth, setup_json_parser_adapter_test, teardown_json_parser_adapter_test);
    tcase_add_test(tc_auth, test_parse_auth_config_psk_valid);
    tcase_add_test(tc_auth, test_parse_auth_config_pubkey_valid);
    tcase_add_test(tc_auth, test_parse_auth_config_null_input);
    tcase_add_test(tc_auth, test_parse_auth_config_invalid_auth_type);
    suite_add_tcase(s, tc_auth);

    /* Proposals Parsing Tests */
    tc_proposals = tcase_create("Proposals Parsing");
    tcase_add_checked_fixture(tc_proposals, setup_json_parser_adapter_test, teardown_json_parser_adapter_test);
    tcase_add_test(tc_proposals, test_parse_proposals_valid_ike);
    tcase_add_test(tc_proposals, test_parse_proposals_valid_esp);
    tcase_add_test(tc_proposals, test_parse_proposals_null_input);
    tcase_add_test(tc_proposals, test_parse_proposals_empty_array);
    suite_add_tcase(s, tc_proposals);

    /* Traffic Selectors Tests */
    tc_ts = tcase_create("Traffic Selectors Parsing");
    tcase_add_checked_fixture(tc_ts, setup_json_parser_adapter_test, teardown_json_parser_adapter_test);
    tcase_add_test(tc_ts, test_parse_traffic_selectors_valid);
    tcase_add_test(tc_ts, test_parse_traffic_selectors_null_input);
    tcase_add_test(tc_ts, test_parse_traffic_selectors_empty_array);
    suite_add_tcase(s, tc_ts);

    /* Child Configuration Tests */
    tc_child = tcase_create("Child Configuration Parsing");
    tcase_add_checked_fixture(tc_child, setup_json_parser_adapter_test, teardown_json_parser_adapter_test);
    tcase_add_test(tc_child, test_parse_child_configs_valid);
    tcase_add_test(tc_child, test_parse_child_configs_null_input);
    tcase_add_test(tc_child, test_parse_child_configs_empty_array);
    suite_add_tcase(s, tc_child);

    /* Configuration Entity Tests */
    tc_entity = tcase_create("Configuration Entity Parsing");
    tcase_add_checked_fixture(tc_entity, setup_json_parser_adapter_test, teardown_json_parser_adapter_test);
    tcase_add_test(tc_entity, test_parse_config_entity_not_implemented);
    suite_add_tcase(s, tc_entity);

    /* Error Handling Tests */
    tc_errors = tcase_create("Error Handling and Edge Cases");
    tcase_add_checked_fixture(tc_errors, setup_json_parser_adapter_test, teardown_json_parser_adapter_test);
    tcase_add_test(tc_errors, test_json_parser_invalid_json_handling);
    tcase_add_test(tc_errors, test_json_parser_mock_failure_simulation);
    tcase_add_test(tc_errors, test_json_parser_memory_stress_test);
    suite_add_tcase(s, tc_errors);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = extsock_json_parser_adapter_suite();
    sr = srunner_create(s);

    printf("Running extsock_json_parser Adapter Tests (Level 2)...\n");
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    if (number_failed == 0) {
        printf("✅ All extsock_json_parser adapter tests passed!\n");
        printf("Level 2 tests for JSON Parser adapter completed successfully.\n");
    } else {
        printf("❌ %d extsock_json_parser adapter test(s) failed.\n", number_failed);
    }

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}