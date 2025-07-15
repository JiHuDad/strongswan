/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_config_entity.h"
#include "../common/extsock_common.h"

#include <config/peer_cfg.h>
#include <config/ike_cfg.h>
#include <config/child_cfg.h>
#include <credentials/auth_cfg.h>
#include <utils/identification.h>
#include <ctype.h>
#include <time.h>

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
    
    /**
     * 자식 설정 목록
     */
    linked_list_t *child_cfgs;
    
    /**
     * JSON 원본 데이터 (디버깅용)
     */
    char *original_json;
};

/**
 * 검증 오류 메시지 설정 (안전한 메모리 관리)
 */
static void set_validation_error(private_extsock_config_entity_t *this, const char *error_msg)
{
    if (this->validation_error) {
        free(this->validation_error);
    }
    
    if (error_msg) {
        this->validation_error = strdup(error_msg);
        if (!this->validation_error) {
            EXTSOCK_DBG(1, "Failed to allocate memory for validation error");
            this->validation_error = NULL;
        }
    } else {
        this->validation_error = NULL;
    }
    this->is_valid = FALSE;
}

/**
 * 연결 이름 검증
 */
static bool validate_connection_name(private_extsock_config_entity_t *this)
{
    if (!this->name || strlen(this->name) == 0) {
        set_validation_error(this, "Connection name is required");
        return FALSE;
    }
    
    if (strlen(this->name) > 64) {
        set_validation_error(this, "Connection name too long (max 64 characters)");
        return FALSE;
    }
    
    // 특수 문자 검증
    for (char *p = this->name; *p; p++) {
        if (!isalnum(*p) && *p != '_' && *p != '-' && *p != '.') {
            set_validation_error(this, "Connection name contains invalid characters (only alphanumeric, _, -, . allowed)");
            return FALSE;
        }
    }
    
    return TRUE;
}

/**
 * IKE 설정 검증
 */
static bool validate_ike_config(private_extsock_config_entity_t *this)
{
    if (!this->ike_cfg) {
        set_validation_error(this, "IKE configuration is required");
        return FALSE;
    }
    
    // IKE 설정의 기본 무결성 체크
    // strongSwan이 이미 생성되었다면 기본적으로 유효하다고 가정
    return TRUE;
}

/**
 * 인증 설정 검증
 */
static bool validate_auth_configs(private_extsock_config_entity_t *this)
{
    // 로컬 인증이 없으면 기본 ANY 인증 추가
    if (!this->local_auths || this->local_auths->get_count(this->local_auths) == 0) {
        EXTSOCK_DBG(2, "No local auth config provided, will use default");
    }
    
    // 원격 인증이 없으면 기본 ANY 인증 추가
    if (!this->remote_auths || this->remote_auths->get_count(this->remote_auths) == 0) {
        EXTSOCK_DBG(2, "No remote auth config provided, will use default");
    }
    
    return TRUE;
}

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
    
    // 단계별 검증 수행
    if (!validate_connection_name(this)) {
        return FALSE;
    }
    
    if (!validate_ike_config(this)) {
        return FALSE;
    }
    
    if (!validate_auth_configs(this)) {
        return FALSE;
    }
    
    this->is_valid = TRUE;
    EXTSOCK_DBG(2, "Config entity '%s' validation successful", this->name);
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
        peer_cfg_create_t peer_create_cfg = {
            .options = OPT_NO_MOBIKE,  // 기본값으로 MOBIKE 비활성화
        };
        
        peer_cfg_t *peer_cfg = peer_cfg_create(this->name, this->ike_cfg, &peer_create_cfg);
        
        if (peer_cfg) {
            // 로컬 인증 설정 추가
            if (this->local_auths && this->local_auths->get_count(this->local_auths) > 0) {
                enumerator_t *enumerator = this->local_auths->create_enumerator(this->local_auths);
                auth_cfg_t *auth;
                while (enumerator->enumerate(enumerator, &auth)) {
                    peer_cfg->add_auth_cfg(peer_cfg, auth->clone(auth), TRUE);
                }
                enumerator->destroy(enumerator);
            } else {
                // 기본 로컬 인증 추가
                auth_cfg_t *default_local_auth = auth_cfg_create();
                default_local_auth->add(default_local_auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_ANY);
                peer_cfg->add_auth_cfg(peer_cfg, default_local_auth, TRUE);
            }
            
            // 원격 인증 설정 추가
            if (this->remote_auths && this->remote_auths->get_count(this->remote_auths) > 0) {
                enumerator_t *enumerator = this->remote_auths->create_enumerator(this->remote_auths);
                auth_cfg_t *auth;
                while (enumerator->enumerate(enumerator, &auth)) {
                    peer_cfg->add_auth_cfg(peer_cfg, auth->clone(auth), FALSE);
                }
                enumerator->destroy(enumerator);
            } else {
                // 기본 원격 인증 추가
                auth_cfg_t *default_remote_auth = auth_cfg_create();
                default_remote_auth->add(default_remote_auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_ANY);
                peer_cfg->add_auth_cfg(peer_cfg, default_remote_auth, FALSE);
            }
            
            // 자식 설정 추가
            if (this->child_cfgs) {
                enumerator_t *enumerator = this->child_cfgs->create_enumerator(this->child_cfgs);
                child_cfg_t *child;
                while (enumerator->enumerate(enumerator, &child)) {
                    peer_cfg->add_child_cfg(peer_cfg, child->get_ref(child));
                }
                enumerator->destroy(enumerator);
            }
            
            // 캐시에 저장
            this->cached_peer_cfg = peer_cfg;
            peer_cfg->get_ref(peer_cfg);
            
            EXTSOCK_DBG(2, "Successfully created peer_cfg '%s' from config entity", 
                       peer_cfg->get_name(peer_cfg));
            return peer_cfg;
        }
    }
    
    EXTSOCK_DBG(1, "to_peer_cfg: No IKE configuration available");
    return NULL;
}

