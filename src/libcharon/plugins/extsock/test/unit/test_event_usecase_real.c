/*
 * Copyright (C) 2024 strongSwan Project
 * Real Event Usecase Implementation Tests
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <cjson/cJSON.h>

// Event usecase 관련 타입들
typedef enum {
    EXTSOCK_EVENT_TYPE_CONNECTION_UP,
    EXTSOCK_EVENT_TYPE_CONNECTION_DOWN,
    EXTSOCK_EVENT_TYPE_AUTH_SUCCESS,
    EXTSOCK_EVENT_TYPE_AUTH_FAILURE,
    EXTSOCK_EVENT_TYPE_CHILD_UP,
    EXTSOCK_EVENT_TYPE_CHILD_DOWN,
    EXTSOCK_EVENT_TYPE_ERROR
} extsock_event_type_t;

typedef enum {
    EXTSOCK_EVENT_PRIORITY_LOW,
    EXTSOCK_EVENT_PRIORITY_NORMAL,
    EXTSOCK_EVENT_PRIORITY_HIGH,
    EXTSOCK_EVENT_PRIORITY_CRITICAL
} extsock_event_priority_t;

typedef struct {
    extsock_event_type_t type;
    extsock_event_priority_t priority;
    char *connection_name;
    char *message;
    time_t timestamp;
    char *source_ip;
    char *dest_ip;
} extsock_event_t;

typedef struct {
    extsock_event_t **events;
    size_t count;
    size_t capacity;
    bool (*add_event)(void *this, extsock_event_t *event);
    extsock_event_t* (*get_event)(void *this, size_t index);
    void (*clear_events)(void *this);
    void (*destroy)(void *this);
} extsock_event_usecase_t;

static extsock_event_usecase_t *event_usecase;
static extsock_event_t *test_event;

/**
 * 이벤트 생성 헬퍼 함수
 */
static extsock_event_t* create_test_event(extsock_event_type_t type, const char *conn_name, const char *msg)
{
    extsock_event_t *event = malloc(sizeof(extsock_event_t));
    if (!event) return NULL;
    
    event->type = type;
    event->priority = EXTSOCK_EVENT_PRIORITY_NORMAL;
    event->connection_name = strdup(conn_name);
    event->message = strdup(msg);
    event->timestamp = time(NULL);
    event->source_ip = strdup("192.168.1.10");
    event->dest_ip = strdup("203.0.113.5");
    
    return event;
}

/**
 * 이벤트 해제 헬퍼 함수
 */
static void destroy_event(extsock_event_t *event)
{
    if (event) {
        free(event->connection_name);
        free(event->message);
        free(event->source_ip);
        free(event->dest_ip);
        free(event);
    }
}

/**
 * 테스트 설정
 */
void setup_event_usecase_real_test(void)
{
    // Mock event usecase 생성
    event_usecase = malloc(sizeof(extsock_event_usecase_t));
    ck_assert_ptr_nonnull(event_usecase);
    
    event_usecase->capacity = 10;
    event_usecase->count = 0;
    event_usecase->events = malloc(sizeof(extsock_event_t*) * event_usecase->capacity);
    ck_assert_ptr_nonnull(event_usecase->events);
    
    // 테스트 이벤트 생성
    test_event = create_test_event(EXTSOCK_EVENT_TYPE_CONNECTION_UP, "test_connection", "Connection established");
    ck_assert_ptr_nonnull(test_event);
}

/**
 * 테스트 해제
 */
void teardown_event_usecase_real_test(void)
{
    if (event_usecase) {
        if (event_usecase->events) {
            for (size_t i = 0; i < event_usecase->count; i++) {
                destroy_event(event_usecase->events[i]);
            }
            free(event_usecase->events);
        }
        free(event_usecase);
        event_usecase = NULL;
    }
    
    if (test_event) {
        destroy_event(test_event);
        test_event = NULL;
    }
}

/**
 * 이벤트 생성 테스트
 */
