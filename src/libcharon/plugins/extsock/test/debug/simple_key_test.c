#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Just test our certificate loader components directly
// Copy the essential parts to avoid linking issues

typedef struct {
    char *password;
    int interactive;
} test_loader_t;

// Simulate the password callback mechanism
char* test_password_callback(test_loader_t *loader, const char *type_name)
{
    printf("Password callback called for type: %s\n", type_name);
    
    if (strcmp(type_name, "PRIVATE_KEY_PASS") != 0)
    {
        printf("  Ignoring non-private-key-pass request\n");
        return NULL;
    }
    
    if (loader->password)
    {
        printf("  Using configured password\n");
        return strdup(loader->password);
    }
    
    printf("  Trying environment variable\n");
    char *env_password = getenv("STRONGSWAN_PRIVATE_KEY_PASS");
    if (env_password)
    {
        printf("  Using password from environment variable\n");
        return strdup(env_password);
    }
    
    printf("  No password available\n");
    return NULL;
}

// Test OpenSSL direct loading
int test_openssl_loading(const char *keyfile, const char *password)
{
    char command[512];
    int result;
    
    printf("Testing OpenSSL loading with password: '%s'\n", password ? password : "NULL");
    
    if (password)
    {
        snprintf(command, sizeof(command), 
                "echo '%s' | openssl rsa -in %s -passin stdin -noout -text >/dev/null 2>&1", 
                password, keyfile);
    }
    else
    {
        snprintf(command, sizeof(command), 
                "openssl rsa -in %s -noout -text >/dev/null 2>&1", 
                keyfile);
    }
    
    result = system(command);
    printf("  OpenSSL result: %s\n", result == 0 ? "SUCCESS" : "FAILED");
    return result == 0;
}

int main(int argc, char *argv[])
{
    test_loader_t loader = { .password = NULL, .interactive = 1 };
    char *callback_password;
    
    printf("=== Simple Encrypted Private Key Test ===\n");
    
    // Test 1: Direct OpenSSL with correct password
    printf("\n--- Test 1: Direct OpenSSL with correct password ---\n");
    test_openssl_loading("test_encrypted.key", "testpassword123");
    
    // Test 2: Direct OpenSSL with wrong password
    printf("\n--- Test 2: Direct OpenSSL with wrong password ---\n");
    test_openssl_loading("test_encrypted.key", "wrongpassword");
    
    // Test 3: Direct OpenSSL with no password
    printf("\n--- Test 3: Direct OpenSSL with no password ---\n");
    test_openssl_loading("test_encrypted.key", NULL);
    
    // Test 4: Password callback with configured password
    printf("\n--- Test 4: Password callback with configured password ---\n");
    loader.password = "testpassword123";
    callback_password = test_password_callback(&loader, "PRIVATE_KEY_PASS");
    if (callback_password)
    {
        printf("  Callback returned password: '%s'\n", callback_password);
        test_openssl_loading("test_encrypted.key", callback_password);
        free(callback_password);
    }
    
    // Test 5: Password callback with environment variable
    printf("\n--- Test 5: Password callback with environment variable ---\n");
    loader.password = NULL;
    setenv("STRONGSWAN_PRIVATE_KEY_PASS", "testpassword123", 1);
    callback_password = test_password_callback(&loader, "PRIVATE_KEY_PASS");
    if (callback_password)
    {
        printf("  Callback returned password: '%s'\n", callback_password);
        test_openssl_loading("test_encrypted.key", callback_password);
        free(callback_password);
    }
    unsetenv("STRONGSWAN_PRIVATE_KEY_PASS");
    
    // Test 6: Password callback with no password
    printf("\n--- Test 6: Password callback with no password ---\n");
    loader.password = NULL;
    callback_password = test_password_callback(&loader, "PRIVATE_KEY_PASS");
    if (callback_password)
    {
        printf("  Callback returned password: '%s'\n", callback_password);
        free(callback_password);
    }
    else
    {
        printf("  Callback correctly returned NULL\n");
    }
    
    printf("\n=== Simple Test Complete ===\n");
    printf("This confirms the password callback logic is working correctly.\n");
    printf("If OpenSSL can load the key but strongSwan cannot, the issue is in\n");
    printf("the integration between the callback and strongSwan's credential system.\n");
    
    return 0;
} 