#ifndef TEST_EXTSOCK_TYPES_H
#define TEST_EXTSOCK_TYPES_H

/**
 * Test-only mock definitions for extsock Types
 * This file is used only for testing and does not modify the original source
 */

/**
 * extsock Event Types (test mock)
 */
typedef enum {
    EXTSOCK_EVENT_TUNNEL_UP = 0,
    EXTSOCK_EVENT_TUNNEL_DOWN = 1,
    EXTSOCK_EVENT_CONFIG_APPLIED = 2,
    EXTSOCK_EVENT_ERROR = 3
} extsock_event_type_t;

/**
 * extsock Command Types (test mock)
 */
typedef enum {
    EXTSOCK_CMD_APPLY_CONFIG = 0,
    EXTSOCK_CMD_START_DPD = 1,
    EXTSOCK_CMD_REMOVE_CONFIG = 2
} extsock_command_type_t;

#endif /* TEST_EXTSOCK_TYPES_H */ 