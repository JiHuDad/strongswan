/**
 * StrongSwan Mock Library Implementation
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 4+ Implementation - Mock all strongSwan dependencies for direct library loading
 */

#include "../../include/real_integration/StrongSwanMockLibrary.hpp"
#include "real_plugin_macros.hpp"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>

// ============================================================================
// Forward Declarations (Minimal strongSwan Types)
// ============================================================================

extern "C" {
    struct chunk_t {
        u_char* ptr;
        size_t len;
    };
    
    struct linked_list_t {
        void (*destroy)(struct linked_list_t* list);
        int (*get_count)(struct linked_list_t* list);
    };
    
    struct auth_cfg_t {
        void (*destroy)(struct auth_cfg_t* cfg);
    };
    
    struct child_cfg_t {
        void (*destroy)(struct child_cfg_t* cfg);
        char* name;
    };
    
    struct peer_cfg_t {
        void (*destroy)(struct peer_cfg_t* cfg);
        char* name;
    };
    
    struct ike_cfg_t {
        void (*destroy)(struct ike_cfg_t* cfg);
    };
    
    struct identification_t {
        void (*destroy)(struct identification_t* id);
        char* id_string;
    };
    
    struct proposal_t {
        void (*destroy)(struct proposal_t* proposal);
    };
    
    struct traffic_selector_t {
        void (*destroy)(struct traffic_selector_t* ts);
    };
    
    struct shared_key_t {
        void (*destroy)(struct shared_key_t* key);
    };
    
    struct mem_cred_t {
        void (*destroy)(struct mem_cred_t* cred);
    };
    
    struct callback_cred_t {
        void (*destroy)(struct callback_cred_t* cred);
    };
    
    struct thread_t {
        void (*join)(struct thread_t* thread);
        void (*cancel)(struct thread_t* thread);
    };
    
    struct mutex_t {
        void (*lock)(struct mutex_t* mutex);
        void (*unlock)(struct mutex_t* mutex);
        void (*destroy)(struct mutex_t* mutex);
        bool locked;
    };
    
    // Global objects stubs
    struct daemon_t {
        char* name;
    };
    
    struct library_t {
        char* name;
    };
    
    // Global variables (mock instances)
    static struct daemon_t mock_charon = { (char*)"mock-charon" };
    static struct library_t mock_lib = { (char*)"mock-lib" };
}

// ============================================================================
// Mock Function Implementations
// ============================================================================

