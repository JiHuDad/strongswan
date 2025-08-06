# extsock 2nd SEGW 지원 구현 계획서

## 📋 프로젝트 개요

### 목표
- extsock 플러그인에 2nd SEGW 자동 failover 기능 구현
- strongSwan의 기존 설정 관리 메커니즘 활용
- IKE_DESTROYING 이벤트 기반 즉시 전환 방식 적용

### 핵심 원칙
- **설정 접근 안전성**: IKE_DESTROYING 시점에서 100% 안전한 설정 정보 접근
- **기존 아키텍처 유지**: Clean Architecture 패턴 준수
- **최소 침습적 구현**: 기존 코드에 최소한의 변경으로 기능 추가

---

## 🚀 Phase 1: 기반 인프라 구축 (1-2주)

### 1.1 Failover Manager 인터페이스 생성

**📁 새 파일 생성:**
```
src/libcharon/plugins/extsock/interfaces/extsock_failover_manager.h
src/libcharon/plugins/extsock/usecases/extsock_failover_manager.c
```

**🔧 작업 내용:**

#### 1.1.1 인터페이스 정의
```c
// interfaces/extsock_failover_manager.h
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
```

#### 1.1.2 기본 구현체 구조
```c
// usecases/extsock_failover_manager.c
#include "extsock_failover_manager.h"
#include "../usecases/extsock_config_usecase.h"

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
};
```

### 1.2 주소 파싱 및 선택 로직 구현

#### 1.2.1 핵심 알고리즘
```c
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
```

#### 1.2.2 헬퍼 함수들
```c
/**
 * 쉼표로 구분된 주소 문자열을 linked_list로 파싱
 */
static void parse_comma_separated_addresses(const char *addr_str, linked_list_t *list)
{
    char *str_copy = strdup(addr_str);
    char *token = strtok(str_copy, ",");
    
    while (token != NULL) {
        // 앞뒤 공백 제거
        char *trimmed = trim_whitespace(token);
        if (strlen(trimmed) > 0) {
            list->insert_last(list, strdup(trimmed));
        }
        token = strtok(NULL, ",");
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
```

### 1.3 Event Usecase 확장

#### 1.3.1 파일 수정
```
📁 수정할 파일:
src/libcharon/plugins/extsock/usecases/extsock_event_usecase.h
src/libcharon/plugins/extsock/usecases/extsock_event_usecase.c
```

#### 1.3.2 구조체 확장
```c
// extsock_event_usecase.h에 추가
struct private_extsock_event_usecase_t {
    /**
     * 공개 인터페이스
     */
    extsock_event_usecase_t public;
    
    /**
     * 이벤트 발행자 인터페이스 구현
     */
    extsock_event_publisher_t event_publisher;
    
    /**
     * 소켓 어댑터
     */
    extsock_socket_adapter_t *socket_adapter;
    
    /**
     * Failover Manager (새로 추가)
     */
    extsock_failover_manager_t *failover_manager;
};

/**
 * Failover Manager 설정
 */
void (*set_failover_manager)(extsock_event_usecase_t *this, 
                            extsock_failover_manager_t *failover_manager);
```

#### 1.3.3 IKE_DESTROYING 핸들러 추가
```c
// extsock_event_usecase.c 수정
METHOD(listener_t, ike_state_change, bool,
    private_extsock_event_usecase_t *this, ike_sa_t *ike_sa, ike_sa_state_t state)
{
    switch (state) {
        case IKE_DESTROYING:
            EXTSOCK_DBG(1, "IKE_DESTROYING detected for IKE SA '%s'", 
                       ike_sa->get_name(ike_sa));
            
            // Failover Manager를 통한 자동 전환
            if (this->failover_manager) {
                this->failover_manager->handle_connection_failure(
                    this->failover_manager, ike_sa);
            }
            break;
            
        case IKE_ESTABLISHED:
            // 기존 로직...
            break;
            
        // ... 기타 상태들 ...
    }
    
    return TRUE;
}

METHOD(extsock_event_usecase_t, set_failover_manager, void,
    private_extsock_event_usecase_t *this, extsock_failover_manager_t *failover_manager)
{
    this->failover_manager = failover_manager;
}
```

---

## 🔧 Phase 2: 핵심 Failover 로직 구현 (2-3주)

### 2.1 설정 정보 추출 및 검증

