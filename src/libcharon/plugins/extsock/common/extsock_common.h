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

#endif /** EXTSOCK_COMMON_H_ @}*/ 