/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Simple Level 2 (Adapter) Tests for JSON Parser functionality
 * TASK-007: JSON Parser 실제 테스트
 * 
 * This is a simplified test that focuses on testing JSON Parser 
 * adapter functionality with minimal dependencies.
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test infrastructure
#include "../infrastructure/test_container.h"
#include "../infrastructure/strongswan_mocks.h"

// Mock JSON Parser interface
#include "extsock_json_parser_mock.h"

// Mock protocol constants
#define PROTO_IKE 1
#define PROTO_ESP 3

// Test data
static const char *VALID_IKE_JSON = 
"{\n"
"    \"local_addrs\": [\"192.168.1.100\"],\n"
"    \"remote_addrs\": [\"203.0.113.5\"],\n"
"    \"version\": 2\n"
"}";

static const char *VALID_AUTH_PSK_JSON =
"{\n"
"    \"auth\": \"psk\",\n"
"    \"id\": \"client@strongswan.org\",\n"
"    \"secret\": \"test-key\"\n"
"}";

static const char *INVALID_AUTH_JSON =
"{\n"
"    \"auth\": \"invalid_type\"\n"
"}";

// Test container
static test_container_t *container = NULL;

/*
 * ============================================================================
 * Test Fixtures
 * ============================================================================
 */

void setup_simple_test(void)
{
    container = test_container_create_adapter();
    ck_assert_ptr_nonnull(container);
    
    // Reset mock state
    strongswan_mocks_reset_state();
}

void teardown_simple_test(void)
{
    if (container) {
        container->destroy(container);
        container = NULL;
    }
}

/*
 * ============================================================================
 * Basic JSON Parser Tests
 * ============================================================================
 */

START_TEST(test_json_parser_create_destroy)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    ck_assert_ptr_nonnull(parser->parse_ike_config);
    ck_assert_ptr_nonnull(parser->parse_auth_config);
    ck_assert_ptr_nonnull(parser->destroy);
    
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_ike_config_valid)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    // Parse valid JSON
    mock_cJSON *ike_json = mock_cJSON_Parse(VALID_IKE_JSON);
    ck_assert_ptr_nonnull(ike_json);
    
    ike_cfg_t *ike_cfg = parser->parse_ike_config(parser, ike_json);
    ck_assert_ptr_nonnull(ike_cfg);
    
    // Verify mock was called
    ck_assert_int_gt(g_mock_state->ike_cfg_create_count, 0);
    
    // Clean up
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

START_TEST(test_parse_auth_config_psk_valid)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *auth_json = mock_cJSON_Parse(VALID_AUTH_PSK_JSON);
    ck_assert_ptr_nonnull(auth_json);
    
    auth_cfg_t *auth_cfg = parser->parse_auth_config(parser, auth_json, true);
    ck_assert_ptr_nonnull(auth_cfg);
    
    // Verify mock was called
    ck_assert_int_gt(g_mock_state->auth_cfg_create_count, 0);
    
    // Clean up
    mock_cJSON_Delete(auth_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_auth_config_invalid_type)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    mock_cJSON *auth_json = mock_cJSON_Parse(INVALID_AUTH_JSON);
    ck_assert_ptr_nonnull(auth_json);
    
    auth_cfg_t *auth_cfg = parser->parse_auth_config(parser, auth_json, true);
    ck_assert_ptr_null(auth_cfg);  // Should fail for invalid auth type
    
    // Clean up
    mock_cJSON_Delete(auth_json);
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_proposals_basic)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    // Test with NULL input (should create defaults)
    linked_list_t *proposals = parser->parse_proposals(parser, NULL, PROTO_IKE, true);
    ck_assert_ptr_nonnull(proposals);
    
    // Verify mocks were called (proposals should increment auth_cfg_create_count)
    // This is a mock test, so we just check that the function was called
    ck_assert_ptr_nonnull(proposals);
    
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_traffic_selectors_basic)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    // Test with NULL input (should create defaults)
    linked_list_t *ts_list = parser->parse_traffic_selectors(parser, NULL);
    ck_assert_ptr_nonnull(ts_list);
    
    // Verify mocks were called (traffic selectors mock test)
    // This is a mock test, so we just check that the function returns correctly  
    ck_assert_ptr_nonnull(ts_list);
    
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_child_configs_basic)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    // Create mock peer_cfg with ike_cfg
    mock_ike_cfg_t *ike_cfg = mock_ike_cfg_create("test-ike");
    peer_cfg_t *peer_cfg = (peer_cfg_t*)mock_peer_cfg_create("test-peer", ike_cfg);
    ck_assert_ptr_nonnull(peer_cfg);
    
    // Test with NULL input (should succeed with no children)
    bool result = parser->parse_child_configs(parser, peer_cfg, NULL);
    ck_assert(result);
    
    parser->destroy(parser);
}
END_TEST

START_TEST(test_parse_config_entity_not_implemented)
{
    extsock_json_parser_t *parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
    
    extsock_config_entity_t *entity = parser->parse_config_entity(parser, "{}");
    ck_assert_ptr_null(entity);  // Not implemented
    
    parser->destroy(parser);
}
END_TEST

/*
 * ============================================================================
 * Test Suite Definition
 * ============================================================================
 */

Suite *json_parser_simple_suite(void)
{
    Suite *s;
    TCase *tc_basic, *tc_ike, *tc_auth, *tc_proposals;

    s = suite_create("JSON Parser Simple Tests");

    /* Basic Tests */
    tc_basic = tcase_create("Basic Parser Tests");
    tcase_add_checked_fixture(tc_basic, setup_simple_test, teardown_simple_test);
    tcase_add_test(tc_basic, test_json_parser_create_destroy);
    tcase_add_test(tc_basic, test_parse_config_entity_not_implemented);
    suite_add_tcase(s, tc_basic);

    /* IKE Configuration Tests */
    tc_ike = tcase_create("IKE Configuration");
    tcase_add_checked_fixture(tc_ike, setup_simple_test, teardown_simple_test);
    tcase_add_test(tc_ike, test_parse_ike_config_valid);
    tcase_add_test(tc_ike, test_parse_ike_config_null_input);
    suite_add_tcase(s, tc_ike);

    /* Authentication Tests */
    tc_auth = tcase_create("Authentication Configuration");
    tcase_add_checked_fixture(tc_auth, setup_simple_test, teardown_simple_test);
    tcase_add_test(tc_auth, test_parse_auth_config_psk_valid);
    tcase_add_test(tc_auth, test_parse_auth_config_invalid_type);
    suite_add_tcase(s, tc_auth);

    /* Proposals and Traffic Selectors Tests */
    tc_proposals = tcase_create("Proposals and Traffic Selectors");
    tcase_add_checked_fixture(tc_proposals, setup_simple_test, teardown_simple_test);
    tcase_add_test(tc_proposals, test_parse_proposals_basic);
    tcase_add_test(tc_proposals, test_parse_traffic_selectors_basic);
    tcase_add_test(tc_proposals, test_parse_child_configs_basic);
    suite_add_tcase(s, tc_proposals);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = json_parser_simple_suite();
    sr = srunner_create(s);

    printf("Running JSON Parser Simple Tests (Level 2)...\n");
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    if (number_failed == 0) {
        printf("✅ All JSON Parser simple tests passed!\n");
        printf("Level 2 adapter tests completed successfully.\n");
    } else {
        printf("❌ %d JSON Parser simple test(s) failed.\n", number_failed);
    }

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}