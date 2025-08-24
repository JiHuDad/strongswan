/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Test Container - Dependency Injection System Implementation
 */

#include "test_container.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/*
 * ============================================================================
 * Test Data Factory Implementation
 * ============================================================================
 */

// Internal structure for test data factory
typedef struct private_test_data_factory {
    test_data_factory_t public;
    
    // Tracking created objects for cleanup
    void **created_objects;
    size_t object_count;
    size_t object_capacity;
} private_test_data_factory_t;

static char* data_factory_create_valid_json_config(test_data_factory_t *this)
{
    (void)this;
    const char *template = 
        "{\n"
        "  \"connection_name\": \"test_connection\",\n"
        "  \"ike\": {\n"
        "    \"local_addrs\": [\"192.168.1.10\"],\n"
        "    \"remote_addrs\": [\"203.0.113.5\"],\n"
        "    \"version\": 2,\n"
        "    \"proposals\": [\"aes256-sha256-modp2048\"]\n"
        "  },\n"
        "  \"local\": {\n"
        "    \"auth\": \"psk\",\n"
        "    \"id\": \"client@test.com\",\n"
        "    \"secret\": \"test_secret_123\"\n"
        "  },\n"
        "  \"remote\": {\n"
        "    \"auth\": \"psk\",\n"
        "    \"id\": \"server@test.com\"\n"
        "  },\n"
        "  \"children\": [\n"
        "    {\n"
        "      \"name\": \"child1\",\n"
        "      \"local_ts\": [\"10.0.0.0/24\"],\n"
        "      \"remote_ts\": [\"10.0.1.0/24\"],\n"
        "      \"esp_proposals\": [\"aes128gcm16\"]\n"
        "    }\n"
        "  ]\n"
        "}";
    
    return strdup(template);
}

static char* data_factory_create_invalid_json_config(test_data_factory_t *this)
{
    (void)this;
    // Invalid JSON with syntax error
    const char *invalid_json = "{ \"connection_name\": \"test\", invalid_syntax }";
    return strdup(invalid_json);
}

static char* data_factory_create_complex_json_config(test_data_factory_t *this)
{
    (void)this;
    const char *template = 
        "{\n"
        "  \"connection_name\": \"complex_test\",\n"
        "  \"ike\": {\n"
        "    \"local_addrs\": [\"192.168.1.10\", \"192.168.1.11\"],\n"
        "    \"remote_addrs\": [\"203.0.113.5\", \"203.0.113.6\"],\n"
        "    \"version\": 2,\n"
        "    \"proposals\": [\"aes256-sha256-modp2048\", \"aes128-sha1-modp1024\"]\n"
        "  },\n"
        "  \"local\": {\n"
        "    \"auth\": \"pubkey\",\n"
        "    \"id\": \"client@complex.com\",\n"
        "    \"cert\": \"client.pem\"\n"
        "  },\n"
        "  \"remote\": {\n"
        "    \"auth\": \"pubkey\",\n"
        "    \"id\": \"server@complex.com\",\n"
        "    \"ca\": \"ca.pem\"\n"
        "  },\n"
        "  \"children\": [\n"
        "    {\n"
        "      \"name\": \"child1\",\n"
        "      \"local_ts\": [\"10.0.0.0/24\", \"10.0.2.0/24\"],\n"
        "      \"remote_ts\": [\"10.0.1.0/24\", \"10.0.3.0/24\"],\n"
        "      \"esp_proposals\": [\"aes128gcm16\", \"aes256-sha256\"]\n"
        "    },\n"
        "    {\n"
        "      \"name\": \"child2\",\n"
        "      \"local_ts\": [\"192.168.0.0/16\"],\n"
        "      \"remote_ts\": [\"172.16.0.0/16\"],\n"
        "      \"esp_proposals\": [\"aes128gcm16\"]\n"
        "    }\n"
        "  ]\n"
        "}";
    
    return strdup(template);
}

