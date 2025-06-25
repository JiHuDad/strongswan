/*
 * Copyright (C) 2024 strongSwan Project
 * Unit tests for Event Usecase
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <library.h>
#include <sa/ike_sa.h>
#include <sa/child_sa.h>
#include "../../usecases/extsock_event_usecase.h"
#include "../../adapters/socket/extsock_socket_adapter.h"
#include "../../common/extsock_common.h"

static extsock_event_usecase_t *event_usecase;

/**
 * Mock Socket Adapter for testing
 */
typedef struct mock_socket_adapter_t {
    extsock_socket_adapter_t public;
    char *last_event_sent;
    int send_count;
    extsock_error_t return_error;
} mock_socket_adapter_t;

static extsock_error_t mock_send_event(mock_socket_adapter_t *this, const char *event_json)
{
    free(this->last_event_sent);
    this->last_event_sent = strdup(event_json);
    this->send_count++;
    return this->return_error;
}

static void mock_socket_adapter_destroy(mock_socket_adapter_t *this)
{
    free(this->last_event_sent);
    free(this);
}

static extsock_socket_adapter_t *create_mock_socket_adapter()
{
    mock_socket_adapter_t *mock;
    
    INIT(mock,
        .public = {
            .send_event = (void*)mock_send_event,
            .destroy = (void*)mock_socket_adapter_destroy,
        },
        .last_event_sent = NULL,
        .send_count = 0,
        .return_error = EXTSOCK_ERROR_NONE,
    );
    
    return &mock->public;
}

/**
 * Mock IKE SA for testing
 */
typedef struct mock_ike_sa_t {
    ike_sa_t public;
    char *name;
    ike_sa_state_t state;
} mock_ike_sa_t;

static char* mock_get_name(mock_ike_sa_t *this)
{
    return this->name;
}

static ike_sa_state_t mock_get_state(mock_ike_sa_t *this)
{
    return this->state;
}

static void mock_ike_sa_destroy(mock_ike_sa_t *this)
{
    free(this->name);
    free(this);
}

static ike_sa_t *create_mock_ike_sa(const char *name, ike_sa_state_t state)
{
    mock_ike_sa_t *mock;
    
    INIT(mock,
        .public = {
            .get_name = (void*)mock_get_name,
            .get_state = (void*)mock_get_state,
            .destroy = (void*)mock_ike_sa_destroy,
        },
        .name = strdup(name),
        .state = state,
    );
    
    return &mock->public;
}

/**
 * Mock Child SA for testing
 */
typedef struct mock_child_sa_t {
    child_sa_t public;
    char *name;
    child_sa_state_t state;
} mock_child_sa_t;

static char* mock_child_get_name(mock_child_sa_t *this)
{
    return this->name;
}

static child_sa_state_t mock_child_get_state(mock_child_sa_t *this)
{
    return this->state;
}

static void mock_child_sa_destroy(mock_child_sa_t *this)
{
    free(this->name);
    free(this);
}

static child_sa_t *create_mock_child_sa(const char *name, child_sa_state_t state)
{
    mock_child_sa_t *mock;
    
    INIT(mock,
        .public = {
            .get_name = (void*)mock_child_get_name,
            .get_state = (void*)mock_child_get_state,
            .destroy = (void*)mock_child_sa_destroy,
        },
        .name = strdup(name),
        .state = state,
    );
    
    return &mock->public;
}

/**
 * 테스트 설정
 */
void setup_event_usecase_test(void)
{
    library_init(NULL, "test-event-usecase");
    
    event_usecase = extsock_event_usecase_create();
    ck_assert_ptr_nonnull(event_usecase);
}

/**
 * 테스트 해제
 */
void teardown_event_usecase_test(void)
{
    if (event_usecase) {
        event_usecase->destroy(event_usecase);
        event_usecase = NULL;
    }
    
    library_deinit();
}

/**
 * Child SA Up 이벤트 처리 테스트
 */
