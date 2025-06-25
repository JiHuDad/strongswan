#include <stdio.h>
#include <stdlib.h>
#include "../common/extsock_errors.h"

int main() {
    printf("Testing extsock_error_create...\n");
    extsock_error_info_t *error = extsock_error_create(EXTSOCK_ERROR_NONE, "test");
    if (error) {
        printf("Error created successfully: code=%d, message=%s\n", error->code, error->message);
        extsock_error_destroy(error);
        printf("Error destroyed successfully\n");
    } else {
        printf("Failed to create error\n");
    }
    return 0;
}
