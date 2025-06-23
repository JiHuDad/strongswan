/*
 * Copyright (C) 2024 strongSwan Project
 * Modularized extsock plugin - Clean Architecture
 */

#include "extsock_plugin.h"
#include "common/extsock_common.h"
#include "adapters/socket/extsock_socket_adapter.h"
#include "adapters/json/extsock_json_parser.h"
#include "usecases/extsock_config_usecase.h"
#include "usecases/extsock_event_usecase.h"

#include <daemon.h>
#include <library.h>

typedef struct private_extsock_plugin_t private_extsock_plugin_t;

/**
 * 모듈화된 extsock 플러그인 내부 구조체
 */
struct private_extsock_plugin_t {
    plugin_t public;                        // 공개 인터페이스
    extsock_socket_adapter_t *socket_adapter;
    extsock_json_parser_t *json_parser;
    extsock_config_usecase_t *config_usecase;
    extsock_event_usecase_t *event_usecase;
    bool initialized;
};

METHOD(plugin_t, get_name, char*, private_extsock_plugin_t *this)
{
    return "extsock";
}

METHOD(plugin_t, get_features, int, private_extsock_plugin_t *this, plugin_feature_t *features[])
{
    *features = NULL;
    return 0;
}

METHOD(plugin_t, destroy, void, private_extsock_plugin_t *this)
{
    if (!this->initialized) {
        free(this);
        return;
    }
    
    EXTSOCK_DBG(1, "Shutting down extsock plugin");
    
    // 정리 작업 (역순으로)
    EXTSOCK_SAFE_DESTROY(this->event_usecase);
    EXTSOCK_SAFE_DESTROY(this->config_usecase);
    
    if (this->socket_adapter) {
        this->socket_adapter->stop_listener(this->socket_adapter);
        EXTSOCK_SAFE_DESTROY(this->socket_adapter);
    }
    EXTSOCK_SAFE_DESTROY(this->json_parser);
    
    EXTSOCK_DBG(1, "extsock plugin unloaded successfully");
    free(this);
}

/**
 * 컴포넌트 초기화 (의존성 주입)
 */
static bool initialize_components(private_extsock_plugin_t *this)
{
    // Infrastructure Layer
    this->socket_adapter = extsock_socket_adapter_create(SOCKET_PATH);
    this->json_parser = extsock_json_parser_create();
    if (!this->socket_adapter || !this->json_parser) {
        EXTSOCK_DBG(1, "Failed to create adapters");
        return FALSE;
    }
    
    // Application Layer
    this->config_usecase = extsock_config_usecase_create(
        this->json_parser, &this->socket_adapter->event_publisher);
    this->event_usecase = extsock_event_usecase_create(
        &this->socket_adapter->event_publisher);
    if (!this->config_usecase || !this->event_usecase) {
        EXTSOCK_DBG(1, "Failed to create use cases");
        return FALSE;
    }
    
    // 이벤트 리스너 등록
    listener_t *listener = this->event_usecase->get_listener(this->event_usecase);
    if (listener) {
        charon->bus->add_listener(charon->bus, listener);
    }
    
    // 소켓 리스너 시작
    extsock_command_handler_t *handler = this->config_usecase->get_command_handler(this->config_usecase);
    if (this->socket_adapter->start_listener(this->socket_adapter, handler) != EXTSOCK_SUCCESS) {
        EXTSOCK_DBG(1, "Failed to start socket listener");
        return FALSE;
    }
    
    return TRUE;
}

/**
 * extsock 플러그인 생성
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
    
    if (!initialize_components(this)) {
        EXTSOCK_DBG(1, "Failed to initialize extsock plugin");
        this->public.destroy(&this->public);
        return NULL;
    }
    
    this->initialized = TRUE;
    EXTSOCK_DBG(1, "extsock plugin loaded with modular architecture");
    
    return &this->public;
} 