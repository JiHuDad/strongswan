/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Advanced Memory Tracking System
 * TASK-004: Memory Tracking System
 * 
 * This provides sophisticated memory leak detection, allocation tracking,
 * and debugging capabilities for the extsock plugin tests.
 */

#ifndef ADVANCED_MEMORY_TRACKER_H
#define ADVANCED_MEMORY_TRACKER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * Advanced Memory Statistics
 * ============================================================================
 */

typedef struct {
    // Basic allocation statistics
    size_t total_allocated;
    size_t total_freed;  
    size_t current_allocated;
    size_t peak_allocated;
    size_t allocation_count;
    size_t free_count;
    
    // Advanced statistics
    size_t realloc_count;
    size_t calloc_count;
    size_t average_allocation_size;
    size_t largest_allocation;
    size_t smallest_allocation;
    
    // Leak detection
    size_t leak_count;
    size_t potential_leaks;
    size_t orphaned_frees;
    
    // Performance metrics
    uint64_t total_allocation_time_ns;
    uint64_t total_free_time_ns;
    uint64_t fastest_allocation_ns;
    uint64_t slowest_allocation_ns;
} advanced_memory_stats_t;

/*
 * ============================================================================
 * Allocation Record for Leak Detection
 * ============================================================================
 */

typedef struct allocation_record {
    void *ptr;                    // Allocated pointer
    size_t size;                  // Size of allocation
    const char *file;             // Source file
    int line;                     // Source line
    const char *function;         // Function name
    uint64_t timestamp_ns;        // Allocation timestamp
    uint32_t allocation_id;       // Unique ID for this allocation
    struct allocation_record *next; // Linked list next
} allocation_record_t;

/*
 * ============================================================================
 * Advanced Memory Tracker Interface
 * ============================================================================
 */

typedef struct advanced_memory_tracker advanced_memory_tracker_t;

struct advanced_memory_tracker {
    /**
     * Start tracking memory allocations
     * 
     * @param this      tracker instance
     */
    void (*start_tracking)(advanced_memory_tracker_t *this);
    
    /**
     * Stop tracking memory allocations
     * 
     * @param this      tracker instance
     */
    void (*stop_tracking)(advanced_memory_tracker_t *this);
    
    /**
     * Get current memory statistics
     * 
     * @param this      tracker instance
     * @param stats     output statistics
     */
    void (*get_stats)(advanced_memory_tracker_t *this, advanced_memory_stats_t *stats);
    
    /**
     * Check for memory leaks
     * 
     * @param this      tracker instance
     * @return          true if no leaks detected
     */
    bool (*check_no_leaks)(advanced_memory_tracker_t *this);
    
    /**
     * Get list of current allocations (for leak detection)
     * 
     * @param this      tracker instance
     * @param count     output count of allocations
     * @return          array of allocation records (caller must not free)
     */
    allocation_record_t* (*get_allocations)(advanced_memory_tracker_t *this, size_t *count);
    
    /**
     * Print detailed memory report
     * 
     * @param this      tracker instance
     * @param show_leaks include leak details
     */
    void (*print_report)(advanced_memory_tracker_t *this, bool show_leaks);
    
    /**
     * Print allocation backtrace (if available)
     * 
     * @param this      tracker instance
     * @param ptr       pointer to check
     */
    void (*print_allocation_backtrace)(advanced_memory_tracker_t *this, void *ptr);
    
    /**
     * Set maximum allowed allocations (for testing allocation limits)
     * 
     * @param this      tracker instance
     * @param max_allocs maximum number of allocations allowed
     */
    void (*set_allocation_limit)(advanced_memory_tracker_t *this, size_t max_allocs);
    
    /**
     * Set memory limit in bytes
     * 
     * @param this      tracker instance
     * @param max_bytes maximum bytes that can be allocated
     */
    void (*set_memory_limit)(advanced_memory_tracker_t *this, size_t max_bytes);
    
    /**
     * Simulate allocation failure at specific count
     * 
     * @param this      tracker instance
     * @param fail_at   allocation count to fail at (0 = no failure)
     */
    void (*set_failure_point)(advanced_memory_tracker_t *this, size_t fail_at);
    