#### 2.1.1 메인 핸들러 구현
```c
METHOD(extsock_failover_manager_t, handle_connection_failure, void,
    private_extsock_failover_manager_t *this, ike_sa_t *ike_sa)
{
    EXTSOCK_DBG(1, "Handling connection failure for IKE SA '%s'", 
               ike_sa->get_name(ike_sa));
    
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
    const char *conn_name = ike_sa->get_name(ike_sa);
    if (is_max_retry_exceeded(this, conn_name)) {
        EXTSOCK_DBG(1, "Max retry count exceeded for connection '%s'", conn_name);
        return;
    }
    
    // 4. 다음 주소 선택
    char *next_addr = this->select_next_segw(this, remote_addrs, current_addr);
    if (!next_addr) {
        EXTSOCK_DBG(1, "No alternative SEGW available");
        return;
    }
    
    EXTSOCK_DBG(1, "Initiating failover: %s -> %s", current_addr, next_addr);
    
    // 5. Failover 수행
    extsock_error_t result = this->create_failover_config(this, peer_cfg, next_addr);
    if (result == EXTSOCK_SUCCESS) {
        EXTSOCK_DBG(1, "Failover to %s initiated successfully", next_addr);
        update_active_segw(this, conn_name, next_addr);
        increment_retry_count(this, conn_name);
    } else {
        EXTSOCK_DBG(1, "Failed to initiate failover to %s", next_addr);
    }
    
    free(next_addr);
}
```

#### 2.1.2 상태 추적 헬퍼 함수들
```c
/**
 * 최대 재시도 횟수 체크
 */
static bool is_max_retry_exceeded(private_extsock_failover_manager_t *this, const char *conn_name)
{
    int *count = this->retry_count_map->get(this->retry_count_map, conn_name);
    return count && (*count >= MAX_FAILOVER_RETRY);  // MAX_FAILOVER_RETRY = 5
}

/**
 * 현재 활성 SEGW 업데이트
 */
static void update_active_segw(private_extsock_failover_manager_t *this, 
                              const char *conn_name, const char *segw_addr)
{
    char *old_addr = this->active_segw_map->remove(this->active_segw_map, conn_name);
    free(old_addr);
    
    this->active_segw_map->put(this->active_segw_map, 
                              strdup(conn_name), strdup(segw_addr));
}

/**
 * 재시도 횟수 증가
 */
static void increment_retry_count(private_extsock_failover_manager_t *this, const char *conn_name)
{
    int *count = this->retry_count_map->get(this->retry_count_map, conn_name);
    if (count) {
        (*count)++;
    } else {
        int *new_count = malloc(sizeof(int));
        *new_count = 1;
        this->retry_count_map->put(this->retry_count_map, strdup(conn_name), new_count);
    }
}
```

### 2.2 새 설정 생성 로직

#### 2.2.1 Failover 설정 생성
```c
METHOD(extsock_failover_manager_t, create_failover_config, extsock_error_t,
    private_extsock_failover_manager_t *this, peer_cfg_t *original_cfg, const char *next_segw_addr)
{
    if (!original_cfg || !next_segw_addr) {
        return EXTSOCK_ERROR_INVALID_PARAMETER;
    }
    
    // 1. 기존 ike_cfg에서 설정 추출
    ike_cfg_t *original_ike_cfg = original_cfg->get_ike_cfg(original_cfg);
    
    // 2. 새 ike_cfg 생성 (주소만 변경)
    ike_cfg_create_t ike_data = {
        .version = original_ike_cfg->get_version(original_ike_cfg),
        .local = original_ike_cfg->get_my_addr(original_ike_cfg),
        .remote = next_segw_addr,  // 핵심: 다음 SEGW 주소로 변경
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
        .rekey_time = original_cfg->get_rekey_time(original_cfg),
        .reauth_time = original_cfg->get_reauth_time(original_cfg),
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
```

#### 2.2.2 설정 복사 헬퍼 함수들
```c
/**
 * IKE proposals 복사
 */
static void copy_ike_proposals(ike_cfg_t *src, ike_cfg_t *dst)
{
    enumerator_t *enumerator;
    proposal_t *proposal;
    
    enumerator = src->create_proposal_enumerator(src, PROTO_IKE);
    while (enumerator->enumerate(enumerator, &proposal)) {
        dst->add_proposal(dst, proposal->clone(proposal, 0));
    }
    enumerator->destroy(enumerator);
}

/**
 * Authentication configs 복사
 */
static bool copy_auth_configs(peer_cfg_t *src, peer_cfg_t *dst)
{
    enumerator_t *enumerator;
    auth_cfg_t *auth_cfg;
    
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
 * Child configs 복사
 */
static bool copy_child_configs(peer_cfg_t *src, peer_cfg_t *dst)
{
    enumerator_t *enumerator;
    child_cfg_t *child_cfg;
    
    enumerator = src->create_child_cfg_enumerator(src);
    while (enumerator->enumerate(enumerator, &child_cfg)) {
        dst->add_child_cfg(dst, child_cfg->get_ref(child_cfg));
    }
    enumerator->destroy(enumerator);
    
    return TRUE;
}
```

