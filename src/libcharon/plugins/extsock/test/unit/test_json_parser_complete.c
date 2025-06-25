/*
 * Copyright (C) 2024 strongSwan Project
 * Complete Unit tests for JSON Parser Adapter
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <library.h>
#include <config/ike_cfg.h>
#include <config/peer_cfg.h>
#include <config/child_cfg.h>
#include <credentials/auth_cfg.h>
#include <selectors/traffic_selector.h>
#include <collections/linked_list.h>

#include "../adapters/json/extsock_json_parser.h"
#include "../common/extsock_common.h"

static extsock_json_parser_t *parser;

/**
 * 테스트 설정
 */
void setup_json_parser_complete_test(void)
{
    library_init(NULL, "test-json-parser-complete");
    parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(parser);
}

/**
 * 테스트 해제
 */
void teardown_json_parser_complete_test(void)
{
    if (parser) {
        parser->destroy(parser);
        parser = NULL;
    }
    library_deinit();
}

/**
 * IKE 제안 파싱 테스트 - 유효한 제안들
 */
START_TEST(test_parse_proposals_ike_valid)
{
    // Given
    cJSON *proposals = cJSON_CreateArray();
    cJSON_AddItemToArray(proposals, cJSON_CreateString("aes256-sha256-modp2048"));
    cJSON_AddItemToArray(proposals, cJSON_CreateString("aes128-sha1-modp1024"));
    
    // When
    linked_list_t *result = parser->parse_proposals(parser, proposals, PROTO_IKE, TRUE);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result->get_count(result), 2);
    
    // 제안들 확인
    enumerator_t *enumerator = result->create_enumerator(result);
    proposal_t *proposal;
    int count = 0;
    while (enumerator->enumerate(enumerator, &proposal)) {
        ck_assert_ptr_nonnull(proposal);
        ck_assert_int_eq(proposal->get_protocol(proposal), PROTO_IKE);
        count++;
    }
    enumerator->destroy(enumerator);
    ck_assert_int_eq(count, 2);
    
    // Cleanup
    result->destroy_offset(result, offsetof(proposal_t, destroy));
    cJSON_Delete(proposals);
}
END_TEST

/**
 * ESP 제안 파싱 테스트
 */
START_TEST(test_parse_proposals_esp_valid)
{
    // Given
    cJSON *proposals = cJSON_CreateArray();
    cJSON_AddItemToArray(proposals, cJSON_CreateString("aes128gcm16-prfsha256"));
    cJSON_AddItemToArray(proposals, cJSON_CreateString("aes256-sha256"));
    
    // When
    linked_list_t *result = parser->parse_proposals(parser, proposals, PROTO_ESP, FALSE);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result->get_count(result), 2);
    
    // Cleanup
    result->destroy_offset(result, offsetof(proposal_t, destroy));
    cJSON_Delete(proposals);
}
END_TEST

/**
 * 빈 제안 배열 처리 테스트 (기본값 추가되어야 함)
 */
START_TEST(test_parse_proposals_empty_default)
{
    // Given
    cJSON *proposals = cJSON_CreateArray();
    
    // When
    linked_list_t *result = parser->parse_proposals(parser, proposals, PROTO_IKE, TRUE);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_int_gt(result->get_count(result), 0); // 기본 제안이 추가되어야 함
    
    // Cleanup
    result->destroy_offset(result, offsetof(proposal_t, destroy));
    cJSON_Delete(proposals);
}
END_TEST

/**
 * 잘못된 제안 문자열 처리 테스트
 */
START_TEST(test_parse_proposals_invalid_string)
{
    // Given
    cJSON *proposals = cJSON_CreateArray();
    cJSON_AddItemToArray(proposals, cJSON_CreateString("invalid-proposal-string"));
    cJSON_AddItemToArray(proposals, cJSON_CreateString("aes128-sha256-modp2048")); // 유효한 것
    
    // When
    linked_list_t *result = parser->parse_proposals(parser, proposals, PROTO_IKE, TRUE);
    
    // Then
    ck_assert_ptr_nonnull(result);
    // 유효한 제안만 포함되거나 기본값이 추가되어야 함
    ck_assert_int_gt(result->get_count(result), 0);
    
    // Cleanup
    result->destroy_offset(result, offsetof(proposal_t, destroy));
    cJSON_Delete(proposals);
}
END_TEST

/**
 * 트래픽 셀렉터 파싱 테스트 - 유효한 CIDR들
 */