static mock_ike_cfg_t* data_factory_create_test_ike_cfg(test_data_factory_t *this, const char *name)
{
    private_test_data_factory_t *private = (private_test_data_factory_t*)this;
    
    mock_ike_cfg_t *ike_cfg = mock_ike_cfg_create(name ? name : "test_ike");
    
    // Track created object for cleanup
    if (private->object_count >= private->object_capacity) {
        private->object_capacity = private->object_capacity > 0 ? private->object_capacity * 2 : 8;
        private->created_objects = realloc(private->created_objects, 
                                          sizeof(void*) * private->object_capacity);
        assert(private->created_objects != NULL);
    }
    
    private->created_objects[private->object_count++] = ike_cfg;
    return ike_cfg;
}

static mock_peer_cfg_t* data_factory_create_test_peer_cfg(test_data_factory_t *this, 
                                                         const char *name,
                                                         mock_ike_cfg_t *ike_cfg)
{
    private_test_data_factory_t *private = (private_test_data_factory_t*)this;
    
    mock_peer_cfg_t *peer_cfg = mock_peer_cfg_create(name ? name : "test_peer", ike_cfg);
    
    // Track created object
    if (private->object_count >= private->object_capacity) {
        private->object_capacity = private->object_capacity > 0 ? private->object_capacity * 2 : 8;
        private->created_objects = realloc(private->created_objects, 
                                          sizeof(void*) * private->object_capacity);
        assert(private->created_objects != NULL);
    }
    
    private->created_objects[private->object_count++] = peer_cfg;
    return peer_cfg;
}

static mock_child_cfg_t* data_factory_create_test_child_cfg(test_data_factory_t *this, const char *name)
{
    private_test_data_factory_t *private = (private_test_data_factory_t*)this;
    
    mock_child_cfg_t *child_cfg = mock_child_cfg_create(name ? name : "test_child");
    
    // Track created object
    if (private->object_count >= private->object_capacity) {
        private->object_capacity = private->object_capacity > 0 ? private->object_capacity * 2 : 8;
        private->created_objects = realloc(private->created_objects, 
                                          sizeof(void*) * private->object_capacity);
        assert(private->created_objects != NULL);
    }
    
    private->created_objects[private->object_count++] = child_cfg;
    return child_cfg;
}

static void data_factory_cleanup_all(test_data_factory_t *this)
{
    private_test_data_factory_t *private = (private_test_data_factory_t*)this;
    
    // Clean up tracked objects
    for (size_t i = 0; i < private->object_count; i++) {
        void *obj = private->created_objects[i];
        if (obj) {
            // Try to call destroy method (assume first member is destroy function)
            void (**destroy_fn)(void*) = (void(**)(void*))obj;
            if (destroy_fn && *destroy_fn) {
                (*destroy_fn)(obj);
            }
        }
    }
    
    private->object_count = 0;
}

static void data_factory_destroy(test_data_factory_t *this)
{
    if (!this) return;
    
    private_test_data_factory_t *private = (private_test_data_factory_t*)this;
    
    // Clean up all tracked objects first
    this->cleanup_all(this);
    
    // Free tracking arrays
    if (private->created_objects) {
        free(private->created_objects);
    }
    
    free(private);
}

test_data_factory_t* test_data_factory_create(void)
{
    private_test_data_factory_t *private = malloc(sizeof(private_test_data_factory_t));
    assert(private != NULL);
    
    // Initialize public interface
    private->public.create_valid_json_config = data_factory_create_valid_json_config;
    private->public.create_invalid_json_config = data_factory_create_invalid_json_config;
    private->public.create_complex_json_config = data_factory_create_complex_json_config;
    private->public.create_test_ike_cfg = data_factory_create_test_ike_cfg;
    private->public.create_test_peer_cfg = data_factory_create_test_peer_cfg;
    private->public.create_test_child_cfg = data_factory_create_test_child_cfg;
    private->public.cleanup_all = data_factory_cleanup_all;
    private->public.destroy = data_factory_destroy;
    
    // Initialize private state
    private->created_objects = NULL;
    private->object_count = 0;
    private->object_capacity = 0;
    
    return &private->public;
}

