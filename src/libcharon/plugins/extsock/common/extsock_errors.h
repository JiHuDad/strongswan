/*
 * Copyright (C) 2024 strongSwan Project
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

/**
 * @defgroup extsock_errors extsock_errors
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_ERRORS_H_
#define EXTSOCK_ERRORS_H_

#include <sys/types.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "extsock_types.h"

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

/**
 * 에러 정보 객체 생성
 */
extsock_error_info_t *extsock_error_create(extsock_error_t code, const char *message);

/**
 * 에러 정보 객체 소멸
 */
void extsock_error_destroy(extsock_error_info_t *error_info);

#endif /** EXTSOCK_ERRORS_H_ @}*/ 