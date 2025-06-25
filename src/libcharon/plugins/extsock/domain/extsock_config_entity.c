/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_config_entity.h"
#include "../common/extsock_common.h"

#include <config/peer_cfg.h>
#include <config/ike_cfg.h>
#include <ctype.h>

typedef struct private_extsock_config_entity_t private_extsock_config_entity_t;

/**
 * 설정 엔티티 내부 구조체
 */
struct private_extsock_config_entity_t {
    
    /**
     * 공개 인터페이스
     */
    extsock_config_entity_t public;
    
    /**
     * 연결 이름
     */
    char *name;
    
    /**
     * 검증 상태
     */
    bool is_valid;
    
    /**
     * 검증 오류 메시지
     */
    char *validation_error;
    
    /**
     * 내부 peer_cfg (캐싱용)
     */
    peer_cfg_t *cached_peer_cfg;
    
    /**
     * IKE 설정
     */
    ike_cfg_t *ike_cfg;
    
    /**
     * 로컬 인증 설정 목록
     */
    linked_list_t *local_auths;
    
    /**
     * 원격 인증 설정 목록
     */
    linked_list_t *remote_auths;
};

METHOD(extsock_config_entity_t, get_name, const char *,
    private_extsock_config_entity_t *this)
{
    return this->name;
}

METHOD(extsock_config_entity_t, validate, bool,
    private_extsock_config_entity_t *this)
{
    // 이미 검증되었다면 결과 반환
    if (this->validation_error || this->is_valid) {
        return this->is_valid;
    }
    
    // 기본 검증 수행
    if (!this->name || strlen(this->name) == 0) {
        this->validation_error = strdup("Connection name is required");
        this->is_valid = FALSE;
        return FALSE;
    }
    
    // 연결 이름 길이 검증
    if (strlen(this->name) > 64) {
        this->validation_error = strdup("Connection name too long (max 64 characters)");
        this->is_valid = FALSE;
        return FALSE;
    }
    
    // 특수 문자 검증
    for (char *p = this->name; *p; p++) {
        if (!isalnum(*p) && *p != '_' && *p != '-') {
            this->validation_error = strdup("Connection name contains invalid characters");
            this->is_valid = FALSE;
            return FALSE;
        }
    }
    
    this->is_valid = TRUE;
    return TRUE;
}

METHOD(extsock_config_entity_t, to_peer_cfg, peer_cfg_t *,
    private_extsock_config_entity_t *this)
{
    // 유효성 검증 먼저 수행
    if (!this->public.validate(&this->public)) {
        EXTSOCK_DBG(1, "Cannot convert invalid config entity to peer_cfg: %s", 
                   this->validation_error ? this->validation_error : "Unknown error");
        return NULL;
    }
    
    // 캐시된 peer_cfg가 있다면 반환
    if (this->cached_peer_cfg) {
        this->cached_peer_cfg->get_ref(this->cached_peer_cfg);
        return this->cached_peer_cfg;
    }
    
    // IKE 설정이 있다면 peer_cfg 생성
    if (this->ike_cfg) {
        peer_cfg_create_t peer_create_cfg = {0};
        peer_cfg_t *peer_cfg = peer_cfg_create(this->name, this->ike_cfg, &peer_create_cfg);
        
        if (peer_cfg) {
            // 인증 설정 추가
            if (this->local_auths) {
                enumerator_t *enumerator = this->local_auths->create_enumerator(this->local_auths);
                auth_cfg_t *auth;
                while (enumerator->enumerate(enumerator, &auth)) {
                    peer_cfg->add_auth_cfg(peer_cfg, auth, TRUE);
                }
                enumerator->destroy(enumerator);
            }
            
            if (this->remote_auths) {
                enumerator_t *enumerator = this->remote_auths->create_enumerator(this->remote_auths);
                auth_cfg_t *auth;
                while (enumerator->enumerate(enumerator, &auth)) {
                    peer_cfg->add_auth_cfg(peer_cfg, auth, FALSE);
                }
                enumerator->destroy(enumerator);
            }
            
            this->cached_peer_cfg = peer_cfg;
            peer_cfg->get_ref(peer_cfg);
            return peer_cfg;
        }
    }
    
    EXTSOCK_DBG(1, "to_peer_cfg: No IKE configuration available");
    return NULL;
}

METHOD(extsock_config_entity_t, clone_, extsock_config_entity_t *,
    private_extsock_config_entity_t *this)
{
    // 간단한 클론 구현 - 이름만 복사
    return extsock_config_entity_create(this->name, this->ike_cfg, this->local_auths, this->remote_auths);
}

METHOD(extsock_config_entity_t, destroy, void,
    private_extsock_config_entity_t *this)
{
    if (this->name) {
        free(this->name);
    }
    if (this->validation_error) {
        free(this->validation_error);
    }
    if (this->cached_peer_cfg) {
        this->cached_peer_cfg->destroy(this->cached_peer_cfg);
    }
    // IKE 설정과 인증 설정들은 외부에서 관리되므로 여기서 해제하지 않음
    free(this);
}

/**
 * 설정 엔티티 생성
 */
extsock_config_entity_t *extsock_config_entity_create(const char *name,
                                                      ike_cfg_t *ike_cfg,
                                                      linked_list_t *local_auths,
                                                      linked_list_t *remote_auths)
{
    private_extsock_config_entity_t *this;

    INIT(this,
        .public = {
            .get_name = _get_name,
            .validate = _validate,
            .to_peer_cfg = _to_peer_cfg,
            .clone = _clone,
            .destroy = _destroy,
        },
        .name = name ? strdup(name) : NULL,
        .is_valid = FALSE,
        .validation_error = NULL,
        .cached_peer_cfg = NULL,
        .ike_cfg = ike_cfg,
        .local_auths = local_auths,
        .remote_auths = remote_auths,
    );

    return &this->public;
} 