START_TEST(test_handle_child_sa_up)
{
    // Given
    ike_sa_t *ike_sa = create_mock_ike_sa("test-connection", IKE_ESTABLISHED);
    child_sa_t *child_sa = create_mock_child_sa("child-tunnel", CHILD_INSTALLED);
    
    extsock_socket_adapter_t *socket_adapter = create_mock_socket_adapter();
    event_usecase->set_socket_adapter(event_usecase, socket_adapter);
    
    // When
    event_usecase->handle_child_updown(event_usecase, ike_sa, child_sa, TRUE);
    
    // Then
    mock_socket_adapter_t *mock_adapter = (mock_socket_adapter_t*)socket_adapter;
    ck_assert_int_eq(mock_adapter->send_count, 1);
    ck_assert_ptr_nonnull(mock_adapter->last_event_sent);
    
    // JSON 이벤트 내용 검증
    cJSON *event_json = cJSON_Parse(mock_adapter->last_event_sent);
    ck_assert_ptr_nonnull(event_json);
    
    cJSON *event_type = cJSON_GetObjectItem(event_json, "event");
    ck_assert_ptr_nonnull(event_type);
    ck_assert_str_eq(cJSON_GetStringValue(event_type), "child_sa_up");
    
    cJSON *ike_sa_name = cJSON_GetObjectItem(event_json, "ike_sa_name");
    ck_assert_ptr_nonnull(ike_sa_name);
    ck_assert_str_eq(cJSON_GetStringValue(ike_sa_name), "test-connection");
    
    cJSON *child_sa_name = cJSON_GetObjectItem(event_json, "child_sa_name");
    ck_assert_ptr_nonnull(child_sa_name);
    ck_assert_str_eq(cJSON_GetStringValue(child_sa_name), "child-tunnel");
    
    // 정리
    cJSON_Delete(event_json);
    ike_sa->destroy(ike_sa);
    child_sa->destroy(child_sa);
    socket_adapter->destroy(socket_adapter);
}
END_TEST

/**
 * Child SA Down 이벤트 처리 테스트
 */
START_TEST(test_handle_child_sa_down)
{
    // Given
    ike_sa_t *ike_sa = create_mock_ike_sa("test-connection", IKE_ESTABLISHED);
    child_sa_t *child_sa = create_mock_child_sa("child-tunnel", CHILD_DESTROYING);
    
    extsock_socket_adapter_t *socket_adapter = create_mock_socket_adapter();
    event_usecase->set_socket_adapter(event_usecase, socket_adapter);
    
    // When
    event_usecase->handle_child_updown(event_usecase, ike_sa, child_sa, FALSE);
    
    // Then
    mock_socket_adapter_t *mock_adapter = (mock_socket_adapter_t*)socket_adapter;
    ck_assert_int_eq(mock_adapter->send_count, 1);
    ck_assert_ptr_nonnull(mock_adapter->last_event_sent);
    
    // JSON 이벤트 내용 검증
    cJSON *event_json = cJSON_Parse(mock_adapter->last_event_sent);
    ck_assert_ptr_nonnull(event_json);
    
    cJSON *event_type = cJSON_GetObjectItem(event_json, "event");
    ck_assert_ptr_nonnull(event_type);
    ck_assert_str_eq(cJSON_GetStringValue(event_type), "child_sa_down");
    
    // 정리
    cJSON_Delete(event_json);
    ike_sa->destroy(ike_sa);
    child_sa->destroy(child_sa);
    socket_adapter->destroy(socket_adapter);
}
END_TEST

/**
 * NULL IKE SA 처리 테스트
 */
START_TEST(test_handle_child_updown_null_ike_sa)
{
    // Given
    child_sa_t *child_sa = create_mock_child_sa("child-tunnel", CHILD_INSTALLED);
    
    extsock_socket_adapter_t *socket_adapter = create_mock_socket_adapter();
    event_usecase->set_socket_adapter(event_usecase, socket_adapter);
    
    // When
    event_usecase->handle_child_updown(event_usecase, NULL, child_sa, TRUE);
    
    // Then - 이벤트가 전송되지 않아야 함
    mock_socket_adapter_t *mock_adapter = (mock_socket_adapter_t*)socket_adapter;
    ck_assert_int_eq(mock_adapter->send_count, 0);
    
    // 정리
    child_sa->destroy(child_sa);
    socket_adapter->destroy(socket_adapter);
}
END_TEST

/**
 * NULL Child SA 처리 테스트
 */
