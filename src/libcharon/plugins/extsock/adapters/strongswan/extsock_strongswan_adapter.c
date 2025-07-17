/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_strongswan_adapter.h"
#include "../../common/extsock_common.h"

#include <daemon.h>
#include <library.h>
#include <sa/ike_sa_manager.h>
#include <sa/ikev2/tasks/ike_dpd.h>
#include <control/controller.h>
#include <bus/listeners/listener.h>
#include <threading/mutex.h>

typedef struct private_extsock_strongswan_adapter_t private_extsock_strongswan_adapter_t;

/**
 * strongSwan 어댑터 내부 구조체
 */
struct private_extsock_strongswan_adapter_t {
    
    /**
     * 공개 인터페이스
     */
    extsock_strongswan_adapter_t public;
    
    /**
     * strongSwan configuration backend 인터페이스
     */
    backend_t backend;
    
    /**
     * 인메모리 자격증명 세트
     */
    mem_cred_t *creds;
    
    /**
     * 관리되는 피어 설정 목록
     */
    linked_list_t *managed_peer_cfgs;
    
    /**
     * 피어 설정 목록 접근 뮤텍스
     */
    mutex_t *peer_cfgs_mutex;
};

/**
 * strongSwan backend - IKE config 열거자 생성
 */
METHOD(backend_t, create_ike_cfg_enumerator, enumerator_t*,
    private_extsock_strongswan_adapter_t *this, host_t *me, host_t *other)
{
    // extsock 플러그인은 peer_cfg를 통해 IKE 설정을 제공하므로 빈 열거자 반환
    return enumerator_create_empty();
}

/**
 * strongSwan backend - Peer config 열거자 생성
 */
METHOD(backend_t, create_peer_cfg_enumerator, enumerator_t*,
    private_extsock_strongswan_adapter_t *this, identification_t *me, identification_t *other)
{
    // CRITICAL: Ultra-defensive NULL checking with detailed logging
    EXTSOCK_DBG(1, "BACKEND ENTRY: create_peer_cfg_enumerator called");
    
    if (!this) {
        EXTSOCK_DBG(1, "BACKEND ERROR: this pointer is NULL");
        return enumerator_create_empty();
    }
    
    EXTSOCK_DBG(1, "BACKEND: this pointer is valid (%p)", this);
    
    if (!this->peer_cfgs_mutex) {
        EXTSOCK_DBG(1, "BACKEND ERROR: peer_cfgs_mutex is NULL");
        return enumerator_create_empty();
    }
    
    EXTSOCK_DBG(1, "BACKEND: peer_cfgs_mutex is valid (%p)", this->peer_cfgs_mutex);
    
    if (!this->managed_peer_cfgs) {
        EXTSOCK_DBG(1, "BACKEND ERROR: managed_peer_cfgs is NULL");
        return enumerator_create_empty();
    }
    
    EXTSOCK_DBG(1, "BACKEND: managed_peer_cfgs is valid (%p)", this->managed_peer_cfgs);
    
    char me_str[64] = "any";
    char other_str[64] = "any";
    
    if (me && me->get_encoding) {
        chunk_t me_chunk = me->get_encoding(me);
        snprintf(me_str, sizeof(me_str), "%.*s", (int)me_chunk.len, me_chunk.ptr);
    }
    if (other && other->get_encoding) {
        chunk_t other_chunk = other->get_encoding(other);
        snprintf(other_str, sizeof(other_str), "%.*s", (int)other_chunk.len, other_chunk.ptr);
    }
    
    EXTSOCK_DBG(1, "BACKEND CALLED! strongSwan is requesting peer_cfg enumerator (me=%s, other=%s)",
               me_str, other_str);
    
    // CRITICAL FIX: Ultra-safe mutex operation with detailed logging
    EXTSOCK_DBG(1, "BACKEND: About to lock mutex...");
    
    // Additional validation before mutex operation
    if (this->peer_cfgs_mutex->lock == NULL) {
        EXTSOCK_DBG(1, "BACKEND ERROR: mutex lock function is NULL");
        return enumerator_create_empty();
    }
    
    this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
    EXTSOCK_DBG(1, "BACKEND: Mutex locked successfully");
    
    int count = this->managed_peer_cfgs->get_count(this->managed_peer_cfgs);
    EXTSOCK_DBG(1, "BACKEND: Peer config count: %d", count);
    
    enumerator_t *enumerator;
    
    if (count == 0) {
        EXTSOCK_DBG(1, "BACKEND: Creating empty enumerator");
        enumerator = enumerator_create_empty();
    } else {
        EXTSOCK_DBG(1, "BACKEND: Creating list enumerator");
        enumerator = this->managed_peer_cfgs->create_enumerator(this->managed_peer_cfgs);
    }
    
    EXTSOCK_DBG(1, "BACKEND: About to unlock mutex...");
    this->peer_cfgs_mutex->unlock(this->peer_cfgs_mutex);
    EXTSOCK_DBG(1, "BACKEND: Mutex unlocked successfully");
    
    EXTSOCK_DBG(1, "BACKEND RESPONSE: Providing %d managed peer configs to strongSwan", count);
    
    return enumerator;
}

