/*
 * Mock implementations for extsock usecases
 * For testing purposes only
 */

#include "../../usecases/extsock_config_usecase.h"
#include "../../usecases/extsock_event_usecase.h"
#include "../../interfaces/extsock_failover_manager.h"
#include "../../adapters/socket/extsock_socket_adapter.h"
#include "../../common/extsock_common.h"

// Mock Config Usecase
typedef struct private_mock_config_usecase_t {
    extsock_config_usecase_t public;
} private_mock_config_usecase_t;

static extsock_error_t mock_config_apply_json(private_mock_config_usecase_t *this, const char *json_config)
{
    EXTSOCK_DBG(1, "Mock config usecase: apply_json called");
    return EXTSOCK_SUCCESS;
}

static void mock_config_destroy(private_mock_config_usecase_t *this)
{
    EXTSOCK_DBG(1, "Mock config usecase: destroy called");
    free(this);
}

extsock_config_usecase_t *extsock_config_usecase_create(extsock_json_parser_t *json_parser, extsock_event_usecase_t *event_usecase)
{
    private_mock_config_usecase_t *this;

    INIT(this,
        .public = {
            .apply_json_config = (extsock_error_t(*)(extsock_config_usecase_t*, const char*))mock_config_apply_json,
            .destroy = (void(*)(extsock_config_usecase_t*))mock_config_destroy,
        },
    );

    EXTSOCK_DBG(1, "Mock config usecase created successfully");
    return &this->public;
}

// Mock Event Usecase
typedef struct private_mock_event_usecase_t {
    extsock_event_usecase_t public;
} private_mock_event_usecase_t;

static void mock_event_set_socket_adapter(private_mock_event_usecase_t *this, extsock_socket_adapter_t *socket_adapter)
{
    EXTSOCK_DBG(1, "Mock event usecase: set_socket_adapter called");
}

static void mock_event_set_failover_manager(private_mock_event_usecase_t *this, extsock_failover_manager_t *failover_manager)
{
    EXTSOCK_DBG(1, "Mock event usecase: set_failover_manager called");
}

static void mock_event_destroy(private_mock_event_usecase_t *this)
{
    EXTSOCK_DBG(1, "Mock event usecase: destroy called");
    free(this);
}

extsock_event_usecase_t *extsock_event_usecase_create()
{
    private_mock_event_usecase_t *this;

    INIT(this,
        .public = {
            .set_socket_adapter = (void(*)(extsock_event_usecase_t*, extsock_socket_adapter_t*))mock_event_set_socket_adapter,
            .set_failover_manager = (void(*)(extsock_event_usecase_t*, extsock_failover_manager_t*))mock_event_set_failover_manager,
            .destroy = (void(*)(extsock_event_usecase_t*))mock_event_destroy,
        },
    );

    EXTSOCK_DBG(1, "Mock event usecase created successfully");
    return &this->public;
}

// Mock Failover Manager
typedef struct private_mock_failover_manager_t {
    extsock_failover_manager_t public;
} private_mock_failover_manager_t;

static void mock_failover_handle_connection_failure(private_mock_failover_manager_t *this, ike_sa_t *ike_sa)
{
    EXTSOCK_DBG(1, "Mock failover manager: handle_connection_failure called");
}

static char* mock_failover_select_next_segw(private_mock_failover_manager_t *this, const char *remote_addrs, const char *current_addr)
{
    EXTSOCK_DBG(1, "Mock failover manager: select_next_segw called");
    return strdup("10.0.0.2"); // Mock return
}

static extsock_error_t mock_failover_create_failover_config(private_mock_failover_manager_t *this, peer_cfg_t *original_cfg, const char *next_segw_addr)
{
    EXTSOCK_DBG(1, "Mock failover manager: create_failover_config called");
    return EXTSOCK_SUCCESS;
}

static bool mock_failover_is_max_retry_exceeded(private_mock_failover_manager_t *this, const char *conn_name)
{
    EXTSOCK_DBG(1, "Mock failover manager: is_max_retry_exceeded called");
    return FALSE;
}

static void mock_failover_reset_retry_count(private_mock_failover_manager_t *this, const char *conn_name)
{
    EXTSOCK_DBG(1, "Mock failover manager: reset_retry_count called");
}

static void mock_failover_destroy(private_mock_failover_manager_t *this)
{
    EXTSOCK_DBG(1, "Mock failover manager: destroy called");
    free(this);
}

extsock_failover_manager_t *extsock_failover_manager_create(extsock_config_usecase_t *config_usecase)
{
    private_mock_failover_manager_t *this;

    INIT(this,
        .public = {
            .handle_connection_failure = (void(*)(extsock_failover_manager_t*, ike_sa_t*))mock_failover_handle_connection_failure,
            .select_next_segw = (char*(*)(extsock_failover_manager_t*, const char*, const char*))mock_failover_select_next_segw,
            .create_failover_config = (extsock_error_t(*)(extsock_failover_manager_t*, peer_cfg_t*, const char*))mock_failover_create_failover_config,
            .is_max_retry_exceeded = (bool(*)(extsock_failover_manager_t*, const char*))mock_failover_is_max_retry_exceeded,
            .reset_retry_count = (void(*)(extsock_failover_manager_t*, const char*))mock_failover_reset_retry_count,
            .destroy = (void(*)(extsock_failover_manager_t*))mock_failover_destroy,
        },
    );

    EXTSOCK_DBG(1, "Mock failover manager created successfully");
    return &this->public;
}

// Mock Socket Adapter
typedef struct private_mock_socket_adapter_t {
    extsock_socket_adapter_t public;
} private_mock_socket_adapter_t;

static extsock_error_t mock_socket_send_event(private_mock_socket_adapter_t *this, const char *event_json)
{
    EXTSOCK_DBG(1, "Mock socket adapter: send_event called");
    return EXTSOCK_SUCCESS;
}

static thread_t* mock_socket_start_listening(private_mock_socket_adapter_t *this)
{
    EXTSOCK_DBG(1, "Mock socket adapter: start_listening called");
    return (thread_t*)0x12345678; // Mock thread pointer
}

static void mock_socket_stop_listening(private_mock_socket_adapter_t *this)
{
    EXTSOCK_DBG(1, "Mock socket adapter: stop_listening called");
}

static void mock_socket_destroy(private_mock_socket_adapter_t *this)
{
    EXTSOCK_DBG(1, "Mock socket adapter: destroy called");
    free(this);
}

extsock_socket_adapter_t *extsock_socket_adapter_create(extsock_config_usecase_t *cfg_usecase)
{
    private_mock_socket_adapter_t *this;

    INIT(this,
        .public = {
            .send_event = (extsock_error_t(*)(extsock_socket_adapter_t*, const char*))mock_socket_send_event,
            .start_listening = (thread_t*(*)(extsock_socket_adapter_t*))mock_socket_start_listening,
            .stop_listening = (void(*)(extsock_socket_adapter_t*))mock_socket_stop_listening,
            .destroy = (void(*)(extsock_socket_adapter_t*))mock_socket_destroy,
        },
    );

    EXTSOCK_DBG(1, "Mock socket adapter created successfully");
    return &this->public;
}