### 2.3 Config Usecase 확장

#### 2.3.1 새 메서드 추가
```c
// extsock_config_usecase.h에 추가
/**
 * Peer 설정 추가 및 즉시 연결 시도 (Failover용)
 */
extsock_error_t (*add_peer_config_and_initiate)(extsock_config_usecase_t *this, 
                                               peer_cfg_t *peer_cfg);
```

#### 2.3.2 구현
```c
// extsock_config_usecase.c에 추가
METHOD(extsock_config_usecase_t, add_peer_config_and_initiate, extsock_error_t,
    private_extsock_config_usecase_t *this, peer_cfg_t *peer_cfg)
{
    if (!peer_cfg) {
        return EXTSOCK_ERROR_INVALID_PARAMETER;
    }
    
    // 1. strongSwan에 설정 등록
    extsock_error_t result = this->strongswan_adapter->add_peer_config(
        this->strongswan_adapter, peer_cfg);
    
    if (result != EXTSOCK_SUCCESS) {
        EXTSOCK_DBG(1, "Failed to add peer config '%s'", peer_cfg->get_name(peer_cfg));
        return result;
    }
    
    // 2. 즉시 연결 시도
    result = this->strongswan_adapter->initiate_connection(
        this->strongswan_adapter, peer_cfg->get_name(peer_cfg));
    
    if (result == EXTSOCK_SUCCESS) {
        EXTSOCK_DBG(1, "Failover connection '%s' initiated successfully", 
                   peer_cfg->get_name(peer_cfg));
    } else {
        EXTSOCK_DBG(1, "Failed to initiate failover connection '%s'", 
                   peer_cfg->get_name(peer_cfg));
    }
    
    return result;
}
```

---

## 🔗 Phase 3: 의존성 주입 및 통합 (1주)

### 3.1 Plugin 레벨 통합

#### 3.1.1 DI 컨테이너 확장
```c
// extsock_plugin.c 수정
typedef struct {
    extsock_socket_adapter_t *socket_adapter;
    extsock_config_usecase_t *config_usecase;
    extsock_event_usecase_t *event_usecase;
    extsock_failover_manager_t *failover_manager;  // 새로 추가
    extsock_json_parser_t *json_parser;
    extsock_strongswan_adapter_t *strongswan_adapter;
} extsock_di_container_t;
```

#### 3.1.2 초기화 로직 수정
```c
/**
 * 의존성 주입 컨테이너 초기화
 */
static bool initialize_container(private_extsock_plugin_t *this)
{
    // 1. 기존 컴포넌트들 초기화 (순서 중요)
    this->container.json_parser = extsock_json_parser_create();
    if (!this->container.json_parser) {
        EXTSOCK_DBG(1, "Failed to create JSON parser");
        return FALSE;
    }
    
    this->container.strongswan_adapter = extsock_strongswan_adapter_create();
    if (!this->container.strongswan_adapter) {
        EXTSOCK_DBG(1, "Failed to create strongSwan adapter");
        return FALSE;
    }
    
    this->container.config_usecase = extsock_config_usecase_create(
        this->container.json_parser,
        this->container.strongswan_adapter
    );
    if (!this->container.config_usecase) {
        EXTSOCK_DBG(1, "Failed to create config usecase");
        return FALSE;
    }
    
    // 2. Failover Manager 생성 (Config Usecase 의존성)
    this->container.failover_manager = extsock_failover_manager_create(
        this->container.config_usecase
    );
    if (!this->container.failover_manager) {
        EXTSOCK_DBG(1, "Failed to create failover manager");
        return FALSE;
    }
    
    // 3. Event Usecase 생성 및 Failover Manager 주입
    this->container.event_usecase = extsock_event_usecase_create();
    if (!this->container.event_usecase) {
        EXTSOCK_DBG(1, "Failed to create event usecase");
        return FALSE;
    }
    
    this->container.event_usecase->set_failover_manager(
        this->container.event_usecase,
        this->container.failover_manager
    );
    
    // 4. Socket Adapter 생성
    this->container.socket_adapter = extsock_socket_adapter_create(
        this->container.config_usecase,
        this->container.event_usecase->get_event_publisher(this->container.event_usecase)
    );
    if (!this->container.socket_adapter) {
        EXTSOCK_DBG(1, "Failed to create socket adapter");
        return FALSE;
    }
    
    EXTSOCK_DBG(1, "DI container initialized successfully with failover support");
    return TRUE;
}
```

