#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <set>
#include <vector>

#include "test_utils.hpp"

using ::testing::_;

/**
 * Week 1 - Error Handling Tests
 * 에러 코드, 에러 메시지, 에러 처리 메커니즘을 테스트합니다.
 */

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::cout << "Setting up ErrorHandlingTest" << std::endl;
    }
    
    void TearDown() override {
        std::cout << "Tearing down ErrorHandlingTest" << std::endl;
    }
};

// 기본 에러 코드 검증
TEST_F(ErrorHandlingTest, BasicErrorCodesAreValid) {
    std::cout << "Testing basic error code values" << std::endl;
    
    // Success는 0이어야 함
    EXPECT_EQ(EXTSOCK_SUCCESS, 0);
    
    // 모든 에러 코드는 0이 아니어야 함
    EXPECT_NE(EXTSOCK_ERROR_JSON_PARSE, 0);
    EXPECT_NE(EXTSOCK_ERROR_CONFIG_INVALID, 0);
    EXPECT_NE(EXTSOCK_ERROR_SOCKET_FAILED, 0);
    EXPECT_NE(EXTSOCK_ERROR_MEMORY_ALLOCATION, 0);
    EXPECT_NE(EXTSOCK_ERROR_STRONGSWAN_API, 0);
    
    std::cout << "Basic error codes are valid" << std::endl;
}

// 에러 코드 고유성 검증
TEST_F(ErrorHandlingTest, ErrorCodesAreUnique) {
    std::cout << "Testing error code uniqueness" << std::endl;
    
    std::vector<extsock_error_t> error_codes = {
        EXTSOCK_SUCCESS,
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID,
        EXTSOCK_ERROR_SOCKET_FAILED,
        EXTSOCK_ERROR_MEMORY_ALLOCATION,
        EXTSOCK_ERROR_STRONGSWAN_API
    };
    
    // 모든 에러 코드가 고유한지 확인
    std::set<extsock_error_t> unique_codes(error_codes.begin(), error_codes.end());
    EXPECT_EQ(unique_codes.size(), error_codes.size()) 
        << "All error codes should be unique";
    
    std::cout << "All error codes are unique" << std::endl;
}

// 에러 코드 범위 검증
TEST_F(ErrorHandlingTest, ErrorCodesAreInValidRange) {
    std::cout << "Testing error code ranges" << std::endl;
    
    // 에러 코드들이 합리적인 범위 내에 있는지 확인
    std::vector<extsock_error_t> error_codes = {
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID,
        EXTSOCK_ERROR_SOCKET_FAILED,
        EXTSOCK_ERROR_MEMORY_ALLOCATION,
        EXTSOCK_ERROR_STRONGSWAN_API
    };
    
    for (auto error_code : error_codes) {
        EXPECT_GT(error_code, 0) << "Error codes should be positive";
        EXPECT_LT(error_code, 1000) << "Error codes should be less than 1000";
    }
    
    std::cout << "Error codes are in valid range" << std::endl;
}

// 커스텀 매처를 사용한 에러 검증
TEST_F(ErrorHandlingTest, CustomErrorMatchersWork) {
    std::cout << "Testing custom error matchers" << std::endl;
    
    // Success 매처 테스트
    extsock_error_t success = EXTSOCK_SUCCESS;
    EXPECT_THAT(success, IsSuccessful());
    
    // 개별 에러 매처 테스트
    extsock_error_t json_error = EXTSOCK_ERROR_JSON_PARSE;
    EXPECT_THAT(json_error, IsFailure());
    EXPECT_THAT(json_error, IsExtsockError(EXTSOCK_ERROR_JSON_PARSE));
    
    extsock_error_t config_error = EXTSOCK_ERROR_CONFIG_INVALID;
    EXPECT_THAT(config_error, IsFailure());
    EXPECT_THAT(config_error, IsExtsockError(EXTSOCK_ERROR_CONFIG_INVALID));
    
    // 다른 에러들도 테스트
    extsock_error_t socket_error = EXTSOCK_ERROR_SOCKET_FAILED;
    EXPECT_THAT(socket_error, IsExtsockError(EXTSOCK_ERROR_SOCKET_FAILED));
    
    extsock_error_t memory_error = EXTSOCK_ERROR_MEMORY_ALLOCATION;
    EXPECT_THAT(memory_error, IsExtsockError(EXTSOCK_ERROR_MEMORY_ALLOCATION));
    
    extsock_error_t api_error = EXTSOCK_ERROR_STRONGSWAN_API;
    EXPECT_THAT(api_error, IsExtsockError(EXTSOCK_ERROR_STRONGSWAN_API));
    
    std::cout << "Custom error matchers work correctly" << std::endl;
}

// 에러 코드 변환 테스트
TEST_F(ErrorHandlingTest, ErrorCodeConversionsWork) {
    std::cout << "Testing error code conversions" << std::endl;
    
    // 정수에서 에러 코드로 변환
    int success_int = 0;
    extsock_error_t success_enum = static_cast<extsock_error_t>(success_int);
    EXPECT_EQ(success_enum, EXTSOCK_SUCCESS);
    
    int json_error_int = static_cast<int>(EXTSOCK_ERROR_JSON_PARSE);
    extsock_error_t json_error_enum = static_cast<extsock_error_t>(json_error_int);
    EXPECT_EQ(json_error_enum, EXTSOCK_ERROR_JSON_PARSE);
    
    std::cout << "Error code conversions work correctly" << std::endl;
}

