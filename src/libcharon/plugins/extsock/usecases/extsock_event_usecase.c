/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_event_usecase.h"
#include "../common/extsock_common.h"
#include "../adapters/socket/extsock_socket_adapter.h"

#include <daemon.h>
#include <sa/child_sa.h>
#include <cjson/cJSON.h>

typedef struct private_extsock_event_usecase_t private_extsock_event_usecase_t;

/**
 * 이벤트 처리 유스케이스 내부 구조체
 */
struct private_extsock_event_usecase_t {
    
    /**
     * 공개 인터페이스
     */
    extsock_event_usecase_t public;
    
    /**
     * 이벤트 발행자 인터페이스 구현
     */
    extsock_event_publisher_t event_publisher;
    
    /**
     * 소켓 어댑터
     */
    extsock_socket_adapter_t *socket_adapter;
};

/**
 * Child SA Up/Down 이벤트 처리 (기존 extsock_child_updown 함수에서 이동)
 */
METHOD(extsock_event_usecase_t, handle_child_updown, void,
    private_extsock_event_usecase_t *this, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up)
{
    if (!ike_sa || !child_sa) {
        return;
    }

    const char *ike_name = ike_sa->get_name(ike_sa);
    const char *child_name = child_sa->get_name(child_sa);
    
    EXTSOCK_DBG(1, "Child SA '%s' of IKE SA '%s' is %s",
               child_name, ike_name, up ? "UP" : "DOWN");

    // JSON 이벤트 생성 및 전송
    cJSON *event_json = cJSON_CreateObject();
    if (!event_json) {
        EXTSOCK_DBG(1, "Failed to create event JSON object");
        return;
    }

    cJSON_AddStringToObject(event_json, "event", up ? "child_sa_up" : "child_sa_down");
    cJSON_AddStringToObject(event_json, "ike_sa_name", ike_name);
    cJSON_AddStringToObject(event_json, "child_sa_name", child_name);
    
    // 상태 이름을 문자열로 변환
    char ike_state[32], child_state[32];
    snprintf(ike_state, sizeof(ike_state), "%d", ike_sa->get_state(ike_sa));
    snprintf(child_state, sizeof(child_state), "%d", child_sa->get_state(child_sa));
    cJSON_AddStringToObject(event_json, "ike_sa_state", ike_state);
    cJSON_AddStringToObject(event_json, "child_sa_state", child_state);

    char *event_string = cJSON_Print(event_json);
    if (event_string) {
        extsock_event_publisher_t *publisher = this->public.get_event_publisher(&this->public);
        if (publisher) {
            publisher->publish_event(&this->event_publisher, event_string);
        }
        free(event_string);
    }
    
    cJSON_Delete(event_json);
}

/**
 * 이벤트 발행 (기존 send_event_to_external 함수 연동)
 */
