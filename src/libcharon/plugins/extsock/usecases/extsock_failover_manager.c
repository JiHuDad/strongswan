/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Failover Manager Implementation for extsock plugin
 */

#include "../interfaces/extsock_failover_manager.h"
#include "../usecases/extsock_config_usecase.h"
#include "../common/extsock_common.h"

#include <collections/hashtable.h>
#include <collections/linked_list.h>
#include <utils/debug.h>
#include <threading/mutex.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* 최대 failover 재시도 횟수 */
#define MAX_FAILOVER_RETRY 5

/**
 * hashtable을 위한 free wrapper 함수
 */
static void free_wrapper(void *key, const void *value)
{
    free(key);
    free((void*)value);
}

typedef struct private_extsock_failover_manager_t private_extsock_failover_manager_t;

/**
 * Failover Manager 내부 구조체
 */
struct private_extsock_failover_manager_t {
    
    /**
     * 공개 인터페이스
     */
    extsock_failover_manager_t public;
    
    /**
     * Config Usecase (의존성 주입)
     */
    extsock_config_usecase_t *config_usecase;
    
    /**
     * 현재 활성 SEGW 추적
     */
    hashtable_t *active_segw_map;  // connection_name -> current_segw_addr
    
    /**
     * Failover 시도 횟수 추적 (무한 루프 방지)
     */
    hashtable_t *retry_count_map;  // connection_name -> retry_count
    
    /**
     * Thread safety를 위한 mutex
     */
    mutex_t *mutex;
};

/**
 * 문자열 앞뒤 공백 제거
 */
static char* trim_whitespace(char *str)
{
    char *end;
    
    // 앞쪽 공백 제거
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    // 뒤쪽 공백 제거
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    
    return str;
}

/**
 * 쉼표로 구분된 주소 문자열을 linked_list로 파싱
 */
static void parse_comma_separated_addresses(const char *addr_str, linked_list_t *list)
{
    if (!addr_str || !list) {
        return;
    }
    
    char *str_copy = strdup(addr_str);
    char *saveptr;
    char *token = strtok_r(str_copy, ",", &saveptr);
    
    while (token != NULL) {
        char *trimmed = trim_whitespace(token);
        if (strlen(trimmed) > 0) {
            list->insert_last(list, strdup(trimmed));
        }
        token = strtok_r(NULL, ",", &saveptr);
    }
    
    free(str_copy);
}

/**
 * 주소 목록에서 특정 주소의 인덱스 찾기
 */
static int find_address_index(linked_list_t *addr_list, const char *target_addr)
{
    enumerator_t *enumerator;
    char *addr;
    int index = 0;
    
    if (!addr_list || !target_addr) {
        return -1;
    }
    
    enumerator = addr_list->create_enumerator(addr_list);
    while (enumerator->enumerate(enumerator, &addr)) {
        if (streq(addr, target_addr)) {
            enumerator->destroy(enumerator);
            return index;
        }
        index++;
    }
    enumerator->destroy(enumerator);
    
    return -1;  // 찾지 못함
}

/**
 * 인덱스로 주소 가져오기
 */
static char* get_address_at_index(linked_list_t *addr_list, int index)
{
    enumerator_t *enumerator;
    char *addr;
    int current_index = 0;
    
    if (!addr_list || index < 0) {
        return NULL;
    }
    
    enumerator = addr_list->create_enumerator(addr_list);
    while (enumerator->enumerate(enumerator, &addr)) {
        if (current_index == index) {
            enumerator->destroy(enumerator);
            return addr;
        }
        current_index++;
    }
    enumerator->destroy(enumerator);
    
    return NULL;
}

/**
 * 현재 활성 SEGW 업데이트
 */
static void update_active_segw(private_extsock_failover_manager_t *this, 
                              const char *conn_name, const char *segw_addr)
{
    if (!conn_name || !segw_addr) {
        return;
    }
    
    this->mutex->lock(this->mutex);
    
    char *old_addr = this->active_segw_map->remove(this->active_segw_map, conn_name);
    free(old_addr);
    
    this->active_segw_map->put(this->active_segw_map, 
                              strdup(conn_name), strdup(segw_addr));
    
    this->mutex->unlock(this->mutex);
}

/**
 * 재시도 횟수 증가
 */
