/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_event_usecase extsock_event_usecase
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_EVENT_USECASE_H_
#define EXTSOCK_EVENT_USECASE_H_

#include "../common/extsock_types.h"
#include "../interfaces/extsock_event_publisher.h"
#include "../interfaces/extsock_failover_manager.h"

#include <bus/bus.h>
#include <sa/ike_sa.h>

typedef struct extsock_event_usecase_t extsock_event_usecase_t;

/**
 * 이벤트 처리 유스케이스
 * IKE SA 상태 변화 및 Child SA up/down 이벤트 처리 비즈니스 로직을 캡슐화
 */
struct extsock_event_usecase_t {
    
    /**
     * 버스 리스너 인터페이스
     */
    listener_t listener;
    
    /**
     * Child SA Up/Down 처리
     *
     * @param this      인스턴스  
     * @param ike_sa    IKE SA
     * @param child_sa  Child SA
     * @param up        UP 상태 여부
     */
    void (*handle_child_updown)(extsock_event_usecase_t *this,
                               ike_sa_t *ike_sa, child_sa_t *child_sa, bool up);
    
    /**
     * 이벤트 발행자 인터페이스 조회
     *
     * @param this      인스턴스
     * @return          이벤트 발행자 인터페이스
     */
    extsock_event_publisher_t* (*get_event_publisher)(extsock_event_usecase_t *this);
    
    /**
     * 소켓 어댑터 설정 (의존성 주입용)
     *
     * @param this              인스턴스
     * @param socket_adapter    소켓 어댑터
     */
    void (*set_socket_adapter)(extsock_event_usecase_t *this,
                              struct extsock_socket_adapter_t *socket_adapter);
    
    /**
     * Failover Manager 설정 (의존성 주입용)
     *
     * @param this              인스턴스
     * @param failover_manager  Failover Manager
     */
    void (*set_failover_manager)(extsock_event_usecase_t *this,
                                extsock_failover_manager_t *failover_manager);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_event_usecase_t *this);
};

/**
 * 이벤트 처리 유스케이스 생성
 *
 * @return  이벤트 처리 유스케이스 인스턴스
 */
extsock_event_usecase_t *extsock_event_usecase_create();

#endif /** EXTSOCK_EVENT_USECASE_H_ @}*/ 