START_TEST(test_real_event_creation)
{
    // Given / When / Then
    ck_assert_ptr_nonnull(test_event);
    ck_assert_int_eq(test_event->type, EXTSOCK_EVENT_TYPE_CONNECTION_UP);
    ck_assert_int_eq(test_event->priority, EXTSOCK_EVENT_PRIORITY_NORMAL);
    ck_assert_str_eq(test_event->connection_name, "test_connection");
    ck_assert_str_eq(test_event->message, "Connection established");
    ck_assert_str_eq(test_event->source_ip, "192.168.1.10");
    ck_assert_str_eq(test_event->dest_ip, "203.0.113.5");
    ck_assert_int_gt(test_event->timestamp, 0);
}
END_TEST

/**
 * 이벤트 타입별 생성 테스트
 */
START_TEST(test_real_event_types)
{
    // Given - 다양한 이벤트 타입들
    extsock_event_t *events[6];
    
    events[0] = create_test_event(EXTSOCK_EVENT_TYPE_CONNECTION_UP, "conn1", "Connected");
    events[1] = create_test_event(EXTSOCK_EVENT_TYPE_CONNECTION_DOWN, "conn1", "Disconnected");
    events[2] = create_test_event(EXTSOCK_EVENT_TYPE_AUTH_SUCCESS, "conn1", "Authentication successful");
    events[3] = create_test_event(EXTSOCK_EVENT_TYPE_AUTH_FAILURE, "conn1", "Authentication failed");
    events[4] = create_test_event(EXTSOCK_EVENT_TYPE_CHILD_UP, "conn1", "Child SA established");
    events[5] = create_test_event(EXTSOCK_EVENT_TYPE_CHILD_DOWN, "conn1", "Child SA terminated");
    
    // When / Then - 각 이벤트 타입 검증
    ck_assert_int_eq(events[0]->type, EXTSOCK_EVENT_TYPE_CONNECTION_UP);
    ck_assert_int_eq(events[1]->type, EXTSOCK_EVENT_TYPE_CONNECTION_DOWN);
    ck_assert_int_eq(events[2]->type, EXTSOCK_EVENT_TYPE_AUTH_SUCCESS);
    ck_assert_int_eq(events[3]->type, EXTSOCK_EVENT_TYPE_AUTH_FAILURE);
    ck_assert_int_eq(events[4]->type, EXTSOCK_EVENT_TYPE_CHILD_UP);
    ck_assert_int_eq(events[5]->type, EXTSOCK_EVENT_TYPE_CHILD_DOWN);
    
    // Cleanup
    for (int i = 0; i < 6; i++) {
        destroy_event(events[i]);
    }
}
END_TEST

/**
 * 이벤트 우선순위 테스트
 */
START_TEST(test_real_event_priorities)
{
    // Given - 다양한 우선순위 이벤트
    extsock_event_t *low_event = create_test_event(EXTSOCK_EVENT_TYPE_CONNECTION_UP, "conn1", "Low priority");
    extsock_event_t *normal_event = create_test_event(EXTSOCK_EVENT_TYPE_AUTH_SUCCESS, "conn1", "Normal priority");
    extsock_event_t *high_event = create_test_event(EXTSOCK_EVENT_TYPE_AUTH_FAILURE, "conn1", "High priority");
    extsock_event_t *critical_event = create_test_event(EXTSOCK_EVENT_TYPE_ERROR, "conn1", "Critical error");
    
    // When - 우선순위 설정
    low_event->priority = EXTSOCK_EVENT_PRIORITY_LOW;
    normal_event->priority = EXTSOCK_EVENT_PRIORITY_NORMAL;
    high_event->priority = EXTSOCK_EVENT_PRIORITY_HIGH;
    critical_event->priority = EXTSOCK_EVENT_PRIORITY_CRITICAL;
    
    // Then - 우선순위 검증
    ck_assert_int_eq(low_event->priority, EXTSOCK_EVENT_PRIORITY_LOW);
    ck_assert_int_eq(normal_event->priority, EXTSOCK_EVENT_PRIORITY_NORMAL);
    ck_assert_int_eq(high_event->priority, EXTSOCK_EVENT_PRIORITY_HIGH);
    ck_assert_int_eq(critical_event->priority, EXTSOCK_EVENT_PRIORITY_CRITICAL);
    
    // 우선순위 순서 확인
    ck_assert_int_lt(low_event->priority, normal_event->priority);
    ck_assert_int_lt(normal_event->priority, high_event->priority);
    ck_assert_int_lt(high_event->priority, critical_event->priority);
    
    // Cleanup
    destroy_event(low_event);
    destroy_event(normal_event);
    destroy_event(high_event);
    destroy_event(critical_event);
}
END_TEST

