/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_config_usecase.h"
#include "../common/extsock_common.h"
#include "../adapters/strongswan/extsock_strongswan_adapter.h"
#include "../adapters/json/extsock_json_parser.h"

#include <cjson/cJSON.h>
#include <daemon.h>
#include <library.h>

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
 * JSON 설정 적용 (기존 apply_ipsec_config 함수에서 이동)
 */
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

    // 연결 이름 파싱
    cJSON *j_conn_name = cJSON_GetObjectItem(root, "name");
    if (!j_conn_name || !cJSON_IsString(j_conn_name) || !j_conn_name->valuestring) {
        EXTSOCK_DBG(1, "apply_json_config: Missing connection 'name' in JSON");
        cJSON_Delete(root);
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }
    const char *conn_name_str = j_conn_name->valuestring;

    // IKE 설정 파싱
    cJSON *j_ike_cfg = cJSON_GetObjectItem(root, "ike_cfg");
    ike_cfg_t *ike_cfg = this->json_parser->parse_ike_config(this->json_parser, j_ike_cfg);
    if (!ike_cfg) {
        EXTSOCK_DBG(1, "apply_json_config: Failed to parse ike_cfg section for %s", conn_name_str);
        cJSON_Delete(root);
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }

    peer_cfg_create_t peer_create_cfg = {0};
    peer_cfg_t *peer_cfg = peer_cfg_create((char*)conn_name_str, ike_cfg, &peer_create_cfg);
    if (!peer_cfg) {
        EXTSOCK_DBG(1, "apply_json_config: Failed to create peer_cfg for %s", conn_name_str);
        ike_cfg->destroy(ike_cfg);
        cJSON_Delete(root);
        return EXTSOCK_ERROR_CONFIG_INVALID;
    }

    // 로컬 인증 설정 파싱 및 추가
    cJSON *j_local_auth = cJSON_GetObjectItem(root, "local_auth");
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
    cJSON *j_remote_auth = cJSON_GetObjectItem(root, "remote_auth");
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
    cJSON *j_children = cJSON_GetObjectItem(root, "children");
    if (!this->json_parser->parse_child_configs(this->json_parser, peer_cfg, j_children)) {
        EXTSOCK_DBG(1, "apply_json_config: Error processing children for %s", conn_name_str);
    }

    EXTSOCK_DBG(1, "Successfully parsed peer_cfg '%s' from JSON.", peer_cfg->get_name(peer_cfg));

    // strongSwan에 피어 설정 추가
    extsock_error_t result = this->strongswan_adapter->add_peer_config(this->strongswan_adapter, peer_cfg);
    cJSON_Delete(root);
    
    if (result == EXTSOCK_SUCCESS) {
        // 성공 이벤트 발행
        if (this->event_publisher) {
            char event_json[256];
            snprintf(event_json, sizeof(event_json),
                "{\"event\":\"config_applied\",\"connection\":\"%s\"}", conn_name_str);
            this->event_publisher->publish_event(this->event_publisher, event_json);
        }
    }
    
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
    private_extsock_config_usecase_t *this, const char *command)
{
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
    private_extsock_config_usecase_t *this, const char *config_json)
{
    return this->public.apply_json_config(&this->public, config_json);
}

METHOD(extsock_command_handler_t, handle_dpd_command, extsock_error_t,
    private_extsock_config_usecase_t *this, const char *ike_sa_name)
{
    return this->public.start_dpd(&this->public, ike_sa_name);
}

METHOD(extsock_command_handler_t, destroy_handler, void,
    private_extsock_config_usecase_t *this)
{
    // 명령 처리기는 유스케이스의 일부이므로 별도 해제 불필요
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
    extsock_event_publisher_t *event_publisher)
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
        .event_publisher = event_publisher,
        .strongswan_adapter = extsock_strongswan_adapter_create(),
    );

    return &this->public;
} 