START_TEST(test_parse_traffic_selectors_valid_cidr)
{
    // Given
    cJSON *ts_array = cJSON_CreateArray();
    cJSON_AddItemToArray(ts_array, cJSON_CreateString("10.0.0.0/24"));
    cJSON_AddItemToArray(ts_array, cJSON_CreateString("192.168.1.0/24"));
    cJSON_AddItemToArray(ts_array, cJSON_CreateString("172.16.0.1/32"));
    
    // When
    linked_list_t *result = parser->parse_traffic_selectors(parser, ts_array);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result->get_count(result), 3);
    
    // 각 TS 확인
    enumerator_t *enumerator = result->create_enumerator(result);
    traffic_selector_t *ts;
    int count = 0;
    while (enumerator->enumerate(enumerator, &ts)) {
        ck_assert_ptr_nonnull(ts);
        count++;
    }
    enumerator->destroy(enumerator);
    ck_assert_int_eq(count, 3);
    
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
    cJSON_AddItemToArray(ts_array, cJSON_CreateString("300.400.500.600/24")); // 잘못된 IP
    cJSON_AddItemToArray(ts_array, cJSON_CreateString("10.0.0.0/33")); // 잘못된 서브넷
    
    // When
    linked_list_t *result = parser->parse_traffic_selectors(parser, ts_array);
    
    // Then
    ck_assert_ptr_nonnull(result);
    // 잘못된 것들은 무시되고 기본 동적 TS가 추가되어야 함
    ck_assert_int_gt(result->get_count(result), 0);
    
    // Cleanup
    result->destroy_offset(result, offsetof(traffic_selector_t, destroy));
    cJSON_Delete(ts_array);
}
END_TEST

/**
 * 빈 트래픽 셀렉터 배열 처리 테스트
 */
START_TEST(test_parse_traffic_selectors_empty_default)
{
    // Given
    cJSON *ts_array = cJSON_CreateArray();
    
    // When
    linked_list_t *result = parser->parse_traffic_selectors(parser, ts_array);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_int_gt(result->get_count(result), 0); // 기본 동적 TS가 추가되어야 함
    
    // Cleanup
    result->destroy_offset(result, offsetof(traffic_selector_t, destroy));
    cJSON_Delete(ts_array);
}
END_TEST

/**
 * IKE 설정 파싱 테스트 - 완전한 설정
 */
START_TEST(test_parse_ike_config_complete)
{
    // Given
    cJSON *ike_json = cJSON_CreateObject();
    
    // 로컬/원격 주소 배열
    cJSON *local_addrs = cJSON_CreateArray();
    cJSON_AddItemToArray(local_addrs, cJSON_CreateString("192.168.1.10"));
    cJSON_AddItemToObject(ike_json, "local_addrs", local_addrs);
    
    cJSON *remote_addrs = cJSON_CreateArray();
    cJSON_AddItemToArray(remote_addrs, cJSON_CreateString("203.0.113.5"));
    cJSON_AddItemToObject(ike_json, "remote_addrs", remote_addrs);
    
    // IKE 버전
    cJSON_AddNumberToObject(ike_json, "version", 2);
    
    // IKE 제안들
    cJSON *proposals = cJSON_CreateArray();
    cJSON_AddItemToArray(proposals, cJSON_CreateString("aes256-sha256-modp2048"));
    cJSON_AddItemToObject(ike_json, "proposals", proposals);
    
    // When
    ike_cfg_t *result = parser->parse_ike_config(parser, ike_json);
    
    // Then
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result->get_version(result), 2);
    
    // 제안들 확인
    linked_list_t *ike_proposals = result->get_proposals(result);
    ck_assert_ptr_nonnull(ike_proposals);
    ck_assert_int_gt(ike_proposals->get_count(ike_proposals), 0);
    
    // Cleanup
    result->destroy(result);
    cJSON_Delete(ike_json);
}
END_TEST

/**
 * IKE 설정 파싱 테스트 - 최소한의 설정
 */
START_TEST(test_parse_ike_config_minimal)
{
    // Given
    cJSON *ike_json = cJSON_CreateObject();
    // version과 proposals만 없는 최소 설정
    
    // When
    ike_cfg_t *result = parser->parse_ike_config(parser, ike_json);
    
    // Then
    ck_assert_ptr_nonnull(result);
    // 기본값들이 설정되어야 함
    
    // Cleanup
    result->destroy(result);
    cJSON_Delete(ike_json);
}
END_TEST

/**
 * 인증 설정 파싱 테스트 - PSK
 */
