/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_strongswan_adapter extsock_strongswan_adapter
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_STRONGSWAN_ADAPTER_H_
#define EXTSOCK_STRONGSWAN_ADAPTER_H_

#include "../../common/extsock_types.h"
#include "../../interfaces/extsock_config_repository.h"
#include <config/peer_cfg.h>
#include <credentials/sets/mem_cred.h>
#include <collections/linked_list.h>
#include <threading/mutex.h>

/**
 * strongSwan API 어댑터
 * strongSwan의 내부 API들을 캡슐화하여 외부 의존성을 격리
 */
struct extsock_strongswan_adapter_t {
    
    /**
     * 설정 저장소 인터페이스 구현
     */
    extsock_config_repository_t config_repository;
    
    /**
     * 피어 설정을 strongSwan에 추가
     *
     * @param this      인스턴스
     * @param peer_cfg  피어 설정
     * @return          성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*add_peer_config)(extsock_strongswan_adapter_t *this,
                                      peer_cfg_t *peer_cfg);
    
    /**
     * 피어 설정을 strongSwan에서 제거
     *
     * @param this      인스턴스
     * @param name      설정 이름
     * @return          성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*remove_peer_config)(extsock_strongswan_adapter_t *this,
                                         const char *name);
    
    /**
     * Child SA 개시
     *
     * @param this          인스턴스
     * @param peer_cfg      피어 설정
     * @param child_cfg     Child SA 설정
     * @return              성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*initiate_child_sa)(extsock_strongswan_adapter_t *this,
                                        peer_cfg_t *peer_cfg,
                                        child_cfg_t *child_cfg);
    
    /**
     * 관리되는 피어 설정 목록 조회
     *
     * @param this      인스턴스
     * @return          피어 설정 목록
     */
    linked_list_t* (*get_managed_configs)(extsock_strongswan_adapter_t *this);
    
    /**
     * 인메모리 자격증명 세트 조회
     *
     * @param this      인스턴스
     * @return          자격증명 세트
     */
    mem_cred_t* (*get_credentials)(extsock_strongswan_adapter_t *this);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_strongswan_adapter_t *this);
};

/**
 * strongSwan 어댑터 생성
 *
 * @return  strongSwan 어댑터 인스턴스
 */
extsock_strongswan_adapter_t *extsock_strongswan_adapter_create();

#endif /** EXTSOCK_STRONGSWAN_ADAPTER_H_ @}*/ 