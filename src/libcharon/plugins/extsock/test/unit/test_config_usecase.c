/*
 * Copyright (C) 2024 strongSwan Project
 * Unit tests for Config Usecase
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <library.h>
#include "../../usecases/extsock_config_usecase.h"
#include "../../adapters/json/extsock_json_parser.h"
#include "../../common/extsock_common.h"

static extsock_config_usecase_t *config_usecase;
static extsock_json_parser_t *json_parser;
static extsock_event_publisher_t *event_publisher;

/**
 * Mock Event Publisher for testing
 */
typedef struct mock_event_publisher_t {
    extsock_event_publisher_t public;
    char *last_event;
    int publish_count;
} mock_event_publisher_t;

static extsock_error_t mock_publish_event(mock_event_publisher_t *this, const char *event_json)
{
    free(this->last_event);
    this->last_event = strdup(event_json);
    this->publish_count++;
    return EXTSOCK_ERROR_NONE;
}

static void mock_event_publisher_destroy(mock_event_publisher_t *this)
{
    free(this->last_event);
    free(this);
}

static extsock_event_publisher_t *create_mock_event_publisher()
{
    mock_event_publisher_t *mock;
    
    INIT(mock,
        .public = {
            .publish_event = (void*)mock_publish_event,
            .destroy = (void*)mock_event_publisher_destroy,
        },
        .last_event = NULL,
        .publish_count = 0,
    );
    
    return &mock->public;
}

/**
 * 테스트 설정
 */
void setup_config_usecase_test(void)
{
    library_init(NULL, "test-config-usecase");
    
    json_parser = extsock_json_parser_create();
    ck_assert_ptr_nonnull(json_parser);
    
    event_publisher = create_mock_event_publisher();
    ck_assert_ptr_nonnull(event_publisher);
    
    config_usecase = extsock_config_usecase_create(json_parser, event_publisher);
    ck_assert_ptr_nonnull(config_usecase);
}

/**
 * 테스트 해제
 */
void teardown_config_usecase_test(void)
{
    if (config_usecase) {
        config_usecase->destroy(config_usecase);
        config_usecase = NULL;
    }
    
    if (event_publisher) {
        event_publisher->destroy(event_publisher);
        event_publisher = NULL;
    }
    
    if (json_parser) {
        json_parser->destroy(json_parser);
        json_parser = NULL;
    }
    
    library_deinit();
}

/**
 * 유효한 JSON 설정 적용 테스트
 */
START_TEST(test_apply_valid_json_config)
{
    // Given
    const char *valid_config = 
        "{"
        "\"name\":\"test-conn\","
        "\"auth\":{"
            "\"id\":\"client1\","
            "\"remote_id\":\"server1\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"aes128-sha256-modp2048\"]"
        "},"
        "\"children\":[{"
            "\"name\":\"net1\","
            "\"mode\":\"tunnel\","
            "\"proposals\":[\"aes128gcm16-prfsha256-modp2048\"],"
            "\"local_ts\":[\"10.0.0.1/32\"],"
            "\"remote_ts\":[\"10.0.0.2/32\"]"
        "}]"
        "}";
    
    // When
    extsock_error_t result = config_usecase->apply_json_config(config_usecase, valid_config);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_NONE);
    
    // 이벤트가 발행되었는지 확인
    mock_event_publisher_t *mock = (mock_event_publisher_t*)event_publisher;
    ck_assert_int_gt(mock->publish_count, 0);
    ck_assert_ptr_nonnull(mock->last_event);
    ck_assert(strstr(mock->last_event, "config_applied") != NULL);
}
END_TEST

/**
 * 잘못된 JSON 형식 처리 테스트
 */
START_TEST(test_apply_invalid_json_format)
{
    // Given
    const char *invalid_json = "{invalid json format";
    
    // When
    extsock_error_t result = config_usecase->apply_json_config(config_usecase, invalid_json);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_JSON_PARSE);
}
END_TEST

/**
 * NULL 설정 처리 테스트
 */
