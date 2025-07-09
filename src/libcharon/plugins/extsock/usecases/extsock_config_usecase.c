/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_config_usecase.h"
#include "../common/extsock_common.h"
#include "../adapters/strongswan/extsock_strongswan_adapter.h"
#include "../adapters/json/extsock_json_parser.h"
#include "extsock_event_usecase.h"

#include <cjson/cJSON.h>
#include <daemon.h>
#include <library.h>

// IKE lifetime 파싱 함수 선언
static void parse_ike_lifetime(cJSON *ike_json, peer_cfg_create_t *peer_cfg);

typedef struct private_extsock_config_usecase_t private_extsock_config_usecase_t;

/**
 * 설정 관리 유스케이스 내부 구조체
 */
struct private_extsock_config_usecase_t {
    
    /**
     * 공개 인터페이스
     */
    extsock_config_usecase_t public;
    
    /**
     * 명령 처리기 인터페이스 구현
     */
    extsock_command_handler_t command_handler;
    
    /**
     * JSON 파싱 어댑터
     */
    extsock_json_parser_t *json_parser;
    
    /**
     * 이벤트 발행자
     */
    extsock_event_publisher_t *event_publisher;
    
    /**
     * strongSwan 어댑터
     */
    extsock_strongswan_adapter_t *strongswan_adapter;
};

/**
 * IKE lifetime 설정 파싱
 */
static void parse_ike_lifetime(cJSON *ike_json, peer_cfg_create_t *peer_cfg)
{
    if (!ike_json) {
        EXTSOCK_DBG(2, "No IKE configuration found for lifetime parsing");
        return;
    }
    
    cJSON *j_lifetime = cJSON_GetObjectItem(ike_json, "lifetime");
    if (!j_lifetime) {
        EXTSOCK_DBG(2, "No IKE lifetime configuration found, using defaults");
        return;
    }
    
    cJSON *j_rekey = cJSON_GetObjectItem(j_lifetime, "rekey_time");
    if (j_rekey && cJSON_IsNumber(j_rekey)) {
        peer_cfg->rekey_time = j_rekey->valueint;
        EXTSOCK_DBG(1, "IKE rekey_time set to %u seconds", peer_cfg->rekey_time);
    }
    
    /*
    cJSON *j_reauth = cJSON_GetObjectItem(j_lifetime, "reauth_time");
    if (j_reauth && cJSON_IsNumber(j_reauth)) {
        peer_cfg->reauth_time = j_reauth->valueint;
        EXTSOCK_DBG(1, "IKE reauth_time set to %u seconds", peer_cfg->reauth_time);
    }
    
    cJSON *j_over = cJSON_GetObjectItem(j_lifetime, "over_time");
    if (j_over && cJSON_IsNumber(j_over)) {
        peer_cfg->over_time = j_over->valueint;
        EXTSOCK_DBG(1, "IKE over_time set to %u seconds", peer_cfg->over_time);
    }
    
    cJSON *j_jitter = cJSON_GetObjectItem(j_lifetime, "jitter_time");
    if (j_jitter && cJSON_IsNumber(j_jitter)) {
        peer_cfg->jitter_time = j_jitter->valueint;
        EXTSOCK_DBG(1, "IKE jitter_time set to %u seconds", peer_cfg->jitter_time);
    }
    */
}

/**
 * 단일 연결 처리 함수 (기존 로직 분리)
 */