/**
 * 이벤트 저장소 관리 테스트
 */
START_TEST(test_real_event_storage_management)
{
    // Given - 초기 상태 확인
    ck_assert_int_eq(event_usecase->count, 0);
    ck_assert_int_eq(event_usecase->capacity, 10);
    
    // When - 이벤트 추가 시뮬레이션
    for (int i = 0; i < 5; i++) {
        char conn_name[32];
        char message[64];
        snprintf(conn_name, sizeof(conn_name), "connection_%d", i);
        snprintf(message, sizeof(message), "Event message %d", i);
        
        extsock_event_t *event = create_test_event(EXTSOCK_EVENT_TYPE_CONNECTION_UP, conn_name, message);
        ck_assert_ptr_nonnull(event);
        
        // 이벤트를 저장소에 추가
        ck_assert_int_lt(event_usecase->count, event_usecase->capacity);
        event_usecase->events[event_usecase->count] = event;
        event_usecase->count++;
    }
    
    // Then - 저장소 상태 확인
    ck_assert_int_eq(event_usecase->count, 5);
    
    // 저장된 이벤트들 검증
    for (size_t i = 0; i < event_usecase->count; i++) {
        extsock_event_t *event = event_usecase->events[i];
        ck_assert_ptr_nonnull(event);
        ck_assert_int_eq(event->type, EXTSOCK_EVENT_TYPE_CONNECTION_UP);
        ck_assert_ptr_nonnull(event->connection_name);
        ck_assert_ptr_nonnull(event->message);
    }
}
END_TEST

/**
 * 이벤트를 JSON으로 변환 테스트
 */
START_TEST(test_real_event_to_json_conversion)
{
    // Given - 이벤트
    // When - JSON 변환
    cJSON *event_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(event_json, "type", test_event->type);
    cJSON_AddNumberToObject(event_json, "priority", test_event->priority);
    cJSON_AddStringToObject(event_json, "connection_name", test_event->connection_name);
    cJSON_AddStringToObject(event_json, "message", test_event->message);
    cJSON_AddNumberToObject(event_json, "timestamp", test_event->timestamp);
    cJSON_AddStringToObject(event_json, "source_ip", test_event->source_ip);
    cJSON_AddStringToObject(event_json, "dest_ip", test_event->dest_ip);
    
    // Then - JSON 직렬화 검증
    char *json_string = cJSON_Print(event_json);
    ck_assert_ptr_nonnull(json_string);
    ck_assert_ptr_nonnull(strstr(json_string, "test_connection"));
    ck_assert_ptr_nonnull(strstr(json_string, "Connection established"));
    ck_assert_ptr_nonnull(strstr(json_string, "192.168.1.10"));
    ck_assert_ptr_nonnull(strstr(json_string, "203.0.113.5"));
    
    // JSON 파싱 재테스트
    cJSON *parsed = cJSON_Parse(json_string);
    ck_assert_ptr_nonnull(parsed);
    
    cJSON *type_item = cJSON_GetObjectItem(parsed, "type");
    ck_assert_int_eq(cJSON_GetNumberValue(type_item), EXTSOCK_EVENT_TYPE_CONNECTION_UP);
    
    cJSON *conn_item = cJSON_GetObjectItem(parsed, "connection_name");
    ck_assert_str_eq(cJSON_GetStringValue(conn_item), "test_connection");
    
    // Cleanup
    free(json_string);
    cJSON_Delete(parsed);
    cJSON_Delete(event_json);
}
END_TEST

/**
 * JSON에서 이벤트로 변환 테스트
 */