/**
 * strongSwan backend - 이름으로 peer config 조회
 */
METHOD(backend_t, get_peer_cfg_by_name, peer_cfg_t*,
    private_extsock_strongswan_adapter_t *this, char *name)
{
    // CRITICAL: NULL check first
    if (!this || !this->peer_cfgs_mutex || !this->managed_peer_cfgs) {
        EXTSOCK_DBG(1, "BACKEND ERROR: Invalid adapter state");
        return NULL;
    }
    
    if (!name) {
        EXTSOCK_DBG(1, "BACKEND CALLED! get_peer_cfg_by_name with NULL name");
        return NULL;
    }
    
    EXTSOCK_DBG(1, "BACKEND CALLED! get_peer_cfg_by_name looking for: '%s'", name);
    
    this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
    enumerator_t *enumerator = this->managed_peer_cfgs->create_enumerator(this->managed_peer_cfgs);
    peer_cfg_t *peer_cfg, *found = NULL;
    int total_configs = this->managed_peer_cfgs->get_count(this->managed_peer_cfgs);
    
    EXTSOCK_DBG(1, "   Searching through %d managed peer configs...", total_configs);
    
    int index = 0;
    while (enumerator->enumerate(enumerator, &peer_cfg)) {
        const char *cfg_name = peer_cfg->get_name(peer_cfg);
        EXTSOCK_DBG(1, "   [%d] Comparing '%s' vs '%s'", index++, name, cfg_name);
        if (streq(cfg_name, name)) {
            found = peer_cfg;
            EXTSOCK_DBG(1, "   MATCH FOUND!");
            break;
        }
    }
    enumerator->destroy(enumerator);
    this->peer_cfgs_mutex->unlock(this->peer_cfgs_mutex);
    
    EXTSOCK_DBG(1, "BACKEND RESPONSE: lookup for '%s': %s", name, found ? "FOUND" : "NOT FOUND");
    return found;
}

/**
 * DPD 시작 (기존 start_dpd 함수에서 이동)
 */
static extsock_error_t start_dpd_internal(const char *ike_sa_name)
{
    // HIGH PRIORITY: NULL 체크 강화
    EXTSOCK_CHECK_NULL_RET(ike_sa_name, EXTSOCK_ERROR_CONFIG_INVALID);
    EXTSOCK_CHECK_NULL_RET(charon, EXTSOCK_ERROR_STRONGSWAN_API);
    EXTSOCK_CHECK_NULL_RET(charon->ike_sa_manager, EXTSOCK_ERROR_STRONGSWAN_API);
    
    ike_sa_t *ike_sa = charon->ike_sa_manager->checkout_by_name(
        charon->ike_sa_manager, (char*)ike_sa_name, ID_MATCH_PERFECT);
    if (!ike_sa) {
        EXTSOCK_DBG(1, "start_dpd: IKE_SA '%s' not found", ike_sa_name);
        return EXTSOCK_ERROR_STRONGSWAN_API;
    }
    
    EXTSOCK_DBG(1, "start_dpd: Starting DPD for IKE_SA '%s'", ike_sa_name);
    
    // CRITICAL FIX: ike_sa->send_dpd() 직접 호출 (queue_task 대신)
    status_t result = ike_sa->send_dpd(ike_sa);
    charon->ike_sa_manager->checkin(charon->ike_sa_manager, ike_sa);
    
    if (result == SUCCESS) {
        EXTSOCK_DBG(1, "DPD successfully triggered for IKE_SA '%s'", ike_sa_name);
        return EXTSOCK_SUCCESS;
    } else {
        EXTSOCK_DBG(1, "DPD failed for IKE_SA '%s' with status %d", ike_sa_name, result);
        return EXTSOCK_ERROR_STRONGSWAN_API;
    }
}