static void increment_retry_count(private_extsock_failover_manager_t *this, const char *conn_name)
{
    if (!conn_name) {
        return;
    }
    
    this->mutex->lock(this->mutex);
    
    int *count = this->retry_count_map->get(this->retry_count_map, conn_name);
    if (count) {
        (*count)++;
    } else {
        int *new_count = malloc(sizeof(int));
        *new_count = 1;
        this->retry_count_map->put(this->retry_count_map, strdup(conn_name), new_count);
    }
    
    this->mutex->unlock(this->mutex);
}

/**
 * IKE proposals 복사
 */
static void copy_ike_proposals(ike_cfg_t *src, ike_cfg_t *dst)
{
    enumerator_t *enumerator;
    proposal_t *proposal;
    
    if (!src || !dst) {
        return;
    }
    
    linked_list_t *proposals = src->get_proposals(src);
    if (proposals) {
        enumerator = proposals->create_enumerator(proposals);
        while (enumerator->enumerate(enumerator, &proposal)) {
            dst->add_proposal(dst, proposal->clone(proposal, 0));
        }
        enumerator->destroy(enumerator);
    }
}

/**
 * Authentication configs 복사
 */
static bool copy_auth_configs(peer_cfg_t *src, peer_cfg_t *dst)
{
    enumerator_t *enumerator;
    auth_cfg_t *auth_cfg;
    
    if (!src || !dst) {
        return FALSE;
    }
    
    // Local auth configs 복사
    enumerator = src->create_auth_cfg_enumerator(src, TRUE);
    while (enumerator->enumerate(enumerator, &auth_cfg)) {
        dst->add_auth_cfg(dst, auth_cfg->clone(auth_cfg), TRUE);
    }
    enumerator->destroy(enumerator);
    
    // Remote auth configs 복사
    enumerator = src->create_auth_cfg_enumerator(src, FALSE);
    while (enumerator->enumerate(enumerator, &auth_cfg)) {
        dst->add_auth_cfg(dst, auth_cfg->clone(auth_cfg), FALSE);
    }
    enumerator->destroy(enumerator);
    
    return TRUE;
}

/**
 * Child configs 복사 (strongSwan의 replace_child_cfgs 활용)
 */
static bool copy_child_configs(peer_cfg_t *src, peer_cfg_t *dst)
{
    enumerator_t *replace_enum;
    
    if (!src || !dst) {
        return FALSE;
    }
    
    // strongSwan의 built-in 메서드로 원자적 복사
    replace_enum = dst->replace_child_cfgs(dst, src);
    if (replace_enum) {
        replace_enum->destroy(replace_enum);
        EXTSOCK_DBG(3, "Successfully replaced child configs using strongSwan API");
        return TRUE;
    }
    
    EXTSOCK_DBG(1, "Failed to replace child configs");
    return FALSE;
}

METHOD(extsock_failover_manager_t, select_next_segw, char*,
    private_extsock_failover_manager_t *this, const char *remote_addrs, const char *current_addr)
{
    if (!remote_addrs || !current_addr) {
        return NULL;
    }
    
    // 1. 쉼표로 구분된 주소 파싱
    linked_list_t *addr_list = linked_list_create();
    parse_comma_separated_addresses(remote_addrs, addr_list);
    
    if (addr_list->get_count(addr_list) < 2) {
        addr_list->destroy_function(addr_list, free);
        return NULL;  // 단일 주소는 failover 불가
    }
    
    // 2. 현재 주소의 인덱스 찾기
    int current_index = find_address_index(addr_list, current_addr);
    if (current_index == -1) {
        current_index = 0;  // 현재 주소를 찾을 수 없으면 첫 번째부터
    }
    
    // 3. 다음 주소 선택 (순환 방식)
    int next_index = (current_index + 1) % addr_list->get_count(addr_list);
    char *next_addr = get_address_at_index(addr_list, next_index);
    
    // 4. 결과 반환 (복사본)
    char *result = next_addr ? strdup(next_addr) : NULL;
    
    addr_list->destroy_function(addr_list, free);
    return result;
}

METHOD(extsock_failover_manager_t, is_max_retry_exceeded, bool,
    private_extsock_failover_manager_t *this, const char *conn_name)
{
    if (!conn_name) {
        return FALSE;
    }
    
    this->mutex->lock(this->mutex);
    
    int *count = this->retry_count_map->get(this->retry_count_map, conn_name);
    bool exceeded = count && (*count >= MAX_FAILOVER_RETRY);
    
    this->mutex->unlock(this->mutex);
    
    return exceeded;
}

