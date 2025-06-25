/*
 * Copyright (C) 2024 strongSwan Project
 * Unit tests for Config Entity
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <library.h>
#include <config/ike_cfg.h>
#include <config/peer_cfg.h>
#include <collections/linked_list.h>
#include "../../domain/extsock_config_entity.h"
#include "../../common/extsock_common.h"

static extsock_config_entity_t *config_entity;

/**
 * 테스트 설정
 */
void setup_config_entity_test(void)
{
    library_init(NULL, "test-config-entity");
    config_entity = NULL;
}

/**
 * 테스트 해제
 */
void teardown_config_entity_test(void)
{
    if (config_entity) {
        config_entity->destroy(config_entity);
        config_entity = NULL;
    }
    
    library_deinit();
}

/**
 * 유효한 JSON으로부터 설정 엔티티 생성 테스트
 */
START_TEST(test_create_from_valid_json)
{
    // Given
    const char *valid_json = 
        "{"
        "\"name\":\"test-connection\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "},"
        "\"children\":[{"
            "\"name\":\"child1\","
            "\"mode\":\"tunnel\","
            "\"proposals\":[\"aes128gcm16-prfsha256-modp2048\"],"
            "\"local_ts\":[\"10.0.0.1/32\"],"
            "\"remote_ts\":[\"10.0.0.2/32\"]"
        "}]"
        "}";
    
    // When
    config_entity = extsock_config_entity_create_from_json(valid_json);
    
    // Then
    ck_assert_ptr_nonnull(config_entity);
    ck_assert_ptr_nonnull(config_entity->get_name(config_entity));
    ck_assert_str_eq(config_entity->get_name(config_entity), "test-connection");
}
END_TEST

/**
 * 잘못된 JSON 형식 처리 테스트
 */
START_TEST(test_create_from_invalid_json)
{
    // Given
    const char *invalid_json = "{invalid json format";
    
    // When
    config_entity = extsock_config_entity_create_from_json(invalid_json);
    
    // Then
    ck_assert_ptr_null(config_entity);
}
END_TEST

/**
 * NULL JSON 처리 테스트
 */
START_TEST(test_create_from_null_json)
{
    // When
    config_entity = extsock_config_entity_create_from_json(NULL);
    
    // Then
    ck_assert_ptr_null(config_entity);
}
END_TEST

/**
 * 빈 JSON 처리 테스트
 */
START_TEST(test_create_from_empty_json)
{
    // When
    config_entity = extsock_config_entity_create_from_json("");
    
    // Then
    ck_assert_ptr_null(config_entity);
}
END_TEST

/**
 * 필수 필드 누락 테스트 - name 필드
 */
START_TEST(test_create_missing_name)
{
    // Given
    const char *missing_name_json = 
        "{"
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "}"
        "}";
    
    // When
    config_entity = extsock_config_entity_create_from_json(missing_name_json);
    
    // Then
    ck_assert_ptr_null(config_entity);
}
END_TEST

/**
 * 필수 필드 누락 테스트 - auth 필드
 */
START_TEST(test_create_missing_auth)
{
    // Given
    const char *missing_auth_json = 
        "{"
        "\"name\":\"test-connection\","
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "}"
        "}";
    
    // When
    config_entity = extsock_config_entity_create_from_json(missing_auth_json);
    
    // Then
    ck_assert_ptr_null(config_entity);
}
END_TEST

/**
 * 설정 유효성 검증 테스트 - 유효한 설정
 */
START_TEST(test_validate_valid_config)
{
    // Given
    const char *valid_json = 
        "{"
        "\"name\":\"valid-config\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "}"
        "}";
    
    config_entity = extsock_config_entity_create_from_json(valid_json);
    ck_assert_ptr_nonnull(config_entity);
    
    // When
    bool is_valid = config_entity->validate(config_entity);
    
    // Then
    ck_assert_int_eq(is_valid, TRUE);
}
END_TEST

/**
 * 설정 유효성 검증 테스트 - 잘못된 설정
 */
START_TEST(test_validate_invalid_config)
{
    // Given - 잘못된 암호화 제안
    const char *invalid_json = 
        "{"
        "\"name\":\"invalid-config\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"invalid-cipher-invalid-hash-invalid-dh\"]"
        "}"
        "}";
    
    config_entity = extsock_config_entity_create_from_json(invalid_json);
    
    if (config_entity) {
        // When
        bool is_valid = config_entity->validate(config_entity);
        
        // Then
        ck_assert_int_eq(is_valid, FALSE);
    }
}
END_TEST

/**
 * strongSwan peer_cfg_t 변환 테스트
 */