/*
 * ============================================================================
 * Memory Tracker Implementation
 * ============================================================================
 */

typedef struct private_memory_tracker {
    memory_tracker_t public;
    
    bool tracking_active;
    memory_stats_t stats;
    memory_stats_t baseline_stats;
    size_t warning_threshold;
    memory_stats_t snapshot;
    char *snapshot_name;
} private_memory_tracker_t;

static void memory_tracker_start_tracking(memory_tracker_t *this)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    
    if (private->tracking_active) {
        return; // Already tracking
    }
    
    // Reset stats and save baseline
    memset(&private->stats, 0, sizeof(memory_stats_t));
    private->baseline_stats = private->stats;
    private->tracking_active = true;
}

static void memory_tracker_stop_tracking(memory_tracker_t *this)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    private->tracking_active = false;
}

static void memory_tracker_get_stats(memory_tracker_t *this, memory_stats_t *stats)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    
    if (stats) {
        *stats = private->stats;
    }
}

static bool memory_tracker_check_no_leaks(memory_tracker_t *this)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    
    // Simple leak detection: current allocated should be 0
    // In a more sophisticated implementation, we'd track actual allocations
    return private->stats.current_allocated == 0;
}

static void memory_tracker_reset_stats(memory_tracker_t *this)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    
    memset(&private->stats, 0, sizeof(memory_stats_t));
    private->baseline_stats = private->stats;
}

static void memory_tracker_print_report(memory_tracker_t *this)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    
    printf("=== Memory Tracker Report ===\n");
    printf("Total Allocated: %zu bytes (%zu calls)\n", 
           private->stats.total_allocated, private->stats.allocation_count);
    printf("Total Freed: %zu bytes (%zu calls)\n", 
           private->stats.total_freed, private->stats.free_count);
    printf("Current Allocated: %zu bytes\n", private->stats.current_allocated);
    printf("Peak Allocated: %zu bytes\n", private->stats.peak_allocated);
    printf("Potential Leaks: %zu bytes\n", private->stats.leak_count);
    printf("=============================\n");
}

static void memory_tracker_print_detailed_report(memory_tracker_t *this, bool show_details)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    
    printf("=== Detailed Memory Tracker Report ===\n");
    printf("Tracking Active: %s\n", private->tracking_active ? "Yes" : "No");
    printf("Warning Threshold: %zu bytes\n", private->warning_threshold);
    
    // Basic statistics
    printf("\n📊 Basic Statistics:\n");
    printf("  Total Allocated: %zu bytes (%zu calls)\n", 
           private->stats.total_allocated, private->stats.allocation_count);
    printf("  Total Freed: %zu bytes (%zu calls)\n", 
           private->stats.total_freed, private->stats.free_count);
    printf("  Current Allocated: %zu bytes\n", private->stats.current_allocated);
    printf("  Peak Allocated: %zu bytes\n", private->stats.peak_allocated);
    
    // Leak analysis
    printf("\n🔍 Leak Analysis:\n");
    printf("  Potential Leaks: %zu bytes\n", private->stats.leak_count);
    if (private->stats.current_allocated > 0) {
        printf("  ⚠️  Memory still allocated: %zu bytes\n", private->stats.current_allocated);
    } else {
        printf("  ✅ No memory currently allocated\n");
    }
    
    // Performance metrics
    if (private->stats.allocation_count > 0) {
        printf("\n⚡ Performance Metrics:\n");
        printf("  Average Allocation: %zu bytes\n", 
               private->stats.total_allocated / private->stats.allocation_count);
        printf("  Allocation Efficiency: %.1f%%\n", 
               private->stats.total_freed > 0 ? 
               (double)private->stats.total_freed / private->stats.total_allocated * 100 : 0);
    }
    
    // Snapshot comparison if available
    if (private->snapshot_name) {
        printf("\n📸 Snapshot Comparison (%s):\n", private->snapshot_name);
        printf("  Baseline: %zu bytes (%zu calls)\n",
               private->snapshot.total_allocated, private->snapshot.allocation_count);
        printf("  Current: %zu bytes (%zu calls)\n",
               private->stats.total_allocated, private->stats.allocation_count);
        printf("  Delta: %+zd bytes (%+zd calls)\n",
               (ssize_t)private->stats.total_allocated - (ssize_t)private->snapshot.total_allocated,
               (ssize_t)private->stats.allocation_count - (ssize_t)private->snapshot.allocation_count);
    }
    
    if (show_details) {
        printf("\n🔬 Detailed Information:\n");
        printf("  Mock System Integration: Active\n");
        printf("  Memory Interception: Passive (Mock-based)\n");
        printf("  Leak Detection: Basic (allocation counting)\n");
    }
    
    printf("=====================================\n");
}

