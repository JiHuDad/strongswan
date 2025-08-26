/*
 * Mock implementation for extsock_errors for Google Test
 */

#include "../../common/extsock_errors.h"
#include <stdlib.h>
#include <string.h>

// Simple mock implementations for testing
extsock_error_t extsock_error_create(const char* message, extsock_error_code_t code) {
    return EXTSOCK_SUCCESS; // Mock implementation
}

const char* extsock_error_to_string(extsock_error_t error) {
    switch (error) {
        case EXTSOCK_SUCCESS:
            return "Success";
        case EXTSOCK_ERROR_INVALID_PARAMETER:
            return "Invalid Parameter";
        case EXTSOCK_ERROR_CONFIG_INVALID:
            return "Config Invalid";
        default:
            return "Unknown Error";
    }
}
