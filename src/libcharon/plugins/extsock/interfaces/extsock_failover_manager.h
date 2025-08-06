/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Failover Manager Interface for extsock plugin
 * Handles automatic failover to secondary SEGW when primary fails
 */

#ifndef EXTSOCK_FAILOVER_MANAGER_H_
#define EXTSOCK_FAILOVER_MANAGER_H_

#include "../common/extsock_common.h"
#include <sa/ike_sa.h>
#include <config/peer_cfg.h>

typedef struct extsock_failover_manager_t extsock_failover_manager_t;

/**
 * Failover Manager 인터페이스
 * 
 * IKE SA 연결 실패 시 다음 SEGW로의 자동 전환을 담당
 */
struct extsock_failover_manager_t {
    
    /**
     * IKE SA 연결 실패 처리 (메인 진입점)
     * 
     * @param this      Failover Manager 인스턴스
     * @param ike_sa    실패한 IKE SA
     */
    void (*handle_connection_failure)(extsock_failover_manager_t *this, ike_sa_t *ike_sa);
    
    /**
     * 다음 SEGW 주소 선택
     * 
     * @param this          Failover Manager 인스턴스  
     * @param remote_addrs  쉼표로 구분된 원격 주소 목록
     * @param current_addr  현재 사용 중인 주소
     * @return              다음 주소 (caller가 free 해야 함), NULL if 없음
     */
    char* (*select_next_segw)(extsock_failover_manager_t *this, 
                              const char *remote_addrs, 
                              const char *current_addr);
    
    /**
     * Failover 설정 생성 및 연결 시도
     * 
     * @param this          Failover Manager 인스턴스
     * @param original_cfg  원본 peer_cfg
     * @param next_segw_addr 다음 SEGW 주소
     * @return              EXTSOCK_SUCCESS if 성공
     */
    extsock_error_t (*create_failover_config)(extsock_failover_manager_t *this,
                                              peer_cfg_t *original_cfg,
                                              const char *next_segw_addr);
    
    /**
     * 최대 재시도 횟수 초과 여부 확인
     * 
     * @param this      Failover Manager 인스턴스
     * @param conn_name 연결 이름
     * @return          TRUE if 초과, FALSE otherwise
     */
    bool (*is_max_retry_exceeded)(extsock_failover_manager_t *this, const char *conn_name);
    
    /**
     * 재시도 횟수 초기화 (연결 성공 시 호출)
     * 
     * @param this      Failover Manager 인스턴스
     * @param conn_name 연결 이름
     */
    void (*reset_retry_count)(extsock_failover_manager_t *this, const char *conn_name);
    
    /**
     * 소멸자
     * 
     * @param this  Failover Manager 인스턴스
     */
    void (*destroy)(extsock_failover_manager_t *this);
};

/**
 * Failover Manager 생성자
 * 
 * @param config_usecase  Config Usecase 인스턴스 (DI)
 * @return                Failover Manager 인스턴스
 */
extsock_failover_manager_t *extsock_failover_manager_create(extsock_config_usecase_t *config_usecase);

#endif /** EXTSOCK_FAILOVER_MANAGER_H_ @}*/ 