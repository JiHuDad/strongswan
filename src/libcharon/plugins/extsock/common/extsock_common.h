/*
 * Copyright (C) 2024 strongSwan Project
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

/**
 * @defgroup extsock_common extsock_common
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_COMMON_H_
#define EXTSOCK_COMMON_H_

#include <utils/debug.h>
#include <utils/utils/object.h>
#include "extsock_types.h"

// 소켓 경로 상수
#define SOCKET_PATH "/tmp/strongswan_extsock.sock"

// 로깅 매크로
#define EXTSOCK_DBG(level, fmt, ...) \
    DBG##level(DBG_LIB, "[extsock] " fmt, ##__VA_ARGS__)

// 에러 처리 매크로
#define EXTSOCK_RETURN_ON_ERROR(result) \
    do { if ((result) != EXTSOCK_SUCCESS) return (result); } while(0)

#define EXTSOCK_RETURN_NULL_ON_ERROR(result) \
    do { if ((result) != EXTSOCK_SUCCESS) return NULL; } while(0)

// 메모리 관리 매크로
#define EXTSOCK_SAFE_FREE(ptr) \
    do { if (ptr) { free(ptr); (ptr) = NULL; } } while(0)

#define EXTSOCK_SAFE_DESTROY(obj) \
    do { if (obj && obj->destroy) { obj->destroy(obj); (obj) = NULL; } } while(0)

// 안전성 강화 매크로들
#define EXTSOCK_SAFE_CALL(ptr, method, ...) \
    ((ptr) ? (ptr)->method(ptr, ##__VA_ARGS__) : EXTSOCK_ERROR_CONFIG_INVALID)

#define EXTSOCK_SAFE_CREATE(create_func, ...) \
    ({ \
        typeof(create_func(__VA_ARGS__)) _result = create_func(__VA_ARGS__); \
        if (!_result) { \
            EXTSOCK_DBG(1, "Failed to create object: %s", #create_func); \
        } \
        _result; \
    })

// NULL 체크 강화 매크로
#define EXTSOCK_CHECK_NULL_RET(ptr, error_code) \
    do { if (!(ptr)) { \
        EXTSOCK_DBG(1, "NULL pointer check failed: %s", #ptr); \
        return (error_code); \
    } } while(0)

#define EXTSOCK_CHECK_NULL_RET_NULL(ptr) \
    do { if (!(ptr)) { \
        EXTSOCK_DBG(1, "NULL pointer check failed: %s", #ptr); \
        return NULL; \
    } } while(0)

// 문자열 안전성 매크로
#define EXTSOCK_SAFE_STRLEN(str) \
    ((str) ? strlen(str) : 0)

#define EXTSOCK_SAFE_STRNCPY(dest, src, size) \
    do { \
        if ((src)) { \
            strncpy((dest), (src), (size) - 1); \
            (dest)[(size) - 1] = '\0'; \
        } \
    } while(0)

// 버퍼 오버플로우 방지 매크로  
#define EXTSOCK_SAFE_SNPRINTF(buffer, size, format, ...) \
    do { \
        int _n = snprintf((buffer), (size), (format), ##__VA_ARGS__); \
        if (_n >= (size)) { \
            EXTSOCK_DBG(1, "Buffer overflow prevented in snprintf"); \
        } \
    } while(0)

// strongSwan API 안전 호출 매크로
#define EXTSOCK_SAFE_STRONGSWAN_CREATE(create_func, ...) \
    ({ \
        typeof(create_func(__VA_ARGS__)) _result = create_func(__VA_ARGS__); \
        if (!_result) { \
            EXTSOCK_DBG(1, "strongSwan API failed: %s", #create_func); \
        } \
        _result; \
    })

#endif /** EXTSOCK_COMMON_H_ @}*/ 