METHOD(extsock_failover_manager_t, reset_retry_count, void,
    private_extsock_failover_manager_t *this, const char *conn_name)
{
    if (!conn_name) {
        return;
    }
    
    this->mutex->lock(this->mutex);
    
    int *count = this->retry_count_map->remove(this->retry_count_map, conn_name);
    free(count);
    
    this->mutex->unlock(this->mutex);
}

METHOD(extsock_failover_manager_t, create_failover_config, extsock_error_t,
    private_extsock_failover_manager_t *this, peer_cfg_t *original_cfg, const char *next_segw_addr)
{
    if (!original_cfg || !next_segw_addr) {
        return EXTSOCK_ERROR_INVALID_PARAMETER;
    }
    
    // 1. 기존 ike_cfg에서 설정 추출
    ike_cfg_t *original_ike_cfg = original_cfg->get_ike_cfg(original_cfg);
    if (!original_ike_cfg) {
        EXTSOCK_DBG(1, "No ike_cfg in original peer_cfg");
        return EXTSOCK_ERROR_INVALID_PARAMETER;
    }
    
    // 2. 새 ike_cfg 생성 (주소만 변경)
    char *remote_addr_copy = strdup(next_segw_addr);
    ike_cfg_create_t ike_data = {
        .version = original_ike_cfg->get_version(original_ike_cfg),
        .local = original_ike_cfg->get_my_addr(original_ike_cfg),
        .remote = remote_addr_copy,  // 핵심: 다음 SEGW 주소로 변경
        .local_port = original_ike_cfg->get_my_port(original_ike_cfg),
        .remote_port = original_ike_cfg->get_other_port(original_ike_cfg),
        .no_certreq = !original_ike_cfg->send_certreq(original_ike_cfg),
        .ocsp_certreq = original_ike_cfg->send_ocsp_certreq(original_ike_cfg),
        .force_encap = original_ike_cfg->force_encap(original_ike_cfg),
        .fragmentation = original_ike_cfg->fragmentation(original_ike_cfg),
        .childless = original_ike_cfg->childless(original_ike_cfg),
        .dscp = original_ike_cfg->get_dscp(original_ike_cfg),
    };
    
    ike_cfg_t *new_ike_cfg = ike_cfg_create(&ike_data);
    free(remote_addr_copy);  // 복사본 정리
    if (!new_ike_cfg) {
        EXTSOCK_DBG(1, "Failed to create new ike_cfg");
        return EXTSOCK_ERROR_CONFIG_CREATION_FAILED;
    }
    
    // 3. IKE proposals 복사
    copy_ike_proposals(original_ike_cfg, new_ike_cfg);
    
    // 4. 새 peer_cfg 생성
    char new_name[128];
    snprintf(new_name, sizeof(new_name), "%s-failover-%s", 
             original_cfg->get_name(original_cfg), next_segw_addr);
    
    peer_cfg_create_t peer_data = {
        .cert_policy = original_cfg->get_cert_policy(original_cfg),
        .unique = original_cfg->get_unique_policy(original_cfg),
        .keyingtries = original_cfg->get_keyingtries(original_cfg),
        .rekey_time = original_cfg->get_rekey_time(original_cfg, FALSE),
        .reauth_time = original_cfg->get_reauth_time(original_cfg, FALSE),
        .jitter_time = 600,  // 기본값
        .over_time = original_cfg->get_over_time(original_cfg),
        .dpd = original_cfg->get_dpd(original_cfg),
        .dpd_timeout = original_cfg->get_dpd_timeout(original_cfg),
    };
    
    peer_cfg_t *new_peer_cfg = peer_cfg_create(new_name, new_ike_cfg, &peer_data);
    if (!new_peer_cfg) {
        new_ike_cfg->destroy(new_ike_cfg);
        EXTSOCK_DBG(1, "Failed to create new peer_cfg");
        return EXTSOCK_ERROR_CONFIG_CREATION_FAILED;
    }
    
    // 5. auth_cfg, child_cfg 복사
    if (!copy_auth_configs(original_cfg, new_peer_cfg) ||
        !copy_child_configs(original_cfg, new_peer_cfg)) {
        new_peer_cfg->destroy(new_peer_cfg);
        return EXTSOCK_ERROR_CONFIG_CREATION_FAILED;
    }
    
    // 6. Config Usecase를 통해 charon에 등록 및 연결 시도
    return this->config_usecase->add_peer_config_and_initiate(
        this->config_usecase, new_peer_cfg);
}