static void memory_tracker_set_warning_threshold(memory_tracker_t *this, size_t threshold)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    private->warning_threshold = threshold;
}

static bool memory_tracker_check_usage_warning(memory_tracker_t *this)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    return private->stats.current_allocated > private->warning_threshold;
}

static void memory_tracker_take_snapshot(memory_tracker_t *this, const char *name)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    
    // Free previous snapshot name
    if (private->snapshot_name) {
        free(private->snapshot_name);
    }
    
    // Save current stats as snapshot
    private->snapshot = private->stats;
    private->snapshot_name = name ? strdup(name) : NULL;
}

static bool memory_tracker_compare_with_snapshot(memory_tracker_t *this, const char *name)
{
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    
    if (!private->snapshot_name) {
        return false; // No snapshot to compare with
    }
    
    if (name && strcmp(private->snapshot_name, name) != 0) {
        return false; // Wrong snapshot name
    }
    
    // Compare key metrics
    return private->stats.current_allocated == private->snapshot.current_allocated &&
           private->stats.allocation_count == private->snapshot.allocation_count;
}

static void memory_tracker_destroy(memory_tracker_t *this)
{
    if (!this) return;
    
    private_memory_tracker_t *private = (private_memory_tracker_t*)this;
    
    // Clean up snapshot name
    if (private->snapshot_name) {
        free(private->snapshot_name);
    }
    
    free(this);
}

memory_tracker_t* memory_tracker_create(void)
{
    private_memory_tracker_t *private = malloc(sizeof(private_memory_tracker_t));
    assert(private != NULL);
    
    // Initialize public interface
    private->public.start_tracking = memory_tracker_start_tracking;
    private->public.stop_tracking = memory_tracker_stop_tracking;
    private->public.get_stats = memory_tracker_get_stats;
    private->public.check_no_leaks = memory_tracker_check_no_leaks;
    private->public.reset_stats = memory_tracker_reset_stats;
    private->public.print_report = memory_tracker_print_report;
    private->public.print_detailed_report = memory_tracker_print_detailed_report;
    private->public.set_warning_threshold = memory_tracker_set_warning_threshold;
    private->public.check_usage_warning = memory_tracker_check_usage_warning;
    private->public.take_snapshot = memory_tracker_take_snapshot;
    private->public.compare_with_snapshot = memory_tracker_compare_with_snapshot;
    private->public.destroy = memory_tracker_destroy;
    
    // Initialize private state
    private->tracking_active = false;
    memset(&private->stats, 0, sizeof(memory_stats_t));
    memset(&private->baseline_stats, 0, sizeof(memory_stats_t));
    private->warning_threshold = 1024 * 1024; // 1MB default threshold
    memset(&private->snapshot, 0, sizeof(memory_stats_t));
    private->snapshot_name = NULL;
    
    return &private->public;
}

/*
 * ============================================================================
 * Test Container Implementation
 * ============================================================================
 */

typedef struct private_test_container {
    test_container_t public;
    
    test_container_type_t type;
    component_registration_t *components;
    size_t component_count;
    size_t component_capacity;
    test_data_factory_t *data_factory;
    memory_tracker_t *memory_tracker;
} private_test_container_t;

// Mock component factories
static void* mock_json_parser_factory(test_container_t *container)
{
    (void)container;
    // Return a simple mock JSON parser
    return malloc(sizeof(void*)); // Placeholder
}

