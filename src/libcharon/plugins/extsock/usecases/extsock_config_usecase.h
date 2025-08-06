/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_config_usecase extsock_config_usecase
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_CONFIG_USECASE_H_
#define EXTSOCK_CONFIG_USECASE_H_

#include "../common/extsock_types.h"
#include "../interfaces/extsock_command_handler.h"
#include "../interfaces/extsock_event_publisher.h"
#include <config/peer_cfg.h>

typedef struct extsock_config_usecase_t extsock_config_usecase_t;
typedef struct extsock_json_parser_t extsock_json_parser_t;

/**
 * 설정 관리 유스케이스
 * IPsec 설정 적용 및 명령 처리 비즈니스 로직을 캡슐화
 */
struct extsock_config_usecase_t {
    
    /**
     * JSON 설정을 적용
     *
     * @param this          인스턴스
     * @param config_json   JSON 설정 문자열
     * @return              성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*apply_json_config)(extsock_config_usecase_t *this,
                                        const char *config_json);
    
    /**
     * 설정 제거
     *
     * @param this      인스턴스
     * @param name      설정 이름
     * @return          성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*remove_config)(extsock_config_usecase_t *this,
                                    const char *name);
    
    /**
     * DPD 시작
     *
     * @param this          인스턴스
     * @param ike_sa_name   IKE SA 이름
     * @return              성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*start_dpd)(extsock_config_usecase_t *this,
                                const char *ike_sa_name);
    
    /**
     * Peer 설정 추가 및 즉시 연결 시도 (Failover용)
     *
     * @param this      인스턴스
     * @param peer_cfg  추가할 peer_cfg (소유권 이전됨)
     * @return          성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*add_peer_config_and_initiate)(extsock_config_usecase_t *this, 
                                                    peer_cfg_t *peer_cfg);
    
    /**
     * 명령 처리기 인터페이스 조회
     *
     * @param this      인스턴스
     * @return          명령 처리기 인터페이스
     */
    extsock_command_handler_t* (*get_command_handler)(extsock_config_usecase_t *this);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_config_usecase_t *this);
};

/**
 * 설정 관리 유스케이스 생성
 *
 * @param json_parser       JSON 파싱 어댑터
 * @param event_usecase     이벤트 처리 유스케이스
 * @return                  설정 관리 유스케이스 인스턴스
 */
extsock_config_usecase_t *extsock_config_usecase_create(
    extsock_json_parser_t *json_parser,
    extsock_event_usecase_t *event_usecase);

#endif /** EXTSOCK_CONFIG_USECASE_H_ @}*/ 