static extsock_error_t process_single_connection(private_extsock_config_usecase_t *this, 
    cJSON *connection_json, const char *conn_name_str)
{
    // IKE 설정 파싱
    cJSON *j_ike_cfg = cJSON_GetObjectItem(connection_json, "ike_cfg");
    ike_cfg_t *ike_cfg = this->json_parser->parse_ike_config(this->json_parser, j_ike_cfg);
    if (!ike_cfg) {
        EXTSOCK_DBG(1, "apply_json_config: Failed to parse ike_cfg section for %s", conn_name_str);
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }

    peer_cfg_create_t peer_create_cfg = {0};
    
    // MOBIKE 설정 파싱 (기본값: 비활성화)
    peer_create_cfg.options = OPT_NO_MOBIKE;  // 기본값으로 MOBIKE 비활성화
    
    cJSON *j_mobike = cJSON_GetObjectItem(connection_json, "mobike");
    if (j_mobike && cJSON_IsBool(j_mobike)) {
        if (cJSON_IsTrue(j_mobike)) {
            peer_create_cfg.options = 0;  // MOBIKE 활성화 (플래그 제거)
            EXTSOCK_DBG(2, "MOBIKE enabled for connection: %s", conn_name_str);
        } else {
            peer_create_cfg.options = OPT_NO_MOBIKE;  // MOBIKE 비활성화
            EXTSOCK_DBG(2, "MOBIKE disabled for connection: %s", conn_name_str);
        }
    } else {
        EXTSOCK_DBG(2, "MOBIKE not specified, using default (disabled) for connection: %s", conn_name_str);
    }
    
    // IKE lifetime 설정 파싱
    parse_ike_lifetime(j_ike_cfg, &peer_create_cfg);
    
    peer_cfg_t *peer_cfg = peer_cfg_create((char*)conn_name_str, ike_cfg, &peer_create_cfg);
    if (!peer_cfg) {
        EXTSOCK_DBG(1, "apply_json_config: Failed to create peer_cfg for %s", conn_name_str);
        ike_cfg->destroy(ike_cfg);
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }

    // 로컬 인증 설정 파싱 및 추가
    cJSON *j_local_auth = cJSON_GetObjectItem(connection_json, "local_auth");
    if (j_local_auth) {
        auth_cfg_t *local_auth_cfg = this->json_parser->parse_auth_config(this->json_parser, j_local_auth, TRUE);
        if (local_auth_cfg) {
            peer_cfg->add_auth_cfg(peer_cfg, local_auth_cfg, TRUE);
        }
    } else {
        auth_cfg_t *default_local_auth = auth_cfg_create();
        default_local_auth->add(default_local_auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_ANY);
        peer_cfg->add_auth_cfg(peer_cfg, default_local_auth, TRUE);
    }

    // 원격 인증 설정 파싱 및 추가
    cJSON *j_remote_auth = cJSON_GetObjectItem(connection_json, "remote_auth");
    if (j_remote_auth) {
        auth_cfg_t *remote_auth_cfg = this->json_parser->parse_auth_config(this->json_parser, j_remote_auth, FALSE);
        if (remote_auth_cfg) {
            peer_cfg->add_auth_cfg(peer_cfg, remote_auth_cfg, FALSE);
        }
    } else {
        auth_cfg_t *default_remote_auth = auth_cfg_create();
        default_remote_auth->add(default_remote_auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_ANY);
        peer_cfg->add_auth_cfg(peer_cfg, default_remote_auth, FALSE);
    }

    // 자식 SA 설정 파싱 및 추가
    cJSON *j_children = cJSON_GetObjectItem(connection_json, "children");
    if (!this->json_parser->parse_child_configs(this->json_parser, peer_cfg, j_children)) {
        EXTSOCK_DBG(1, "apply_json_config: Error processing children for %s", conn_name_str);
    }

    EXTSOCK_DBG(1, "Successfully parsed peer_cfg '%s' from JSON.", peer_cfg->get_name(peer_cfg));

    // strongSwan에 피어 설정 추가
    extsock_error_t result = this->strongswan_adapter->add_peer_config(this->strongswan_adapter, peer_cfg);
    
    if (result == EXTSOCK_SUCCESS) {
        // HIGH PRIORITY: 버퍼 오버플로우 방지
        if (this->event_publisher) {
            char event_json[512];  // 버퍼 크기 증가
            size_t name_len = EXTSOCK_SAFE_STRLEN(conn_name_str);
            if (name_len > 400) {  // 안전한 길이 체크 (여유 공간 고려)
                EXTSOCK_DBG(1, "Connection name too long for event JSON, truncating");
                char safe_name[401];
                EXTSOCK_SAFE_STRNCPY(safe_name, conn_name_str, sizeof(safe_name));
                EXTSOCK_SAFE_SNPRINTF(event_json, sizeof(event_json),
                    "{\"event\":\"config_applied\",\"connection\":\"%s\"}", safe_name);
            } else {
                EXTSOCK_SAFE_SNPRINTF(event_json, sizeof(event_json),
                    "{\"event\":\"config_applied\",\"connection\":\"%s\"}", conn_name_str);
            }
            this->event_publisher->publish_event(this->event_publisher, event_json);
        }
    }
    
    return result;
}

