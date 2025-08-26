/*
 * Backend Crash Test - extsock strongSwan adapter backend method testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// strongSwan headers
#include <library.h>
#include <daemon.h>
#include <networking/host.h>

// extsock headers  
#include "../adapters/strongswan/extsock_strongswan_adapter.h"

/**
 * Test backend method calls through strongSwan's backend manager
 */
static void test_backend_through_manager()
{
    extsock_strongswan_adapter_t *adapter;
    enumerator_t *enumerator;
    peer_cfg_t *peer_cfg;
    identification_t *me, *other;
    host_t *my_host, *other_host;
    
    printf("=== Backend Manager Crash Test ===\n");
    
    // Create adapter (this should register the backend with charon)
    printf("1. Creating strongSwan adapter...\n");
    adapter = extsock_strongswan_adapter_create();
    if (!adapter) {
        printf("ERROR: Failed to create adapter\n");
        return;
    }
    printf("   ✓ Adapter created successfully\n");
    
    // Verify charon backends exist
    if (!charon || !charon->backends) {
        printf("ERROR: charon or backends not available\n");
        goto cleanup_adapter;
    }
    printf("   ✓ charon backend manager is available\n");
    
    // Create test identities and hosts
    printf("2. Creating test identities and hosts...\n");
    me = identification_create_from_string("client@example.com");
    other = identification_create_from_string("server@example.com");
    my_host = host_create_from_string("192.168.1.10", 500);
    other_host = host_create_from_string("192.168.1.1", 500);
    
    if (!me || !other || !my_host || !other_host) {
        printf("ERROR: Failed to create identities or hosts\n");
        goto cleanup_adapter;
    }
    printf("   ✓ Identities and hosts created successfully\n");
    
    // Test 1: Call create_peer_cfg_enumerator through backend manager
    // This should eventually call our backend's create_peer_cfg_enumerator method
    printf("3. Testing backend manager create_peer_cfg_enumerator...\n");
    printf("   About to call charon->backends->create_peer_cfg_enumerator...\n");
    printf("   This should trigger our backend method and potential SEG fault!\n");
    
    // Call through backend manager - this should hit our backend
    enumerator = charon->backends->create_peer_cfg_enumerator(charon->backends, 
                                                              my_host, other_host,
                                                              me, other, IKEV2);
    
    if (enumerator) {
        printf("   ✓ Enumerator created successfully\n");
        
        // Test enumerator usage
        printf("4. Testing enumerator usage...\n");
        int count = 0;
        while (enumerator->enumerate(enumerator, &peer_cfg)) {
            printf("   Found peer config: %s\n", 
                   peer_cfg->get_name(peer_cfg));
            count++;
        }
        printf("   Found %d peer configs\n", count);
        enumerator->destroy(enumerator);
        printf("   ✓ Enumerator destroyed successfully\n");
    } else {
        printf("   ✓ Enumerator is NULL (expected for empty backend)\n");
    }
    
    // Test 2: Call get_peer_cfg_by_name through backend manager
    printf("5. Testing backend manager get_peer_cfg_by_name...\n");
    peer_cfg = charon->backends->get_peer_cfg_by_name(charon->backends, "test_peer");
    if (peer_cfg) {
        printf("   Found peer config: %s\n", peer_cfg->get_name(peer_cfg));
        peer_cfg->destroy(peer_cfg);
    } else {
        printf("   ✓ No peer config found (expected)\n");
    }
    
    // Test 3: Stress test - call multiple times
    printf("6. Stress test - multiple backend manager calls...\n");
    for (int i = 0; i < 10; i++) {
        printf("   Call %d: ", i + 1);
        enumerator = charon->backends->create_peer_cfg_enumerator(charon->backends,
                                                                  my_host, other_host,
                                                                  me, other, IKEV2);
        if (enumerator) {
            int count = 0;
            while (enumerator->enumerate(enumerator, &peer_cfg)) {
                count++;
            }
            enumerator->destroy(enumerator);
            printf("✓ (found %d configs)\n", count);
        } else {
            printf("✓ (null enumerator)\n");
        }
        
        // Small delay
        usleep(1000);
    }
    
    // Test 4: Call with NULL parameters to test NULL handling
    printf("7. Testing NULL parameter handling...\n");
    
    printf("   Testing with me=NULL...\n");
    enumerator = charon->backends->create_peer_cfg_enumerator(charon->backends,
                                                              my_host, other_host,
                                                              NULL, other, IKEV2);
    if (enumerator) {
        enumerator->destroy(enumerator);
        printf("   ✓ Handled me=NULL successfully\n");
    } else {
        printf("   ✓ Returned NULL for me=NULL\n");
    }
    
    printf("   Testing with other=NULL...\n");
    enumerator = charon->backends->create_peer_cfg_enumerator(charon->backends,
                                                              my_host, other_host,
                                                              me, NULL, IKEV2);
    if (enumerator) {
        enumerator->destroy(enumerator);
        printf("   ✓ Handled other=NULL successfully\n");
    } else {
        printf("   ✓ Returned NULL for other=NULL\n");
    }
    
    printf("   Testing with hosts=NULL...\n");
    enumerator = charon->backends->create_peer_cfg_enumerator(charon->backends,
                                                              NULL, NULL,
                                                              me, other, IKEV2);
    if (enumerator) {
        enumerator->destroy(enumerator);
        printf("   ✓ Handled hosts=NULL successfully\n");
    } else {
        printf("   ✓ Returned NULL for hosts=NULL\n");
    }
    
    printf("\n=== Test completed successfully! ===\n");
    
cleanup_identities:
    if (me) me->destroy(me);
    if (other) other->destroy(other);
    if (my_host) my_host->destroy(my_host);
    if (other_host) other_host->destroy(other_host);
    
cleanup_adapter:
    if (adapter) adapter->destroy(adapter);
}

/**
 * Initialize minimal strongSwan environment
 */
static bool init_strongswan()
{
    printf("Initializing strongSwan environment...\n");
    
    // Initialize library
    if (!library_init(NULL, "test-backend")) {
        printf("ERROR: Failed to initialize library\n");
        return FALSE;
    }
    
    // Initialize charon
    if (!libcharon_init()) {
        printf("ERROR: Failed to initialize libcharon\n");
        library_deinit();
        return FALSE;
    }
    
    // Initialize daemon with minimal plugins (no network plugins to avoid permission issues)
    if (!charon->initialize(charon, "random nonce x509 pem openssl extsock")) {
        printf("ERROR: Failed to initialize charon daemon\n");
        libcharon_deinit();
        library_deinit();
        return FALSE;
    }
    
    printf("✓ strongSwan environment initialized\n");
    return TRUE;
}

/**
 * Cleanup strongSwan environment
 */
static void cleanup_strongswan()
{
    printf("Cleaning up strongSwan environment...\n");
    libcharon_deinit();
    library_deinit();
    printf("✓ Cleanup completed\n");
}

/**
 * Main test function
 */
int main(int argc, char *argv[])
{
    printf("=== extsock Backend Crash Test ===\n\n");
    
    if (!init_strongswan()) {
        return 1;
    }
    
    test_backend_through_manager();
    
    cleanup_strongswan();
    
    printf("\nTest completed without crash!\n");
    printf("If SEG fault was going to happen, it should have occurred during step 3.\n");
    return 0;
} 