START_TEST(test_parse_auth_config_psk)
{
    // Given
    cJSON *auth_json = cJSON_CreateObject();
    cJSON_AddStringToObject(auth_json, "auth", "psk");
    cJSON_AddStringToObject(auth_json, "id", "client@example.com");
    cJSON_AddStringToObject(auth_json, "secret", "supersecret123");
    
    // When
    auth_cfg_t *result = parser->parse_auth_config(parser, auth_json, TRUE);
    
    // Then
    ck_assert_ptr_nonnull(result);
    
    // 인증 클래스 확인
    auth_class_t auth_class = (auth_class_t)result->get(result, AUTH_RULE_AUTH_CLASS);
    ck_assert_int_eq(auth_class, AUTH_CLASS_PSK);
    
    // ID 확인
    identification_t *identity = (identification_t*)result->get(result, AUTH_RULE_IDENTITY);
    ck_assert_ptr_nonnull(identity);
    
    // Cleanup
    result->destroy(result);
    cJSON_Delete(auth_json);
}
END_TEST

/**
 * 인증 설정 파싱 테스트 - 공개키
 */
START_TEST(test_parse_auth_config_pubkey)
{
    // Given
    cJSON *auth_json = cJSON_CreateObject();
    cJSON_AddStringToObject(auth_json, "auth", "pubkey");
    cJSON_AddStringToObject(auth_json, "id", "server@example.com");
    
    // When
    auth_cfg_t *result = parser->parse_auth_config(parser, auth_json, FALSE);
    
    // Then
    ck_assert_ptr_nonnull(result);
    
    // 인증 클래스 확인
    auth_class_t auth_class = (auth_class_t)result->get(result, AUTH_RULE_AUTH_CLASS);
    ck_assert_int_eq(auth_class, AUTH_CLASS_PUBKEY);
    
    // Cleanup
    result->destroy(result);
    cJSON_Delete(auth_json);
}
END_TEST

/**
 * 인증 설정 파싱 테스트 - 잘못된 인증 타입
 */
START_TEST(test_parse_auth_config_invalid_type)
{
    // Given
    cJSON *auth_json = cJSON_CreateObject();
    cJSON_AddStringToObject(auth_json, "auth", "invalid_auth_type");
    cJSON_AddStringToObject(auth_json, "id", "user@example.com");
    
    // When
    auth_cfg_t *result = parser->parse_auth_config(parser, auth_json, TRUE);
    
    // Then
    ck_assert_ptr_null(result); // 잘못된 타입은 NULL 반환해야 함
    
    // Cleanup
    cJSON_Delete(auth_json);
}
END_TEST

/**
 * 인증 설정 파싱 테스트 - 누락된 필드들
 */
START_TEST(test_parse_auth_config_missing_fields)
{
    // Given - auth 타입 누락
    cJSON *auth_json = cJSON_CreateObject();
    cJSON_AddStringToObject(auth_json, "id", "user@example.com");
    
    // When
    auth_cfg_t *result = parser->parse_auth_config(parser, auth_json, TRUE);
    
    // Then
    ck_assert_ptr_null(result); // 필수 필드 누락시 NULL 반환
    
    // Cleanup
    cJSON_Delete(auth_json);
}
END_TEST

/**
 * Child SA 설정 파싱 테스트 - 완전한 설정
 */
START_TEST(test_parse_child_configs_complete)
{
    // Given
    peer_cfg_create_t peer_create_cfg = {0};
    ike_cfg_t *ike_cfg = ike_cfg_create(&(ike_cfg_create_t){
        .local = "192.168.1.10",
        .remote = "203.0.113.5",
        .version = IKE_ANY
    });
    peer_cfg_t *peer_cfg = peer_cfg_create("test-peer", ike_cfg, &peer_create_cfg);
    
    cJSON *children_json = cJSON_CreateArray();
    
    // 첫 번째 Child SA
    cJSON *child1 = cJSON_CreateObject();
    cJSON_AddStringToObject(child1, "name", "child1");
    cJSON_AddStringToObject(child1, "start_action", "start");
    cJSON_AddStringToObject(child1, "dpd_action", "clear");
    
    // 로컬 TS
    cJSON *local_ts = cJSON_CreateArray();
    cJSON_AddItemToArray(local_ts, cJSON_CreateString("10.0.0.0/24"));
    cJSON_AddItemToObject(child1, "local_ts", local_ts);
    
    // 원격 TS
    cJSON *remote_ts = cJSON_CreateArray();
    cJSON_AddItemToArray(remote_ts, cJSON_CreateString("10.0.1.0/24"));
    cJSON_AddItemToObject(child1, "remote_ts", remote_ts);
    
    // ESP 제안
    cJSON *esp_proposals = cJSON_CreateArray();
    cJSON_AddItemToArray(esp_proposals, cJSON_CreateString("aes128gcm16"));
    cJSON_AddItemToObject(child1, "esp_proposals", esp_proposals);
    
    cJSON_AddItemToArray(children_json, child1);
    
    // When
    bool result = parser->parse_child_configs(parser, peer_cfg, children_json);
    
    // Then
    ck_assert_int_eq(result, TRUE);
    
    // Child SA가 추가되었는지 확인
    linked_list_t *child_cfgs = peer_cfg->get_child_cfgs(peer_cfg);
    ck_assert_ptr_nonnull(child_cfgs);
    ck_assert_int_eq(child_cfgs->get_count(child_cfgs), 1);
    
    // Cleanup
    peer_cfg->destroy(peer_cfg);
    cJSON_Delete(children_json);
}
END_TEST