#### 3.1.3 정리 로직 수정
```c
/**
 * 의존성 주입 컨테이너 해제
 */
static void destroy_container(private_extsock_plugin_t *this)
{
    // 역순으로 해제
    if (this->container.socket_adapter) {
        this->container.socket_adapter->destroy(this->container.socket_adapter);
    }
    if (this->container.event_usecase) {
        this->container.event_usecase->destroy(this->container.event_usecase);
    }
    if (this->container.failover_manager) {  // 새로 추가
        this->container.failover_manager->destroy(this->container.failover_manager);
    }
    if (this->container.config_usecase) {
        this->container.config_usecase->destroy(this->container.config_usecase);
    }
    if (this->container.strongswan_adapter) {
        this->container.strongswan_adapter->destroy(this->container.strongswan_adapter);
    }
    if (this->container.json_parser) {
        this->container.json_parser->destroy(this->container.json_parser);
    }
}
```

### 3.2 빌드 시스템 업데이트

#### 3.2.1 Makefile 수정
```makefile
# src/libcharon/plugins/extsock/Makefile.am

libstrongswan_extsock_la_SOURCES = \
    extsock_plugin.h extsock_plugin.c \
    common/extsock_common.h \
    interfaces/extsock_config_usecase.h \
    interfaces/extsock_event_usecase.h \
    interfaces/extsock_failover_manager.h \
    interfaces/extsock_socket_adapter.h \
    interfaces/extsock_json_parser.h \
    interfaces/extsock_strongswan_adapter.h \
    interfaces/extsock_event_publisher.h \
    usecases/extsock_config_usecase.c \
    usecases/extsock_event_usecase.c \
    usecases/extsock_failover_manager.c \
    adapters/socket/extsock_socket_adapter.c \
    adapters/json/extsock_json_parser.c \
    adapters/strongswan/extsock_strongswan_adapter.c \
    domain/extsock_config_entity.c

libstrongswan_extsock_la_LDFLAGS = -module -avoid-version
libstrongswan_extsock_la_LIBADD = $(top_builddir)/src/libstrongswan/libstrongswan.la \
                                 $(top_builddir)/src/libcharon/libcharon.la \
                                 -lcjson
```

---

## 🧪 Phase 4: 테스트 및 검증 (1-2주)

### 4.1 단위 테스트

#### 4.1.1 테스트 파일 구조
```
tests/plugins/extsock/
├── test_failover_manager.c
├── test_address_parsing.c
├── test_config_creation.c
└── mocks/
    ├── mock_config_usecase.c
    ├── mock_ike_sa.c
    └── mock_peer_cfg.c
```

#### 4.1.2 핵심 테스트 케이스
```c
// test_address_parsing.c
#include <check.h>
#include "../../../src/libcharon/plugins/extsock/usecases/extsock_failover_manager.h"

START_TEST(test_select_next_segw_basic)
{
    extsock_failover_manager_t *manager = create_test_manager();
    
    // "10.0.0.1,10.0.0.2" → current: "10.0.0.1" → expected: "10.0.0.2"
    char *next = manager->select_next_segw(manager, "10.0.0.1,10.0.0.2", "10.0.0.1");
    
    ck_assert_str_eq(next, "10.0.0.2");
    
    free(next);
    manager->destroy(manager);
}
END_TEST

START_TEST(test_select_next_segw_circular)
{
    extsock_failover_manager_t *manager = create_test_manager();
    
    // "10.0.0.1,10.0.0.2" → current: "10.0.0.2" → expected: "10.0.0.1" (순환)
    char *next = manager->select_next_segw(manager, "10.0.0.1,10.0.0.2", "10.0.0.2");
    
    ck_assert_str_eq(next, "10.0.0.1");
    
    free(next);
    manager->destroy(manager);
}
END_TEST

START_TEST(test_select_next_segw_single_address)
{
    extsock_failover_manager_t *manager = create_test_manager();
    
    // 단일 주소는 failover 불가
    char *next = manager->select_next_segw(manager, "10.0.0.1", "10.0.0.1");
    
    ck_assert_ptr_null(next);
    
    manager->destroy(manager);
}
END_TEST

START_TEST(test_select_next_segw_three_addresses)
{
    extsock_failover_manager_t *manager = create_test_manager();
    
    // "10.0.0.1,10.0.0.2,10.0.0.3" → current: "10.0.0.2" → expected: "10.0.0.3"
    char *next = manager->select_next_segw(manager, "10.0.0.1,10.0.0.2,10.0.0.3", "10.0.0.2");
    
    ck_assert_str_eq(next, "10.0.0.3");
    
    free(next);
    manager->destroy(manager);
}
END_TEST

Suite *address_parsing_suite(void)
{
    Suite *s;
    TCase *tc_core;
    
    s = suite_create("Address Parsing");
    tc_core = tcase_create("Core");
    
    tcase_add_test(tc_core, test_select_next_segw_basic);
    tcase_add_test(tc_core, test_select_next_segw_circular);
    tcase_add_test(tc_core, test_select_next_segw_single_address);
    tcase_add_test(tc_core, test_select_next_segw_three_addresses);
    
    suite_add_tcase(s, tc_core);
    
    return s;
}
```