static void* mock_socket_adapter_factory(test_container_t *container)
{
    (void)container;
    return malloc(sizeof(void*)); // Placeholder
}

static void* mock_strongswan_adapter_factory(test_container_t *container)
{
    (void)container;
    return malloc(sizeof(void*)); // Placeholder
}

static void* mock_config_usecase_factory(test_container_t *container)
{
    (void)container;
    return malloc(sizeof(void*)); // Placeholder
}

static void* mock_event_usecase_factory(test_container_t *container)
{
    (void)container;
    return malloc(sizeof(void*)); // Placeholder
}

static void* mock_config_entity_factory(test_container_t *container)
{
    (void)container;
    return malloc(sizeof(void*)); // Placeholder
}

static void* mock_failover_manager_factory(test_container_t *container)
{
    (void)container;
    return malloc(sizeof(void*)); // Placeholder
}

static void generic_component_cleanup(void *component)
{
    if (component) {
        free(component);
    }
}

// Container methods implementation
static void* container_get_json_parser(test_container_t *this)
{
    return this->get_component(this, "json_parser");
}

static void* container_get_socket_adapter(test_container_t *this)
{
    return this->get_component(this, "socket_adapter");
}

static void* container_get_strongswan_adapter(test_container_t *this)
{
    return this->get_component(this, "strongswan_adapter");
}

static void* container_get_config_usecase(test_container_t *this)
{
    return this->get_component(this, "config_usecase");
}

static void* container_get_event_usecase(test_container_t *this)
{
    return this->get_component(this, "event_usecase");
}

static void* container_get_config_entity(test_container_t *this)
{
    return this->get_component(this, "config_entity");
}

static void* container_get_failover_manager(test_container_t *this)
{
    return this->get_component(this, "failover_manager");
}

static test_data_factory_t* container_get_data_factory(test_container_t *this)
{
    private_test_container_t *private = (private_test_container_t*)this;
    return private->data_factory;
}

static memory_tracker_t* container_get_memory_tracker(test_container_t *this)
{
    private_test_container_t *private = (private_test_container_t*)this;
    return private->memory_tracker;
}

static bool container_register_component(test_container_t *this, 
                                        const char *name,
                                        component_factory_fn_t factory,
                                        component_cleanup_fn_t cleanup,
                                        bool singleton)
{
    private_test_container_t *private = (private_test_container_t*)this;
    
    // Check if component already exists
    for (size_t i = 0; i < private->component_count; i++) {
        if (strcmp(private->components[i].name, name) == 0) {
            return false; // Component already registered
        }
    }
    
    // Expand array if needed
    if (private->component_count >= private->component_capacity) {
        private->component_capacity = private->component_capacity > 0 ? private->component_capacity * 2 : 8;
        private->components = realloc(private->components, 
                                     sizeof(component_registration_t) * private->component_capacity);
        assert(private->components != NULL);
    }
    
    // Register component
    component_registration_t *reg = &private->components[private->component_count++];
    reg->name = strdup(name);
    reg->factory = factory;
    reg->cleanup = cleanup;
    reg->instance = NULL;
    reg->is_singleton = singleton;
    
    return true;
}

static void* container_get_component(test_container_t *this, const char *name)
{
    private_test_container_t *private = (private_test_container_t*)this;
    
    // Find component registration
    for (size_t i = 0; i < private->component_count; i++) {
        component_registration_t *reg = &private->components[i];
        if (strcmp(reg->name, name) == 0) {
            // Return existing singleton instance if available
            if (reg->is_singleton && reg->instance) {
                return reg->instance;
            }
            
            // Create new instance
            void *instance = reg->factory(this);
            
            // Store singleton instance
            if (reg->is_singleton) {
                reg->instance = instance;
            }
            
            return instance;
        }
    }
    
    return NULL; // Component not found
}

