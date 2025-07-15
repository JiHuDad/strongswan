#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test the basic functionality without full strongSwan initialization
int main(int argc, char *argv[])
{
    printf("=== Testing Fixed strongSwan Certificate Loader ===\n");
    
    // Test 1: Verify encrypted key can be decrypted with OpenSSL directly
    printf("\n--- Test 1: Direct OpenSSL verification ---\n");
    
    int result = system("echo 'test123' | openssl rsa -in debug_encrypted.key -passin stdin -noout -text >/dev/null 2>&1");
    if (result == 0)
    {
        printf("SUCCESS: OpenSSL can decrypt the encrypted key with password 'test123'\n");
    }
    else
    {
        printf("FAILED: OpenSSL cannot decrypt the encrypted key with password 'test123'\n");
        return 1;
    }
    
    // Test 2: Verify key is actually encrypted
    printf("\n--- Test 2: Verify key is encrypted ---\n");
    result = system("openssl rsa -in debug_encrypted.key -noout -text >/dev/null 2>&1");
    if (result != 0)
    {
        printf("SUCCESS: Key is properly encrypted (cannot be read without password)\n");
    }
    else
    {
        printf("WARNING: Key appears to be unencrypted\n");
    }
    
    // Test 3: Check that our PEM approach should work
    printf("\n--- Test 3: File format verification ---\n");
    
    FILE *f = fopen("debug_encrypted.key", "r");
    if (f)
    {
        char line[256];
        if (fgets(line, sizeof(line), f))
        {
            if (strstr(line, "-----BEGIN"))
            {
                printf("SUCCESS: File is in PEM format (should work with BUILD_BLOB_PEM)\n");
                
                if (strstr(line, "ENCRYPTED"))
                {
                    printf("INFO: File header indicates encryption\n");
                }
                else
                {
                    printf("INFO: File header: %s", line);
                }
            }
            else
            {
                printf("WARNING: File does not appear to be PEM format\n");
            }
        }
        fclose(f);
    }
    
    printf("\n=== Summary ===\n");
    printf("Our fix changes the private key loading approach from:\n");
    printf("  OLD: BUILD_FROM_FILE (bypasses PEM plugin password resolution)\n");
    printf("  NEW: BUILD_BLOB_PEM (uses PEM plugin with proper password callbacks)\n");
    printf("\nThe PEM plugin will call our password callback when it detects encryption.\n");
    printf("This should resolve the encrypted private key loading issue.\n");
    printf("\nTo test fully, you would need to:\n");
    printf("1. Initialize strongSwan library with pem plugin loaded\n");
    printf("2. Create extsock_cert_loader instance\n");
    printf("3. Set password or environment variable\n");
    printf("4. Call load_private_key() or load_private_key_auto()\n");
    printf("5. Verify the key loads successfully\n");
    
    return 0;
} 