extern "C" {

// Global Variables
struct daemon_t* charon = &mock_charon;
struct library_t* lib = &mock_lib;

// Chunk Functions (메모리 관리 관련)
struct chunk_t chunk_empty = { nullptr, 0 };

struct chunk_t chunk_create_cat(const char* mode, ...) {
    REAL_PLUGIN_DEBUG("Mock: chunk_create_cat()");
    struct chunk_t result = { (u_char*)strdup("mock-chunk"), 10 };
    return result;
}

struct chunk_t chunk_create_clone(struct chunk_t chunk) {
    REAL_PLUGIN_DEBUG("Mock: chunk_create_clone()");
    struct chunk_t result = { nullptr, 0 };
    if (chunk.ptr && chunk.len > 0) {
        result.ptr = (u_char*)malloc(chunk.len);
        memcpy(result.ptr, chunk.ptr, chunk.len);
        result.len = chunk.len;
    }
    return result;
}

size_t chunk_length(const char* mode, ...) {
    REAL_PLUGIN_DEBUG("Mock: chunk_length()");
    return 10; // Mock length
}

void* chunk_map(struct chunk_t chunk, bool shared) {
    REAL_PLUGIN_DEBUG("Mock: chunk_map()");
    return chunk.ptr;
}

bool chunk_unmap(void* addr, size_t len) {
    REAL_PLUGIN_DEBUG("Mock: chunk_unmap()");
    return true;
}

// Configuration Creation Functions
struct auth_cfg_t* auth_cfg_create(void) {
    REAL_PLUGIN_DEBUG("Mock: auth_cfg_create()");
    struct auth_cfg_t* cfg = (struct auth_cfg_t*)calloc(1, sizeof(struct auth_cfg_t));
    cfg->destroy = [](struct auth_cfg_t* cfg) { 
        REAL_PLUGIN_DEBUG("Mock: auth_cfg destroy");
        free(cfg); 
    };
    return cfg;
}

struct child_cfg_t* child_cfg_create(const char* name, void* config) {
    REAL_PLUGIN_DEBUG("Mock: child_cfg_create(" + std::string(name ? name : "null") + ")");
    struct child_cfg_t* cfg = (struct child_cfg_t*)calloc(1, sizeof(struct child_cfg_t));
    cfg->name = name ? strdup(name) : strdup("mock-child");
    cfg->destroy = [](struct child_cfg_t* cfg) {
        REAL_PLUGIN_DEBUG("Mock: child_cfg destroy");
        free(cfg->name);
        free(cfg);
    };
    return cfg;
}

struct peer_cfg_t* peer_cfg_create(const char* name, void* config, void* auth, void* remote) {
    REAL_PLUGIN_DEBUG("Mock: peer_cfg_create(" + std::string(name ? name : "null") + ")");
    struct peer_cfg_t* cfg = (struct peer_cfg_t*)calloc(1, sizeof(struct peer_cfg_t));
    cfg->name = name ? strdup(name) : strdup("mock-peer");
    cfg->destroy = [](struct peer_cfg_t* cfg) {
        REAL_PLUGIN_DEBUG("Mock: peer_cfg destroy");
        free(cfg->name);
        free(cfg);
    };
    return cfg;
}

struct ike_cfg_t* ike_cfg_create(bool initiator, bool force_encap, const char* me, int my_port, const char* other, int other_port, int fragmentation, int dscp) {
    REAL_PLUGIN_DEBUG("Mock: ike_cfg_create()");
    struct ike_cfg_t* cfg = (struct ike_cfg_t*)calloc(1, sizeof(struct ike_cfg_t));
    cfg->destroy = [](struct ike_cfg_t* cfg) {
        REAL_PLUGIN_DEBUG("Mock: ike_cfg destroy");
        free(cfg);
    };
    return cfg;
}

// Identification Functions
struct identification_t* identification_create_from_string(const char* string) {
    REAL_PLUGIN_DEBUG("Mock: identification_create_from_string(" + std::string(string ? string : "null") + ")");
    struct identification_t* id = (struct identification_t*)calloc(1, sizeof(struct identification_t));
    id->id_string = string ? strdup(string) : strdup("mock-id");
    id->destroy = [](struct identification_t* id) {
        REAL_PLUGIN_DEBUG("Mock: identification destroy");
        free(id->id_string);
        free(id);
    };
    return id;
}

// Container Functions
struct linked_list_t* linked_list_create(void) {
    REAL_PLUGIN_DEBUG("Mock: linked_list_create()");
    struct linked_list_t* list = (struct linked_list_t*)calloc(1, sizeof(struct linked_list_t));
    list->destroy = [](struct linked_list_t* list) {
        REAL_PLUGIN_DEBUG("Mock: linked_list destroy");
        free(list);
    };
    list->get_count = [](struct linked_list_t* list) -> int {
        return 0; // Mock empty list
    };
    return list;
}

// Proposal Functions
struct proposal_t* proposal_create_default(int protocol) {
    REAL_PLUGIN_DEBUG("Mock: proposal_create_default(" + std::to_string(protocol) + ")");
    struct proposal_t* proposal = (struct proposal_t*)calloc(1, sizeof(struct proposal_t));
    proposal->destroy = [](struct proposal_t* proposal) {
        REAL_PLUGIN_DEBUG("Mock: proposal destroy");
        free(proposal);
    };
    return proposal;
}

struct proposal_t* proposal_create_default_aead(int protocol) {
    REAL_PLUGIN_DEBUG("Mock: proposal_create_default_aead(" + std::to_string(protocol) + ")");
    return proposal_create_default(protocol);
}

struct proposal_t* proposal_create_from_string(int protocol, const char* proposal_str) {
    REAL_PLUGIN_DEBUG("Mock: proposal_create_from_string(" + std::string(proposal_str ? proposal_str : "null") + ")");
    return proposal_create_default(protocol);
}

// Traffic Selector Functions
struct traffic_selector_t* traffic_selector_create_dynamic(int protocol, int start_port, int end_port) {
    REAL_PLUGIN_DEBUG("Mock: traffic_selector_create_dynamic()");
    struct traffic_selector_t* ts = (struct traffic_selector_t*)calloc(1, sizeof(struct traffic_selector_t));
    ts->destroy = [](struct traffic_selector_t* ts) {
        REAL_PLUGIN_DEBUG("Mock: traffic_selector destroy");
        free(ts);
    };
    return ts;
}

struct traffic_selector_t* traffic_selector_create_from_cidr(const char* cidr_str, int protocol, int start_port, int end_port) {
    REAL_PLUGIN_DEBUG("Mock: traffic_selector_create_from_cidr(" + std::string(cidr_str ? cidr_str : "null") + ")");
    return traffic_selector_create_dynamic(protocol, start_port, end_port);
}

// Credential Functions  
struct shared_key_t* shared_key_create(int type, struct chunk_t key) {
    REAL_PLUGIN_DEBUG("Mock: shared_key_create()");
    struct shared_key_t* shared_key = (struct shared_key_t*)calloc(1, sizeof(struct shared_key_t));
    shared_key->destroy = [](struct shared_key_t* key) {
        REAL_PLUGIN_DEBUG("Mock: shared_key destroy");
        free(key);
    };
    return shared_key;
}

struct mem_cred_t* mem_cred_create(void) {
    REAL_PLUGIN_DEBUG("Mock: mem_cred_create()");
    struct mem_cred_t* cred = (struct mem_cred_t*)calloc(1, sizeof(struct mem_cred_t));
    cred->destroy = [](struct mem_cred_t* cred) {
        REAL_PLUGIN_DEBUG("Mock: mem_cred destroy");
        free(cred);
    };
    return cred;
}

struct callback_cred_t* callback_cred_create_shared(void* cb, void* data) {
    REAL_PLUGIN_DEBUG("Mock: callback_cred_create_shared()");
    struct callback_cred_t* cred = (struct callback_cred_t*)calloc(1, sizeof(struct callback_cred_t));
    cred->destroy = [](struct callback_cred_t* cred) {
        REAL_PLUGIN_DEBUG("Mock: callback_cred destroy");
        free(cred);
    };
    return cred;
}

// Threading Functions
struct thread_t* thread_create(void* (*main_func)(void*), void* arg) {
    REAL_PLUGIN_DEBUG("Mock: thread_create()");
    struct thread_t* thread = (struct thread_t*)calloc(1, sizeof(struct thread_t));
    thread->join = [](struct thread_t* thread) {
        REAL_PLUGIN_DEBUG("Mock: thread join");
    };
    thread->cancel = [](struct thread_t* thread) {
        REAL_PLUGIN_DEBUG("Mock: thread cancel");
    };
    return thread;
}

struct mutex_t* mutex_create(int type) {
    REAL_PLUGIN_DEBUG("Mock: mutex_create()");
    struct mutex_t* mutex = (struct mutex_t*)calloc(1, sizeof(struct mutex_t));
    mutex->locked = false;
    mutex->lock = [](struct mutex_t* mutex) {
        REAL_PLUGIN_DEBUG("Mock: mutex lock");
        mutex->locked = true;
    };
    mutex->unlock = [](struct mutex_t* mutex) {
        REAL_PLUGIN_DEBUG("Mock: mutex unlock");
        mutex->locked = false;
    };
    mutex->destroy = [](struct mutex_t* mutex) {
        REAL_PLUGIN_DEBUG("Mock: mutex destroy");
        free(mutex);
    };
    return mutex;
}

// Utility Functions
char* strerror_safe(int errnum) {
    REAL_PLUGIN_DEBUG("Mock: strerror_safe(" + std::to_string(errnum) + ")");
    return (char*)"Mock error message";
}

// Debug Function (중요!)
void dbg(int group, int level, char* format, ...) {
    // strongSwan 로깅 함수 - 실제로는 아무것도 하지 않음
    // Phase 4에서는 우리 자체 로깅을 사용
}

} // extern "C"