METHOD(extsock_config_entity_t, clone_, extsock_config_entity_t *,
    private_extsock_config_entity_t *this)
{
    // 깊은 복사 구현
    linked_list_t *cloned_local_auths = NULL;
    linked_list_t *cloned_remote_auths = NULL;
    
    // 로컬 인증 복사
    if (this->local_auths) {
        cloned_local_auths = linked_list_create();
        enumerator_t *enumerator = this->local_auths->create_enumerator(this->local_auths);
        auth_cfg_t *auth;
        while (enumerator->enumerate(enumerator, &auth)) {
            cloned_local_auths->insert_last(cloned_local_auths, auth->clone(auth));
        }
        enumerator->destroy(enumerator);
    }
    
    // 원격 인증 복사
    if (this->remote_auths) {
        cloned_remote_auths = linked_list_create();
        enumerator_t *enumerator = this->remote_auths->create_enumerator(this->remote_auths);
        auth_cfg_t *auth;
        while (enumerator->enumerate(enumerator, &auth)) {
            cloned_remote_auths->insert_last(cloned_remote_auths, auth->clone(auth));
        }
        enumerator->destroy(enumerator);
    }
    
    // IKE 설정 참조 증가
    ike_cfg_t *cloned_ike_cfg = NULL;
    if (this->ike_cfg) {
        cloned_ike_cfg = this->ike_cfg;
        cloned_ike_cfg->get_ref(cloned_ike_cfg);
    }
    
    return extsock_config_entity_create(this->name, cloned_ike_cfg, 
                                       cloned_local_auths, cloned_remote_auths);
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
    if (this->original_json) {
        free(this->original_json);
    }
    if (this->cached_peer_cfg) {
        this->cached_peer_cfg->destroy(this->cached_peer_cfg);
    }
    if (this->ike_cfg) {
        this->ike_cfg->destroy(this->ike_cfg);
    }
    if (this->local_auths) {
        this->local_auths->destroy_offset(this->local_auths, 
                                         offsetof(auth_cfg_t, destroy));
    }
    if (this->remote_auths) {
        this->remote_auths->destroy_offset(this->remote_auths, 
                                          offsetof(auth_cfg_t, destroy));
    }
    if (this->child_cfgs) {
        this->child_cfgs->destroy_offset(this->child_cfgs, 
                                        offsetof(child_cfg_t, destroy));
    }
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

    // 안전한 이름 복사
    char *safe_name = NULL;
    if (name) {
        safe_name = strdup(name);
        if (!safe_name) {
            EXTSOCK_DBG(1, "Failed to allocate memory for connection name");
            return NULL;
        }
    }

    INIT(this,
        .public = {
            .get_name = _get_name,
            .validate = _validate,
            .to_peer_cfg = _to_peer_cfg,
            .clone_ = _clone_,
            .destroy = _destroy,
        },
        .name = safe_name,
        .is_valid = FALSE,
        .validation_error = NULL,
        .cached_peer_cfg = NULL,
        .ike_cfg = ike_cfg,
        .local_auths = local_auths,
        .remote_auths = remote_auths,
        .child_cfgs = linked_list_create(),
        .original_json = NULL,
    );

    return &this->public;
}

/**
 * JSON으로부터 설정 엔티티 생성
 * Domain Layer의 핵심 기능 - JSON을 도메인 모델로 변환
 * 
 * 현재는 기본 구조만 구현하고, 실제 JSON 파싱은 JSON Parser와 통합 예정
 */
extsock_config_entity_t *extsock_config_entity_create_from_json(const char *config_json)
{
    if (!config_json) {
        EXTSOCK_DBG(1, "create_from_json: NULL JSON configuration provided");
        return NULL;
    }

    EXTSOCK_DBG(2, "create_from_json: Basic config entity creation from JSON");

    // 기본 연결 이름 생성 (실제 JSON 파싱은 JSON Parser에서 수행)
    char default_name[64];
    snprintf(default_name, sizeof(default_name), "config_entity_%ld", (long)time(NULL));

    // 기본 엔티티 생성
    private_extsock_config_entity_t *this;
    
    char *safe_name = strdup(default_name);
    if (!safe_name) {
        EXTSOCK_DBG(1, "create_from_json: Failed to allocate memory for name");
        return NULL;
    }

    INIT(this,
        .public = {
            .get_name = _get_name,
            .validate = _validate,
            .to_peer_cfg = _to_peer_cfg,
            .clone_ = _clone_,
            .destroy = _destroy,
        },
        .name = safe_name,
        .is_valid = FALSE,
        .validation_error = NULL,
        .cached_peer_cfg = NULL,
        .ike_cfg = NULL,
        .local_auths = linked_list_create(),
        .remote_auths = linked_list_create(),
        .child_cfgs = linked_list_create(),
        .original_json = strdup(config_json),
    );

    EXTSOCK_DBG(1, "create_from_json: Basic config entity created with name '%s'", safe_name);
    EXTSOCK_DBG(1, "create_from_json: Full JSON parsing integration with JSON Parser pending");
    EXTSOCK_DBG(1, "create_from_json: This function will be completed in Phase 2 of implementation");

    return &this->public;
} 