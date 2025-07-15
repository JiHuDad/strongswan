#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include config.h first
#include "../config.h"

// Include strongSwan headers
#include <library.h>
#include <utils/debug.h>

// Include our cert loader
#include "../src/libcharon/plugins/extsock/adapters/crypto/extsock_cert_loader.h"

int main(int argc, char *argv[])
{
    extsock_cert_loader_t *loader;
    private_key_t *key;
    
    printf("=== strongSwan extsock Certificate Loader Debug Test ===\n");
    
    // Initialize strongSwan library
    if (!library_init(NULL, "test"))
    {
        printf("Failed to initialize strongSwan library\n");
        return 1;
    }
    
    // Load essential plugins manually
    if (!lib->plugins->load(lib->plugins, "openssl gmp random nonce revocation constraints pubkey"))
    {
        printf("Failed to load essential strongSwan plugins\n");
        library_deinit();
        return 1;
    }
    
    printf("strongSwan library initialized successfully\n");
    
    // Create certificate loader
    loader = extsock_cert_loader_create();
    if (!loader)
    {
        printf("Failed to create certificate loader\n");
        library_deinit();
        return 1;
    }
    
    printf("Certificate loader created successfully\n");
    
    // Test 1: Load encrypted key with explicit password
    printf("\n--- Test 1: Load encrypted key with explicit password ---\n");
    key = loader->load_private_key(loader, "test_encrypted.key", "testpassword123");
    if (key)
    {
        printf("SUCCESS: Encrypted private key loaded with explicit password!\n");
        printf("Key type: %d, Key size: %d bits\n", 
               key->get_type(key), key->get_keysize(key));
        key->destroy(key);
    }
    else
    {
        printf("FAILED: Could not load encrypted private key with explicit password\n");
    }
    
    // Test 2: Load encrypted key with auto resolution (configured password)
    printf("\n--- Test 2: Load encrypted key with auto resolution (configured password) ---\n");
    loader->set_password(loader, "testpassword123");
    key = loader->load_private_key_auto(loader, "test_encrypted.key");
    if (key)
    {
        printf("SUCCESS: Encrypted private key loaded with auto resolution (configured)!\n");
        printf("Key type: %d, Key size: %d bits\n", 
               key->get_type(key), key->get_keysize(key));
        key->destroy(key);
    }
    else
    {
        printf("FAILED: Could not load encrypted private key with auto resolution (configured)\n");
    }
    
    // Test 3: Load encrypted key with environment variable
    printf("\n--- Test 3: Load encrypted key with environment variable ---\n");
    loader->set_password(loader, NULL);  // Clear configured password
    setenv("STRONGSWAN_PRIVATE_KEY_PASS", "testpassword123", 1);
    key = loader->load_private_key_auto(loader, "test_encrypted.key");
    if (key)
    {
        printf("SUCCESS: Encrypted private key loaded with environment variable!\n");
        printf("Key type: %d, Key size: %d bits\n", 
               key->get_type(key), key->get_keysize(key));
        key->destroy(key);
    }
    else
    {
        printf("FAILED: Could not load encrypted private key with environment variable\n");
    }
    unsetenv("STRONGSWAN_PRIVATE_KEY_PASS");
    
    // Test 4: Load encrypted key with wrong password (should fail)
    printf("\n--- Test 4: Load encrypted key with wrong password (should fail) ---\n");
    key = loader->load_private_key(loader, "test_encrypted.key", "wrongpassword");
    if (key)
    {
        printf("UNEXPECTED: Encrypted private key loaded with wrong password (this should not happen!)\n");
        key->destroy(key);
    }
    else
    {
        printf("EXPECTED: Encrypted private key correctly failed to load with wrong password\n");
    }
    
    // Test 5: Load encrypted key with no password (should fail)
    printf("\n--- Test 5: Load encrypted key with no password (should fail) ---\n");
    loader->set_password(loader, NULL);
    key = loader->load_private_key_auto(loader, "test_encrypted.key");
    if (key)
    {
        printf("UNEXPECTED: Encrypted private key loaded with no password (this should not happen!)\n");
        key->destroy(key);
    }
    else
    {
        printf("EXPECTED: Encrypted private key correctly failed to load with no password\n");
    }
    
    // Cleanup
    loader->destroy(loader);
    library_deinit();
    
    printf("\n=== Debug Test Complete ===\n");
    
    return 0;
} 