#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include <thread>
#include <set>
#include <chrono>
#include <fstream>
#include <cstdarg>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <cjson/cJSON.h>
#include <functional>

// Include our test-only mock headers
#include "c_wrappers/extsock_errors.h"
#include "c_wrappers/extsock_types.h"

/**
 * Memory Tracker Class
 * Tracks memory allocations and deallocations for leak detection
 */
class MemoryTracker {
public:
    static MemoryTracker& getInstance();
    void recordAllocation(void* ptr, size_t size, const std::string& location);
    void recordDeallocation(void* ptr);
    size_t getAllocatedBytes() const;
    size_t getAllocationCount() const;
    void reset();
    bool hasLeaks() const;
    std::vector<std::string> getLeakReport() const;

private:
    struct AllocationInfo {
        size_t size;
        std::string location;
    };
    std::map<void*, AllocationInfo> allocations_;
    size_t totalAllocated_;
    size_t allocationCount_;
};

/**
 * String Utilities Class
 * Common string operations for tests
 */
class StringUtils {
public:
    static std::string trim(const std::string& str);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static std::string join(const std::vector<std::string>& parts, const std::string& delimiter);
    static bool startsWith(const std::string& str, const std::string& prefix);
    static bool endsWith(const std::string& str, const std::string& suffix);
    static std::string toLower(const std::string& str);
    static std::string toUpper(const std::string& str);
};

/**
 * JSON Test Helper Class
 * Helper functions for creating and manipulating test JSON data
 */
class JsonTestHelper {
public:
    static std::string createTestConfig(const std::string& id, const std::string& type);
    static std::string createInvalidJson();
    static std::string createMinimalConfig();
    static std::string createComplexConfig();
    static bool isValidJson(const std::string& json);
};

/**
 * File System Helper Class
 * Helper functions for file operations during tests
 */
class FileSystemHelper {
public:
    static bool fileExists(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool removeFile(const std::string& path);
    static std::string readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::string& content);
    static std::string getTempDirectory();
    static std::string createTempFile(const std::string& content = "");
};

/**
 * Time Helper Class
 * Time measurement utilities for performance tests
 */
class TimeHelper {
public:
    void start();
    double elapsed() const;
    static void sleep(int milliseconds);

private:
    std::chrono::high_resolution_clock::time_point startTime_;
};

/**
 * Test Data Factory
 * Factory methods for creating test data
 */
class TestDataFactory {
public:
    static std::vector<std::string> createTestConfigs(size_t count);
    static std::vector<extsock_error_t> createErrorCodes();
    static std::vector<extsock_event_type_t> createEventTypes();
    static std::vector<extsock_command_type_t> createCommandTypes();
};

/**
 * Resource Guard Class
 * RAII helper for automatic resource cleanup
 */
template<typename T>
class ResourceGuard {
public:
    ResourceGuard(T* resource, std::function<void(T*)> cleanup)
        : resource_(resource), cleanup_(cleanup) {}
    
    ~ResourceGuard() {
        if (resource_ && cleanup_) {
            cleanup_(resource_);
        }
    }
    
    T* get() { return resource_; }
    T* release() { 
        T* temp = resource_;
        resource_ = nullptr;
        return temp;
    }

private:
    T* resource_;
    std::function<void(T*)> cleanup_;
};

/**
 * Custom Google Test Matchers
 */
MATCHER_P(IsExtsockError, expected_error, "matches extsock error code") {
    return arg == expected_error;
}

MATCHER(IsSuccessful, "is successful (EXTSOCK_SUCCESS)") {
    return arg == EXTSOCK_SUCCESS;
}

MATCHER(IsFailure, "is a failure (not EXTSOCK_SUCCESS)") {
    return arg != EXTSOCK_SUCCESS;
}

/**
 * Test Macros
 */
#define EXPECT_EXTSOCK_SUCCESS(expr) EXPECT_THAT(expr, IsSuccessful())
#define EXPECT_EXTSOCK_ERROR(expr, error) EXPECT_THAT(expr, IsExtsockError(error))
#define ASSERT_EXTSOCK_SUCCESS(expr) ASSERT_THAT(expr, IsSuccessful())
#define ASSERT_EXTSOCK_ERROR(expr, error) ASSERT_THAT(expr, IsExtsockError(error))

/**
 * Memory Leak Test Base Class
 */
class MemoryLeakTest : public ::testing::Test {
protected:
    void SetUp() override {
        MemoryTracker::getInstance().reset();
    }
    
    void TearDown() override {
        EXPECT_FALSE(MemoryTracker::getInstance().hasLeaks()) 
            << "Memory leaks detected:\n" 
            << StringUtils::join(MemoryTracker::getInstance().getLeakReport(), "\n");
    }
};

#endif /* TEST_UTILS_HPP */ 