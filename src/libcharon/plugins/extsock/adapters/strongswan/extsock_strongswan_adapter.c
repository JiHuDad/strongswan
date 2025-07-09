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
 * DPD 시작 (기존 start_dpd 함수에서 이동)
 */
static extsock_error_t start_dpd_internal(const char *ike_sa_name)
{
    // 🔴 HIGH PRIORITY: NULL 체크 강화
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
    
    // 🟡 MEDIUM PRIORITY: strongSwan API 안전 호출
    ike_dpd_t *dpd = EXTSOCK_SAFE_STRONGSWAN_CREATE(ike_dpd_create, TRUE);
    if (!dpd) {
        charon->ike_sa_manager->checkin(charon->ike_sa_manager, ike_sa);
        return EXTSOCK_ERROR_STRONGSWAN_API;
    }
    
    ike_sa->queue_task(ike_sa, (task_t*)dpd);
    charon->ike_sa_manager->checkin(charon->ike_sa_manager, ike_sa);
    
    return EXTSOCK_SUCCESS;
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
    // 🔴 HIGH PRIORITY: NULL 체크 강화
    EXTSOCK_CHECK_NULL_RET(this, EXTSOCK_ERROR_CONFIG_INVALID);
    EXTSOCK_CHECK_NULL_RET(peer_cfg, EXTSOCK_ERROR_CONFIG_INVALID);
    EXTSOCK_CHECK_NULL_RET(this->peer_cfgs_mutex, EXTSOCK_ERROR_STRONGSWAN_API);
    EXTSOCK_CHECK_NULL_RET(this->managed_peer_cfgs, EXTSOCK_ERROR_STRONGSWAN_API);
    
    this->peer_cfgs_mutex->lock(this->peer_cfgs_mutex);
    this->managed_peer_cfgs->insert_last(this->managed_peer_cfgs, peer_cfg);
    EXTSOCK_DBG(1, "Added peer_cfg '%s' to managed list", peer_cfg->get_name(peer_cfg));
    
    // start_action이 ACTION_START인 Child SA들에 대해 SA 개시
    enumerator_t *child_enum = peer_cfg->create_child_cfg_enumerator(peer_cfg);
    child_cfg_t *current_child;
    if (child_enum) {
        while (child_enum->enumerate(child_enum, &current_child)) {
            if (current_child && current_child->get_start_action(current_child) == ACTION_START) {
                EXTSOCK_DBG(1, "Initiating CHILD_SA '%s' for peer '%s'",
                           current_child->get_name(current_child), peer_cfg->get_name(peer_cfg));
                
                // 🟡 MEDIUM PRIORITY: charon->controller 안전성 체크
                if (charon && charon->controller) {
                    charon->controller->initiate(charon->controller,
                                               peer_cfg, current_child,
                                               NULL, NULL, 0, 0, FALSE);
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
        .managed_peer_cfgs = linked_list_create(),
        .peer_cfgs_mutex = mutex_create(MUTEX_TYPE_DEFAULT),
        .creds = mem_cred_create(),
    );

    if (this->creds) {
        lib->credmgr->add_set(lib->credmgr, &this->creds->set);
    }

    return &this->public;
} 