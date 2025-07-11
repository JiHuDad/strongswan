/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_event_usecase.h"
#include "../common/extsock_common.h"
#include "../adapters/socket/extsock_socket_adapter.h"

#include <daemon.h>
#include <sa/child_sa.h>
#include <selectors/traffic_selector.h>
#include <networking/host.h>
#include <crypto/proposal/proposal.h>
#include <collections/enumerator.h>
#include <cjson/cJSON.h>
#include <stddef.h>  /* offsetof를 위해 추가 */

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

    // 통합된 터널 이벤트 생성 및 전송 (기본 상태 정보 + 터널 상세 정보)
    cJSON *tunnel_json = cJSON_CreateObject();
    if (!tunnel_json) {
        EXTSOCK_DBG(1, "Failed to create tunnel event JSON object");
        return;
    }

    // 기본 터널 이벤트 정보
    cJSON_AddStringToObject(tunnel_json, "event", up ? "tunnel_up" : "tunnel_down");
    cJSON_AddStringToObject(tunnel_json, "ike_sa_name", ike_name);
    cJSON_AddStringToObject(tunnel_json, "child_sa_name", child_name);

    // 기본 상태 정보도 포함 (기존 child_sa 이벤트 정보)
    char ike_state[32], child_state[32];
    EXTSOCK_SAFE_SNPRINTF(ike_state, sizeof(ike_state), "%d", ike_sa->get_state(ike_sa));
    EXTSOCK_SAFE_SNPRINTF(child_state, sizeof(child_state), "%d", child_sa->get_state(child_sa));
    cJSON_AddStringToObject(tunnel_json, "ike_sa_state", ike_state);
    cJSON_AddStringToObject(tunnel_json, "child_sa_state", child_state);

    // Child SA 상세 정보 수집
    uint32_t spi = 0;
    char proto_str[16] = "esp";
    char mode_str[16] = "tunnel";
    char enc_alg[32] = "unknown";
    char integ_alg[32] = "unknown";
    char src_str[64] = "unknown";
    char dst_str[64] = "unknown";
    char local_ts[128] = "unknown";
    char remote_ts[128] = "unknown";

    // SPI 정보 추출
    if (child_sa) {
        spi = child_sa->get_spi(child_sa, TRUE); // TRUE = inbound SPI
        
        // 프로토콜 정보
        protocol_id_t protocol = child_sa->get_protocol(child_sa);
        switch (protocol) {
            case PROTO_ESP:
                strncpy(proto_str, "esp", sizeof(proto_str)-1);
                break;
            case PROTO_AH:
                strncpy(proto_str, "ah", sizeof(proto_str)-1);
                break;
            default:
                strncpy(proto_str, "unknown", sizeof(proto_str)-1);
                break;
        }
        
        // 모드 정보
        ipsec_mode_t mode = child_sa->get_mode(child_sa);
        switch (mode) {
            case MODE_TUNNEL:
                strncpy(mode_str, "tunnel", sizeof(mode_str)-1);
                break;
            case MODE_TRANSPORT:
                strncpy(mode_str, "transport", sizeof(mode_str)-1);
                break;
            default:
                strncpy(mode_str, "unknown", sizeof(mode_str)-1);
                break;
        }
    }

    // IKE SA 호스트 정보
    if (ike_sa) {
        host_t *src = ike_sa->get_my_host(ike_sa);
        host_t *dst = ike_sa->get_other_host(ike_sa);
        if (src) {
            snprintf(src_str, sizeof(src_str), "%H", src);
        }
        if (dst) {
            snprintf(dst_str, sizeof(dst_str), "%H", dst);
        }
    }

    // Traffic Selector 정보 추출
    if (child_sa) {
        enumerator_t *ts_enum = child_sa->create_policy_enumerator(child_sa);
        traffic_selector_t *local_traffic_sel, *remote_traffic_sel;
        
        if (ts_enum && ts_enum->enumerate(ts_enum, &local_traffic_sel, &remote_traffic_sel)) {
            // Traffic Selector를 문자열로 변환
            if (local_traffic_sel) {
                snprintf(local_ts, sizeof(local_ts), "%R", local_traffic_sel);
            }
            if (remote_traffic_sel) {
                snprintf(remote_ts, sizeof(remote_ts), "%R", remote_traffic_sel);
            }
        }
        
        if (ts_enum) {
            ts_enum->destroy(ts_enum);
        }
    }

    // JSON에 터널 상세 정보 추가
    cJSON_AddNumberToObject(tunnel_json, "spi", spi);
    cJSON_AddStringToObject(tunnel_json, "proto", proto_str);
    cJSON_AddStringToObject(tunnel_json, "mode", mode_str);
    cJSON_AddStringToObject(tunnel_json, "enc_alg", enc_alg);
    cJSON_AddStringToObject(tunnel_json, "integ_alg", integ_alg);
    cJSON_AddStringToObject(tunnel_json, "src", src_str);
    cJSON_AddStringToObject(tunnel_json, "dst", dst_str);
    cJSON_AddStringToObject(tunnel_json, "local_ts", local_ts);
    cJSON_AddStringToObject(tunnel_json, "remote_ts", remote_ts);
    cJSON_AddStringToObject(tunnel_json, "direction", "out");
    cJSON_AddStringToObject(tunnel_json, "policy_action", "protect");

    // 통합된 터널 이벤트 전송 (publish_tunnel_event 사용)
    char *tunnel_event_string = cJSON_Print(tunnel_json);
    if (tunnel_event_string) {
        extsock_event_publisher_t *publisher = this->public.get_event_publisher(&this->public);
        if (publisher) {
            // 터널 이벤트만 전송 (중복 제거)
            publisher->publish_tunnel_event(&this->event_publisher, tunnel_event_string);
        }
        free(tunnel_event_string);
    }
    
    cJSON_Delete(tunnel_json);
}