/**
 * Child SA 설정 파싱 테스트 - 빈 배열
 */
START_TEST(test_parse_child_configs_empty)
{
    // Given
    peer_cfg_create_t peer_create_cfg = {0};
    ike_cfg_t *ike_cfg = ike_cfg_create(&(ike_cfg_create_t){
        .local = "192.168.1.10",
        .remote = "203.0.113.5",
        .version = IKE_ANY
    });
    peer_cfg_t *peer_cfg = peer_cfg_create("test-peer", ike_cfg, &peer_create_cfg);
    
    cJSON *children_json = cJSON_CreateArray();
    
    // When
    bool result = parser->parse_child_configs(parser, peer_cfg, children_json);
    
    // Then
    ck_assert_int_eq(result, TRUE); // 빈 배열도 성공해야 함
    
    // Cleanup
    peer_cfg->destroy(peer_cfg);
    cJSON_Delete(children_json);
}
END_TEST

/**
 * Child SA 설정 파싱 테스트 - 잘못된 Child SA (이름 누락)
 */
START_TEST(test_parse_child_configs_missing_name)
{
    // Given
    peer_cfg_create_t peer_create_cfg = {0};
    ike_cfg_t *ike_cfg = ike_cfg_create(&(ike_cfg_create_t){
        .local = "192.168.1.10",
        .remote = "203.0.113.5",
        .version = IKE_ANY
    });
    peer_cfg_t *peer_cfg = peer_cfg_create("test-peer", ike_cfg, &peer_create_cfg);
    
    cJSON *children_json = cJSON_CreateArray();
    
    // 이름이 누락된 Child SA
    cJSON *child1 = cJSON_CreateObject();
    cJSON_AddStringToObject(child1, "start_action", "start");
    // name 필드 누락
    cJSON_AddItemToArray(children_json, child1);
    
    // When
    bool result = parser->parse_child_configs(parser, peer_cfg, children_json);
    
    // Then
    ck_assert_int_eq(result, TRUE); // 잘못된 Child SA는 무시하고 성공해야 함
    
    // Child SA가 추가되지 않았는지 확인
    linked_list_t *child_cfgs = peer_cfg->get_child_cfgs(peer_cfg);
    ck_assert_int_eq(child_cfgs->get_count(child_cfgs), 0);
    
    // Cleanup
    peer_cfg->destroy(peer_cfg);
    cJSON_Delete(children_json);
}
END_TEST

/**
 * parse_config_entity 테스트 - 현재 미구현
 */
START_TEST(test_parse_config_entity_not_implemented)
{
    // Given
    const char *config_json = "{\"name\":\"test\"}";
    
    // When
    extsock_config_entity_t *result = parser->parse_config_entity(parser, config_json);
    
    // Then
    ck_assert_ptr_null(result); // 현재 미구현이므로 NULL 반환
}
END_TEST

/**
 * NULL 입력 안전성 테스트들
 */
START_TEST(test_null_pointer_safety)
{
    // parse_proposals with NULL
    linked_list_t *result1 = parser->parse_proposals(parser, NULL, PROTO_IKE, TRUE);
    ck_assert_ptr_nonnull(result1);
    ck_assert_int_gt(result1->get_count(result1), 0); // 기본값 추가
    result1->destroy_offset(result1, offsetof(proposal_t, destroy));
    
    // parse_traffic_selectors with NULL
    linked_list_t *result2 = parser->parse_traffic_selectors(parser, NULL);
    ck_assert_ptr_nonnull(result2);
    ck_assert_int_gt(result2->get_count(result2), 0); // 기본값 추가
    result2->destroy_offset(result2, offsetof(traffic_selector_t, destroy));
    
    // parse_ike_config with NULL
    ike_cfg_t *result3 = parser->parse_ike_config(parser, NULL);
    ck_assert_ptr_null(result3); // NULL 입력시 NULL 반환
    
    // parse_auth_config with NULL
    auth_cfg_t *result4 = parser->parse_auth_config(parser, NULL, TRUE);
    ck_assert_ptr_null(result4); // NULL 입력시 NULL 반환
    
    // parse_child_configs with NULL children
    peer_cfg_create_t peer_create_cfg = {0};
    ike_cfg_t *ike_cfg = ike_cfg_create(&(ike_cfg_create_t){
        .local = "192.168.1.10",
        .remote = "203.0.113.5",
        .version = IKE_ANY
    });
    peer_cfg_t *peer_cfg = peer_cfg_create("test-peer", ike_cfg, &peer_create_cfg);
    
    bool result5 = parser->parse_child_configs(parser, peer_cfg, NULL);
    ck_assert_int_eq(result5, TRUE); // NULL children도 성공
    
    peer_cfg->destroy(peer_cfg);
    
    // parse_config_entity with NULL
    extsock_config_entity_t *result6 = parser->parse_config_entity(parser, NULL);
    ck_assert_ptr_null(result6);
}
END_TEST