    /**
     * Reset all statistics and tracking data
     * 
     * @param this      tracker instance
     */
    void (*reset)(advanced_memory_tracker_t *this);
    
    /**
     * Destroy tracker
     */
    void (*destroy)(advanced_memory_tracker_t *this);
    
    // Internal state (opaque to users)
    void *private_data;
};

/*
 * ============================================================================
 * Factory Functions
 * ============================================================================
 */

/**
 * Create advanced memory tracker
 * 
 * @return      memory tracker instance
 */
advanced_memory_tracker_t* advanced_memory_tracker_create(void);

/*
 * ============================================================================
 * Memory Interception Macros (for testing)
 * ============================================================================
 */

#ifdef ENABLE_MEMORY_TRACKING

// Memory allocation interception macros
#define tracked_malloc(size) \
    _tracked_malloc((size), __FILE__, __LINE__, __FUNCTION__)

#define tracked_calloc(count, size) \
    _tracked_calloc((count), (size), __FILE__, __LINE__, __FUNCTION__)

#define tracked_realloc(ptr, size) \
    _tracked_realloc((ptr), (size), __FILE__, __LINE__, __FUNCTION__)

#define tracked_free(ptr) \
    _tracked_free((ptr), __FILE__, __LINE__, __FUNCTION__)

// Underlying functions (implemented in .c file)
void* _tracked_malloc(size_t size, const char *file, int line, const char *func);
void* _tracked_calloc(size_t count, size_t size, const char *file, int line, const char *func);
void* _tracked_realloc(void *ptr, size_t size, const char *file, int line, const char *func);
void  _tracked_free(void *ptr, const char *file, int line, const char *func);

// Set global tracker for interception
void set_global_memory_tracker(advanced_memory_tracker_t *tracker);

#else

// When tracking is disabled, use standard functions
#define tracked_malloc(size) malloc(size)
#define tracked_calloc(count, size) calloc(count, size)  
#define tracked_realloc(ptr, size) realloc(ptr, size)
#define tracked_free(ptr) free(ptr)

#endif // ENABLE_MEMORY_TRACKING

/*
 * ============================================================================
 * Test Helper Macros
 * ============================================================================
 */

// Memory tracking test assertions
#define ASSERT_NO_MEMORY_LEAKS(tracker) \
    do { \
        if (!(tracker)->check_no_leaks(tracker)) { \
            advanced_memory_stats_t stats; \
            (tracker)->get_stats(tracker, &stats); \
            fprintf(stderr, "MEMORY_LEAK_DETECTED: %zu bytes in %zu allocations\n", \
                    stats.current_allocated, stats.leak_count); \
            (tracker)->print_report(tracker, true); \
            abort(); \
        } \
    } while(0)

#define ASSERT_MEMORY_USAGE_UNDER(tracker, max_bytes) \
    do { \
        advanced_memory_stats_t stats; \
        (tracker)->get_stats(tracker, &stats); \
        if (stats.current_allocated > (max_bytes)) { \
            fprintf(stderr, "MEMORY_USAGE_EXCEEDED: %zu > %zu bytes\n", \
                    stats.current_allocated, (size_t)(max_bytes)); \
            abort(); \
        } \
    } while(0)

#define ASSERT_ALLOCATION_COUNT_UNDER(tracker, max_count) \
    do { \
        advanced_memory_stats_t stats; \
        (tracker)->get_stats(tracker, &stats); \
        if (stats.allocation_count > (max_count)) { \
            fprintf(stderr, "ALLOCATION_COUNT_EXCEEDED: %zu > %zu\n", \
                    stats.allocation_count, (size_t)(max_count)); \
            abort(); \
        } \
    } while(0)

/*
 * ============================================================================
 * Testing Support Functions
 * ============================================================================
 */

/**
 * Create memory tracker with default settings for testing
 */
advanced_memory_tracker_t* create_test_memory_tracker(void);

/**
 * Run memory tracker self-test
 * 
 * @return      true if all tests pass
 */
bool run_memory_tracker_self_test(void);

#endif // ADVANCED_MEMORY_TRACKER_H