/**
 * 이벤트 발행 (기존 send_event_to_external 함수 연동)
 */
METHOD(extsock_event_publisher_t, publish_event, extsock_error_t,
    extsock_event_publisher_t *publisher, const char *event_json)
{
    private_extsock_event_usecase_t *this;
    
    // HIGH PRIORITY: NULL 포인터 체크 강화
    EXTSOCK_CHECK_NULL_RET(publisher, EXTSOCK_ERROR_CONFIG_INVALID);
    EXTSOCK_CHECK_NULL_RET(event_json, EXTSOCK_ERROR_CONFIG_INVALID);
    
    /* Container-of 패턴으로 전체 구조체 포인터 계산 */
    this = (private_extsock_event_usecase_t*)
        ((char*)publisher - offsetof(private_extsock_event_usecase_t, event_publisher));
    
    EXTSOCK_DBG(2, "Publishing event: %s", event_json);
    
    if (this->socket_adapter) {
        return this->socket_adapter->send_event(this->socket_adapter, event_json);
    }
    
    return EXTSOCK_ERROR_STRONGSWAN_API;
}

METHOD(extsock_event_publisher_t, publish_tunnel_event, extsock_error_t,
    extsock_event_publisher_t *publisher, const char *tunnel_event_json)
{
    EXTSOCK_CHECK_NULL_RET(publisher, EXTSOCK_ERROR_CONFIG_INVALID);
    return publisher->publish_event(publisher, tunnel_event_json);
}

METHOD(extsock_event_publisher_t, destroy_publisher, void,
    extsock_event_publisher_t *publisher)
{
    // NULL 체크 추가 (안전성 강화)
    if (!publisher) return;
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
        // 🟠 LOW-MEDIUM PRIORITY: 안전한 문자열 포맷팅
        EXTSOCK_SAFE_SNPRINTF(state_str, sizeof(state_str), "%d", ike_sa->get_state(ike_sa));
        EXTSOCK_SAFE_SNPRINTF(event_json, sizeof(event_json),
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
    
    // 1. 기본 Child SA rekey 이벤트 생성 및 전송
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

    // 2. 새로운 Child SA에 대한 터널 이벤트 생성 (tunnel_up)
    // rekey 후 새로운 터널이 활성화되었음을 알림
    this->public.handle_child_updown(&this->public, ike_sa, new, TRUE);
    
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