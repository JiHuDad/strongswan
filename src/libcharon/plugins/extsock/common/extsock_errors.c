/*
 * Copyright (C) 2024 strongSwan Project
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "extsock_errors.h"
#include "extsock_common.h"  // 안전성 매크로 사용을 위해 추가

/**
 * 에러 정보 객체 생성
 */
extsock_error_info_t *extsock_error_create(extsock_error_t code, const char *message)
{
    // 🔴 HIGH PRIORITY: 안전한 메모리 할당
    extsock_error_info_t *error_info = malloc(sizeof(extsock_error_info_t));
    if (!error_info) {
        // 메모리 할당 실패 시 안전하게 처리
        return NULL;
    }

    error_info->code = code;
    error_info->severity = EXTSOCK_ERROR_SEVERITY_ERROR;
    
    // 🔴 HIGH PRIORITY: 안전한 문자열 복사
    if (message) {
        error_info->message = strdup(message);
        if (!error_info->message) {
            // strdup 실패 시 메모리 누수 방지
            free(error_info);
            return NULL;
        }
    } else {
        error_info->message = NULL;
    }
    
    error_info->context = NULL;
    error_info->timestamp = time(NULL);
    error_info->thread_id = (uint32_t)syscall(SYS_gettid);
    error_info->recoverable = false;
    error_info->retry_recommended = false;

    return error_info;
}

/**
 * 에러 정보 객체 소멸
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

// Placeholder implementation for error handling functions
// This is a minimal implementation to support the test build

const char* extsock_error_to_string(extsock_error_t error) {
    switch (error) {
        case EXTSOCK_SUCCESS:
            return "Success";
        case EXTSOCK_ERROR_JSON_PARSE:
            return "JSON Parse Error";
        case EXTSOCK_ERROR_CONFIG_INVALID:
            return "Invalid Configuration";
        case EXTSOCK_ERROR_SOCKET_FAILED:
            return "Socket Operation Failed";
        case EXTSOCK_ERROR_MEMORY_ALLOCATION:
            return "Memory Allocation Error";
        case EXTSOCK_ERROR_STRONGSWAN_API:
            return "strongSwan API Error";
        default:
            return "Unknown Error";
    }
} 