### 4.2 통합 테스트

#### 4.2.1 테스트 시나리오
```c
// test_failover_integration.c
START_TEST(test_basic_failover_scenario)
{
    // 1. Mock IKE SA 생성 (1st SEGW 실패 상황)
    ike_sa_t *mock_ike_sa = create_mock_ike_sa("10.0.0.1,10.0.0.2", "10.0.0.1");
    
    // 2. Failover Manager를 통한 처리
    extsock_failover_manager_t *manager = create_test_manager_with_mocks();
    manager->handle_connection_failure(manager, mock_ike_sa);
    
    // 3. Config Usecase Mock에서 2nd SEGW 연결 시도 확인
    ck_assert_int_eq(get_mock_config_call_count(), 1);
    ck_assert_str_eq(get_mock_config_last_address(), "10.0.0.2");
    
    cleanup_mocks();
}
END_TEST

START_TEST(test_circular_failover_scenario)
{
    // 1st → 2nd → 1st 순환 테스트
    ike_sa_t *mock_ike_sa1 = create_mock_ike_sa("10.0.0.1,10.0.0.2", "10.0.0.1");
    ike_sa_t *mock_ike_sa2 = create_mock_ike_sa("10.0.0.1,10.0.0.2", "10.0.0.2");
    
    extsock_failover_manager_t *manager = create_test_manager_with_mocks();
    
    // 1st → 2nd
    manager->handle_connection_failure(manager, mock_ike_sa1);
    ck_assert_str_eq(get_mock_config_last_address(), "10.0.0.2");
    
    // 2nd → 1st
    reset_mock_config();
    manager->handle_connection_failure(manager, mock_ike_sa2);
    ck_assert_str_eq(get_mock_config_last_address(), "10.0.0.1");
    
    cleanup_mocks();
}
END_TEST
```

### 4.3 실제 환경 테스트

#### 4.3.1 테스트 환경 구성
```bash
#!/bin/bash
# test_environment_setup.sh

# 1st SEGW 시뮬레이터 (실패하도록 설정)
docker run -d --name segw1 \
  -p 10.0.0.1:500:500/udp \
  --network test-network \
  strongswan-simulator:fail

# 2nd SEGW 시뮬레이터 (정상 동작)
docker run -d --name segw2 \
  -p 10.0.0.2:500:500/udp \
  --network test-network \
  strongswan-simulator:success

# extsock 테스트 클라이언트
docker run -d --name extsock-test \
  --network test-network \
  -v ./test-config:/etc/strongswan \
  strongswan-extsock:test
```

#### 4.3.2 테스트 설정 파일
```json
// test-config/connections.json
{
    "connections": {
        "test-failover": {
            "remote_addrs": "10.0.0.1,10.0.0.2",
            "local_auth": {
                "auth": "psk",
                "id": "client@test.com"
            },
            "remote_auth": {
                "auth": "psk",
                "id": "segw@test.com"
            },
            "children": {
                "test-child": {
                    "local_ts": "10.1.0.0/24",
                    "remote_ts": "10.2.0.0/24",
                    "esp_proposals": ["aes128-sha256-modp2048"]
                }
            }
        }
    },
    "secrets": {
        "ike": [
            {
                "id": "client@test.com",
                "secret": "test-psk-secret"
            }
        ]
    }
}
```

