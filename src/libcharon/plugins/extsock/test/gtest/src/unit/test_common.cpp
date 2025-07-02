#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <set>

#include "test_utils.hpp"

using ::testing::_;
using ::testing::Return;

/**
 * Week 1 - Common Module Tests
 * 공통 모듈의 기본 기능을 테스트합니다.
 */

class CommonModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::cout << "Setting up CommonModuleTest" << std::endl;
    }
    
    void TearDown() override {
        std::cout << "Tearing down CommonModuleTest" << std::endl;
    }
};

// 기본적인 에러 코드 상수 테스트
TEST_F(CommonModuleTest, ErrorCodesAreDefined) {
    std::cout << "Testing error code constants" << std::endl;
    
    // Given & When & Then
    EXPECT_EQ(EXTSOCK_SUCCESS, 0);
    EXPECT_NE(EXTSOCK_ERROR_JSON_PARSE, EXTSOCK_SUCCESS);
    EXPECT_NE(EXTSOCK_ERROR_CONFIG_INVALID, EXTSOCK_SUCCESS);
    EXPECT_NE(EXTSOCK_ERROR_SOCKET_FAILED, EXTSOCK_SUCCESS);
    EXPECT_NE(EXTSOCK_ERROR_MEMORY_ALLOCATION, EXTSOCK_SUCCESS);
    EXPECT_NE(EXTSOCK_ERROR_STRONGSWAN_API, EXTSOCK_SUCCESS);
    
    std::cout << "All error codes are properly defined" << std::endl;
}

// 이벤트 타입 상수 테스트
TEST_F(CommonModuleTest, EventTypesAreDefined) {
    std::cout << "Testing event type constants" << std::endl;
    
    // Given & When & Then
    // 이벤트 타입들이 서로 다른 값을 가지는지 확인
    std::set<int> event_types = {
        EXTSOCK_EVENT_TUNNEL_UP,
        EXTSOCK_EVENT_TUNNEL_DOWN,
        EXTSOCK_EVENT_CONFIG_APPLIED,
        EXTSOCK_EVENT_ERROR
    };
    
    EXPECT_EQ(event_types.size(), 4) << "All event types should have unique values";
    
    std::cout << "All event types are properly defined" << std::endl;
}

// 명령 타입 상수 테스트
TEST_F(CommonModuleTest, CommandTypesAreDefined) {
    std::cout << "Testing command type constants" << std::endl;
    
    // Given & When & Then
    std::set<int> command_types = {
        EXTSOCK_CMD_APPLY_CONFIG,
        EXTSOCK_CMD_START_DPD,
        EXTSOCK_CMD_REMOVE_CONFIG
    };
    
    EXPECT_EQ(command_types.size(), 3) << "All command types should have unique values";
    
    std::cout << "All command types are properly defined" << std::endl;
}

// 테스트 유틸리티 기능 테스트
TEST_F(CommonModuleTest, StringUtilitiesWork) {
    std::cout << "Testing string utility functions" << std::endl;
    
    // String utilities
    EXPECT_TRUE(StringUtils::startsWith("hello world", "hello"));
    EXPECT_FALSE(StringUtils::startsWith("hello world", "world"));
    EXPECT_TRUE(StringUtils::endsWith("hello world", "world"));
    EXPECT_FALSE(StringUtils::endsWith("hello world", "hello"));
    
    std::string trimmed = StringUtils::trim("  hello  ");
    EXPECT_EQ(trimmed, "hello");
    
    std::vector<std::string> parts = StringUtils::split("a,b,c", ',');
    EXPECT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
    
    std::cout << "String utilities work correctly" << std::endl;
}

// JSON 유틸리티 기능 테스트
TEST_F(CommonModuleTest, JsonUtilitiesWork) {
    std::cout << "Testing JSON utilities" << std::endl;
    
    // Valid JSON check
    std::string valid_json = R"({"name": "test", "value": 123})";
    EXPECT_TRUE(JsonTestHelper::isValidJson(valid_json));
    
    // Invalid JSON check
    std::string invalid_json = "{ invalid json";
    EXPECT_FALSE(JsonTestHelper::isValidJson(invalid_json));
    
    // Create test configs
    std::string test_config = JsonTestHelper::createTestConfig("test_id", "test_type");
    EXPECT_FALSE(test_config.empty());
    EXPECT_TRUE(JsonTestHelper::isValidJson(test_config));
    
    std::cout << "JSON utilities work correctly" << std::endl;
}

// 메모리 추적 기능 테스트
TEST_F(CommonModuleTest, MemoryTrackingWorks) {
    std::cout << "Testing memory tracking" << std::endl;
    
    MemoryTracker& tracker = MemoryTracker::getInstance();
    tracker.reset();
    
    // Initial state
    EXPECT_EQ(tracker.getAllocatedBytes(), 0);
    EXPECT_EQ(tracker.getAllocationCount(), 0);
    EXPECT_FALSE(tracker.hasLeaks());
    
    // Simulate allocation
    void* ptr1 = malloc(100);
    tracker.recordAllocation(ptr1, 100, "test_location_1");
    EXPECT_EQ(tracker.getAllocatedBytes(), 100);
    EXPECT_EQ(tracker.getAllocationCount(), 1);
    EXPECT_TRUE(tracker.hasLeaks());
    
    void* ptr2 = malloc(200);
    tracker.recordAllocation(ptr2, 200, "test_location_2");
    EXPECT_EQ(tracker.getAllocatedBytes(), 300);
    EXPECT_EQ(tracker.getAllocationCount(), 2);
    
    // Simulate deallocation
    tracker.recordDeallocation(ptr1);
    free(ptr1);
    EXPECT_EQ(tracker.getAllocatedBytes(), 200);
    EXPECT_TRUE(tracker.hasLeaks());
    
    tracker.recordDeallocation(ptr2);
    free(ptr2);
    EXPECT_EQ(tracker.getAllocatedBytes(), 0);
    EXPECT_FALSE(tracker.hasLeaks());
    
    std::cout << "Memory tracking works correctly" << std::endl;
}

