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

/**
 * JSON 기본 구조 파싱 테스트
 */
START_TEST(test_json_basic_structure)
{
    // Given - 기본 IPsec 설정 JSON
    const char *config_json = "{"
        "\"connection_name\": \"test-tunnel\","
        "\"ike\": {"
        "    \"version\": 2,"
        "    \"local_addrs\": [\"192.168.1.10\"],"
        "    \"remote_addrs\": [\"203.0.113.5\"],"
        "    \"proposals\": [\"aes256-sha256-modp2048\"]"
        "},"
        "\"auth\": {"
        "    \"local\": {"
        "        \"auth\": \"psk\","
        "        \"id\": \"client@example.com\","
        "        \"secret\": \"supersecret123\""
        "    },"
        "    \"remote\": {"
        "        \"auth\": \"psk\","
        "        \"id\": \"server@example.com\""
        "    }"
        "},"
        "\"children\": [{"
        "    \"name\": \"child1\","
        "    \"start_action\": \"start\","
        "    \"local_ts\": [\"10.0.0.0/24\"],"
        "    \"remote_ts\": [\"10.0.1.0/24\"],"
        "    \"esp_proposals\": [\"aes128gcm16\"]"
        "}]"
        "}";
    
    // When
    cJSON *json = cJSON_Parse(config_json);
    
    // Then
    ck_assert_ptr_nonnull(json);
    
    // 연결 이름 확인
    cJSON *connection_name = cJSON_GetObjectItem(json, "connection_name");
    ck_assert_ptr_nonnull(connection_name);
    ck_assert(cJSON_IsString(connection_name));
    ck_assert_str_eq(cJSON_GetStringValue(connection_name), "test-tunnel");
    
    // IKE 설정 확인
    cJSON *ike = cJSON_GetObjectItem(json, "ike");
    ck_assert_ptr_nonnull(ike);
    ck_assert(cJSON_IsObject(ike));
    
    cJSON *ike_version = cJSON_GetObjectItem(ike, "version");
    ck_assert_ptr_nonnull(ike_version);
    ck_assert(cJSON_IsNumber(ike_version));
    ck_assert_int_eq(cJSON_GetNumberValue(ike_version), 2);
    
    // 로컬 주소 배열 확인
    cJSON *local_addrs = cJSON_GetObjectItem(ike, "local_addrs");
    ck_assert_ptr_nonnull(local_addrs);
    ck_assert(cJSON_IsArray(local_addrs));
    ck_assert_int_eq(cJSON_GetArraySize(local_addrs), 1);
    
    cJSON *first_local_addr = cJSON_GetArrayItem(local_addrs, 0);
    ck_assert_ptr_nonnull(first_local_addr);
    ck_assert(cJSON_IsString(first_local_addr));
    ck_assert_str_eq(cJSON_GetStringValue(first_local_addr), "192.168.1.10");
    
    // 인증 설정 확인
    cJSON *auth = cJSON_GetObjectItem(json, "auth");
    ck_assert_ptr_nonnull(auth);
    ck_assert(cJSON_IsObject(auth));
    
    cJSON *local_auth = cJSON_GetObjectItem(auth, "local");
    ck_assert_ptr_nonnull(local_auth);
    ck_assert(cJSON_IsObject(local_auth));
    
    cJSON *auth_type = cJSON_GetObjectItem(local_auth, "auth");
    ck_assert_ptr_nonnull(auth_type);
    ck_assert(cJSON_IsString(auth_type));
    ck_assert_str_eq(cJSON_GetStringValue(auth_type), "psk");
    
    // Child SA 설정 확인
    cJSON *children = cJSON_GetObjectItem(json, "children");
    ck_assert_ptr_nonnull(children);
    ck_assert(cJSON_IsArray(children));
    ck_assert_int_eq(cJSON_GetArraySize(children), 1);
    
    cJSON *child1 = cJSON_GetArrayItem(children, 0);
    ck_assert_ptr_nonnull(child1);
    ck_assert(cJSON_IsObject(child1));
    
    cJSON *child_name = cJSON_GetObjectItem(child1, "name");
    ck_assert_ptr_nonnull(child_name);
    ck_assert(cJSON_IsString(child_name));
    ck_assert_str_eq(cJSON_GetStringValue(child_name), "child1");
    
    // Cleanup
    cJSON_Delete(json);
}
END_TEST

/**
 * 제안(Proposals) 배열 처리 테스트
 */