METHOD(extsock_config_usecase_t, apply_json_config, extsock_error_t,
    private_extsock_config_usecase_t *this, const char *config_json)
{
    if (!config_json) {
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    EXTSOCK_DBG(1, "apply_json_config: received config: %s", config_json);
    
    cJSON *root = cJSON_Parse(config_json);
    if (!root) {
        EXTSOCK_DBG(1, "apply_json_config: Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return EXTSOCK_ERROR_JSON_PARSE;
    }

    extsock_error_t result = EXTSOCK_SUCCESS;
    
    // 새로운 connections 배열 형식 확인
    cJSON *j_connections = cJSON_GetObjectItem(root, "connections");
    if (j_connections && cJSON_IsArray(j_connections)) {
        // 새로운 connections 배열 방식 처리
        EXTSOCK_DBG(1, "apply_json_config: Processing connections array format");
        
        cJSON *connection_json;
        cJSON_ArrayForEach(connection_json, j_connections) {
            if (!cJSON_IsObject(connection_json)) {
                EXTSOCK_DBG(1, "apply_json_config: Invalid connection object in array");
                continue;
            }
            
            // 연결 이름 파싱
            cJSON *j_conn_name = cJSON_GetObjectItem(connection_json, "name");
            if (!j_conn_name || !cJSON_IsString(j_conn_name) || !j_conn_name->valuestring) {
                EXTSOCK_DBG(1, "apply_json_config: Missing connection 'name' in connections array");
                continue;
            }
            const char *conn_name_str = j_conn_name->valuestring;
            
            // 단일 연결 처리
            extsock_error_t single_result = process_single_connection(this, connection_json, conn_name_str);
            if (single_result != EXTSOCK_SUCCESS) {
                EXTSOCK_DBG(1, "apply_json_config: Failed to process connection '%s'", conn_name_str);
                result = single_result; // 마지막 에러를 기록하되 계속 진행
            }
        }
    } else {
        // 기존 단일 연결 방식 (하위 호환성)
        EXTSOCK_DBG(1, "apply_json_config: Processing legacy single connection format");
        
        // 연결 이름 파싱
        cJSON *j_conn_name = cJSON_GetObjectItem(root, "name");
        if (!j_conn_name || !cJSON_IsString(j_conn_name) || !j_conn_name->valuestring) {
            EXTSOCK_DBG(1, "apply_json_config: Missing connection 'name' in JSON");
            cJSON_Delete(root);
            return EXTSOCK_ERROR_CONFIG_INVALID;
        }
        const char *conn_name_str = j_conn_name->valuestring;
        
        // 단일 연결 처리
        result = process_single_connection(this, root, conn_name_str);
    }
    
    cJSON_Delete(root);
    return result;
}

METHOD(extsock_config_usecase_t, remove_config, extsock_error_t,
    private_extsock_config_usecase_t *this, const char *name)
{
    if (!name) {
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    return this->strongswan_adapter->remove_peer_config(this->strongswan_adapter, name);
}

METHOD(extsock_config_usecase_t, start_dpd, extsock_error_t,
    private_extsock_config_usecase_t *this, const char *ike_sa_name)
{
    if (!ike_sa_name) {
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    return this->strongswan_adapter->config_repository.start_dpd(
        &this->strongswan_adapter->config_repository, ike_sa_name);
}



/**
 * 외부 명령 처리 (기존 handle_external_command 함수에서 이동)
 */
METHOD(extsock_command_handler_t, handle_command, extsock_error_t,
    extsock_config_usecase_t *config_usecase, const char *command)
{
    /*
     * 타입 캐스팅 설명:
     * - config_usecase는 extsock_config_usecase_t* (공개 인터페이스)
     * - private_extsock_config_usecase_t의 첫 번째 멤버가 public이므로
     * - C 표준에 의해 구조체 시작 주소 = 첫 번째 멤버 주소
     * - 따라서 안전하게 private 구조체로 캐스팅 가능
     * - offsetof(private_extsock_config_usecase_t, public) == 0
     */
    private_extsock_config_usecase_t *this = (private_extsock_config_usecase_t *)config_usecase;
    
    if (!command) {
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    
    EXTSOCK_DBG(2, "Processing external command: %s", command);
    
    // "START_DPD <ike_sa_name>" 명령 처리
    if (strncmp(command, "START_DPD ", 10) == 0) {
        return this->public.start_dpd(&this->public, command + 10);
    }
    // "APPLY_CONFIG <json_config>" 명령 처리
    else if (strncmp(command, "APPLY_CONFIG ", 13) == 0) {
        return this->public.apply_json_config(&this->public, command + 13);
    }
    // "REMOVE_CONFIG <name>" 명령 처리
    else if (strncmp(command, "REMOVE_CONFIG ", 14) == 0) {
        return this->public.remove_config(&this->public, command + 14);
    }
    else {
        EXTSOCK_DBG(1, "Unknown command: %s", command);
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
}

METHOD(extsock_command_handler_t, handle_config_command, extsock_error_t,
    extsock_config_usecase_t *config_usecase, const char *config_json)
{
    /*
     * 타입 캐스팅: config_usecase를 private 구조체로 안전하게 변환
     * (첫 번째 멤버 public의 주소가 구조체 시작 주소와 동일)
     */
    private_extsock_config_usecase_t *this = (private_extsock_config_usecase_t *)config_usecase;
    
    return this->public.apply_json_config(&this->public, config_json);
}

METHOD(extsock_command_handler_t, handle_dpd_command, extsock_error_t,
    extsock_config_usecase_t *config_usecase, const char *ike_sa_name)
{
    /*
     * 타입 캐스팅: config_usecase를 private 구조체로 안전하게 변환
     * (첫 번째 멤버 public의 주소가 구조체 시작 주소와 동일)
     */
    private_extsock_config_usecase_t *this = (private_extsock_config_usecase_t *)config_usecase;
    
    return this->public.start_dpd(&this->public, ike_sa_name);
}

METHOD(extsock_command_handler_t, destroy_handler, void,
    extsock_config_usecase_t *config_usecase)
{
    /*
     * 타입 캐스팅: config_usecase를 private 구조체로 안전하게 변환
     * 명령 처리기는 유스케이스의 일부이므로 별도 해제 불필요
     * config_usecase는 이미 존재하는 인스턴스이므로 여기서 해제하지 않음
     */
    // private_extsock_config_usecase_t *this = (private_extsock_config_usecase_t *)config_usecase;
    
    // 실제로는 아무것도 할 필요 없음 - 유스케이스가 전체적으로 해제될 때 처리됨
}

METHOD(extsock_config_usecase_t, get_command_handler, extsock_command_handler_t *,
    private_extsock_config_usecase_t *this)
{
    return &this->command_handler;
}

METHOD(extsock_config_usecase_t, destroy, void,
    private_extsock_config_usecase_t *this)
{
    if (this->strongswan_adapter) {
        this->strongswan_adapter->destroy(this->strongswan_adapter);
    }
    free(this);
}

/**
 * 설정 관리 유스케이스 생성
 */
extsock_config_usecase_t *extsock_config_usecase_create(
    extsock_json_parser_t *json_parser,
    extsock_event_usecase_t *event_usecase)
{
    private_extsock_config_usecase_t *this;

    INIT(this,
        .public = {
            .apply_json_config = _apply_json_config,
            .remove_config = _remove_config,
            .start_dpd = _start_dpd,
            .get_command_handler = _get_command_handler,
            .destroy = _destroy,
        },
        .command_handler = {
            .handle_command = _handle_command,
            .handle_config_command = _handle_config_command,
            .handle_dpd_command = _handle_dpd_command,
            .destroy = _destroy_handler,
        },
        .json_parser = json_parser,
        .event_publisher = event_usecase ? event_usecase->get_event_publisher(event_usecase) : NULL,
        .strongswan_adapter = extsock_strongswan_adapter_create(),
    );

    return &this->public;
} 