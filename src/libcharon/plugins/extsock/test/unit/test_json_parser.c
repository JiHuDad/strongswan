/*
 * Copyright (C) 2024 strongSwan Project
 * Unit tests for JSON Parser Adapter
 */

#include <check.h>
#include <cjson/cJSON.h>
#include "../adapters/json/extsock_json_parser.h"
#include "../common/extsock_common.h"

static extsock_json_parser_t *parser;

void setup_json_parser_test(void)
{
    parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
}

void teardown_json_parser_test(void)
{
    if (parser) {
        parser->destroy(parser);
        parser = NULL;
    }
}

/**
 * 유효한 JSON 배열에서 제안(proposals) 파싱 테스트
 */
START_TEST(test_parse_proposals_valid_json)
{
    // Given
    cJSON *proposals = cJSON_CreateArray();
    cJSON_AddItemToArray(proposals, cJSON_CreateString("aes256-sha256-modp2048"));
    cJSON_AddItemToArray(proposals, cJSON_CreateString("aes128-sha1-modp1024"));
    
    // When
    linked_list_t *result = parser->parse_proposals_from_json_array(parser, proposals, PROTO_IKE, TRUE);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result->get_count(result), 2);
    
    // Cleanup
    result->destroy_offset(result, offsetof(proposal_t, destroy));
    cJSON_Delete(proposals);
}
END_TEST

/**
 * 빈 JSON 배열에서 기본 제안 생성 테스트
 */
START_TEST(test_parse_proposals_empty_json)
{
    // Given
    cJSON *proposals = cJSON_CreateArray();
    
    // When
    linked_list_t *result = parser->parse_proposals_from_json_array(parser, proposals, PROTO_IKE, TRUE);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_int_gt(result->get_count(result), 0); // 기본 제안이 추가되어야 함
    
    // Cleanup
    result->destroy_offset(result, offsetof(proposal_t, destroy));
    cJSON_Delete(proposals);
}
END_TEST

/**
 * 유효한 트래픽 셀렉터 JSON 파싱 테스트
 */
START_TEST(test_parse_traffic_selectors_valid)
{
    // Given
    cJSON *ts_array = cJSON_CreateArray();
    cJSON_AddItemToArray(ts_array, cJSON_CreateString("10.0.0.0/24"));
    cJSON_AddItemToArray(ts_array, cJSON_CreateString("192.168.1.0/24"));
    
    // When
    linked_list_t *result = parser->parse_ts_from_json_array(parser, ts_array);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result->get_count(result), 2);
    
    // Cleanup
    result->destroy_offset(result, offsetof(traffic_selector_t, destroy));
    cJSON_Delete(ts_array);
}
END_TEST

/**
 * 잘못된 CIDR 형식 처리 테스트
 */
START_TEST(test_parse_traffic_selectors_invalid_cidr)
{
    // Given
    cJSON *ts_array = cJSON_CreateArray();
    cJSON_AddItemToArray(ts_array, cJSON_CreateString("invalid-cidr"));
    cJSON_AddItemToArray(ts_array, cJSON_CreateString("10.0.0.0/24")); // 유효한 것
    
    // When
    linked_list_t *result = parser->parse_ts_from_json_array(parser, ts_array);
    
    // Then
    ck_assert_ptr_nonnull(result);
    // 잘못된 것은 무시되고 유효한 것만 포함되거나, 동적 TS가 추가되어야 함
    ck_assert_int_gt(result->get_count(result), 0);
    
    // Cleanup
    result->destroy_offset(result, offsetof(traffic_selector_t, destroy));
    cJSON_Delete(ts_array);
}
END_TEST

/**
 * JSON 배열을 쉼표 구분 문자열로 변환 테스트
 */
START_TEST(test_json_array_to_comma_separated_string)
{
    // Given
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToArray(array, cJSON_CreateString("value1"));
    cJSON_AddItemToArray(array, cJSON_CreateString("value2"));
    cJSON_AddItemToArray(array, cJSON_CreateString("value3"));
    
    // When
    char *result = parser->json_array_to_comma_separated_string(parser, array);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "value1,value2,value3");
    
    // Cleanup
    free(result);
    cJSON_Delete(array);
}
END_TEST

/**
 * 빈 배열에서 기본값 반환 테스트
 */
START_TEST(test_json_array_to_comma_separated_string_empty)
{
    // Given
    cJSON *array = cJSON_CreateArray();
    
    // When
    char *result = parser->json_array_to_comma_separated_string(parser, array);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "%any");
    
    // Cleanup
    free(result);
    cJSON_Delete(array);
}
END_TEST

/**
 * IKE 설정 JSON 파싱 테스트
 */
START_TEST(test_parse_ike_cfg_from_json)
{
    // Given
    cJSON *ike_json = cJSON_CreateObject();
    cJSON_AddStringToObject(ike_json, "local", "192.168.1.10");
    cJSON_AddStringToObject(ike_json, "remote", "203.0.113.5");
    cJSON_AddNumberToObject(ike_json, "version", 2);
    
    cJSON *proposals = cJSON_CreateArray();
    cJSON_AddItemToArray(proposals, cJSON_CreateString("aes256-sha256-modp2048"));
    cJSON_AddItemToObject(ike_json, "proposals", proposals);
    
    // When
    ike_cfg_t *result = parser->parse_ike_cfg_from_json(parser, ike_json);
    
    // Then
    ck_assert_ptr_nonnull(result);
    
    // Cleanup
    result->destroy(result);
    cJSON_Delete(ike_json);
}
END_TEST

/**
 * 인증 설정 JSON 파싱 테스트 (PSK)
 */
START_TEST(test_parse_auth_cfg_psk)
{
    // Given
    cJSON *auth_json = cJSON_CreateObject();
    cJSON_AddStringToObject(auth_json, "type", "psk");
    cJSON_AddStringToObject(auth_json, "id", "CN=testuser");
    cJSON_AddStringToObject(auth_json, "secret", "supersecret");
    
    // When
    auth_cfg_t *result = parser->parse_auth_cfg_from_json(parser, auth_json, TRUE);
    
    // Then
    ck_assert_ptr_nonnull(result);
    
    // Cleanup
    result->destroy(result);
    cJSON_Delete(auth_json);
}
END_TEST

/**
 * 테스트 스위트 생성
 */
Suite *json_parser_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("JSON Parser");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup_json_parser_test, teardown_json_parser_test);
    
    tcase_add_test(tc_core, test_parse_proposals_valid_json);
    tcase_add_test(tc_core, test_parse_proposals_empty_json);
    tcase_add_test(tc_core, test_parse_traffic_selectors_valid);
    tcase_add_test(tc_core, test_parse_traffic_selectors_invalid_cidr);
    tcase_add_test(tc_core, test_json_array_to_comma_separated_string);
    tcase_add_test(tc_core, test_json_array_to_comma_separated_string_empty);
    tcase_add_test(tc_core, test_parse_ike_cfg_from_json);
    tcase_add_test(tc_core, test_parse_auth_cfg_psk);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = json_parser_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 