// 시간 측정 기능 테스트
TEST_F(CommonModuleTest, TimeHelperWorks) {
    std::cout << "Testing time measurement" << std::endl;
    
    TimeHelper timer;
    timer.start();
    
    // 약간의 지연 후 시간 측정
    TimeHelper::sleep(10);
    double elapsed = timer.elapsed();
    EXPECT_GE(elapsed, 8.0);  // 최소 8ms (약간의 여유)
    EXPECT_LT(elapsed, 100.0); // 최대 100ms
    
    std::cout << "Time measurement works correctly" << std::endl;
}

// 커스텀 매처 테스트
TEST_F(CommonModuleTest, CustomMatchersWork) {
    std::cout << "Testing custom matchers" << std::endl;
    
    // Success matcher
    extsock_error_t success_code = EXTSOCK_SUCCESS;
    EXPECT_THAT(success_code, IsSuccessful());
    
    // Error matchers
    extsock_error_t json_error = EXTSOCK_ERROR_JSON_PARSE;
    EXPECT_THAT(json_error, IsExtsockError(EXTSOCK_ERROR_JSON_PARSE));
    EXPECT_THAT(json_error, IsFailure());
    
    extsock_error_t config_error = EXTSOCK_ERROR_CONFIG_INVALID;
    EXPECT_THAT(config_error, IsExtsockError(EXTSOCK_ERROR_CONFIG_INVALID));
    EXPECT_THAT(config_error, IsFailure());
    
    std::cout << "Custom matchers work correctly" << std::endl;
}

// 테스트 데이터 팩토리 테스트
TEST_F(CommonModuleTest, TestDataFactoryWorks) {
    std::cout << "Testing test data factory" << std::endl;
    
    // Create test configs
    std::vector<std::string> configs = TestDataFactory::createTestConfigs(3);
    EXPECT_EQ(configs.size(), 3);
    for (const auto& config : configs) {
        EXPECT_FALSE(config.empty());
        EXPECT_TRUE(JsonTestHelper::isValidJson(config));
    }
    
    // Create error codes
    std::vector<extsock_error_t> errors = TestDataFactory::createErrorCodes();
    EXPECT_GT(errors.size(), 0);
    EXPECT_EQ(errors[0], EXTSOCK_SUCCESS);
    
    // Create event types
    std::vector<extsock_event_type_t> events = TestDataFactory::createEventTypes();
    EXPECT_GT(events.size(), 0);
    
    // Create command types
    std::vector<extsock_command_type_t> commands = TestDataFactory::createCommandTypes();
    EXPECT_GT(commands.size(), 0);
    
    std::cout << "Test data factory works correctly" << std::endl;
}

// 파일 시스템 헬퍼 테스트
TEST_F(CommonModuleTest, FileSystemHelperWorks) {
    std::cout << "Testing file system helpers" << std::endl;
    
    // Create a temporary file
    std::string content = "test content";
    std::string temp_file = FileSystemHelper::createTempFile(content);
    EXPECT_FALSE(temp_file.empty());
    
    // Check if file exists
    EXPECT_TRUE(FileSystemHelper::fileExists(temp_file));
    
    // Read file content
    std::string read_content = FileSystemHelper::readFile(temp_file);
    EXPECT_EQ(read_content, content);
    
    // Remove file
    EXPECT_TRUE(FileSystemHelper::removeFile(temp_file));
    EXPECT_FALSE(FileSystemHelper::fileExists(temp_file));
    
    std::cout << "File system helpers work correctly" << std::endl;
}

// 통합 테스트
TEST_F(CommonModuleTest, IntegratedUtilitiesWork) {
    std::cout << "Testing integrated utilities" << std::endl;
    
    // Test comprehensive workflow
    TimeHelper timer;
    timer.start();
    
    // Create config using factory
    std::string config = TestDataFactory::createTestConfigs(1)[0];
    EXPECT_FALSE(config.empty());
    
    // Validate JSON
    EXPECT_TRUE(JsonTestHelper::isValidJson(config));
    
    // Memory tracking
    MemoryTracker& tracker = MemoryTracker::getInstance();
    tracker.reset();
    
    void* test_mem = malloc(256);
    tracker.recordAllocation(test_mem, 256, "integrated_test");
    
    // String processing
    std::vector<std::string> parts = StringUtils::split(config, '"');
    EXPECT_GT(parts.size(), 0);
    
    // Cleanup
    tracker.recordDeallocation(test_mem);
    free(test_mem);
    
    double elapsed = timer.elapsed();
    EXPECT_GT(elapsed, 0.0);
    
    // Verify no leaks
    EXPECT_FALSE(tracker.hasLeaks());
    
    std::cout << "All utilities work together correctly" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "Starting Common Module Tests (Week 1)" << std::endl;
    
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    
    if (result == 0) {
        std::cout << "All common module tests passed!" << std::endl;
    } else {
        std::cout << "Some common module tests failed!" << std::endl;
    }
    
    return result;
} 