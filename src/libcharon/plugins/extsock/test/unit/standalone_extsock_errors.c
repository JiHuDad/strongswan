/*
 * Copyright (C) 2024 strongSwan Project
 * Standalone version of extsock_errors.c for testing
 * 헤더 의존성 없이 독립적으로 컴파일 가능한 버전
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <stdint.h>

// 타입 정의들 (헤더 파일 의존성 제거)
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_JSON_PARSE,
    EXTSOCK_ERROR_CONFIG_INVALID,
    EXTSOCK_ERROR_SOCKET_FAILED,
    EXTSOCK_ERROR_MEMORY_ALLOCATION,
    EXTSOCK_ERROR_STRONGSWAN_API
} extsock_error_t;

typedef enum {
    EXTSOCK_ERROR_SEVERITY_TRACE = 0,
    EXTSOCK_ERROR_SEVERITY_DEBUG,
    EXTSOCK_ERROR_SEVERITY_INFO,
    EXTSOCK_ERROR_SEVERITY_WARNING,
    EXTSOCK_ERROR_SEVERITY_ERROR,
    EXTSOCK_ERROR_SEVERITY_CRITICAL
} extsock_error_severity_t;

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
 * 에러 정보 객체 생성 (원본 구현)
 */
extsock_error_info_t *extsock_error_create(extsock_error_t code, const char *message)
{
    extsock_error_info_t *error_info = malloc(sizeof(extsock_error_info_t));
    if (!error_info) {
        return NULL;
    }

    error_info->code = code;
    error_info->severity = EXTSOCK_ERROR_SEVERITY_ERROR;
    error_info->message = message ? strdup(message) : NULL;
    error_info->context = NULL;
    error_info->timestamp = time(NULL);
    error_info->thread_id = (uint32_t)syscall(SYS_gettid);
    error_info->recoverable = false;
    error_info->retry_recommended = false;

    return error_info;
}

/**
 * 에러 정보 객체 소멸 (원본 구현)
 */
void extsock_error_destroy(extsock_error_info_t *error_info)
{
    if (!error_info) {
        return;
    }

    if (error_info->message) {
        free(error_info->message);
    }
    
    if (error_info->context) {
        free(error_info->context);
    }

    free(error_info);
} 