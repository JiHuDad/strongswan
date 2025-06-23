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

/**
 * 에러 정보 객체 생성
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