START_TEST(test_handle_child_updown_null_child_sa)
{
    // Given
    ike_sa_t *ike_sa = create_mock_ike_sa("test-connection", IKE_ESTABLISHED);
    
    extsock_socket_adapter_t *socket_adapter = create_mock_socket_adapter();
    event_usecase->set_socket_adapter(event_usecase, socket_adapter);
    
    // When
    event_usecase->handle_child_updown(event_usecase, ike_sa, NULL, TRUE);
    
    // Then - 이벤트가 전송되지 않아야 함
    mock_socket_adapter_t *mock_adapter = (mock_socket_adapter_t*)socket_adapter;
    ck_assert_int_eq(mock_adapter->send_count, 0);
    
    // 정리
    ike_sa->destroy(ike_sa);
    socket_adapter->destroy(socket_adapter);
}
END_TEST

/**
 * 소켓 어댑터 없이 이벤트 처리 테스트
 */
START_TEST(test_handle_child_updown_no_socket_adapter)
{
    // Given
    ike_sa_t *ike_sa = create_mock_ike_sa("test-connection", IKE_ESTABLISHED);
    child_sa_t *child_sa = create_mock_child_sa("child-tunnel", CHILD_INSTALLED);
    
    // 소켓 어댑터를 설정하지 않음
    
    // When - 예외가 발생하지 않아야 함
    event_usecase->handle_child_updown(event_usecase, ike_sa, child_sa, TRUE);
    
    // Then - 테스트 통과면 OK (크래시되지 않음)
    
    // 정리
    ike_sa->destroy(ike_sa);
    child_sa->destroy(child_sa);
}
END_TEST

/**
 * 이벤트 발행자 인터페이스 조회 테스트
 */
START_TEST(test_get_event_publisher)
{
    // When
    extsock_event_publisher_t *publisher = event_usecase->get_event_publisher(event_usecase);
    
    // Then
    ck_assert_ptr_nonnull(publisher);
}
END_TEST

/**
 * 소켓 어댑터 설정 테스트
 */
START_TEST(test_set_socket_adapter)
{
    // Given
    extsock_socket_adapter_t *socket_adapter = create_mock_socket_adapter();
    
    // When
    event_usecase->set_socket_adapter(event_usecase, socket_adapter);
    
    // Then - 예외가 발생하지 않으면 성공
    
    // 정리
    socket_adapter->destroy(socket_adapter);
}
END_TEST

/**
 * NULL 소켓 어댑터 설정 테스트
 */
START_TEST(test_set_null_socket_adapter)
{
    // When
    event_usecase->set_socket_adapter(event_usecase, NULL);
    
    // Then - 예외가 발생하지 않으면 성공
}
END_TEST

/**
 * 이벤트 발행자를 통한 직접 이벤트 발행 테스트
 */
START_TEST(test_event_publisher_publish)
{
    // Given
    extsock_socket_adapter_t *socket_adapter = create_mock_socket_adapter();
    event_usecase->set_socket_adapter(event_usecase, socket_adapter);
    
    extsock_event_publisher_t *publisher = event_usecase->get_event_publisher(event_usecase);
    ck_assert_ptr_nonnull(publisher);
    
    const char *test_event = 
        "{"
        "\"event\":\"custom_event\","
        "\"data\":\"test_data\","
        "\"timestamp\":\"2024-01-01T00:00:00Z\""
        "}";
    
    // When
    extsock_error_t result = publisher->publish_event(publisher, test_event);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_NONE);
    
    mock_socket_adapter_t *mock_adapter = (mock_socket_adapter_t*)socket_adapter;
    ck_assert_int_eq(mock_adapter->send_count, 1);
    ck_assert_ptr_nonnull(mock_adapter->last_event_sent);
    ck_assert_str_eq(mock_adapter->last_event_sent, test_event);
    
    // 정리
    socket_adapter->destroy(socket_adapter);
}
END_TEST

/**
 * 이벤트 발행자를 통한 NULL 이벤트 발행 테스트
 */