#### 4.3.3 자동화된 테스트 스크립트
```bash
#!/bin/bash
# automated_failover_test.sh

# 테스트 결과 추적
TEST_RESULTS=()

# 1. 초기 연결 시도 (1st SEGW 실패 예상)
echo "Testing initial connection failure and failover..."
timeout 30s ./trigger_connection.sh test-failover
if check_tunnel_established 10.0.0.2; then
    TEST_RESULTS+=("✅ Basic failover: PASS")
else
    TEST_RESULTS+=("❌ Basic failover: FAIL")
fi

# 2. 2nd SEGW 다운 후 1st SEGW 복구 테스트
echo "Testing circular failover..."
./shutdown_segw.sh 10.0.0.2
./restore_segw.sh 10.0.0.1
./trigger_dpd_timeout.sh

sleep 15
if check_tunnel_established 10.0.0.1; then
    TEST_RESULTS+=("✅ Circular failover: PASS")
else
    TEST_RESULTS+=("❌ Circular failover: FAIL")
fi

# 3. 결과 출력
echo "=== Test Results ==="
for result in "${TEST_RESULTS[@]}"; do
    echo "$result"
done
```

---

## ⚡ Phase 5: 고도화 및 최적화 (1-2주)

### 5.1 상태 추적 강화

#### 5.1.1 Failover 이력 로깅
```c
// extsock_failover_history.h
typedef struct {
    time_t timestamp;
    char connection_name[64];
    char from_addr[64];
    char to_addr[64];
    char reason[128];
    extsock_error_t result;
} failover_history_entry_t;

typedef struct extsock_failover_history_t {
    void (*add_entry)(extsock_failover_history_t *this, failover_history_entry_t *entry);
    linked_list_t* (*get_recent_entries)(extsock_failover_history_t *this, int count);
    void (*clear_old_entries)(extsock_failover_history_t *this, time_t before);
    void (*destroy)(extsock_failover_history_t *this);
} extsock_failover_history_t;
```

#### 5.1.2 고급 재시도 정책
```c
// 지수 백오프 적용
typedef struct {
    int base_delay;      // 기본 지연 시간 (초)
    int max_delay;       // 최대 지연 시간 (초)
    float multiplier;    // 지수 증가 배수
    int max_retries;     // 최대 재시도 횟수
} retry_policy_t;

static int calculate_retry_delay(retry_policy_t *policy, int attempt_count)
{
    int delay = policy->base_delay * pow(policy->multiplier, attempt_count);
    return min(delay, policy->max_delay);
}
```

### 5.2 에러 처리 강화

#### 5.2.1 상세 에러 코드 정의
```c
// extsock_common.h에 추가
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_INVALID_PARAMETER,
    EXTSOCK_ERROR_CONFIG_CREATION_FAILED,
    EXTSOCK_ERROR_NO_ALTERNATIVE_SEGW,
    EXTSOCK_ERROR_MAX_RETRY_EXCEEDED,
    EXTSOCK_ERROR_MEMORY_ALLOCATION_FAILED,
    EXTSOCK_ERROR_STRONGSWAN_API_FAILED,
    EXTSOCK_ERROR_PARSING_FAILED,
    EXTSOCK_ERROR_CONNECTION_TIMEOUT,
} extsock_error_t;

const char* extsock_error_to_string(extsock_error_t error);
```

#### 5.2.2 메모리 누수 방지
```c
// 자동 해제 매크로
#define AUTO_FREE __attribute__((cleanup(auto_free_cleanup)))

static inline void auto_free_cleanup(void *p) {
    void **pp = (void**)p;
    if (*pp) {
        free(*pp);
        *pp = NULL;
    }
}

// 사용 예시
METHOD(extsock_failover_manager_t, select_next_segw, char*,
    private_extsock_failover_manager_t *this, const char *remote_addrs, const char *current_addr)
{
    AUTO_FREE char *str_copy = strdup(remote_addrs);  // 자동 해제
    AUTO_FREE linked_list_t *addr_list = linked_list_create();
    
    // ... 로직 ...
    
    return result ? strdup(result) : NULL;  // 반환값만 수동 관리
}
```

### 5.3 성능 최적화

