/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Minimal types for Phase 4 testing
 * This file provides minimal type definitions for domain testing
 * without requiring full strongSwan dependencies
 */

#ifndef TEST_EXTSOCK_TYPES_MINIMAL_H_
#define TEST_EXTSOCK_TYPES_MINIMAL_H_

#include <stdbool.h>
#include <stddef.h>

// Forward declarations for types that will be fully defined in Phase 5
typedef struct ike_cfg_t ike_cfg_t;
typedef struct peer_cfg_t peer_cfg_t;
typedef struct linked_list_t linked_list_t;
typedef struct auth_cfg_t auth_cfg_t;

// Minimal strongSwan library functions
#define INIT(this, ...) do { \
    memset(this, 0, sizeof(*this)); \
    *this = (typeof(*this)){ __VA_ARGS__ }; \
} while(0)

#define METHOD(interface, method, ret, ...) \
    static ret _##method(__VA_ARGS__)

// Basic types we need for testing
typedef struct extsock_config_entity_t extsock_config_entity_t;

/**
 * Error codes
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

// Boolean values for compatibility
#ifndef TRUE
#define TRUE true
#endif
#ifndef FALSE
#define FALSE false
#endif

#endif /** TEST_EXTSOCK_TYPES_MINIMAL_H_ @}*/