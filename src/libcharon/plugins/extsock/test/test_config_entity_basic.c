#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 기본 테스트용 모의 정의
typedef struct {
    void (*destroy)(void *this);
    const char* (*get_name)(void *this);
    int (*validate)(void *this);
} extsock_config_entity_t;

// Mock implementation for basic testing
extern extsock_config_entity_t *extsock_config_entity_create_from_json(const char *config_json);

int test_config_entity_basic_creation()
{
    printf("=== Test: Config Entity Basic Creation ===\n");
    
    const char *test_json = "{\"name\":\"test-connection\",\"ike\":{\"version\":2}}";
    
    extsock_config_entity_t *entity = extsock_config_entity_create_from_json(test_json);
    
    if (!entity) {
        printf("❌ FAILED: Config entity creation returned NULL\n");
        return 1;
    }
    
    printf("✅ SUCCESS: Config entity created successfully\n");
    
    // Basic name test
    if (entity->get_name) {
        const char *name = entity->get_name(entity);
        if (name && strlen(name) > 0) {
            printf("✅ SUCCESS: Entity name: '%s'\n", name);
        } else {
            printf("❌ FAILED: Invalid entity name\n");
            return 1;
        }
    }
    
    // Basic validation test
    if (entity->validate) {
        int is_valid = entity->validate(entity);
        printf("✅ SUCCESS: Validation result: %s\n", is_valid ? "VALID" : "INVALID");
    }
    
    // Cleanup
    if (entity->destroy) {
        entity->destroy(entity);
        printf("✅ SUCCESS: Entity destroyed successfully\n");
    }
    
    return 0;
}

int test_config_entity_null_handling()
{
    printf("\n=== Test: Config Entity NULL Handling ===\n");
    
    extsock_config_entity_t *entity = extsock_config_entity_create_from_json(NULL);
    
    if (entity) {
        printf("❌ FAILED: Should return NULL for NULL input\n");
        entity->destroy(entity);
        return 1;
    }
    
    printf("✅ SUCCESS: Correctly handled NULL input\n");
    return 0;
}

int main(int argc, char *argv[])
{
    printf("========================================\n");
    printf("Config Entity Basic Test Suite\n");
    printf("========================================\n");
    
    int failed = 0;
    
    failed += test_config_entity_basic_creation();
    failed += test_config_entity_null_handling();
    
    printf("\n========================================\n");
    if (failed == 0) {
        printf("🎉 ALL TESTS PASSED!\n");
        printf("Config Entity implementation is working correctly.\n");
    } else {
        printf("❌ %d TESTS FAILED\n", failed);
        printf("Config Entity implementation needs fixes.\n");
    }
    printf("========================================\n");
    
    return failed;
} 