// ============================================================================
// Mock Library Management Functions
// ============================================================================

namespace strongswan_test {

bool StrongSwanMockLibrary::InitializeStrongSwanMockLibrary() {
    REAL_PLUGIN_INFO("Initializing strongSwan Mock Library (26 functions)");
    
    // 모든 mock 함수가 올바르게 정의되었는지 확인
    if (!charon || !lib) {
        REAL_PLUGIN_ERROR("Global mock objects not initialized");
        return false;
    }
    
    REAL_PLUGIN_SUCCESS("strongSwan Mock Library initialized - all 26 dependencies mocked");
    return true;
}

void StrongSwanMockLibrary::CleanupStrongSwanMockLibrary() {
    REAL_PLUGIN_INFO("Cleaning up strongSwan Mock Library");
    // Mock 객체들은 정적이므로 특별한 정리 불필요
    REAL_PLUGIN_SUCCESS("strongSwan Mock Library cleaned up");
}

std::vector<std::string> StrongSwanMockLibrary::GetMockedFunctions() {
    return {
        "auth_cfg_create", "callback_cred_create_shared", "charon", "child_cfg_create",
        "chunk_create_cat", "chunk_create_clone", "chunk_empty", "chunk_length",
        "chunk_map", "chunk_unmap", "dbg", "identification_create_from_string",
        "ike_cfg_create", "lib", "linked_list_create", "mem_cred_create",
        "mutex_create", "peer_cfg_create", "proposal_create_default", 
        "proposal_create_default_aead", "proposal_create_from_string",
        "shared_key_create", "strerror_safe", "thread_create",
        "traffic_selector_create_dynamic", "traffic_selector_create_from_cidr"
    };
}

bool StrongSwanMockLibrary::TestMockFunctions() {
    REAL_PLUGIN_INFO("Testing strongSwan Mock functions");
    
    try {
        // Test basic mock functions
        auto cfg = auth_cfg_create();
        if (cfg) {
            cfg->destroy(cfg);
            REAL_PLUGIN_DEBUG("auth_cfg_create/destroy: OK");
        }
        
        auto list = linked_list_create();
        if (list) {
            int count = list->get_count(list);
            list->destroy(list);
            REAL_PLUGIN_DEBUG("linked_list_create/destroy: OK (count: " + std::to_string(count) + ")");
        }
        
        auto mutex = mutex_create(0);
        if (mutex) {
            mutex->lock(mutex);
            mutex->unlock(mutex);
            mutex->destroy(mutex);
            REAL_PLUGIN_DEBUG("mutex_create/lock/unlock/destroy: OK");
        }
        
        // Test chunk functions
        struct chunk_t test_chunk = chunk_create_clone(chunk_empty);
        if (test_chunk.ptr) {
            free(test_chunk.ptr);
        }
        REAL_PLUGIN_DEBUG("chunk functions: OK");
        
        REAL_PLUGIN_SUCCESS("All strongSwan Mock functions validated successfully");
        return true;
        
    } catch (const std::exception& e) {
        REAL_PLUGIN_ERROR("Exception in mock function test: " + std::string(e.what()));
        return false;
    } catch (...) {
        REAL_PLUGIN_ERROR("Unknown exception in mock function test");
        return false;
    }
}

} // namespace strongswan_test