#### 5.3.1 주소 파싱 캐싱
```c
// 파싱 결과 캐싱
typedef struct {
    char *original_str;
    linked_list_t *parsed_addrs;
    time_t cache_time;
} address_cache_entry_t;

static hashtable_t *address_cache = NULL;
static const int CACHE_TTL = 300;  // 5분

static linked_list_t* get_cached_addresses(const char *addr_str)
{
    if (!address_cache) {
        address_cache = hashtable_create(hashtable_hash_str, hashtable_equals_str, 32);
    }
    
    address_cache_entry_t *entry = address_cache->get(address_cache, addr_str);
    if (entry && (time(NULL) - entry->cache_time) < CACHE_TTL) {
        return entry->parsed_addrs;  // 캐시 히트
    }
    
    return NULL;  // 캐시 미스
}
```

#### 5.3.2 설정 복사 최적화
```c
// 불필요한 설정 복사 방지 - 참조 카운팅 활용
static peer_cfg_t* create_lightweight_failover_config(peer_cfg_t *original, const char *new_addr)
{
    // 새 ike_cfg만 생성하고 나머지는 참조 공유
    ike_cfg_t *new_ike_cfg = create_ike_cfg_with_new_address(original, new_addr);
    
    // peer_cfg는 새로 생성하지만 child_cfg, auth_cfg는 참조 공유
    peer_cfg_t *new_peer_cfg = peer_cfg_create_minimal(new_ike_cfg);
    
    // 참조 카운트 증가로 공유
    share_auth_configs(original, new_peer_cfg);
    share_child_configs(original, new_peer_cfg);
    
    return new_peer_cfg;
}
```

---

## 📅 전체 일정표

| Phase | 작업 내용 | 기간 | 주요 산출물 | 담당자 |
|-------|----------|------|-------------|--------|
| **Phase 1** | **기반 인프라 구축** | **1-2주** | | |
| 1.1 | Failover Manager 인터페이스 | 2일 | extsock_failover_manager.h/c | 개발자 |
| 1.2 | 주소 파싱/선택 로직 | 3일 | select_next_segw 구현 | 개발자 |
| 1.3 | Event Usecase 확장 | 2일 | IKE_DESTROYING 핸들러 | 개발자 |
| **Phase 2** | **핵심 Failover 로직** | **2-3주** | | |
| 2.1 | 설정 정보 추출/검증 | 4일 | handle_connection_failure | 개발자 |
| 2.2 | 새 설정 생성 로직 | 5일 | create_failover_config | 개발자 |
| 2.3 | Config Usecase 연동 | 3일 | add_peer_config_and_initiate | 개발자 |
| **Phase 3** | **의존성 주입 및 통합** | **1주** | | |
| 3.1 | Plugin 레벨 통합 | 3일 | DI 컨테이너 수정 | 개발자 |
| 3.2 | 빌드 시스템 업데이트 | 2일 | Makefile 수정 | 개발자 |
| **Phase 4** | **테스트 및 검증** | **1-2주** | | |
| 4.1 | 단위 테스트 | 3일 | test_*.c 파일들 | 개발자 |
| 4.2 | 통합 테스트 | 3일 | 시나리오 테스트 | 개발자 + QA |
| 4.3 | 실제 환경 테스트 | 3일 | Docker 테스트 환경 | QA |
| **Phase 5** | **고도화 및 최적화** | **1-2주** | | |
| 5.1 | 상태 추적 강화 | 3일 | Failover History | 개발자 |
| 5.2 | 에러 처리 강화 | 2일 | 상세 에러 코드 | 개발자 |
| 5.3 | 성능 최적화 | 3일 | 캐싱, 참조 공유 | 개발자 |
| | | | | |
| **전체** | | **6-10주** | **완전한 2nd SEGW 지원** | |

---

## 🎯 성공 기준

### 기능적 요구사항
- ✅ **자동 Failover**: 1st SEGW 실패 시 자동으로 2nd SEGW 연결
- ✅ **순환 지원**: 2nd SEGW 실패 시 1st SEGW로 복귀
- ✅ **설정 호환성**: 기존 JSON 설정 형식과 완전 호환
- ✅ **무중단 운영**: 기존 연결에 영향 없이 새 연결만 failover

### 성능 요구사항
- ⏱️ **Failover 시간**: IKE_DESTROYING 감지 후 10초 이내 새 연결 시도
- 💾 **메모리 사용량**: 기존 대비 5% 이하 증가
- 🔄 **CPU 오버헤드**: 평상시 1% 이하, failover 시 5% 이하
- 📊 **처리량**: 기존 연결 처리 성능에 영향 없음

