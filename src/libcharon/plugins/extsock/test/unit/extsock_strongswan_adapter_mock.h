/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Mock version of extsock_strongswan_adapter for adapter unit tests
 * TASK-009: strongSwan Adapter 실제 테스트
 * 
 * This is a simplified mock implementation that doesn't require strongSwan
 * but provides the same interface for testing adapter layer functionality.
 */

#ifndef EXTSOCK_STRONGSWAN_ADAPTER_MOCK_H_
#define EXTSOCK_STRONGSWAN_ADAPTER_MOCK_H_

#include <stdbool.h>
#include <stddef.h>

// Mock definitions without strongSwan dependencies
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_CONFIG_INVALID = -1,
    EXTSOCK_ERROR_STRONGSWAN_API = -2
} extsock_error_t;

// Forward declarations - use infrastructure mocks
typedef struct mock_peer_cfg_t peer_cfg_t;
typedef struct mock_child_cfg_t child_cfg_t;
typedef struct mock_linked_list_t linked_list_t;
typedef struct mem_cred_t mem_cred_t;
typedef struct extsock_config_entity_t extsock_config_entity_t;

// Mock config repository interface
typedef struct extsock_config_repository_t {
    extsock_error_t (*apply_config)(struct extsock_config_repository_t *this, 
                                   extsock_config_entity_t *config);
    extsock_error_t (*remove_config)(struct extsock_config_repository_t *this, 
                                    const char *name);
    extsock_error_t (*start_dpd)(struct extsock_config_repository_t *this, 
                                const char *ike_sa_name);
    void (*destroy)(struct extsock_config_repository_t *this);
} extsock_config_repository_t;

/**
 * strongSwan API 어댑터 (Mock version)
 * strongSwan의 내부 API들을 캡슐화하여 외부 의존성을 격리
 */
typedef struct extsock_strongswan_adapter_t {
    
    /**
     * 설정 저장소 인터페이스 구현
     */
    extsock_config_repository_t config_repository;
    
    /**
     * 피어 설정을 strongSwan에 추가
     */
    extsock_error_t (*add_peer_config)(struct extsock_strongswan_adapter_t *this,
                                      peer_cfg_t *peer_cfg);
    
    /**
     * 피어 설정을 strongSwan에서 제거
     */
    extsock_error_t (*remove_peer_config)(struct extsock_strongswan_adapter_t *this,
                                         const char *name);
    
    /**
     * Child SA 개시
     */
    extsock_error_t (*initiate_child_sa)(struct extsock_strongswan_adapter_t *this,
                                        peer_cfg_t *peer_cfg,
                                        child_cfg_t *child_cfg);
    
    /**
     * 관리되는 피어 설정 목록 조회
     */
    linked_list_t* (*get_managed_configs)(struct extsock_strongswan_adapter_t *this);
    
    /**
     * 인메모리 자격증명 세트 조회
     */
    mem_cred_t* (*get_credentials)(struct extsock_strongswan_adapter_t *this);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(struct extsock_strongswan_adapter_t *this);
} extsock_strongswan_adapter_t;

// Mock mem_cred structure
struct mem_cred_t {
    int initialized;
    void (*destroy)(mem_cred_t *this);
};

// Mock config entity structure
struct extsock_config_entity_t {
    char *config_data;
    peer_cfg_t* (*to_peer_cfg)(extsock_config_entity_t *this);
    void (*destroy)(extsock_config_entity_t *this);
};

/**
 * Mock creation functions (additional ones, reusing infrastructure)
 */
mem_cred_t *mock_mem_cred_create();
extsock_config_entity_t *mock_config_entity_create(const char *config_data);

/**
 * strongSwan 어댑터 생성 (Mock version)
 */
extsock_strongswan_adapter_t *extsock_strongswan_adapter_create();

/**
 * Mock verification functions for testing
 */
typedef struct mock_strongswan_state_t {
    int add_peer_config_calls;
    int remove_peer_config_calls;
    int initiate_child_sa_calls;
    int apply_config_calls;
    int remove_config_calls;
    int start_dpd_calls;
    char last_peer_name[256];
    char last_removed_name[256];
    char last_ike_sa_name[256];
    bool simulate_failure;
    extsock_error_t failure_error;
} mock_strongswan_state_t;

/**
 * Mock state management
 */
void mock_strongswan_reset_state();
mock_strongswan_state_t* mock_strongswan_get_state();
void mock_strongswan_simulate_failure(bool enable, extsock_error_t error);

#endif /** EXTSOCK_STRONGSWAN_ADAPTER_MOCK_H_ @}*/