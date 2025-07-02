#ifndef EXTSOCK_TEST_UTILS_HPP
#define EXTSOCK_TEST_UTILS_HPP

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <fstream>
#include <cstdarg>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <cjson/cJSON.h>

// C 헤더들
extern "C" {
    #include "../../common/extsock_types.h"
    #include "../../common/extsock_errors.h"
}

namespace extsock_test {

/**
 * 테스트 출력 헬퍼 클래스
 * 일관된 형태의 테스트 메시지 출력 제공
 */
class TestOutput {
public:
    static void info(const std::string& message);
    static void success(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
};

/**
 * 메모리 추적 헬퍼 클래스
 * 테스트 중 메모리 누수 감지
 */
class MemoryTracker {
public:
    MemoryTracker();
    ~MemoryTracker();
    
    void* allocate(size_t size);
    void deallocate(void* ptr);
    
    size_t getAllocatedBlocks() const;
    size_t getTotalAllocated() const;

private:
    size_t allocated_blocks_;
    size_t total_allocated_;
    std::unordered_map<void*, size_t> allocated_ptrs_;
};

/**
 * 문자열 유틸리티 클래스
 */
class StringUtils {
public:
    static std::string format(const char* format, ...);
    static bool startsWith(const std::string& str, const std::string& prefix);
    static bool endsWith(const std::string& str, const std::string& suffix);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static std::string trim(const std::string& str);
};

/**
 * JSON 테스트 헬퍼 클래스
 */
class JsonTestHelper {
public:
    JsonTestHelper();
    ~JsonTestHelper();
    
    bool parse(const std::string& json_str);
    cJSON* getRoot() const;
    
    // 테스트용 JSON 생성
    static std::string createBasicConfig();
    static std::string createInvalidJson();
    
    // JSON 필드 검증
    bool hasField(const std::string& field_name) const;
    std::string getStringField(const std::string& field_name) const;

private:
    cJSON* root_;
};

/**
 * 파일 시스템 헬퍼 클래스
 */
class FileSystemHelper {
public:
    static bool fileExists(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool removeFile(const std::string& path);
    static std::string readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::string& content);
};

/**
 * 시간 측정 헬퍼 클래스
 */
class TimeHelper {
public:
    TimeHelper();
    void reset();
    double getElapsedMs() const;
    double getElapsedSeconds() const;

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * Google Test 커스텀 매처들
 */

// extsock_error_t를 위한 매처
class ExtsockErrorMatcher {
public:
    explicit ExtsockErrorMatcher(extsock_error_t expected) : expected_error_(expected) {}
    
    bool MatchAndExplain(extsock_error_t actual, ::testing::MatchResultListener* listener) const;
    
    void DescribeTo(std::ostream* os) const {
        *os << "equals " << getErrorString(expected_error_);
    }
    
    void DescribeNegationTo(std::ostream* os) const {
        *os << "does not equal " << getErrorString(expected_error_);
    }

private:
    extsock_error_t expected_error_;
    std::string getErrorString(extsock_error_t error) const;
};

// 매처 헬퍼 함수
inline ::testing::Matcher<extsock_error_t> IsExtsockError(extsock_error_t expected) {
    return ::testing::MakeMatcher(new ExtsockErrorMatcher(expected));
}

// 자주 사용되는 매처들
inline ::testing::Matcher<extsock_error_t> IsSuccess() {
    return IsExtsockError(EXTSOCK_SUCCESS);
}

inline ::testing::Matcher<extsock_error_t> IsJsonParseError() {
    return IsExtsockError(EXTSOCK_ERROR_JSON_PARSE);
}

inline ::testing::Matcher<extsock_error_t> IsConfigInvalid() {
    return IsExtsockError(EXTSOCK_ERROR_CONFIG_INVALID);
}

/**
 * 테스트 데이터 팩토리
 */
class TestDataFactory {
public:
    // JSON 설정 생성
    static std::string createBasicIkeConfig() {
        return R"({
            "name": "basic_connection",
            "local": "192.168.1.10",
            "remote": "203.0.113.5",
            "auth": {
                "type": "psk",
                "secret": "test_secret123"
            },
            "ike_proposal": "aes256-sha256-modp2048",
            "esp_proposal": "aes256gcm16"
        })";
    }
    
    static std::string createComplexIpsecConfig() {
        return R"({
            "name": "complex_connection",
            "local": "10.0.0.1",
            "remote": "10.0.1.1",
            "auth": {
                "type": "psk",
                "id": "client@example.com",
                "secret": "supersecret"
            },
            "ike_proposal": "aes256-sha256-modp2048",
            "esp_proposal": "aes256gcm16-modp2048",
            "children": [
                {
                    "name": "child1",
                    "local_ts": "10.0.0.0/24",
                    "remote_ts": "10.1.0.0/24"
                },
                {
                    "name": "child2", 
                    "local_ts": "10.0.1.0/24",
                    "remote_ts": "10.1.1.0/24"
                }
            ]
        })";
    }
    
    static std::string createInvalidJson() {
        return "{ invalid json structure without closing brace";
    }
    
    static std::string createEmptyConfig() {
        return "{}";
    }
};

/**
 * RAII 스타일 리소스 관리 헬퍼
 */
template<typename T, void(*Deleter)(T*)>
class ResourceGuard {
public:
    explicit ResourceGuard(T* resource) : resource_(resource) {}
    
    ~ResourceGuard() {
        if (resource_) {
            Deleter(resource_);
        }
    }
    
    T* get() const { return resource_; }
    T* release() {
        T* tmp = resource_;
        resource_ = nullptr;
        return tmp;
    }
    
    ResourceGuard(const ResourceGuard&) = delete;
    ResourceGuard& operator=(const ResourceGuard&) = delete;
    
    ResourceGuard(ResourceGuard&& other) noexcept : resource_(other.release()) {}
    ResourceGuard& operator=(ResourceGuard&& other) noexcept {
        if (this != &other) {
            if (resource_) Deleter(resource_);
            resource_ = other.release();
        }
        return *this;
    }

private:
    T* resource_;
};

// cJSON을 위한 RAII 래퍼
using JsonGuard = ResourceGuard<cJSON, cJSON_Delete>;

} // namespace extsock_test

// Google Test에서 extsock_error_t 출력을 위한 PrintTo 함수
namespace testing {
    std::string PrintToString(const extsock_test::ExtsockErrorMatcher& matcher);
}

#endif // EXTSOCK_TEST_UTILS_HPP 