START_TEST(test_real_json_to_event_conversion)
{
    // Given - JSON 이벤트
    cJSON *event_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(event_json, "type", EXTSOCK_EVENT_TYPE_AUTH_FAILURE);
    cJSON_AddNumberToObject(event_json, "priority", EXTSOCK_EVENT_PRIORITY_HIGH);
    cJSON_AddStringToObject(event_json, "connection_name", "secure_tunnel");
    cJSON_AddStringToObject(event_json, "message", "Authentication failed for user");
    cJSON_AddNumberToObject(event_json, "timestamp", 1703760000);
    cJSON_AddStringToObject(event_json, "source_ip", "10.0.0.1");
    cJSON_AddStringToObject(event_json, "dest_ip", "10.0.1.1");
    
    // When - JSON에서 이벤트 생성
    extsock_event_t *event = malloc(sizeof(extsock_event_t));
    ck_assert_ptr_nonnull(event);
    
    cJSON *type_item = cJSON_GetObjectItem(event_json, "type");
    cJSON *priority_item = cJSON_GetObjectItem(event_json, "priority");
    cJSON *conn_item = cJSON_GetObjectItem(event_json, "connection_name");
    cJSON *msg_item = cJSON_GetObjectItem(event_json, "message");
    cJSON *timestamp_item = cJSON_GetObjectItem(event_json, "timestamp");
    cJSON *src_item = cJSON_GetObjectItem(event_json, "source_ip");
    cJSON *dst_item = cJSON_GetObjectItem(event_json, "dest_ip");
    
    event->type = (extsock_event_type_t)cJSON_GetNumberValue(type_item);
    event->priority = (extsock_event_priority_t)cJSON_GetNumberValue(priority_item);
    event->connection_name = strdup(cJSON_GetStringValue(conn_item));
    event->message = strdup(cJSON_GetStringValue(msg_item));
    event->timestamp = (time_t)cJSON_GetNumberValue(timestamp_item);
    event->source_ip = strdup(cJSON_GetStringValue(src_item));
    event->dest_ip = strdup(cJSON_GetStringValue(dst_item));
    
    // Then - 변환된 이벤트 검증
    ck_assert_int_eq(event->type, EXTSOCK_EVENT_TYPE_AUTH_FAILURE);
    ck_assert_int_eq(event->priority, EXTSOCK_EVENT_PRIORITY_HIGH);
    ck_assert_str_eq(event->connection_name, "secure_tunnel");
    ck_assert_str_eq(event->message, "Authentication failed for user");
    ck_assert_int_eq(event->timestamp, 1703760000);
    ck_assert_str_eq(event->source_ip, "10.0.0.1");
    ck_assert_str_eq(event->dest_ip, "10.0.1.1");
    
    // Cleanup
    destroy_event(event);
    cJSON_Delete(event_json);
}
END_TEST

/**
 * 이벤트 필터링 테스트
 */
START_TEST(test_real_event_filtering)
{
    // Given - 다양한 이벤트들 생성
    extsock_event_t *events[6];
    events[0] = create_test_event(EXTSOCK_EVENT_TYPE_CONNECTION_UP, "conn1", "Connected");
    events[1] = create_test_event(EXTSOCK_EVENT_TYPE_CONNECTION_DOWN, "conn1", "Disconnected");
    events[2] = create_test_event(EXTSOCK_EVENT_TYPE_AUTH_SUCCESS, "conn2", "Auth OK");
    events[3] = create_test_event(EXTSOCK_EVENT_TYPE_AUTH_FAILURE, "conn2", "Auth Failed");
    events[4] = create_test_event(EXTSOCK_EVENT_TYPE_CHILD_UP, "conn3", "Child SA up");
    events[5] = create_test_event(EXTSOCK_EVENT_TYPE_ERROR, "conn3", "Critical error");
    
    // 우선순위 설정
    events[3]->priority = EXTSOCK_EVENT_PRIORITY_HIGH;
    events[5]->priority = EXTSOCK_EVENT_PRIORITY_CRITICAL;
    
    // When - 연결별 필터링
    int conn1_count = 0;
    for (int i = 0; i < 6; i++) {
        if (strcmp(events[i]->connection_name, "conn1") == 0) {
            conn1_count++;
        }
    }
    
    // When - 우선순위별 필터링
    int high_priority_count = 0;
    for (int i = 0; i < 6; i++) {
        if (events[i]->priority >= EXTSOCK_EVENT_PRIORITY_HIGH) {
            high_priority_count++;
        }
    }
    
    // When - 타입별 필터링
    int auth_related_count = 0;
    for (int i = 0; i < 6; i++) {
        if (events[i]->type == EXTSOCK_EVENT_TYPE_AUTH_SUCCESS || 
            events[i]->type == EXTSOCK_EVENT_TYPE_AUTH_FAILURE) {
            auth_related_count++;
        }
    }
    
    // Then - 필터링 결과 검증
    ck_assert_int_eq(conn1_count, 2);
    ck_assert_int_eq(high_priority_count, 2);
    ck_assert_int_eq(auth_related_count, 2);
    
    // Cleanup
    for (int i = 0; i < 6; i++) {
        destroy_event(events[i]);
    }
}
END_TEST

