/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Pure unit test types (without strongSwan dependencies)
 * This file provides type definitions for Level 1 Pure tests
 * that don't require strongSwan library dependencies.
 */

#ifndef EXTSOCK_TYPES_PURE_H_
#define EXTSOCK_TYPES_PURE_H_

#include <sys/types.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * 에러 코드 정의 (Pure - strongSwan 독립)
 */
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_JSON_PARSE,
    EXTSOCK_ERROR_CONFIG_INVALID,
    EXTSOCK_ERROR_SOCKET_FAILED,
    EXTSOCK_ERROR_MEMORY_ALLOCATION,
    EXTSOCK_ERROR_STRONGSWAN_API,
    EXTSOCK_ERROR_INVALID_PARAMETER,
    EXTSOCK_ERROR_CONFIG_CREATION_FAILED
} extsock_error_t;

/**
 * 에러 심각도 정의 (Pure)
 */
typedef enum {
    EXTSOCK_ERROR_SEVERITY_TRACE = 0,
    EXTSOCK_ERROR_SEVERITY_DEBUG,
    EXTSOCK_ERROR_SEVERITY_INFO,
    EXTSOCK_ERROR_SEVERITY_WARNING,
    EXTSOCK_ERROR_SEVERITY_ERROR,
    EXTSOCK_ERROR_SEVERITY_CRITICAL
} extsock_error_severity_t;

/**
 * 에러 정보 구조체 (Pure)
 */
typedef struct extsock_error_info_t {
    extsock_error_t code;
    extsock_error_severity_t severity;
    char *message;
    char *context;
    time_t timestamp;
    uint32_t thread_id;
    bool recoverable;
    bool retry_recommended;
} extsock_error_info_t;

/**
 * 명령 타입 정의 (Pure)
 */
typedef enum {
    EXTSOCK_CMD_APPLY_CONFIG,
    EXTSOCK_CMD_START_DPD,
    EXTSOCK_CMD_REMOVE_CONFIG
} extsock_command_type_t;

/**
 * 이벤트 타입 정의 (Pure)
 */
typedef enum {
    EXTSOCK_EVENT_TUNNEL_UP,
    EXTSOCK_EVENT_TUNNEL_DOWN,
    EXTSOCK_EVENT_CONFIG_APPLIED,
    EXTSOCK_EVENT_ERROR
} extsock_event_type_t;

#endif /* EXTSOCK_TYPES_PURE_H_ */