Suite *json_parser_complete_suite(void)
{
    Suite *s;
    TCase *tc_proposals, *tc_ts, *tc_ike, *tc_auth, *tc_child, *tc_entity, *tc_safety;

    s = suite_create("JSON Parser Complete Tests");

    /* 제안 파싱 테스트 */
    tc_proposals = tcase_create("Proposal Parsing");
    tcase_add_checked_fixture(tc_proposals, setup_json_parser_complete_test, teardown_json_parser_complete_test);
    tcase_add_test(tc_proposals, test_parse_proposals_ike_valid);
    tcase_add_test(tc_proposals, test_parse_proposals_esp_valid);
    tcase_add_test(tc_proposals, test_parse_proposals_empty_default);
    tcase_add_test(tc_proposals, test_parse_proposals_invalid_string);
    suite_add_tcase(s, tc_proposals);

    /* 트래픽 셀렉터 파싱 테스트 */
    tc_ts = tcase_create("Traffic Selector Parsing");
    tcase_add_checked_fixture(tc_ts, setup_json_parser_complete_test, teardown_json_parser_complete_test);
    tcase_add_test(tc_ts, test_parse_traffic_selectors_valid_cidr);
    tcase_add_test(tc_ts, test_parse_traffic_selectors_invalid_cidr);
    tcase_add_test(tc_ts, test_parse_traffic_selectors_empty_default);
    suite_add_tcase(s, tc_ts);

    /* IKE 설정 파싱 테스트 */
    tc_ike = tcase_create("IKE Config Parsing");
    tcase_add_checked_fixture(tc_ike, setup_json_parser_complete_test, teardown_json_parser_complete_test);
    tcase_add_test(tc_ike, test_parse_ike_config_complete);
    tcase_add_test(tc_ike, test_parse_ike_config_minimal);
    suite_add_tcase(s, tc_ike);

    /* 인증 설정 파싱 테스트 */
    tc_auth = tcase_create("Auth Config Parsing");
    tcase_add_checked_fixture(tc_auth, setup_json_parser_complete_test, teardown_json_parser_complete_test);
    tcase_add_test(tc_auth, test_parse_auth_config_psk);
    tcase_add_test(tc_auth, test_parse_auth_config_pubkey);
    tcase_add_test(tc_auth, test_parse_auth_config_invalid_type);
    tcase_add_test(tc_auth, test_parse_auth_config_missing_fields);
    suite_add_tcase(s, tc_auth);

    /* Child SA 설정 파싱 테스트 */
    tc_child = tcase_create("Child Config Parsing");
    tcase_add_checked_fixture(tc_child, setup_json_parser_complete_test, teardown_json_parser_complete_test);
    tcase_add_test(tc_child, test_parse_child_configs_complete);
    tcase_add_test(tc_child, test_parse_child_configs_empty);
    tcase_add_test(tc_child, test_parse_child_configs_missing_name);
    suite_add_tcase(s, tc_child);

    /* 엔티티 파싱 테스트 */
    tc_entity = tcase_create("Config Entity Parsing");
    tcase_add_checked_fixture(tc_entity, setup_json_parser_complete_test, teardown_json_parser_complete_test);
    tcase_add_test(tc_entity, test_parse_config_entity_not_implemented);
    suite_add_tcase(s, tc_entity);

    /* 안전성 테스트 */
    tc_safety = tcase_create("NULL Pointer Safety");
    tcase_add_checked_fixture(tc_safety, setup_json_parser_complete_test, teardown_json_parser_complete_test);
    tcase_add_test(tc_safety, test_null_pointer_safety);
    suite_add_tcase(s, tc_safety);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = json_parser_complete_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 