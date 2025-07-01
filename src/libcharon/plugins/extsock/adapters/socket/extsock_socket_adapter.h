/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_socket_adapter extsock_socket_adapter
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_SOCKET_ADAPTER_H_
#define EXTSOCK_SOCKET_ADAPTER_H_

#include "../../common/extsock_types.h"
#include "../../interfaces/extsock_event_publisher.h"
#include "../../interfaces/extsock_command_handler.h"

#include <threading/thread.h>

/**
 * 소켓 통신 어댑터
 * 외부 애플리케이션과의 소켓 통신을 담당
 */
struct extsock_socket_adapter_t {
    
    /**
     * 이벤트 발행자 인터페이스 구현
     */
    extsock_event_publisher_t event_publisher;
    
    /**
     * 이벤트 전송
     *
     * @param this          인스턴스
     * @param event_json    전송할 이벤트 JSON
     * @return              성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*send_event)(extsock_socket_adapter_t *this,
                                 const char *event_json);
    
    /**
     * 소켓 리스너 시작
     *
     * @param this      인스턴스
     * @return          소켓 스레드 인스턴스
     */
    thread_t* (*start_listening)(extsock_socket_adapter_t *this);
    
    /**
     * 소켓 리스너 중지
     *
     * @param this      인스턴스
     */
    void (*stop_listening)(extsock_socket_adapter_t *this);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_socket_adapter_t *this);
};

/**
 * 소켓 어댑터 생성
 *
 * @param cfg_usecase       설정 유스케이스
 * @return                  소켓 어댑터 인스턴스
 */
extsock_socket_adapter_t *extsock_socket_adapter_create(
    extsock_config_usecase_t *cfg_usecase);

#endif /** EXTSOCK_SOCKET_ADAPTER_H_ @}*/ 