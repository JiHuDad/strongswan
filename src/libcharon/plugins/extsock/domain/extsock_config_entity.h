/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_config_entity extsock_config_entity
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_CONFIG_ENTITY_H_
#define EXTSOCK_CONFIG_ENTITY_H_

#include "../common/extsock_types.h"
#include <config/ike_cfg.h>
#include <config/peer_cfg.h>
#include <collections/linked_list.h>

typedef struct extsock_config_entity_t extsock_config_entity_t;

/**
 * IPsec 설정 엔티티
 * 도메인 모델로서 strongSwan 설정의 비즈니스 로직을 캡슐화
 */
struct extsock_config_entity_t {
    
    /**
     * 연결 이름 조회
     *
     * @param this      인스턴스
     * @return          연결 이름
     */
    const char* (*get_name)(extsock_config_entity_t *this);
    
    /**
     * 설정 유효성 검증
     *
     * @param this      인스턴스
     * @return          유효하면 TRUE
     */
    bool (*validate)(extsock_config_entity_t *this);
    
    /**
     * strongSwan peer_cfg_t로 변환
     *
     * @param this      인스턴스
     * @return          peer_cfg_t 객체, 실패 시 NULL
     */
    peer_cfg_t* (*to_peer_cfg)(extsock_config_entity_t *this);
    
    /**
     * 복제본 생성
     *
     * @param this      인스턴스
     * @return          복제된 설정 엔티티
     */
    extsock_config_entity_t* (*clone_)(extsock_config_entity_t *this);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_config_entity_t *this);
};

/**
 * 설정 엔티티 생성
 *
 * @param name          연결 이름
 * @param ike_cfg       IKE 설정
 * @param local_auths   로컬 인증 설정 목록
 * @param remote_auths  원격 인증 설정 목록
 * @return              설정 엔티티 인스턴스
 */
extsock_config_entity_t *extsock_config_entity_create(const char *name,
                                                     ike_cfg_t *ike_cfg,
                                                     linked_list_t *local_auths,
                                                     linked_list_t *remote_auths);

/**
 * JSON으로부터 설정 엔티티 생성
 *
 * @param config_json   JSON 설정 문자열
 * @return              설정 엔티티 인스턴스, 실패 시 NULL
 */
extsock_config_entity_t *extsock_config_entity_create_from_json(const char *config_json);

#endif /** EXTSOCK_CONFIG_ENTITY_H_ @}*/ 