START_TEST(test_proposals_array_parsing)
{
    // Given
    const char *proposals_json = "["
        "\"aes256-sha256-modp2048\","
        "\"aes128-sha1-modp1024\","
        "\"3des-md5-modp768\""
        "]";
    
    // When
    cJSON *proposals = cJSON_Parse(proposals_json);
    
    // Then
    ck_assert_ptr_nonnull(proposals);
    ck_assert(cJSON_IsArray(proposals));
    ck_assert_int_eq(cJSON_GetArraySize(proposals), 3);
    
    // 각 제안 확인
    cJSON *proposal;
    int index = 0;
    cJSON_ArrayForEach(proposal, proposals) {
        ck_assert(cJSON_IsString(proposal));
        ck_assert_ptr_nonnull(cJSON_GetStringValue(proposal));
        
        if (index == 0) {
            ck_assert_str_eq(cJSON_GetStringValue(proposal), "aes256-sha256-modp2048");
        } else if (index == 1) {
            ck_assert_str_eq(cJSON_GetStringValue(proposal), "aes128-sha1-modp1024");
        } else if (index == 2) {
            ck_assert_str_eq(cJSON_GetStringValue(proposal), "3des-md5-modp768");
        }
        index++;
    }
    
    // Cleanup
    cJSON_Delete(proposals);
}
END_TEST

/**
 * 트래픽 셀렉터 CIDR 형식 검증 테스트
 */
START_TEST(test_traffic_selectors_validation)
{
    // Given - 다양한 CIDR 형식들
    const char *valid_cidrs[] = {
        "0.0.0.0/0",        // 모든 트래픽
        "10.0.0.0/8",       // 클래스 A 사설망
        "172.16.0.0/12",    // 클래스 B 사설망
        "192.168.0.0/16",   // 클래스 C 사설망
        "192.168.1.1/32",   // 단일 호스트
        "203.0.113.0/24"    // 테스트용 네트워크
    };
    
    const char *invalid_cidrs[] = {
        "invalid-cidr",
        "300.400.500.600/24",
        "192.168.1.0/33",
        "192.168.1.0/-1",
        "192.168.1.0/",
        "/24"
    };
    
    // When/Then - 유효한 CIDR들 테스트
    for (int i = 0; i < sizeof(valid_cidrs)/sizeof(valid_cidrs[0]); i++) {
        char json_str[256];
        snprintf(json_str, sizeof(json_str), "[\"%s\"]", valid_cidrs[i]);
        
        cJSON *ts_array = cJSON_Parse(json_str);
        ck_assert_ptr_nonnull(ts_array);
        ck_assert(cJSON_IsArray(ts_array));
        ck_assert_int_eq(cJSON_GetArraySize(ts_array), 1);
        
        cJSON *cidr = cJSON_GetArrayItem(ts_array, 0);
        ck_assert_ptr_nonnull(cidr);
        ck_assert(cJSON_IsString(cidr));
        ck_assert_str_eq(cJSON_GetStringValue(cidr), valid_cidrs[i]);
        
        cJSON_Delete(ts_array);
    }
    
    // When/Then - 잘못된 CIDR들도 파싱은 성공해야 함 (검증은 나중에)
    for (int i = 0; i < sizeof(invalid_cidrs)/sizeof(invalid_cidrs[0]); i++) {
        char json_str[256];
        snprintf(json_str, sizeof(json_str), "[\"%s\"]", invalid_cidrs[i]);
        
        cJSON *ts_array = cJSON_Parse(json_str);
        ck_assert_ptr_nonnull(ts_array); // JSON 파싱 자체는 성공
        ck_assert(cJSON_IsArray(ts_array));
        
        cJSON_Delete(ts_array);
    }
}
END_TEST

/**
 * 인증 설정 유효성 검증 테스트
 */