METHOD(extsock_event_publisher_t, publish_event, extsock_error_t,
    private_extsock_event_usecase_t *this, const char *event_json)
{
    if (!event_json) {
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    EXTSOCK_DBG(2, "Publishing event: %s", event_json);
    
    if (this->socket_adapter) {
        return this->socket_adapter->send_event(this->socket_adapter, event_json);
    }
    
    return EXTSOCK_ERROR_STRONGSWAN_API;
}

METHOD(extsock_event_publisher_t, publish_tunnel_event, extsock_error_t,
    private_extsock_event_usecase_t *this, const char *tunnel_event_json)
{
    return this->event_publisher.publish_event(&this->event_publisher, tunnel_event_json);
}

METHOD(extsock_event_publisher_t, destroy_publisher, void,
    private_extsock_event_usecase_t *this)
{
    // Publisher는 event_usecase의 일부이므로 별도 정리 불필요
}

/**
 * 버스 리스너 이벤트 처리
 */
METHOD(listener_t, ike_updown, bool,
    private_extsock_event_usecase_t *this, ike_sa_t *ike_sa, bool up)
{
    // IKE SA 상태 변화 이벤트 처리
    if (ike_sa) {
        const char *ike_name = ike_sa->get_name(ike_sa);
        EXTSOCK_DBG(1, "IKE SA '%s' is %s", ike_name, up ? "UP" : "DOWN");
        
        char event_json[256];
        char state_str[32];
        snprintf(state_str, sizeof(state_str), "%d", ike_sa->get_state(ike_sa));
        snprintf(event_json, sizeof(event_json),
                "{\"event\":\"ike_sa_%s\",\"ike_sa_name\":\"%s\",\"state\":\"%s\"}",
                up ? "up" : "down", ike_name, state_str);
        
        extsock_event_publisher_t *publisher = this->public.get_event_publisher(&this->public);
        if (publisher) {
            publisher->publish_event(&this->event_publisher, event_json);
        }
    }
    
    return TRUE;
}

METHOD(listener_t, child_updown, bool,
    private_extsock_event_usecase_t *this, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up)
{
    this->public.handle_child_updown(&this->public, ike_sa, child_sa, up);
    return TRUE;
}

/**
 * IKE SA rekey 이벤트 처리
 */
METHOD(listener_t, ike_rekey, bool,
    private_extsock_event_usecase_t *this, ike_sa_t *old, ike_sa_t *new)
{
    if (!old || !new) {
        return TRUE;
    }
    
    const char *old_name = old->get_name(old);
    const char *new_name = new->get_name(new);
    EXTSOCK_DBG(1, "IKE SA rekey event: %s -> %s", old_name, new_name);
    
    // JSON 이벤트 생성 및 전송
    cJSON *event_json = cJSON_CreateObject();
    if (!event_json) {
        EXTSOCK_DBG(1, "Failed to create IKE rekey event JSON object");
        return TRUE;
    }
    
    cJSON_AddStringToObject(event_json, "event", "ike_rekey");
    cJSON_AddStringToObject(event_json, "old_ike_sa_name", old_name);
    cJSON_AddStringToObject(event_json, "new_ike_sa_name", new_name);
    
    char *event_string = cJSON_Print(event_json);
    if (event_string) {
        extsock_event_publisher_t *publisher = this->public.get_event_publisher(&this->public);
        if (publisher) {
            publisher->publish_event(&this->event_publisher, event_string);
        }
        free(event_string);
    }
    
    cJSON_Delete(event_json);
    return TRUE;
}

/**
 * CHILD SA rekey 이벤트 처리
 */
METHOD(listener_t, child_rekey, bool,
    private_extsock_event_usecase_t *this, ike_sa_t *ike_sa, child_sa_t *old, child_sa_t *new)
{
    if (!ike_sa || !old || !new) {
        return TRUE;
    }
    
    const char *ike_name = ike_sa->get_name(ike_sa);
    const char *old_child_name = old->get_name(old);
    const char *new_child_name = new->get_name(new);
    EXTSOCK_DBG(1, "CHILD SA rekey event: %s/%s -> %s/%s", 
               ike_name, old_child_name, ike_name, new_child_name);
    
    // JSON 이벤트 생성 및 전송
    cJSON *event_json = cJSON_CreateObject();
    if (!event_json) {
        EXTSOCK_DBG(1, "Failed to create CHILD rekey event JSON object");
        return TRUE;
    }
    
    cJSON_AddStringToObject(event_json, "event", "child_rekey");
    cJSON_AddStringToObject(event_json, "ike_sa_name", ike_name);
    cJSON_AddStringToObject(event_json, "old_child_sa_name", old_child_name);
    cJSON_AddStringToObject(event_json, "new_child_sa_name", new_child_name);
    
    char *event_string = cJSON_Print(event_json);
    if (event_string) {
        extsock_event_publisher_t *publisher = this->public.get_event_publisher(&this->public);
        if (publisher) {
            publisher->publish_event(&this->event_publisher, event_string);
        }
        free(event_string);
    }
    
    cJSON_Delete(event_json);
    return TRUE;
}

METHOD(extsock_event_usecase_t, get_event_publisher, extsock_event_publisher_t *,
    private_extsock_event_usecase_t *this)
{
    return &this->event_publisher;
}

METHOD(extsock_event_usecase_t, set_socket_adapter, void,
    private_extsock_event_usecase_t *this, extsock_socket_adapter_t *socket_adapter)
{
    this->socket_adapter = socket_adapter;
}

METHOD(extsock_event_usecase_t, destroy, void,
    private_extsock_event_usecase_t *this)
{
    // 버스 리스너 제거
    charon->bus->remove_listener(charon->bus, &this->public.listener);
    free(this);
}

/**
 * 이벤트 처리 유스케이스 생성
 */
extsock_event_usecase_t *extsock_event_usecase_create()
{
    private_extsock_event_usecase_t *this;

    INIT(this,
        .public = {
            .listener = {
                .ike_updown = _ike_updown,
                .child_updown = _child_updown,
                .ike_rekey = _ike_rekey,
                .child_rekey = _child_rekey,
            },
            .handle_child_updown = _handle_child_updown,
            .get_event_publisher = _get_event_publisher,
            .set_socket_adapter = _set_socket_adapter,
            .destroy = _destroy,
        },
        .event_publisher = {
            .publish_event = _publish_event,
            .publish_tunnel_event = _publish_tunnel_event,
            .destroy = _destroy_publisher,
        },
        .socket_adapter = NULL, // 의존성 주입으로 설정됨
    );

    // 버스 리스너 등록
    charon->bus->add_listener(charon->bus, &this->public.listener);

    return &this->public;
} 