METHOD(extsock_config_repository_t, apply_config, extsock_error_t,
    private_extsock_strongswan_adapter_t *this, extsock_config_entity_t *config)
{
    if (!config) {
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    // 설정 엔티티를 peer_cfg로 변환
    peer_cfg_t *peer_cfg = config->to_peer_cfg(config);
    if (!peer_cfg) {
        EXTSOCK_DBG(1, "Failed to convert config entity to peer_cfg");
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    return this->public.add_peer_config(&this->public, peer_cfg);
}

METHOD(extsock_config_repository_t, remove_config, extsock_error_t,
    private_extsock_strongswan_adapter_t *this, const char *name)
{
    return this->public.remove_peer_config(&this->public, name);
}

METHOD(extsock_config_repository_t, start_dpd, extsock_error_t,
    private_extsock_strongswan_adapter_t *this, const char *ike_sa_name)
{
    return start_dpd_internal(ike_sa_name);
}

METHOD(extsock_config_repository_t, destroy_repository, void,
    private_extsock_strongswan_adapter_t *this)
{
    // 설정 저장소는 어댑터의 일부이므로 별도 해제 불필요
}

METHOD(extsock_strongswan_adapter_t, add_peer_config, extsock_error_t,
    private_extsock_strongswan_adapter_t *this, peer_cfg_t *peer_cfg)
{
    // HIGH PRIORITY: NULL 체크 강화
    EXTSOCK_CHECK_NULL_RET(this, EXTSOCK_ERROR_CONFIG_INVALID);
    EXTSOCK_CHECK_NULL_RET(peer_cfg, EXTSOCK_ERROR_CONFIG_INVALID);
    EXTSOCK_CHECK_NULL_RET(this->peer_cfgs_mutex, EXTSOCK_ERROR_STRONGSWAN_API);
    EXTSOCK_CHECK_NULL_RET(this->managed_peer_cfgs, EXTSOCK_ERROR_STRONGSWAN_API);
    
    this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
    
    // CRITICAL: peer_cfg를 관리 목록에 추가
    this->managed_peer_cfgs->insert_last(this->managed_peer_cfgs, peer_cfg);
    EXTSOCK_DBG(1, "Added peer_cfg '%s' to managed list (backend already registered)", peer_cfg->get_name(peer_cfg));
    
    // start_action이 ACTION_START인 Child SA들에 대해 SA 개시
    enumerator_t *child_enum = peer_cfg->create_child_cfg_enumerator(peer_cfg);
    child_cfg_t *current_child;
    if (child_enum) {
        while (child_enum->enumerate(child_enum, &current_child)) {
            if (current_child && current_child->get_start_action(current_child) == ACTION_START) {
                EXTSOCK_DBG(1, "Initiating CHILD_SA '%s' for peer '%s'",
                           current_child->get_name(current_child), peer_cfg->get_name(peer_cfg));
                
                // MEDIUM PRIORITY: charon->controller 안전성 체크
                if (charon && charon->controller) {
                    charon->controller->initiate(charon->controller,
                                               peer_cfg, current_child,
                                               NULL, NULL, 0, 0, FALSE);
                    EXTSOCK_DBG(1, "CHILD_SA initiation requested for '%s'", current_child->get_name(current_child));
                } else {
                    EXTSOCK_DBG(1, "Warning: charon->controller not available");
                }
            }
        }
        child_enum->destroy(child_enum);
    }
    this->peer_cfgs_mutex->unlock(this->peer_cfgs_mutex);
    
    return EXTSOCK_SUCCESS;
}

METHOD(extsock_strongswan_adapter_t, remove_peer_config, extsock_error_t,
    private_extsock_strongswan_adapter_t *this, const char *name)
{
    if (!name) {
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
    enumerator_t *enumerator = this->managed_peer_cfgs->create_enumerator(this->managed_peer_cfgs);
    peer_cfg_t *peer_cfg;
    bool found = FALSE;
    
    while (enumerator->enumerate(enumerator, &peer_cfg)) {
        if (streq(peer_cfg->get_name(peer_cfg), name)) {
            this->managed_peer_cfgs->remove_at(this->managed_peer_cfgs, enumerator);
            peer_cfg->destroy(peer_cfg);
            found = TRUE;
            break;
        }
    }
    enumerator->destroy(enumerator);
    this->peer_cfgs_mutex->unlock(this->peer_cfgs_mutex);
    
    EXTSOCK_DBG(1, "Peer config '%s' %s", name, found ? "removed" : "not found");
    return found ? EXTSOCK_SUCCESS : EXTSOCK_ERROR_CONFIG_INVALID;
}

METHOD(extsock_strongswan_adapter_t, initiate_child_sa, extsock_error_t,
    private_extsock_strongswan_adapter_t *this, peer_cfg_t *peer_cfg, child_cfg_t *child_cfg)
{
    if (!peer_cfg || !child_cfg) {
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    EXTSOCK_DBG(1, "Initiating CHILD_SA '%s' for peer '%s'",
               child_cfg->get_name(child_cfg), peer_cfg->get_name(peer_cfg));
    
    charon->controller->initiate(charon->controller, peer_cfg, child_cfg,
                               NULL, NULL, 0, 0, FALSE);
    
    return EXTSOCK_SUCCESS;
}

METHOD(extsock_strongswan_adapter_t, get_managed_configs, linked_list_t *,
    private_extsock_strongswan_adapter_t *this)
{
    return this->managed_peer_cfgs;
}

METHOD(extsock_strongswan_adapter_t, get_credentials, mem_cred_t *,
    private_extsock_strongswan_adapter_t *this)
{
    return this->creds;
}

METHOD(extsock_strongswan_adapter_t, destroy, void,
    private_extsock_strongswan_adapter_t *this)
{
    // CRITICAL: strongSwan backend 제거
    if (charon && charon->backends) {
        charon->backends->remove_backend(charon->backends, &this->backend);
        EXTSOCK_DBG(1, "extsock configuration backend removed from strongSwan");
    }
    
    // 관리되는 피어 설정들 해제
    if (this->managed_peer_cfgs) {
        this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
        peer_cfg_t *cfg_to_destroy;
        while (this->managed_peer_cfgs->remove_first(this->managed_peer_cfgs, (void**)&cfg_to_destroy) == SUCCESS) {
            cfg_to_destroy->destroy(cfg_to_destroy);
        }
        this->peer_cfgs_mutex->unlock(this->peer_cfgs_mutex);
        this->managed_peer_cfgs->destroy(this->managed_peer_cfgs);
    }
    
    // 뮤텍스 해제
    if (this->peer_cfgs_mutex) {
        this->peer_cfgs_mutex->destroy(this->peer_cfgs_mutex);
    }
    
    // 자격증명 세트 해제
    if (this->creds) {
        lib->credmgr->remove_set(lib->credmgr, &this->creds->set);
        this->creds->destroy(this->creds);
    }
    
    free(this);
}

/**
 * strongSwan 어댑터 생성
 */
extsock_strongswan_adapter_t *extsock_strongswan_adapter_create()
{
    private_extsock_strongswan_adapter_t *this;

    INIT(this,
        .public = {
            .config_repository = {
                .apply_config = _apply_config,
                .remove_config = _remove_config,
                .start_dpd = _start_dpd,
                .destroy = _destroy_repository,
            },
            .add_peer_config = _add_peer_config,
            .remove_peer_config = _remove_peer_config,
            .initiate_child_sa = _initiate_child_sa,
            .get_managed_configs = _get_managed_configs,
            .get_credentials = _get_credentials,
            .destroy = _destroy,
        },
        .backend = {
            .create_ike_cfg_enumerator = _create_ike_cfg_enumerator,
            .create_peer_cfg_enumerator = _create_peer_cfg_enumerator,
            .get_peer_cfg_by_name = _get_peer_cfg_by_name,
        },
        .managed_peer_cfgs = linked_list_create(),
        .peer_cfgs_mutex = mutex_create(MUTEX_TYPE_DEFAULT),
        .creds = mem_cred_create(),
    );

    // CRITICAL FIX: NULL check for initialization failures
    if (!this->managed_peer_cfgs || !this->peer_cfgs_mutex || !this->creds) {
        EXTSOCK_DBG(1, "Failed to initialize strongSwan adapter components");
        
        // Safe cleanup of partially initialized components
        if (this->managed_peer_cfgs) {
            this->managed_peer_cfgs->destroy(this->managed_peer_cfgs);
        }
        if (this->peer_cfgs_mutex) {
            this->peer_cfgs_mutex->destroy(this->peer_cfgs_mutex);
        }
        if (this->creds) {
            this->creds->destroy(this->creds);
        }
        free(this);
        return NULL;
    }

    if (this->creds) {
        lib->credmgr->add_set(lib->credmgr, &this->creds->set);
    }

    // CRITICAL: strongSwan backend 등록
    if (charon && charon->backends) {
        charon->backends->add_backend(charon->backends, &this->backend);
        EXTSOCK_DBG(1, "extsock configuration backend registered with strongSwan");
    } else {
        EXTSOCK_DBG(1, "Warning: charon->backends not available during initialization");
    }

    return &this->public;
} 