/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Test Container - Dependency Injection System for Tests
 * 
 * This system provides a centralized way to manage dependencies
 * for different types of tests (unit, adapter, integration).
 */

#ifndef TEST_CONTAINER_H
#define TEST_CONTAINER_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct test_container test_container_t;
typedef struct test_data_factory test_data_factory_t;
typedef struct memory_tracker memory_tracker_t;

// Include actual and mock types
#include "strongswan_mocks.h"

// Actual implementation headers (will be added in Phase 5)
#ifdef INTEGRATION_TEST
// TODO Phase 5: Add real implementation headers when available:
// #include "../../adapters/json/extsock_json_parser.h"
// #include "../../usecases/extsock_config_usecase.h"
// #include "../../usecases/extsock_event_usecase.h"
// #include "../../domain/extsock_config_entity.h"
#endif

/*
 * ============================================================================
 * Test Container Types
 * ============================================================================
 */

/**
 * Test container configuration
 */
typedef enum {
    TEST_CONTAINER_UNIT_PURE,       // Pure unit tests (no strongSwan)
    TEST_CONTAINER_UNIT_ADAPTER,    // Adapter tests (mock strongSwan)
    TEST_CONTAINER_INTEGRATION      // Integration tests (real strongSwan)
} test_container_type_t;

/**
 * Component factory function types
 */
typedef void* (*component_factory_fn_t)(test_container_t *container);
typedef void (*component_cleanup_fn_t)(void *component);

/**
 * Component registration structure
 */
typedef struct {
    const char *name;
    component_factory_fn_t factory;
    component_cleanup_fn_t cleanup;
    void *instance;
    bool is_singleton;
} component_registration_t;

/*
 * ============================================================================
 * Test Container Interface
 * ============================================================================
 */

/**
 * Test container main structure
 */
struct test_container {
    /**
     * Get JSON parser (real or mock based on container type)
     * 
     * @param this      container instance
     * @return          JSON parser instance
     */
    void* (*get_json_parser)(test_container_t *this);
    
    /**
     * Get socket adapter (real or mock)
     * 
     * @param this      container instance
     * @return          socket adapter instance
     */
    void* (*get_socket_adapter)(test_container_t *this);
    
    /**
     * Get strongSwan adapter (real or mock)
     * 
     * @param this      container instance
     * @return          strongSwan adapter instance
     */
    void* (*get_strongswan_adapter)(test_container_t *this);
    
    /**
     * Get config usecase (real or mock)
     * 
     * @param this      container instance
     * @return          config usecase instance
     */
    void* (*get_config_usecase)(test_container_t *this);
    
    /**
     * Get event usecase (real or mock)
     * 
     * @param this      container instance
     * @return          event usecase instance
     */
    void* (*get_event_usecase)(test_container_t *this);
    
    /**
     * Get config entity (real or mock)
     * 
     * @param this      container instance
     * @return          config entity instance
     */
    void* (*get_config_entity)(test_container_t *this);
    
    /**
     * Get failover manager (real or mock)
     * 
     * @param this      container instance
     * @return          failover manager instance
     */
    void* (*get_failover_manager)(test_container_t *this);
    
    /**
     * Get test data factory
     * 
     * @param this      container instance
     * @return          test data factory instance
     */
    test_data_factory_t* (*get_data_factory)(test_container_t *this);
    
    /**
     * Get memory tracker
     * 
     * @param this      container instance
     * @return          memory tracker instance
     */
    memory_tracker_t* (*get_memory_tracker)(test_container_t *this);
    
    /**
     * Register a component factory
     * 
     * @param this      container instance
     * @param name      component name
     * @param factory   factory function
     * @param cleanup   cleanup function
     * @param singleton whether component is singleton
     * @return          true on success
     */
    bool (*register_component)(test_container_t *this, 
                              const char *name,
                              component_factory_fn_t factory,
                              component_cleanup_fn_t cleanup,
                              bool singleton);
    
    /**
     * Get component by name
     * 
     * @param this      container instance
     * @param name      component name
     * @return          component instance or NULL
     */
    void* (*get_component)(test_container_t *this, const char *name);
    
    /**
     * Reset all singleton instances (for test isolation)
     * 
     * @param this      container instance
     */
    void (*reset_singletons)(test_container_t *this);
    
    /**
     * Destroy container and all managed components
     */
    void (*destroy)(test_container_t *this);
    
    // Internal state
    test_container_type_t type;
    component_registration_t *components;
    size_t component_count;
    size_t component_capacity;
    test_data_factory_t *data_factory;
    memory_tracker_t *memory_tracker;
};

/*
 * ============================================================================
 * Test Data Factory Interface
 * ============================================================================
 */

/**
 * Test data factory for creating test data
 */
