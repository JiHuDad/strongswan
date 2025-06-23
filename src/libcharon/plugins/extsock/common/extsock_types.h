/*
 * Copyright (C) 2024 strongSwan Project
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

/**
 * @defgroup extsock_types extsock_types
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_TYPES_H_
#define EXTSOCK_TYPES_H_

#include <library.h>
#include <collections/linked_list.h>
#include <config/ike_cfg.h>
#include <config/peer_cfg.h>
#include <config/child_cfg.h>
#include <credentials/auth_cfg.h>

typedef struct extsock_config_entity_t extsock_config_entity_t;
typedef struct extsock_json_parser_t extsock_json_parser_t;
typedef struct extsock_socket_adapter_t extsock_socket_adapter_t;
typedef struct extsock_strongswan_adapter_t extsock_strongswan_adapter_t;
typedef struct extsock_config_usecase_t extsock_config_usecase_t;
typedef struct extsock_event_usecase_t extsock_event_usecase_t;
typedef struct extsock_dpd_usecase_t extsock_dpd_usecase_t;

/**
 * 에러 코드 정의
 */
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_JSON_PARSE,
    EXTSOCK_ERROR_CONFIG_INVALID,
    EXTSOCK_ERROR_SOCKET_FAILED,
    EXTSOCK_ERROR_MEMORY_ALLOCATION,
    EXTSOCK_ERROR_STRONGSWAN_API
} extsock_error_t;

/**
 * 명령 타입 정의
 */
typedef enum {
    EXTSOCK_CMD_APPLY_CONFIG,
    EXTSOCK_CMD_START_DPD,
    EXTSOCK_CMD_REMOVE_CONFIG
} extsock_command_type_t;

/**
 * 이벤트 타입 정의
 */
typedef enum {
    EXTSOCK_EVENT_TUNNEL_UP,
    EXTSOCK_EVENT_TUNNEL_DOWN,
    EXTSOCK_EVENT_CONFIG_APPLIED,
    EXTSOCK_EVENT_ERROR
} extsock_event_type_t;

#endif /** EXTSOCK_TYPES_H_ @}*/ 