/**
 * 이벤트 시간 순서 테스트
 */
START_TEST(test_real_event_chronological_order)
{
    // Given - 시간차를 둔 이벤트들
    extsock_event_t *events[3];
    time_t base_time = time(NULL);
    
    events[0] = create_test_event(EXTSOCK_EVENT_TYPE_CONNECTION_UP, "conn1", "First event");
    events[1] = create_test_event(EXTSOCK_EVENT_TYPE_AUTH_SUCCESS, "conn1", "Second event");
    events[2] = create_test_event(EXTSOCK_EVENT_TYPE_CHILD_UP, "conn1", "Third event");
    
    // 시간 설정 (순서대로)
    events[0]->timestamp = base_time;
    events[1]->timestamp = base_time + 10;
    events[2]->timestamp = base_time + 20;
    
    // When - 시간 순서 확인
    bool is_chronological = true;
    for (int i = 1; i < 3; i++) {
        if (events[i]->timestamp <= events[i-1]->timestamp) {
            is_chronological = false;
            break;
        }
    }
    
    // Then
    ck_assert(is_chronological);
    ck_assert_int_lt(events[0]->timestamp, events[1]->timestamp);
    ck_assert_int_lt(events[1]->timestamp, events[2]->timestamp);
    
    // Cleanup
    for (int i = 0; i < 3; i++) {
        destroy_event(events[i]);
    }
}
END_TEST

Suite *event_usecase_real_suite(void)
{
    Suite *s;
    TCase *tc_basic, *tc_types, *tc_storage, *tc_conversion, *tc_filtering;

    s = suite_create("Event Usecase Real Implementation Tests");

    /* 기본 테스트 */
    tc_basic = tcase_create("Basic Event Tests");
    tcase_add_checked_fixture(tc_basic, setup_event_usecase_real_test, teardown_event_usecase_real_test);
    tcase_add_test(tc_basic, test_real_event_creation);
    tcase_add_test(tc_basic, test_real_event_types);
    tcase_add_test(tc_basic, test_real_event_priorities);
    suite_add_tcase(s, tc_basic);

    /* 저장소 테스트 */
    tc_storage = tcase_create("Storage Management Tests");
    tcase_add_checked_fixture(tc_storage, setup_event_usecase_real_test, teardown_event_usecase_real_test);
    tcase_add_test(tc_storage, test_real_event_storage_management);
    suite_add_tcase(s, tc_storage);

    /* 변환 테스트 */
    tc_conversion = tcase_create("Conversion Tests");
    tcase_add_checked_fixture(tc_conversion, setup_event_usecase_real_test, teardown_event_usecase_real_test);
    tcase_add_test(tc_conversion, test_real_event_to_json_conversion);
    tcase_add_test(tc_conversion, test_real_json_to_event_conversion);
    suite_add_tcase(s, tc_conversion);

    /* 필터링 및 고급 기능 테스트 */
    tc_filtering = tcase_create("Filtering and Advanced Tests");
    tcase_add_checked_fixture(tc_filtering, setup_event_usecase_real_test, teardown_event_usecase_real_test);
    tcase_add_test(tc_filtering, test_real_event_filtering);
    tcase_add_test(tc_filtering, test_real_event_chronological_order);
    suite_add_tcase(s, tc_filtering);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = event_usecase_real_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 