struct test_data_factory {
    /**
     * Create valid JSON config string
     * 
     * @param this      factory instance
     * @return          JSON config string (caller must free)
     */
    char* (*create_valid_json_config)(test_data_factory_t *this);
    
    /**
     * Create invalid JSON config string
     * 
     * @param this      factory instance
     * @return          invalid JSON string (caller must free)
     */
    char* (*create_invalid_json_config)(test_data_factory_t *this);
    
    /**
     * Create complex JSON config with multiple children
     * 
     * @param this      factory instance
     * @return          complex JSON string (caller must free)
     */
    char* (*create_complex_json_config)(test_data_factory_t *this);
    
    /**
     * Create test IKE config (mock)
     * 
     * @param this      factory instance
     * @param name      config name
     * @return          mock IKE config
     */
    mock_ike_cfg_t* (*create_test_ike_cfg)(test_data_factory_t *this, const char *name);
    
    /**
     * Create test peer config (mock)
     * 
     * @param this      factory instance
     * @param name      config name
     * @param ike_cfg   associated IKE config
     * @return          mock peer config
     */
    mock_peer_cfg_t* (*create_test_peer_cfg)(test_data_factory_t *this, 
                                            const char *name,
                                            mock_ike_cfg_t *ike_cfg);
    
    /**
     * Create test child config (mock)
     * 
     * @param this      factory instance
     * @param name      config name
     * @return          mock child config
     */
    mock_child_cfg_t* (*create_test_child_cfg)(test_data_factory_t *this, const char *name);
    
    /**
     * Clean up all created test data
     * 
     * @param this      factory instance
     */
    void (*cleanup_all)(test_data_factory_t *this);
    
    /**
     * Destroy factory
     */
    void (*destroy)(test_data_factory_t *this);
    
    // Internal state for tracking created objects
    void **created_objects;
    size_t object_count;
    size_t object_capacity;
};

/*
 * ============================================================================
 * Memory Tracker Interface
 * ============================================================================
 */

/**
 * Memory usage statistics
 */
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_allocated;
    size_t peak_allocated;
    size_t allocation_count;
    size_t free_count;
    size_t leak_count;
} memory_stats_t;

/**
 * Memory tracker for detecting leaks and monitoring allocations
 */
struct memory_tracker {
    /**
     * Start tracking memory
     * 
     * @param this      tracker instance
     */
    void (*start_tracking)(memory_tracker_t *this);
    
    /**
     * Stop tracking memory
     * 
     * @param this      tracker instance
     */
    void (*stop_tracking)(memory_tracker_t *this);
    
    /**
     * Get current memory statistics
     * 
     * @param this      tracker instance
     * @param stats     output statistics
     */
    void (*get_stats)(memory_tracker_t *this, memory_stats_t *stats);
    
    /**
     * Check for memory leaks
     * 
     * @param this      tracker instance
     * @return          true if no leaks detected
     */
    bool (*check_no_leaks)(memory_tracker_t *this);
    
    /**
     * Reset statistics
     * 
     * @param this      tracker instance
     */
    void (*reset_stats)(memory_tracker_t *this);
    
    /**
     * Print memory report
     * 
     * @param this      tracker instance
     */
    void (*print_report)(memory_tracker_t *this);
    
    /**
     * Print detailed memory report with leak information
     * 
     * @param this      tracker instance
     * @param show_details include allocation details
     */
    void (*print_detailed_report)(memory_tracker_t *this, bool show_details);
    
    /**
     * Set memory usage warning threshold
     * 
     * @param this      tracker instance
     * @param threshold warning threshold in bytes
     */
    void (*set_warning_threshold)(memory_tracker_t *this, size_t threshold);
    
    /**
     * Check if memory usage exceeds threshold
     * 
     * @param this      tracker instance
     * @return          true if usage exceeds warning threshold
     */
    bool (*check_usage_warning)(memory_tracker_t *this);
    
    /**
     * Take memory snapshot for comparison
     * 
     * @param this      tracker instance
     * @param name      snapshot name for identification
     */
    void (*take_snapshot)(memory_tracker_t *this, const char *name);
    
    /**
     * Compare current state with last snapshot
     * 
     * @param this      tracker instance
     * @param name      snapshot name to compare with
     * @return          true if memory usage is same as snapshot
     */
    bool (*compare_with_snapshot)(memory_tracker_t *this, const char *name);
    
    /**
     * Destroy tracker
     */
    void (*destroy)(memory_tracker_t *this);
    
    // Internal state
    bool tracking_active;
    memory_stats_t stats;
    memory_stats_t baseline_stats;
    size_t warning_threshold;
    memory_stats_t snapshot;
    char *snapshot_name;
};

/*
 * ============================================================================
 * Factory Functions
 * ============================================================================
 */