// 에러 상황 시뮬레이션 테스트
TEST_F(ErrorHandlingTest, ErrorScenariosSimulation) {
    std::cout << "Testing error scenario simulations" << std::endl;
    
    // JSON 파싱 에러 시뮬레이션
    auto simulate_json_parse_error = []() -> extsock_error_t {
        // 잘못된 JSON 문자열 파싱 시뮬레이션
        if (!JsonTestHelper::isValidJson("{ invalid json")) {
            return EXTSOCK_ERROR_JSON_PARSE;
        }
        return EXTSOCK_SUCCESS;
    };
    
    extsock_error_t result = simulate_json_parse_error();
    EXPECT_THAT(result, IsExtsockError(EXTSOCK_ERROR_JSON_PARSE));
    
    // 설정 검증 에러 시뮬레이션
    auto simulate_config_validation_error = []() -> extsock_error_t {
        std::string config = "{}"; // 빈 설정
        if (JsonTestHelper::isValidJson(config)) {
            // 필수 필드가 없으면 설정 에러
            if (config.find("name") == std::string::npos || 
                config.find("local") == std::string::npos) {
                return EXTSOCK_ERROR_CONFIG_INVALID;
            }
        }
        return EXTSOCK_SUCCESS;
    };
    
    result = simulate_config_validation_error();
    EXPECT_THAT(result, IsExtsockError(EXTSOCK_ERROR_CONFIG_INVALID));
    
    std::cout << "Error scenario simulations work correctly" << std::endl;
}

// 에러 처리 함수 동작 테스트
TEST_F(ErrorHandlingTest, ErrorHandlingFunctionsWork) {
    std::cout << "Testing error handling functions" << std::endl;
    
    // 에러를 성공으로 변환하는 함수 테스트
    auto try_operation_with_fallback = [](bool should_fail) -> extsock_error_t {
        if (should_fail) {
            return EXTSOCK_ERROR_JSON_PARSE;
        }
        return EXTSOCK_SUCCESS;
    };
    
    // 실패 케이스
    extsock_error_t result = try_operation_with_fallback(true);
    EXPECT_THAT(result, IsExtsockError(EXTSOCK_ERROR_JSON_PARSE));
    
    // 성공 케이스
    result = try_operation_with_fallback(false);
    EXPECT_THAT(result, IsSuccessful());
    
    std::cout << "Error handling functions work correctly" << std::endl;
}

// 연속적인 에러 처리 테스트
TEST_F(ErrorHandlingTest, ChainedErrorHandlingWorks) {
    std::cout << "Testing chained error handling" << std::endl;
    
    // 여러 단계의 연산에서 에러 전파 테스트
    auto step1 = [](bool should_fail) -> extsock_error_t {
        return should_fail ? EXTSOCK_ERROR_JSON_PARSE : EXTSOCK_SUCCESS;
    };
    
    auto step2 = [](extsock_error_t prev_result) -> extsock_error_t {
        if (prev_result != EXTSOCK_SUCCESS) {
            return prev_result; // 이전 에러 전파
        }
        return EXTSOCK_SUCCESS;
    };
    
    auto step3 = [](extsock_error_t prev_result) -> extsock_error_t {
        if (prev_result != EXTSOCK_SUCCESS) {
            return prev_result; // 이전 에러 전파
        }
        return EXTSOCK_SUCCESS;
    };
    
    // 에러 전파 테스트
    extsock_error_t result1 = step1(true);
    extsock_error_t result2 = step2(result1);
    extsock_error_t result3 = step3(result2);
    
    EXPECT_THAT(result1, IsExtsockError(EXTSOCK_ERROR_JSON_PARSE));
    EXPECT_THAT(result2, IsExtsockError(EXTSOCK_ERROR_JSON_PARSE));
    EXPECT_THAT(result3, IsExtsockError(EXTSOCK_ERROR_JSON_PARSE));
    
    // 정상 흐름 테스트
    result1 = step1(false);
    result2 = step2(result1);
    result3 = step3(result2);
    
    EXPECT_THAT(result1, IsSuccessful());
    EXPECT_THAT(result2, IsSuccessful());
    EXPECT_THAT(result3, IsSuccessful());
    
    std::cout << "Chained error handling works correctly" << std::endl;
}

// 에러 복구 메커니즘 테스트
TEST_F(ErrorHandlingTest, ErrorRecoveryMechanismsWork) {
    std::cout << "Testing error recovery mechanisms" << std::endl;
    
    // 재시도 로직 시뮬레이션 (각 테스트 케이스별로 독립적)
    auto operation_with_retry = [](int max_retries, bool eventually_succeed) -> extsock_error_t {
        for (int attempts = 1; attempts <= max_retries; attempts++) {
            if (eventually_succeed && attempts >= max_retries) {
                return EXTSOCK_SUCCESS;
            }
            if (attempts >= max_retries) {
                return EXTSOCK_ERROR_SOCKET_FAILED;
            }
        }
        return EXTSOCK_ERROR_SOCKET_FAILED;
    };
    
    // 실패 후 성공 케이스
    extsock_error_t result = operation_with_retry(3, true);
    EXPECT_THAT(result, IsSuccessful());
    
    // 계속 실패 케이스
    result = operation_with_retry(3, false);
    EXPECT_THAT(result, IsExtsockError(EXTSOCK_ERROR_SOCKET_FAILED));
    
    std::cout << "Error recovery mechanisms work correctly" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "Starting Error Handling Tests (Week 1)" << std::endl;
    
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    
    if (result == 0) {
        std::cout << "All error handling tests passed!" << std::endl;
    } else {
        std::cout << "Some error handling tests failed!" << std::endl;
    }
    
    return result;
} 