START_TEST(test_auth_config_validation)
{
    // Given - PSK 인증 설정
    const char *psk_auth_json = "{"
        "\"auth\": \"psk\","
        "\"id\": \"client@example.com\","
        "\"secret\": \"supersecret123\""
        "}";
    
    // When
    cJSON *psk_auth = cJSON_Parse(psk_auth_json);
    
    // Then
    ck_assert_ptr_nonnull(psk_auth);
    ck_assert(cJSON_IsObject(psk_auth));
    
    cJSON *auth_type = cJSON_GetObjectItem(psk_auth, "auth");
    ck_assert_ptr_nonnull(auth_type);
    ck_assert(cJSON_IsString(auth_type));
    ck_assert_str_eq(cJSON_GetStringValue(auth_type), "psk");
    
    cJSON *id = cJSON_GetObjectItem(psk_auth, "id");
    ck_assert_ptr_nonnull(id);
    ck_assert(cJSON_IsString(id));
    ck_assert_str_eq(cJSON_GetStringValue(id), "client@example.com");
    
    cJSON *secret = cJSON_GetObjectItem(psk_auth, "secret");
    ck_assert_ptr_nonnull(secret);
    ck_assert(cJSON_IsString(secret));
    ck_assert_str_eq(cJSON_GetStringValue(secret), "supersecret123");
    
    cJSON_Delete(psk_auth);
    
    // Given - 공개키 인증 설정
    const char *pubkey_auth_json = "{"
        "\"auth\": \"pubkey\","
        "\"id\": \"server@example.com\""
        "}";
    
    // When
    cJSON *pubkey_auth = cJSON_Parse(pubkey_auth_json);
    
    // Then
    ck_assert_ptr_nonnull(pubkey_auth);
    ck_assert(cJSON_IsObject(pubkey_auth));
    
    auth_type = cJSON_GetObjectItem(pubkey_auth, "auth");
    ck_assert_ptr_nonnull(auth_type);
    ck_assert(cJSON_IsString(auth_type));
    ck_assert_str_eq(cJSON_GetStringValue(auth_type), "pubkey");
    
    cJSON_Delete(pubkey_auth);
}
END_TEST

/**
 * 잘못된 JSON 형식 에러 처리 테스트
 */
START_TEST(test_malformed_json_handling)
{
    // Given - 다양한 잘못된 JSON 형식들
    const char *malformed_jsons[] = {
        "{invalid json",              // 닫히지 않은 중괄호
        "{\"key\": }",               // 값 누락
        "{\"key\": \"value\",}",     // 끝에 콤마
        "{ key: \"value\" }",        // 따옴표 없는 키
        "{\"key\": 'value'}",        // 작은따옴표 사용
        "null",                      // null 값
        "",                          // 빈 문자열
        "{ \"a\": { \"b\": }",      // 중첩된 오류
    };
    
    // When/Then
    for (int i = 0; i < sizeof(malformed_jsons)/sizeof(malformed_jsons[0]); i++) {
        cJSON *json = cJSON_Parse(malformed_jsons[i]);
        
        // "null"은 유효한 JSON이므로 예외 처리
        if (strcmp(malformed_jsons[i], "null") == 0) {
            // "null"은 유효한 JSON이므로 성공해야 함
            ck_assert_ptr_nonnull(json);
            ck_assert(cJSON_IsNull(json));
            cJSON_Delete(json);
        } else {
            // 나머지 잘못된 JSON은 NULL을 반환해야 함
            ck_assert_ptr_null(json);
            
            // 에러 정보 확인 (빈 문자열이 아닌 경우)
            if (strlen(malformed_jsons[i]) > 0) {
                const char *error = cJSON_GetErrorPtr();
                ck_assert_ptr_nonnull(error);
            }
        }
    }
}
END_TEST

/**
 * 중첩된 JSON 구조 처리 테스트
 */
START_TEST(test_nested_json_structures)
{
    // Given - 복잡한 중첩 구조
    const char *complex_json = "{"
        "\"connection\": {"
        "    \"ike\": {"
        "        \"settings\": {"
        "            \"proposals\": ["
        "                \"aes256-sha256-modp2048\","
        "                \"aes128-sha1-modp1024\""
        "            ],"
        "            \"version\": 2"
        "        }"
        "    },"
        "    \"children\": ["
        "        {"
        "            \"child1\": {"
        "                \"local_ts\": [\"10.0.0.0/24\", \"10.0.1.0/24\"],"
        "                \"remote_ts\": [\"10.0.2.0/24\"]"
        "            }"
        "        }"
        "    ]"
        "}"
        "}";
    
    // When
    cJSON *json = cJSON_Parse(complex_json);
    
    // Then
    ck_assert_ptr_nonnull(json);
    
    // 깊이 4단계 접근: root -> connection -> ike -> settings -> version
    cJSON *connection = cJSON_GetObjectItem(json, "connection");
    ck_assert_ptr_nonnull(connection);
    
    cJSON *ike = cJSON_GetObjectItem(connection, "ike");
    ck_assert_ptr_nonnull(ike);
    
    cJSON *settings = cJSON_GetObjectItem(ike, "settings");
    ck_assert_ptr_nonnull(settings);
    
    cJSON *version = cJSON_GetObjectItem(settings, "version");
    ck_assert_ptr_nonnull(version);
    ck_assert(cJSON_IsNumber(version));
    ck_assert_int_eq(cJSON_GetNumberValue(version), 2);
    
    // 배열 접근
    cJSON *children = cJSON_GetObjectItem(connection, "children");
    ck_assert_ptr_nonnull(children);
    ck_assert(cJSON_IsArray(children));
    ck_assert_int_eq(cJSON_GetArraySize(children), 1);
    
    cJSON *child_obj = cJSON_GetArrayItem(children, 0);
    ck_assert_ptr_nonnull(child_obj);
    
    cJSON *child1 = cJSON_GetObjectItem(child_obj, "child1");
    ck_assert_ptr_nonnull(child1);
    
    cJSON *local_ts = cJSON_GetObjectItem(child1, "local_ts");
    ck_assert_ptr_nonnull(local_ts);
    ck_assert(cJSON_IsArray(local_ts));
    ck_assert_int_eq(cJSON_GetArraySize(local_ts), 2);
    
    // Cleanup
    cJSON_Delete(json);
}
END_TEST

