/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_command_handler extsock_command_handler
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_COMMAND_HANDLER_H_
#define EXTSOCK_COMMAND_HANDLER_H_

#include "../common/extsock_types.h"

typedef struct extsock_command_handler_t extsock_command_handler_t;

/**
 * 명령 처리 인터페이스
 * 외부에서 받은 명령을 처리하는 기능을 추상화
 */
struct extsock_command_handler_t {
    
    /**
     * 일반 명령 처리
     *
     * @param this      인스턴스
     * @param command   명령 문자열
     * @return          성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*handle_command)(extsock_command_handler_t *this,
                                     const char *command);
    
    /**
     * 설정 적용 명령 처리
     *
     * @param this          인스턴스
     * @param config_json   JSON 설정 문자열
     * @return              성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*handle_config_command)(extsock_command_handler_t *this,
                                            const char *config_json);
    
    /**
     * DPD 시작 명령 처리
     *
     * @param this          인스턴스
     * @param ike_sa_name   IKE SA 이름
     * @return              성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*handle_dpd_command)(extsock_command_handler_t *this,
                                         const char *ike_sa_name);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_command_handler_t *this);
};

#endif /** EXTSOCK_COMMAND_HANDLER_H_ @}*/ 