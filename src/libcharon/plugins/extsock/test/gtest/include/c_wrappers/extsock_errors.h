#ifndef TEST_EXTSOCK_ERRORS_H
#define TEST_EXTSOCK_ERRORS_H

/**
 * Test-only mock definitions for extsock Error Codes
 * This file is used only for testing and does not modify the original source
 */
typedef enum {
    EXTSOCK_SUCCESS = 0,
    EXTSOCK_ERROR_JSON_PARSE = 1,
    EXTSOCK_ERROR_CONFIG_INVALID = 2,
    EXTSOCK_ERROR_SOCKET_FAILED = 3,
    EXTSOCK_ERROR_MEMORY_ALLOCATION = 4,
    EXTSOCK_ERROR_STRONGSWAN_API = 5
} extsock_error_t;

/**
 * Convert error code to string representation (test mock)
 */
const char* extsock_error_to_string(extsock_error_t error);

#endif /* TEST_EXTSOCK_ERRORS_H */ 