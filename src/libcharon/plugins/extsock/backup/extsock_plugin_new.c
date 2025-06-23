/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Modularized extsock plugin - Clean Architecture implementation
 */

#include "extsock_plugin.h"
#include "common/extsock_common.h"
#include "adapters/socket/extsock_socket_adapter.h"
#include "adapters/json/extsock_json_parser.h"
#include "usecases/extsock_config_usecase.h"
#include "usecases/extsock_event_usecase.h"

#include <daemon.h>
#include <library.h>
#include <bus/listeners/listener.h>

typedef struct private_extsock_plugin_t private_extsock_plugin_t;

/**
 * 모듈화된 extsock 플러그인 내부 구조체
 */
struct private_extsock_plugin_t {
    
    /**
     * 공개 플러그인 인터페이스
     */
    plugin_t public;
    
    /**
     * 어댑터들 (Infrastructure Layer)
     */
    extsock_socket_adapter_t *socket_adapter;
    extsock_json_parser_t *json_parser;
    
    /**
     * 유스케이스들 (Application Layer)
     */
    extsock_config_usecase_t *config_usecase;
    extsock_event_usecase_t *event_usecase;
    
    /**
     * 플러그인 상태
     */
    bool initialized;
};

METHOD(plugin_t, get_name, char*,
    private_extsock_plugin_t *this)
{
    return "extsock";
}

METHOD(plugin_t, get_features, int,
    private_extsock_plugin_t *this, plugin_feature_t *features[])
{
    *features = NULL;
    return 0;
}

METHOD(plugin_t, destroy, void,
    private_extsock_plugin_t *this)
{
    if (!this->initialized) {
        free(this);
        return;
    }
    
    EXTSOCK_DBG(1, "Shutting down extsock plugin");
    
    // 유스케이스 정리
    EXTSOCK_SAFE_DESTROY(this->event_usecase);
    EXTSOCK_SAFE_DESTROY(this->config_usecase);
    
    // 어댑터 정리
    if (this->socket_adapter) {
        this->socket_adapter->stop_listener(this->socket_adapter);
        EXTSOCK_SAFE_DESTROY(this->socket_adapter);
    }
    EXTSOCK_SAFE_DESTROY(this->json_parser);
    
    EXTSOCK_DBG(1, "extsock plugin unloaded successfully");
    free(this);
}

/**
 * 의존성 주입을 통한 컴포넌트 초기화
 */
static bool initialize_components(private_extsock_plugin_t *this)
{
    // 1. 어댑터 생성 (Infrastructure Layer)
    this->socket_adapter = extsock_socket_adapter_create(SOCKET_PATH);
    if (!this->socket_adapter) {
        EXTSOCK_DBG(1, "Failed to create socket adapter");
        return FALSE;
    }
    
    this->json_parser = extsock_json_parser_create();
    if (!this->json_parser) {
        EXTSOCK_DBG(1, "Failed to create JSON parser");
        return FALSE;
    }
    
    // 2. 유스케이스 생성 (Application Layer) - 의존성 주입
    this->config_usecase = extsock_config_usecase_create(
        this->json_parser,
        &this->socket_adapter->event_publisher);
    if (!this->config_usecase) {
        EXTSOCK_DBG(1, "Failed to create config usecase");
        return FALSE;
    }
    
    this->event_usecase = extsock_event_usecase_create(
        &this->socket_adapter->event_publisher);
    if (!this->event_usecase) {
        EXTSOCK_DBG(1, "Failed to create event usecase");
        return FALSE;
    }
    
    // 3. 이벤트 리스너 등록
    listener_t *event_listener = this->event_usecase->get_listener(this->event_usecase);
    if (event_listener) {
        charon->bus->add_listener(charon->bus, event_listener);
    }
    
    // 4. 소켓 리스너 시작
    extsock_command_handler_t *command_handler = this->config_usecase->get_command_handler(this->config_usecase);
    if (this->socket_adapter->start_listener(this->socket_adapter, command_handler) != EXTSOCK_SUCCESS) {
        EXTSOCK_DBG(1, "Failed to start socket listener");
        return FALSE;
    }
    
    return TRUE;
}

/**
 * extsock 플러그인 생성 - Clean Architecture 진입점
 */
plugin_t* extsock_plugin_create()
{
    private_extsock_plugin_t *this;
    
    INIT(this,
        .public = {
            .get_name = _get_name,
            .get_features = _get_features,
            .destroy = _destroy,
        },
        .initialized = FALSE,
    );
    
    // 의존성 주입을 통한 컴포넌트 초기화
    if (!initialize_components(this)) {
        EXTSOCK_DBG(1, "Failed to initialize extsock plugin components");
        this->public.destroy(&this->public);
        return NULL;
    }
    
    this->initialized = TRUE;
    EXTSOCK_DBG(1, "extsock plugin loaded successfully with modular architecture");
    
    return &this->public;
} 