START_TEST(test_event_publisher_publish_null)
{
    // Given
    extsock_event_publisher_t *publisher = event_usecase->get_event_publisher(event_usecase);
    ck_assert_ptr_nonnull(publisher);
    
    // When
    extsock_error_t result = publisher->publish_event(publisher, NULL);
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * 이벤트 발행자를 통한 빈 이벤트 발행 테스트
 */
START_TEST(test_event_publisher_publish_empty)
{
    // Given
    extsock_event_publisher_t *publisher = event_usecase->get_event_publisher(event_usecase);
    ck_assert_ptr_nonnull(publisher);
    
    // When
    extsock_error_t result = publisher->publish_event(publisher, "");
    
    // Then
    ck_assert_int_eq(result, EXTSOCK_ERROR_CONFIG_INVALID);
}
END_TEST

/**
 * 다중 Child SA 이벤트 처리 테스트
 */
START_TEST(test_handle_multiple_child_events)
{
    // Given
    ike_sa_t *ike_sa = create_mock_ike_sa("multi-connection", IKE_ESTABLISHED);
    child_sa_t *child_sa1 = create_mock_child_sa("child-1", CHILD_INSTALLED);
    child_sa_t *child_sa2 = create_mock_child_sa("child-2", CHILD_INSTALLED);
    
    extsock_socket_adapter_t *socket_adapter = create_mock_socket_adapter();
    event_usecase->set_socket_adapter(event_usecase, socket_adapter);
    
    // When
    event_usecase->handle_child_updown(event_usecase, ike_sa, child_sa1, TRUE);
    event_usecase->handle_child_updown(event_usecase, ike_sa, child_sa2, TRUE);
    
    // Then
    mock_socket_adapter_t *mock_adapter = (mock_socket_adapter_t*)socket_adapter;
    ck_assert_int_eq(mock_adapter->send_count, 2);
    
    // 정리
    ike_sa->destroy(ike_sa);
    child_sa1->destroy(child_sa1);
    child_sa2->destroy(child_sa2);
    socket_adapter->destroy(socket_adapter);
}
END_TEST

/**
 * 이벤트 전송 실패 처리 테스트
 */
START_TEST(test_handle_event_send_failure)
{
    // Given
    ike_sa_t *ike_sa = create_mock_ike_sa("test-connection", IKE_ESTABLISHED);
    child_sa_t *child_sa = create_mock_child_sa("child-tunnel", CHILD_INSTALLED);
    
    extsock_socket_adapter_t *socket_adapter = create_mock_socket_adapter();
    mock_socket_adapter_t *mock_adapter = (mock_socket_adapter_t*)socket_adapter;
    mock_adapter->return_error = EXTSOCK_ERROR_SOCKET; // 전송 실패 설정
    
    event_usecase->set_socket_adapter(event_usecase, socket_adapter);
    
    // When - 예외가 발생하지 않아야 함
    event_usecase->handle_child_updown(event_usecase, ike_sa, child_sa, TRUE);
    
    // Then - 전송은 시도되었지만 실패
    ck_assert_int_eq(mock_adapter->send_count, 1);
    
    // 정리
    ike_sa->destroy(ike_sa);
    child_sa->destroy(child_sa);
    socket_adapter->destroy(socket_adapter);
}
END_TEST

/**
 * 테스트 스위트 생성
 */
Suite *event_usecase_suite(void)
{
    Suite *s;
    TCase *tc_core, *tc_error, *tc_advanced;

    s = suite_create("Event Usecase");

    /* Core test case */
    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup_event_usecase_test, teardown_event_usecase_test);
    tcase_add_test(tc_core, test_handle_child_sa_up);
    tcase_add_test(tc_core, test_handle_child_sa_down);
    tcase_add_test(tc_core, test_get_event_publisher);
    tcase_add_test(tc_core, test_set_socket_adapter);
    tcase_add_test(tc_core, test_event_publisher_publish);
    suite_add_tcase(s, tc_core);

    /* Error handling test case */
    tc_error = tcase_create("Error Handling");
    tcase_add_checked_fixture(tc_error, setup_event_usecase_test, teardown_event_usecase_test);
    tcase_add_test(tc_error, test_handle_child_updown_null_ike_sa);
    tcase_add_test(tc_error, test_handle_child_updown_null_child_sa);
    tcase_add_test(tc_error, test_handle_child_updown_no_socket_adapter);
    tcase_add_test(tc_error, test_set_null_socket_adapter);
    tcase_add_test(tc_error, test_event_publisher_publish_null);
    tcase_add_test(tc_error, test_event_publisher_publish_empty);
    tcase_add_test(tc_error, test_handle_event_send_failure);
    suite_add_tcase(s, tc_error);

    /* Advanced test case */
    tc_advanced = tcase_create("Advanced");
    tcase_add_checked_fixture(tc_advanced, setup_event_usecase_test, teardown_event_usecase_test);
    tcase_add_test(tc_advanced, test_handle_multiple_child_events);
    suite_add_tcase(s, tc_advanced);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = event_usecase_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 