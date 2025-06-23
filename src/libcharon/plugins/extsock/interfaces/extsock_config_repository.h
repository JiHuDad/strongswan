/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_config_repository extsock_config_repository
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_CONFIG_REPOSITORY_H_
#define EXTSOCK_CONFIG_REPOSITORY_H_

#include "../common/extsock_types.h"
#include "../domain/extsock_config_entity.h"

typedef struct extsock_config_repository_t extsock_config_repository_t;

/**
 * 설정 저장소 인터페이스
 * 설정 적용, 제거, DPD 시작 등의 기능을 제공
 */
struct extsock_config_repository_t {
    
    /**
     * 설정 적용
     *
     * @param this      인스턴스
     * @param config    설정 엔티티
     * @return          성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*apply_config)(extsock_config_repository_t *this,
                                   extsock_config_entity_t *config);
    
    /**
     * 설정 제거
     *
     * @param this      인스턴스
     * @param name      설정 이름
     * @return          성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*remove_config)(extsock_config_repository_t *this,
                                    const char *name);
    
    /**
     * DPD 시작
     *
     * @param this          인스턴스
     * @param ike_sa_name   IKE SA 이름
     * @return              성공 시 EXTSOCK_SUCCESS
     */
    extsock_error_t (*start_dpd)(extsock_config_repository_t *this,
                                const char *ike_sa_name);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_config_repository_t *this);
};

#endif /** EXTSOCK_CONFIG_REPOSITORY_H_ @}*/ 