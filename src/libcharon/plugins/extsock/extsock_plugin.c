/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_plugin.h"
#include "common/extsock_common.h"
#include "adapters/json/extsock_json_parser.h"
#include "adapters/socket/extsock_socket_adapter.h"
#include "usecases/extsock_config_usecase.h"
#include "usecases/extsock_event_usecase.h"

#include <daemon.h>
#include <threading/thread.h>

typedef struct private_extsock_plugin_t private_extsock_plugin_t;

/**
 * 의존성 주입 컨테이너
 */
typedef struct extsock_di_container_t {
    extsock_json_parser_t *json_parser;
    extsock_socket_adapter_t *socket_adapter;
    extsock_config_usecase_t *config_usecase;
    extsock_event_usecase_t *event_usecase;
} extsock_di_container_t;

/**
 * extsock 플러그인 내부 구조체
 */
struct private_extsock_plugin_t {
    
    /**
     * 공개 인터페이스
     */
    extsock_plugin_t public;
    
    /**
     * 의존성 주입 컨테이너
     */
    extsock_di_container_t container;
    
    /**
     * 소켓 스레드
     */
    thread_t *socket_thread;
};

/**
 * 의존성 주입 컨테이너 초기화
 */
static bool initialize_container(private_extsock_plugin_t *this)
{
    // JSON 파서 생성
    this->container.json_parser = extsock_json_parser_create();
    if (!this->container.json_parser) {
        EXTSOCK_DBG(1, "Failed to create JSON parser");
        return FALSE;
    }
    
    // 이벤트 유스케이스 생성
    this->container.event_usecase = extsock_event_usecase_create();
    if (!this->container.event_usecase) {
        EXTSOCK_DBG(1, "Failed to create event usecase");
        return FALSE;
    }
    
    // 설정 유스케이스 생성 (JSON 파서와 이벤트 발행자 주입)
    this->container.config_usecase = extsock_config_usecase_create(
        this->container.json_parser,
        this->container.event_usecase->get_event_publisher(this->container.event_usecase)
    );
    if (!this->container.config_usecase) {
        EXTSOCK_DBG(1, "Failed to create config usecase");
        return FALSE;
    }
    
    // 소켓 어댑터 생성 (명령 처리기 주입)
    this->container.socket_adapter = extsock_socket_adapter_create(
        this->container.config_usecase
    );
    if (!this->container.socket_adapter) {
        EXTSOCK_DBG(1, "Failed to create socket adapter");
        return FALSE;
    }
    
    // 이벤트 유스케이스에 소켓 어댑터 주입 (순환 참조 해결)
    this->container.event_usecase->set_socket_adapter(
        this->container.event_usecase, this->container.socket_adapter);
    
    EXTSOCK_DBG(1, "Dependency injection container initialized successfully");
    return TRUE;
}

/**
 * 의존성 주입 컨테이너 해제
 */
static void destroy_container(private_extsock_plugin_t *this)
{
    if (this->container.socket_adapter) {
        this->container.socket_adapter->destroy(this->container.socket_adapter);
    }
    if (this->container.config_usecase) {
        this->container.config_usecase->destroy(this->container.config_usecase);
    }
    if (this->container.event_usecase) {
        this->container.event_usecase->destroy(this->container.event_usecase);
    }
    if (this->container.json_parser) {
        this->container.json_parser->destroy(this->container.json_parser);
    }
}

METHOD(plugin_t, get_name, char*,
    private_extsock_plugin_t *this)
{
    return "extsock";
}

METHOD(plugin_t, get_features, int,
    private_extsock_plugin_t *this, plugin_feature_t *features[])
{
    static plugin_feature_t f[] = {
        PLUGIN_NOOP,
            PLUGIN_PROVIDE(CUSTOM, "extsock"),
    };
    *features = f;
    return countof(f);
}

METHOD(plugin_t, destroy, void,
    private_extsock_plugin_t *this)
{
    // 소켓 스레드 정리
    if (this->socket_thread) {
        this->socket_thread->cancel(this->socket_thread);
        this->socket_thread->join(this->socket_thread);
    }
    
    // 컨테이너 해제
    destroy_container(this);
    
    free(this);
    EXTSOCK_DBG(1, "extsock plugin destroyed");
}

/**
 * extsock 플러그인 생성
 */
plugin_t *extsock_plugin_create()
{
    private_extsock_plugin_t *this;

    INIT(this,
        .public = {
            .plugin = {
                .get_name = _get_name,
                .get_features = _get_features,
                .destroy = _destroy,
            },
        },
    );

    EXTSOCK_DBG(1, "extsock plugin starting...");
    
    // 의존성 주입 컨테이너 초기화
    if (!initialize_container(this)) {
        EXTSOCK_DBG(1, "Failed to initialize dependency container");
        destroy_container(this);
        free(this);
        return NULL;
    }
    
    // 소켓 스레드 시작
    this->socket_thread = this->container.socket_adapter->start_listening(this->container.socket_adapter);
    if (!this->socket_thread) {
        EXTSOCK_DBG(1, "Failed to start socket thread");
        destroy_container(this);
        free(this);
        return NULL;
    }
    
    EXTSOCK_DBG(1, "extsock plugin initialized successfully");
    return &this->public.plugin;
} 