### 안정성 요구사항
- 🔒 **메모리 안전성**: Valgrind 검사 통과, 메모리 누수 없음
- ⚡ **장시간 운영**: 72시간 연속 동작 테스트 통과
- 🔄 **반복 테스트**: 1000회 failover 테스트 통과
- 🛡️ **에러 처리**: 모든 예외 상황에서 안전한 동작

### 품질 요구사항
- 📝 **코드 커버리지**: 단위 테스트 90% 이상
- 🧪 **테스트 통과**: 모든 단위/통합 테스트 100% 통과
- 📚 **문서화**: API 문서, 사용자 가이드 완비
- 🔍 **코드 리뷰**: 모든 코드 동료 리뷰 완료

---

## 🚨 위험 요소 및 대응 방안

### 기술적 위험

| 위험 요소 | 확률 | 영향도 | 대응 방안 | 담당자 |
|----------|------|--------|----------|--------|
| strongSwan API 비호환성 | 낮음 | 높음 | 버전별 호환성 테스트, 추상화 레이어 | 개발자 |
| IKE_DESTROYING 타이밍 이슈 | 낮음 | 높음 | 상세 타이밍 분석 완료, 안전성 검증 | 개발자 |
| 메모리 누수 | 중간 | 중간 | Valgrind 정기 검사, RAII 패턴 | 개발자 |
| 설정 복사 오버헤드 | 중간 | 낮음 | 참조 공유, 지연 복사 | 개발자 |

### 일정 위험

| 위험 요소 | 확률 | 영향도 | 대응 방안 | 담당자 |
|----------|------|--------|----------|--------|
| 복잡한 설정 복사 로직 | 중간 | 중간 | 단계별 구현, 조기 프로토타입 | PM |
| 테스트 환경 구축 지연 | 중간 | 낮음 | Docker 환경 미리 준비 | QA |
| strongSwan 소스 분석 시간 초과 | 낮음 | 중간 | 사전 분석 완료, 전문가 자문 | 개발자 |

### 운영 위험

| 위험 요소 | 확률 | 영향도 | 대응 방안 | 담당자 |
|----------|------|--------|----------|--------|
| 기존 설정과 충돌 | 낮음 | 높음 | 네임스페이스 분리, 철저한 테스트 | 개발자 |
| 무한 failover 루프 | 낮음 | 중간 | 재시도 횟수 제한, 백오프 정책 | 개발자 |
| 로그 폭증 | 중간 | 낮음 | 로그 레벨 조정, 순환 로그 | 개발자 |

---

## 📚 참고 자료

### strongSwan 핵심 분석
- **설정 관리**: `src/libcharon/config/ike_cfg.c:491-515` (쉼표 구분 주소 파싱)
- **IKE SA 생명주기**: `src/libcharon/sa/ike_sa.c:3053-3150` (destroy 메서드 타이밍)
- **Bus 시스템**: `src/libcharon/bus/bus.c:487-512` (동기 이벤트 전달)
- **설정 복사**: `src/libcharon/config/peer_cfg.c` (peer_cfg 관리)

### extsock 아키텍처
- **Clean Architecture**: 기존 레이어 구조 유지
- **의존성 주입**: Plugin 레벨 DI 컨테이너 활용
- **이벤트 처리**: 기존 Event Usecase 확장

### 개발 가이드라인
- **코딩 스타일**: strongSwan 프로젝트 스타일 준수
- **메모리 관리**: RAII 패턴, 자동 해제 매크로 활용
- **에러 처리**: 상세 에러 코드, 로그 레벨별 메시지
- **테스트**: TDD 접근, Mock 객체 활용

---

## 📞 연락처 및 리소스

### 개발팀
- **Lead Developer**: [이름] - 전체 아키텍처, 핵심 로직
- **Developer**: [이름] - 테스트, 문서화
- **QA Engineer**: [이름] - 통합 테스트, 성능 검증

### 외부 리소스
- **strongSwan 문서**: https://docs.strongswan.org/
- **strongSwan 소스**: https://github.com/strongswan/strongswan
- **C Check 테스트**: https://libcheck.github.io/check/

### 프로젝트 관리
- **이슈 트래킹**: [Jira/GitHub Issues URL]
- **코드 리뷰**: [Pull Request Process]
- **일일 스탠드업**: 매일 오전 9시
- **주간 리뷰**: 매주 금요일 오후 3시

---

이 구현 계획을 따라 단계별로 진행하면 안정적이고 효율적인 2nd SEGW 자동 failover 기능을 성공적으로 구현할 수 있습니다! 🚀 