/**
 * Create test container for specific test type
 * 
 * @param type      container type (pure, adapter, integration)
 * @return          container instance
 */
test_container_t* test_container_create(test_container_type_t type);

/**
 * Create test data factory
 * 
 * @return          data factory instance
 */
test_data_factory_t* test_data_factory_create(void);

/**
 * Create memory tracker
 * 
 * @return          memory tracker instance
 */
memory_tracker_t* memory_tracker_create(void);

/*
 * ============================================================================
 * Pre-configured Container Factories
 * ============================================================================
 */

/**
 * Create container for pure unit tests (no strongSwan dependencies)
 */
test_container_t* test_container_create_pure(void);

/**
 * Create container for adapter tests (mock strongSwan)
 */
test_container_t* test_container_create_adapter(void);

/**
 * Create container for integration tests (real strongSwan)
 */
test_container_t* test_container_create_integration(void);

/*
 * ============================================================================
 * Container Assertion Macros
 * ============================================================================
 */

#define CONTAINER_ASSERT_COMPONENT_NOT_NULL(container, component_name) \
    do { \
        void *component = container->get_component(container, component_name); \
        if (!component) { \
            fprintf(stderr, "CONTAINER_ASSERT_FAILED: Component '%s' is NULL\n", component_name); \
            abort(); \
        } \
    } while(0)

#define CONTAINER_ASSERT_NO_MEMORY_LEAKS(container) \
    do { \
        memory_tracker_t *tracker = container->get_memory_tracker(container); \
        if (!tracker->check_no_leaks(tracker)) { \
            fprintf(stderr, "CONTAINER_ASSERT_FAILED: Memory leaks detected\n"); \
            tracker->print_detailed_report(tracker, true); \
            abort(); \
        } \
    } while(0)

#define CONTAINER_ASSERT_MEMORY_USAGE_UNDER(container, max_bytes) \
    do { \
        memory_tracker_t *tracker = container->get_memory_tracker(container); \
        memory_stats_t stats; \
        tracker->get_stats(tracker, &stats); \
        if (stats.current_allocated > (max_bytes)) { \
            fprintf(stderr, "CONTAINER_ASSERT_FAILED: Memory usage %zu > %zu bytes\n", \
                    stats.current_allocated, (size_t)(max_bytes)); \
            tracker->print_detailed_report(tracker, true); \
            abort(); \
        } \
    } while(0)

#define CONTAINER_SET_MEMORY_WARNING_THRESHOLD(container, threshold) \
    do { \
        memory_tracker_t *tracker = container->get_memory_tracker(container); \
        tracker->set_warning_threshold(tracker, threshold); \
    } while(0)

#define CONTAINER_TAKE_MEMORY_SNAPSHOT(container, name) \
    do { \
        memory_tracker_t *tracker = container->get_memory_tracker(container); \
        tracker->take_snapshot(tracker, name); \
    } while(0)

#define CONTAINER_ASSERT_MEMORY_UNCHANGED_SINCE_SNAPSHOT(container, name) \
    do { \
        memory_tracker_t *tracker = container->get_memory_tracker(container); \
        if (!tracker->compare_with_snapshot(tracker, name)) { \
            fprintf(stderr, "CONTAINER_ASSERT_FAILED: Memory changed since snapshot '%s'\n", name); \
            tracker->print_detailed_report(tracker, true); \
            abort(); \
        } \
    } while(0)

#define CONTAINER_ASSERT_COMPONENT_TYPE(container, component_name, expected_type) \
    do { \
        void *component = container->get_component(container, component_name); \
        if (!component) { \
            fprintf(stderr, "CONTAINER_ASSERT_FAILED: Component '%s' is NULL\n", component_name); \
            abort(); \
        } \
        /* Type checking would be implementation-specific */ \
    } while(0)

/*
 * ============================================================================
 * Test Fixture Helpers
 * ============================================================================
 */

/**
 * Standard test setup using container
 * Call this in your test setup function
 */
#define CONTAINER_SETUP(container_var, container_type) \
    container_var = test_container_create(container_type); \
    if (container_var) { \
        container_var->get_memory_tracker(container_var)->start_tracking( \
            container_var->get_memory_tracker(container_var)); \
    }

/**
 * Standard test teardown using container
 * Call this in your test teardown function
 */
#define CONTAINER_TEARDOWN(container_var) \
    if (container_var) { \
        memory_tracker_t *tracker = container_var->get_memory_tracker(container_var); \
        tracker->stop_tracking(tracker); \
        CONTAINER_ASSERT_NO_MEMORY_LEAKS(container_var); \
        container_var->destroy(container_var); \
        container_var = NULL; \
    }

#endif // TEST_CONTAINER_H