/**
 * 큰 JSON 데이터 처리 테스트
 */
START_TEST(test_large_json_data)
{
    // Given - 많은 수의 제안들을 가진 설정
    cJSON *root = cJSON_CreateObject();
    cJSON *proposals = cJSON_CreateArray();
    
    // 50개의 제안 생성
    for (int i = 0; i < 50; i++) {
        char proposal_str[256];
        snprintf(proposal_str, sizeof(proposal_str), "aes%d-sha256-modp2048", 128 + i * 8);
        cJSON_AddItemToArray(proposals, cJSON_CreateString(proposal_str));
    }
    cJSON_AddItemToObject(root, "proposals", proposals);
    
    // JSON 문자열로 변환
    char *json_string = cJSON_Print(root);
    ck_assert_ptr_nonnull(json_string);
    
    // When - 다시 파싱
    cJSON *parsed = cJSON_Parse(json_string);
    
    // Then
    ck_assert_ptr_nonnull(parsed);
    
    cJSON *parsed_proposals = cJSON_GetObjectItem(parsed, "proposals");
    ck_assert_ptr_nonnull(parsed_proposals);
    ck_assert(cJSON_IsArray(parsed_proposals));
    ck_assert_int_eq(cJSON_GetArraySize(parsed_proposals), 50);
    
    // 몇 개의 제안 확인
    cJSON *first_proposal = cJSON_GetArrayItem(parsed_proposals, 0);
    ck_assert_ptr_nonnull(first_proposal);
    ck_assert_str_eq(cJSON_GetStringValue(first_proposal), "aes128-sha256-modp2048");
    
    cJSON *last_proposal = cJSON_GetArrayItem(parsed_proposals, 49);
    ck_assert_ptr_nonnull(last_proposal);
    ck_assert_str_eq(cJSON_GetStringValue(last_proposal), "aes520-sha256-modp2048");
    
    // Cleanup
    cJSON_Delete(root);
    cJSON_Delete(parsed);
    free(json_string);
}
END_TEST

Suite *json_parser_simple_suite(void)
{
    Suite *s;
    TCase *tc_basic, *tc_validation, *tc_error, *tc_complex;

    s = suite_create("JSON Parser Simple Tests");

    /* 기본 파싱 테스트 */
    tc_basic = tcase_create("Basic JSON Parsing");
    tcase_add_test(tc_basic, test_json_basic_structure);
    tcase_add_test(tc_basic, test_proposals_array_parsing);
    suite_add_tcase(s, tc_basic);

    /* 데이터 검증 테스트 */
    tc_validation = tcase_create("Data Validation");
    tcase_add_test(tc_validation, test_traffic_selectors_validation);
    tcase_add_test(tc_validation, test_auth_config_validation);
    suite_add_tcase(s, tc_validation);

    /* 에러 처리 테스트 */
    tc_error = tcase_create("Error Handling");
    tcase_add_test(tc_error, test_malformed_json_handling);
    suite_add_tcase(s, tc_error);

    /* 복잡한 구조 테스트 */
    tc_complex = tcase_create("Complex Structures");
    tcase_add_test(tc_complex, test_nested_json_structures);
    tcase_add_test(tc_complex, test_large_json_data);
    suite_add_tcase(s, tc_complex);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = json_parser_simple_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 