START_TEST(test_apply_null_config)
{
    // When
    extsock_error_t result = config_usecase->apply_json_config(config_usecase, NULL);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * 빈 설정 처리 테스트
 */
START_TEST(test_apply_empty_config)
{
    // When
    extsock_error_t result = config_usecase->apply_json_config(config_usecase, "");
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * 필수 필드 누락 테스트
 */
START_TEST(test_apply_missing_required_fields)
{
    // Given - name 필드 누락
    const char *incomplete_config = 
        "{"
        "\"auth\":{"
            "\"id\":\"client1\","
            "\"remote_id\":\"server1\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "}"
        "}";
    
    // When
    extsock_error_t result = config_usecase->apply_json_config(config_usecase, incomplete_config);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * DPD 시작 테스트
 */
START_TEST(test_start_dpd_valid)
{
    // Given
    const char *ike_sa_name = "test-connection";
    
    // When
    extsock_error_t result = config_usecase->start_dpd(config_usecase, ike_sa_name);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_NONE);
    
    // DPD 이벤트가 발행되었는지 확인
    mock_event_publisher_t *mock = (mock_event_publisher_t*)event_publisher;
    ck_assert_int_gt(mock->publish_count, 0);
    ck_assert_ptr_nonnull(mock->last_event);
    ck_assert(strstr(mock->last_event, "dpd_started") != NULL);
    ck_assert(strstr(mock->last_event, ike_sa_name) != NULL);
}
END_TEST

/**
 * NULL IKE SA 이름으로 DPD 시작 테스트
 */
START_TEST(test_start_dpd_null_name)
{
    // When
    extsock_error_t result = config_usecase->start_dpd(config_usecase, NULL);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * 빈 IKE SA 이름으로 DPD 시작 테스트
 */
START_TEST(test_start_dpd_empty_name)
{
    // When
    extsock_error_t result = config_usecase->start_dpd(config_usecase, "");
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * 설정 제거 테스트
 */
START_TEST(test_remove_config_valid)
{
    // Given
    const char *config_name = "test-connection";
    
    // When
    extsock_error_t result = config_usecase->remove_config(config_usecase, config_name);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_NONE);
    
    // 설정 제거 이벤트가 발행되었는지 확인
    mock_event_publisher_t *mock = (mock_event_publisher_t*)event_publisher;
    ck_assert_int_gt(mock->publish_count, 0);
    ck_assert_ptr_nonnull(mock->last_event);
    ck_assert(strstr(mock->last_event, "config_removed") != NULL);
    ck_assert(strstr(mock->last_event, config_name) != NULL);
}
END_TEST

/**
 * NULL 이름으로 설정 제거 테스트
 */
START_TEST(test_remove_config_null_name)
{
    // When
    extsock_error_t result = config_usecase->remove_config(config_usecase, NULL);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * 명령 처리기 조회 테스트
 */
START_TEST(test_get_command_handler)
{
    // When
    extsock_command_handler_t *handler = config_usecase->get_command_handler(config_usecase);
    
    // Then
    ck_assert_ptr_nonnull(handler);
}
END_TEST

/**
 * 복잡한 설정 적용 테스트
 */
START_TEST(test_apply_complex_config)
{
    // Given
    const char *complex_config = 
        "{"
        "\"name\":\"complex-conn\","
        "\"auth\":{"
            "\"id\":\"client@example.com\","
            "\"remote_id\":\"server@example.com\","
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
    extsock_error_t result = config_usecase->apply_json_config(config_usecase, complex_config);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_NONE);
    
    // 복잡한 설정에 대한 이벤트 발행 확인
    mock_event_publisher_t *mock = (mock_event_publisher_t*)event_publisher;
    ck_assert_int_gt(mock->publish_count, 0);
    ck_assert_ptr_nonnull(mock->last_event);
}
END_TEST

/**
 * 잘못된 암호화 설정 테스트
 */
START_TEST(test_apply_invalid_crypto_config)
{
    // Given - 지원하지 않는 암호화 알고리즘
    const char *invalid_crypto_config = 
        "{"
        "\"name\":\"invalid-crypto\","
        "\"auth\":{"
            "\"id\":\"client1\","
            "\"remote_id\":\"server1\","
            "\"method\":\"psk\","
            "\"psk\":\"secret123\""
        "},"
        "\"ike\":{"
            "\"version\":2,"
            "\"proposals\":[\"invalid-cipher-invalid-hash-invalid-dh\"]"
        "}"
        "}";
    
    // When
    extsock_error_t result = config_usecase->apply_json_config(config_usecase, invalid_crypto_config);
    
    // Then
    ck_assert_int_ne(result, EXTSOCK_ERROR_NONE);
}
END_TEST

/**
 * 테스트 스위트 생성
 */
Suite *config_usecase_suite(void)
{
    Suite *s;
    TCase *tc_core, *tc_error, *tc_advanced;

    s = suite_create("Config Usecase");

    /* Core test case */
    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup_config_usecase_test, teardown_config_usecase_test);
    tcase_add_test(tc_core, test_apply_valid_json_config);
    tcase_add_test(tc_core, test_start_dpd_valid);
    tcase_add_test(tc_core, test_remove_config_valid);
    tcase_add_test(tc_core, test_get_command_handler);
    suite_add_tcase(s, tc_core);

    /* Error handling test case */
    tc_error = tcase_create("Error Handling");
    tcase_add_checked_fixture(tc_error, setup_config_usecase_test, teardown_config_usecase_test);
    tcase_add_test(tc_error, test_apply_invalid_json_format);
    tcase_add_test(tc_error, test_apply_null_config);
    tcase_add_test(tc_error, test_apply_empty_config);
    tcase_add_test(tc_error, test_apply_missing_required_fields);
    tcase_add_test(tc_error, test_start_dpd_null_name);
    tcase_add_test(tc_error, test_start_dpd_empty_name);
    tcase_add_test(tc_error, test_remove_config_null_name);
    tcase_add_test(tc_error, test_apply_invalid_crypto_config);
    suite_add_tcase(s, tc_error);

    /* Advanced test case */
    tc_advanced = tcase_create("Advanced");
    tcase_add_checked_fixture(tc_advanced, setup_config_usecase_test, teardown_config_usecase_test);
    tcase_add_test(tc_advanced, test_apply_complex_config);
    suite_add_tcase(s, tc_advanced);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = config_usecase_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 