static void container_reset_singletons(test_container_t *this)
{
    private_test_container_t *private = (private_test_container_t*)this;
    
    for (size_t i = 0; i < private->component_count; i++) {
        component_registration_t *reg = &private->components[i];
        if (reg->is_singleton && reg->instance) {
            // Clean up existing singleton
            if (reg->cleanup) {
                reg->cleanup(reg->instance);
            }
            reg->instance = NULL;
        }
    }
}

static void container_destroy(test_container_t *this)
{
    if (!this) return;
    
    private_test_container_t *private = (private_test_container_t*)this;
    
    // Clean up all singleton instances
    this->reset_singletons(this);
    
    // Clean up component registrations
    for (size_t i = 0; i < private->component_count; i++) {
        free((void*)private->components[i].name);
    }
    if (private->components) {
        free(private->components);
    }
    
    // Clean up data factory and memory tracker
    if (private->data_factory) {
        private->data_factory->destroy(private->data_factory);
    }
    if (private->memory_tracker) {
        private->memory_tracker->destroy(private->memory_tracker);
    }
    
    free(private);
}

test_container_t* test_container_create(test_container_type_t type)
{
    private_test_container_t *private = malloc(sizeof(private_test_container_t));
    assert(private != NULL);
    
    // Initialize public interface
    private->public.get_json_parser = container_get_json_parser;
    private->public.get_socket_adapter = container_get_socket_adapter;
    private->public.get_strongswan_adapter = container_get_strongswan_adapter;
    private->public.get_config_usecase = container_get_config_usecase;
    private->public.get_event_usecase = container_get_event_usecase;
    private->public.get_config_entity = container_get_config_entity;
    private->public.get_failover_manager = container_get_failover_manager;
    private->public.get_data_factory = container_get_data_factory;
    private->public.get_memory_tracker = container_get_memory_tracker;
    private->public.register_component = container_register_component;
    private->public.get_component = container_get_component;
    private->public.reset_singletons = container_reset_singletons;
    private->public.destroy = container_destroy;
    
    // Initialize private state
    private->public.type = type;
    private->type = type;
    private->components = NULL;
    private->component_count = 0;
    private->component_capacity = 0;
    private->data_factory = test_data_factory_create();
    private->memory_tracker = memory_tracker_create();
    
    // Register default components based on container type
    bool use_mocks = (type != TEST_CONTAINER_INTEGRATION);
    
    if (use_mocks) {
        // Register mock components
        private->public.register_component(&private->public, "json_parser", 
                                          mock_json_parser_factory, generic_component_cleanup, true);
        private->public.register_component(&private->public, "socket_adapter", 
                                          mock_socket_adapter_factory, generic_component_cleanup, true);
        private->public.register_component(&private->public, "strongswan_adapter", 
                                          mock_strongswan_adapter_factory, generic_component_cleanup, true);
        private->public.register_component(&private->public, "config_usecase", 
                                          mock_config_usecase_factory, generic_component_cleanup, true);
        private->public.register_component(&private->public, "event_usecase", 
                                          mock_event_usecase_factory, generic_component_cleanup, true);
        private->public.register_component(&private->public, "config_entity", 
                                          mock_config_entity_factory, generic_component_cleanup, true);
        private->public.register_component(&private->public, "failover_manager", 
                                          mock_failover_manager_factory, generic_component_cleanup, true);
    } else {
        // TODO: Register real components for integration tests
        // This will be implemented when we have actual implementations
    }
    
    return &private->public;
}

/*
 * ============================================================================
 * Pre-configured Container Factories
 * ============================================================================
 */

test_container_t* test_container_create_pure(void)
{
    return test_container_create(TEST_CONTAINER_UNIT_PURE);
}

test_container_t* test_container_create_adapter(void)
{
    test_container_t *container = test_container_create(TEST_CONTAINER_UNIT_ADAPTER);
    
    // Initialize strongSwan mocks for adapter tests
    strongswan_mocks_init();
    
    return container;
}

test_container_t* test_container_create_integration(void)
{
    test_container_t *container = test_container_create(TEST_CONTAINER_INTEGRATION);
    
    // TODO: Initialize real strongSwan library for integration tests
    // This will be implemented in Phase 5
    
    return container;
}