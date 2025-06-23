/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_event_publisher extsock_event_publisher
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_EVENT_PUBLISHER_H_
#define EXTSOCK_EVENT_PUBLISHER_H_

#include "../common/extsock_types.h"

typedef struct extsock_event_publisher_t extsock_event_publisher_t;

/**
 * 이벤트 발행자 인터페이스
 * 외부 시스템으로 이벤트를 전송하는 기능을 추상화
 */
struct extsock_event_publisher_t {
    
    /**
     * 이벤트 발행
     *
     * @param this          인스턴스
     * @param event_json    JSON 형태의 이벤트 데이터
     * @return              성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*publish_event)(extsock_event_publisher_t *this,
                                    const char *event_json);
    
    /**
     * 터널 관련 이벤트 발행
     *
     * @param this              인스턴스
     * @param tunnel_event_json 터널 이벤트 JSON
     * @return                  성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*publish_tunnel_event)(extsock_event_publisher_t *this,
                                           const char *tunnel_event_json);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_event_publisher_t *this);
};

#endif /** EXTSOCK_EVENT_PUBLISHER_H_ @}*/ 