START_TEST(test_to_peer_cfg)
{
    // Given
    const char *valid_json = 
        "{"
        "\"name\":\"peer-config\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "}"
        "}";
    
    config_entity = extsock_config_entity_create_from_json(valid_json);
    ck_assert_ptr_nonnull(config_entity);
    
    // When
    peer_cfg_t *peer_cfg = config_entity->to_peer_cfg(config_entity);
    
    // Then
    ck_assert_ptr_nonnull(peer_cfg);
    ck_assert_str_eq(peer_cfg->get_name(peer_cfg), "peer-config");
    
    // 정리
    peer_cfg->destroy(peer_cfg);
}
END_TEST

/**
 * 설정 엔티티 복제 테스트
 */
START_TEST(test_clone_config_entity)
{
    // Given
    const char *valid_json = 
        "{"
        "\"name\":\"original-config\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "}"
        "}";
    
    config_entity = extsock_config_entity_create_from_json(valid_json);
    ck_assert_ptr_nonnull(config_entity);
    
    // When
    extsock_config_entity_t *cloned = config_entity->clone(config_entity);
    
    // Then
    ck_assert_ptr_nonnull(cloned);
    ck_assert_ptr_ne(cloned, config_entity); // 다른 인스턴스여야 함
    ck_assert_str_eq(cloned->get_name(cloned), config_entity->get_name(config_entity));
    
    // 정리
    cloned->destroy(cloned);
}
END_TEST

/**
 * 복잡한 설정 생성 테스트 (다중 Child SA)
 */
START_TEST(test_create_complex_config)
{
    // Given
    const char *complex_json = 
        "{"
        "\"name\":\"complex-connection\","
        "\"auth\":{"
            "\"id\":\"client@company.com\","
            "\"remote_id\":\"gateway@company.com\","
            "\"method\":\"psk\","
            "\"psk\":\"verylongsecretkey123456789\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":["
                "\"aes256-sha512-modp4096\","
                "\"aes128-sha256-modp2048\""
            "]"
        "},"
        "\"children\":["
            "{"
                "\"name\":\"subnet1\","
                "\"mode\":\"tunnel\","
                "\"proposals\":[\"aes256gcm16-prfsha512-modp4096\"],"
                "\"local_ts\":[\"192.168.1.0/24\"],"
                "\"remote_ts\":[\"10.0.0.0/8\"]"
            "},"
            "{"
                "\"name\":\"subnet2\","
                "\"mode\":\"tunnel\","
                "\"proposals\":[\"aes128gcm16-prfsha256-modp2048\"],"
                "\"local_ts\":[\"192.168.2.0/24\"],"
                "\"remote_ts\":[\"172.16.0.0/12\"]"
            "}"
        "],"
        "\"dpd\":{"
            "\"delay\":30,"
            "\"timeout\":120"
        "}"
        "}";
    
    // When
    config_entity = extsock_config_entity_create_from_json(complex_json);
    
    // Then
    ck_assert_ptr_nonnull(config_entity);
    ck_assert_str_eq(config_entity->get_name(config_entity), "complex-connection");
    ck_assert_int_eq(config_entity->validate(config_entity), TRUE);
    
    // peer_cfg 변환도 성공해야 함
    peer_cfg_t *peer_cfg = config_entity->to_peer_cfg(config_entity);
    ck_assert_ptr_nonnull(peer_cfg);
    
    // 정리
    peer_cfg->destroy(peer_cfg);
}
END_TEST

/**
 * 잘못된 IKE 버전 테스트
 */
START_TEST(test_invalid_ike_version)
{
    // Given
    const char *invalid_version_json = 
        "{"
        "\"name\":\"invalid-version\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":3,"  // 지원하지 않는 버전
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "}"
        "}";
    
    // When
    config_entity = extsock_config_entity_create_from_json(invalid_version_json);
    
    // Then
    if (config_entity) {
        ck_assert_int_eq(config_entity->validate(config_entity), FALSE);
    }
}
END_TEST

/**
 * 잘못된 인증 방법 테스트
 */
START_TEST(test_invalid_auth_method)
{
    // Given
    const char *invalid_auth_json = 
        "{"
        "\"name\":\"invalid-auth\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"unknown-method\","  // 지원하지 않는 인증 방법
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "}"
        "}";
    
    // When
    config_entity = extsock_config_entity_create_from_json(invalid_auth_json);
    
    // Then
    if (config_entity) {
        ck_assert_int_eq(config_entity->validate(config_entity), FALSE);
    }
}
END_TEST

/**
 * 빈 제안 목록 테스트
 */
START_TEST(test_empty_proposals)
{
    // Given
    const char *empty_proposals_json = 
        "{"
        "\"name\":\"empty-proposals\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[]"  // 빈 제안 목록
        "}"
        "}";
    
    // When
    config_entity = extsock_config_entity_create_from_json(empty_proposals_json);
    
    // Then
    if (config_entity) {
        ck_assert_int_eq(config_entity->validate(config_entity), FALSE);
    }
}
END_TEST

/**
 * 잘못된 트래픽 셀렉터 테스트
 */
START_TEST(test_invalid_traffic_selectors)
{
    // Given
    const char *invalid_ts_json = 
        "{"
        "\"name\":\"invalid-ts\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "},"
        "\"children\":[{"
            "\"name\":\"child1\","
            "\"mode\":\"tunnel\","
            "\"proposals\":[\"aes128gcm16-prfsha256-modp2048\"],"
            "\"local_ts\":[\"invalid-ip-range\"],"  // 잘못된 IP 범위
            "\"remote_ts\":[\"10.0.0.2/32\"]"
        "}]"
        "}";
    
    // When
    config_entity = extsock_config_entity_create_from_json(invalid_ts_json);
    
    // Then
    if (config_entity) {
        ck_assert_int_eq(config_entity->validate(config_entity), FALSE);
    }
}
END_TEST