METHOD(extsock_failover_manager_t, handle_connection_failure, void,
    private_extsock_failover_manager_t *this, ike_sa_t *ike_sa)
{
    if (!ike_sa) {
        return;
    }
    
    const char *ike_name = ike_sa->get_name(ike_sa);
    EXTSOCK_DBG(1, "Handling connection failure for IKE SA '%s'", ike_name);
    
    // 1. 설정 정보 추출 (100% 안전 - 타이밍 분석에서 검증됨)
    peer_cfg_t *peer_cfg = ike_sa->get_peer_cfg(ike_sa);
    if (!peer_cfg) {
        EXTSOCK_DBG(1, "No peer_cfg available for failover");
        return;
    }
    
    ike_cfg_t *ike_cfg = peer_cfg->get_ike_cfg(peer_cfg);
    if (!ike_cfg) {
        EXTSOCK_DBG(1, "No ike_cfg available for failover");
        return;
    }
    
    char *remote_addrs = ike_cfg->get_other_addr(ike_cfg);
    if (!remote_addrs || !strchr(remote_addrs, ',')) {
        EXTSOCK_DBG(1, "No multiple addresses configured (remote_addrs: %s)", 
                   remote_addrs ? remote_addrs : "NULL");
        return;
    }
    
    // 2. 현재 주소 확인
    host_t *current_host = ike_sa->get_other_host(ike_sa);
    if (!current_host) {
        EXTSOCK_DBG(1, "No current remote host available");
        return;
    }
    
    char current_addr[64];
    snprintf(current_addr, sizeof(current_addr), "%H", current_host);
    
    // 3. 재시도 횟수 체크 (무한 루프 방지)
    if (this->public.is_max_retry_exceeded(&this->public, ike_name)) {
        EXTSOCK_DBG(1, "Max retry count exceeded for connection '%s'", ike_name);
        return;
    }
    
    // 4. 다음 주소 선택
    char *next_addr = this->public.select_next_segw(&this->public, remote_addrs, current_addr);
    if (!next_addr) {
        EXTSOCK_DBG(1, "No alternative SEGW available");
        return;
    }
    
    EXTSOCK_DBG(1, "Initiating failover: %s -> %s", current_addr, next_addr);
    
    // 5. Failover 수행
    extsock_error_t result = this->public.create_failover_config(&this->public, peer_cfg, next_addr);
    if (result == EXTSOCK_SUCCESS) {
        EXTSOCK_DBG(1, "Failover to %s initiated successfully", next_addr);
        update_active_segw(this, ike_name, next_addr);
        increment_retry_count(this, ike_name);
    } else {
        EXTSOCK_DBG(1, "Failed to initiate failover to %s (error: %d)", next_addr, result);
    }
    
    free(next_addr);
}

METHOD(extsock_failover_manager_t, destroy, void,
    private_extsock_failover_manager_t *this)
{
    if (this->active_segw_map) {
        this->active_segw_map->destroy_function(this->active_segw_map, free_wrapper);
    }
    if (this->retry_count_map) {
        this->retry_count_map->destroy_function(this->retry_count_map, free_wrapper);
    }
    if (this->mutex) {
        this->mutex->destroy(this->mutex);
    }
    free(this);
}

/*
 * Described in header
 */
extsock_failover_manager_t *extsock_failover_manager_create(extsock_config_usecase_t *config_usecase)
{
    private_extsock_failover_manager_t *this;
    
    if (!config_usecase) {
        return NULL;
    }
    
    INIT(this,
        .public = {
            .handle_connection_failure = _handle_connection_failure,
            .select_next_segw = _select_next_segw,
            .create_failover_config = _create_failover_config,
            .is_max_retry_exceeded = _is_max_retry_exceeded,
            .reset_retry_count = _reset_retry_count,
            .destroy = _destroy,
        },
        .config_usecase = config_usecase,
        .active_segw_map = hashtable_create(hashtable_hash_str, hashtable_equals_str, 32),
        .retry_count_map = hashtable_create(hashtable_hash_str, hashtable_equals_str, 32),
        .mutex = mutex_create(MUTEX_TYPE_DEFAULT),
    );
    
    EXTSOCK_DBG(2, "Failover Manager created successfully");
    
    return &this->public;
} 