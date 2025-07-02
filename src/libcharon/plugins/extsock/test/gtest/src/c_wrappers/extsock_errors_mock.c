#include "../../include/c_wrappers/extsock_errors.h"

/**
 * Test-only mock implementation for extsock error handling
 * This file provides simple implementations for testing purposes only
 */

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
            return "Memory Allocation Failed";
        case EXTSOCK_ERROR_STRONGSWAN_API:
            return "strongSwan API Error";
        default:
            return "Unknown Error";
    }
} 