/**
 * 직접 생성자를 사용한 설정 엔티티 생성 테스트
 */
START_TEST(test_direct_constructor)
{
    // Given
    ike_cfg_create_t ike_create_cfg = {
        .version = IKEV2,
        .local = "0.0.0.0",
        .local_port = 500,
        .remote = "0.0.0.0", 
        .remote_port = 500,
        .no_certreq = FALSE,
        .ocsp_certreq = FALSE,
        .force_encap = FALSE,
        .fragmentation = FRAGMENTATION_NO,
        .childless = CHILDLESS_NEVER,
        .dscp = 0
    };
    
    ike_cfg_t *ike_cfg = ike_cfg_create(&ike_create_cfg);
    ck_assert_ptr_nonnull(ike_cfg);
    
    linked_list_t *local_auths = linked_list_create();
    linked_list_t *remote_auths = linked_list_create();
    
    // When
    config_entity = extsock_config_entity_create("direct-test", ike_cfg, 
                                                 local_auths, remote_auths);
    
    // Then
    ck_assert_ptr_nonnull(config_entity);
    ck_assert_str_eq(config_entity->get_name(config_entity), "direct-test");
    
    // 정리 - ike_cfg와 lists는 config_entity가 소유권을 가짐
}
END_TEST

/**
 * NULL 파라미터로 직접 생성자 호출 테스트
 */
START_TEST(test_direct_constructor_null_params)
{
    // When
    config_entity = extsock_config_entity_create(NULL, NULL, NULL, NULL);
    
    // Then
    ck_assert_ptr_null(config_entity);
}
END_TEST

/**
 * 테스트 스위트 생성
 */
Suite *config_entity_suite(void)
{
    Suite *s;
    TCase *tc_core, *tc_error, *tc_validation, *tc_advanced;

    s = suite_create("Config Entity");

    /* Core test case */
    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup_config_entity_test, teardown_config_entity_test);
    tcase_add_test(tc_core, test_create_from_valid_json);
    tcase_add_test(tc_core, test_to_peer_cfg);
    tcase_add_test(tc_core, test_clone_config_entity);
    tcase_add_test(tc_core, test_direct_constructor);
    suite_add_tcase(s, tc_core);

    /* Error handling test case */
    tc_error = tcase_create("Error Handling");
    tcase_add_checked_fixture(tc_error, setup_config_entity_test, teardown_config_entity_test);
    tcase_add_test(tc_error, test_create_from_invalid_json);
    tcase_add_test(tc_error, test_create_from_null_json);
    tcase_add_test(tc_error, test_create_from_empty_json);
    tcase_add_test(tc_error, test_create_missing_name);
    tcase_add_test(tc_error, test_create_missing_auth);
    tcase_add_test(tc_error, test_direct_constructor_null_params);
    suite_add_tcase(s, tc_error);

    /* Validation test case */
    tc_validation = tcase_create("Validation");
    tcase_add_checked_fixture(tc_validation, setup_config_entity_test, teardown_config_entity_test);
    tcase_add_test(tc_validation, test_validate_valid_config);
    tcase_add_test(tc_validation, test_validate_invalid_config);
    tcase_add_test(tc_validation, test_invalid_ike_version);
    tcase_add_test(tc_validation, test_invalid_auth_method);
    tcase_add_test(tc_validation, test_empty_proposals);
    tcase_add_test(tc_validation, test_invalid_traffic_selectors);
    suite_add_tcase(s, tc_validation);

    /* Advanced test case */
    tc_advanced = tcase_create("Advanced");
    tcase_add_checked_fixture(tc_advanced, setup_config_entity_test, teardown_config_entity_test);
    tcase_add_test(tc_advanced, test_create_complex_config);
    suite_add_tcase(s, tc_advanced);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = config_entity_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 