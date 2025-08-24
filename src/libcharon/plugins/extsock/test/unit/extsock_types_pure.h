/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Pure Unit Test Types for extsock_errors module
 * TASK-005: extsock_errors 실제 테스트
 * 
 * This file provides type definitions needed for pure unit tests
 * without strongSwan dependencies.
 */

#ifndef EXTSOCK_TYPES_PURE_H_
#define EXTSOCK_TYPES_PURE_H_

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

/*
 * ============================================================================
 * Basic Error Types (extracted from extsock_types.h)
 * ============================================================================
 */

/**
 * 에러 코드 정의
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

/*
 * ============================================================================
 * Error Severity and Info Types (from extsock_errors.h)
 * ============================================================================
 */

/**
 * 에러 심각도 정의
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
 * 간단한 에러 정보 구조체
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

/*
 * ============================================================================
 * Function Declarations
 * ============================================================================
 */

/**
 * 에러 정보 객체 생성
 */
extsock_error_info_t *extsock_error_create(extsock_error_t code, const char *message);

/**
 * 에러 정보 객체 소멸
 */
void extsock_error_destroy(extsock_error_info_t *error_info);

/**
 * 에러 코드를 문자열로 변환
 */
const char* extsock_error_to_string(extsock_error_t error);

#endif /* EXTSOCK_TYPES_PURE_H_ */