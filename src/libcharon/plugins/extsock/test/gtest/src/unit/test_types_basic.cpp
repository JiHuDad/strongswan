#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <set>
#include <string>

#include "test_utils.hpp"

using ::testing::_;

/**
 * Week 1 - Basic Types Tests
 * 기본 타입 정의, enum, 기본 데이터 구조를 테스트합니다.
 */

class BasicTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::cout << "Setting up BasicTypesTest" << std::endl;
    }
    
    void TearDown() override {
        std::cout << "Tearing down BasicTypesTest" << std::endl;
    }
};

// 기본 enum 크기 및 값 테스트
TEST_F(BasicTypesTest, EnumSizesAndValuesAreValid) {
    std::cout << "Testing enum sizes and values" << std::endl;
    
    // extsock_error_t enum 테스트
    EXPECT_GE(sizeof(extsock_error_t), sizeof(int));
    
    // 에러 코드 값들이 정의된 범위 내에 있는지 확인
    EXPECT_EQ(EXTSOCK_SUCCESS, 0);
    EXPECT_GT(EXTSOCK_ERROR_JSON_PARSE, 0);
    EXPECT_GT(EXTSOCK_ERROR_CONFIG_INVALID, 0);
    EXPECT_GT(EXTSOCK_ERROR_SOCKET_FAILED, 0);
    EXPECT_GT(EXTSOCK_ERROR_MEMORY_ALLOCATION, 0);
    EXPECT_GT(EXTSOCK_ERROR_STRONGSWAN_API, 0);
    
    std::cout << "Enum sizes and values are valid" << std::endl;
}

// 이벤트 타입 enum 테스트
TEST_F(BasicTypesTest, EventTypeEnumIsValid) {
    std::cout << "Testing event type enum" << std::endl;
    
    // 이벤트 타입들이 서로 다른 값을 가지는지 확인
    std::vector<extsock_event_type_t> event_types = {
        EXTSOCK_EVENT_TUNNEL_UP,
        EXTSOCK_EVENT_TUNNEL_DOWN,
        EXTSOCK_EVENT_CONFIG_APPLIED,
        EXTSOCK_EVENT_ERROR
    };
    
    // 모든 이벤트 타입이 고유한지 확인
    std::set<extsock_event_type_t> unique_types(event_types.begin(), event_types.end());
    EXPECT_EQ(unique_types.size(), event_types.size()) 
        << "All event types should be unique";
    
    // 타입들이 적절한 범위에 있는지 확인
    for (auto event_type : event_types) {
        EXPECT_GE(event_type, 0) << "Event types should be non-negative";
        EXPECT_LT(event_type, 100) << "Event types should be less than 100";
    }
    
    std::cout << "Event type enum is valid" << std::endl;
}

// 명령 타입 enum 테스트
TEST_F(BasicTypesTest, CommandTypeEnumIsValid) {
    std::cout << "Testing command type enum" << std::endl;
    
    std::vector<extsock_command_type_t> command_types = {
        EXTSOCK_CMD_APPLY_CONFIG,
        EXTSOCK_CMD_START_DPD,
        EXTSOCK_CMD_REMOVE_CONFIG
    };
    
    // 모든 명령 타입이 고유한지 확인
    std::set<extsock_command_type_t> unique_types(command_types.begin(), command_types.end());
    EXPECT_EQ(unique_types.size(), command_types.size()) 
        << "All command types should be unique";
    
    // 타입들이 적절한 범위에 있는지 확인
    for (auto cmd_type : command_types) {
        EXPECT_GE(cmd_type, 0) << "Command types should be non-negative";
        EXPECT_LT(cmd_type, 100) << "Command types should be less than 100";
    }
    
    std::cout << "Command type enum is valid" << std::endl;
}

// enum 값들 간의 관계 테스트
TEST_F(BasicTypesTest, EnumRelationshipsAreConsistent) {
    std::cout << "Testing enum relationships" << std::endl;
    
    // 에러 코드들이 서로 다른 카테고리를 나타내는지 확인
    EXPECT_NE(EXTSOCK_ERROR_JSON_PARSE, EXTSOCK_ERROR_CONFIG_INVALID);
    EXPECT_NE(EXTSOCK_ERROR_JSON_PARSE, EXTSOCK_ERROR_SOCKET_FAILED);
    EXPECT_NE(EXTSOCK_ERROR_CONFIG_INVALID, EXTSOCK_ERROR_MEMORY_ALLOCATION);
    
    // 이벤트 타입들이 논리적으로 의미있는 순서인지 확인
    EXPECT_NE(EXTSOCK_EVENT_TUNNEL_UP, EXTSOCK_EVENT_TUNNEL_DOWN);
    EXPECT_NE(EXTSOCK_EVENT_CONFIG_APPLIED, EXTSOCK_EVENT_ERROR);
    
    // 명령 타입들이 서로 다른지 확인
    EXPECT_NE(EXTSOCK_CMD_APPLY_CONFIG, EXTSOCK_CMD_START_DPD);
    EXPECT_NE(EXTSOCK_CMD_START_DPD, EXTSOCK_CMD_REMOVE_CONFIG);
    
    std::cout << "Enum relationships are consistent" << std::endl;
}

// C와 C++ 간 타입 호환성 테스트
TEST_F(BasicTypesTest, CAndCppTypeCompatibility) {
    std::cout << "Testing C and C++ type compatibility" << std::endl;
    
    // C enum을 C++ int로 캐스팅
    extsock_error_t c_error = EXTSOCK_ERROR_JSON_PARSE;
    int cpp_error = static_cast<int>(c_error);
    EXPECT_EQ(cpp_error, static_cast<int>(EXTSOCK_ERROR_JSON_PARSE));
    
    // C++ int를 C enum으로 캐스팅
    int cpp_value = 1;
    extsock_error_t c_value = static_cast<extsock_error_t>(cpp_value);
    EXPECT_EQ(static_cast<int>(c_value), cpp_value);
    
    // 이벤트 타입도 동일하게 테스트
    extsock_event_type_t c_event = EXTSOCK_EVENT_TUNNEL_UP;
    int cpp_event = static_cast<int>(c_event);
    extsock_event_type_t c_event_back = static_cast<extsock_event_type_t>(cpp_event);
    EXPECT_EQ(c_event, c_event_back);
    
    std::cout << "C and C++ type compatibility is maintained" << std::endl;
}

// 타입 크기 일관성 테스트
TEST_F(BasicTypesTest, TypeSizeConsistency) {
    std::cout << "Testing type size consistency" << std::endl;
    
    // enum들이 합리적인 크기를 가지는지 확인
    EXPECT_GE(sizeof(extsock_error_t), 1);
    EXPECT_LE(sizeof(extsock_error_t), 8);
    
    EXPECT_GE(sizeof(extsock_event_type_t), 1);
    EXPECT_LE(sizeof(extsock_event_type_t), 8);
    
    EXPECT_GE(sizeof(extsock_command_type_t), 1);
    EXPECT_LE(sizeof(extsock_command_type_t), 8);
    
    // 일반적으로 enum은 int 크기여야 함
    EXPECT_EQ(sizeof(extsock_error_t), sizeof(int));
    EXPECT_EQ(sizeof(extsock_event_type_t), sizeof(int));
    EXPECT_EQ(sizeof(extsock_command_type_t), sizeof(int));
    
    std::cout << "Type sizes are consistent" << std::endl;
}

// 기본 타입 연산 테스트
TEST_F(BasicTypesTest, BasicTypeOperations) {
    std::cout << "Testing basic type operations" << std::endl;
    
    // 비교 연산
    extsock_error_t error1 = EXTSOCK_SUCCESS;
    extsock_error_t error2 = EXTSOCK_ERROR_JSON_PARSE;
    
    EXPECT_EQ(error1, EXTSOCK_SUCCESS);
    EXPECT_NE(error1, error2);
    EXPECT_NE(error2, EXTSOCK_SUCCESS);
    
    // 대입 연산
    extsock_error_t error3 = error1;
    EXPECT_EQ(error3, error1);
    EXPECT_EQ(error3, EXTSOCK_SUCCESS);
    
    error3 = error2;
    EXPECT_EQ(error3, error2);
    EXPECT_EQ(error3, EXTSOCK_ERROR_JSON_PARSE);
    
    std::cout << "Basic type operations work correctly" << std::endl;
}

// 배열과 컨테이너에서의 타입 사용 테스트
TEST_F(BasicTypesTest, TypesInContainers) {
    std::cout << "Testing types in containers" << std::endl;
    
    // C++ 벡터에 enum 저장
    std::vector<extsock_error_t> error_list = {
        EXTSOCK_SUCCESS,
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID
    };
    
    EXPECT_EQ(error_list.size(), 3);
    EXPECT_EQ(error_list[0], EXTSOCK_SUCCESS);
    EXPECT_EQ(error_list[1], EXTSOCK_ERROR_JSON_PARSE);
    EXPECT_EQ(error_list[2], EXTSOCK_ERROR_CONFIG_INVALID);
    
    // 이벤트 타입 벡터
    std::vector<extsock_event_type_t> event_list = {
        EXTSOCK_EVENT_TUNNEL_UP,
        EXTSOCK_EVENT_TUNNEL_DOWN
    };
    
    EXPECT_EQ(event_list.size(), 2);
    EXPECT_EQ(event_list[0], EXTSOCK_EVENT_TUNNEL_UP);
    EXPECT_EQ(event_list[1], EXTSOCK_EVENT_TUNNEL_DOWN);
    
    std::cout << "Types work correctly in containers" << std::endl;
}

// Switch 문에서의 enum 사용 테스트
TEST_F(BasicTypesTest, EnumsInSwitchStatements) {
    std::cout << "Testing enums in switch statements" << std::endl;
    
    auto get_error_category = [](extsock_error_t error) -> std::string {
        switch (error) {
            case EXTSOCK_SUCCESS:
                return "success";
            case EXTSOCK_ERROR_JSON_PARSE:
                return "parsing";
            case EXTSOCK_ERROR_CONFIG_INVALID:
                return "configuration";
            case EXTSOCK_ERROR_SOCKET_FAILED:
                return "network";
            case EXTSOCK_ERROR_MEMORY_ALLOCATION:
                return "memory";
            case EXTSOCK_ERROR_STRONGSWAN_API:
                return "api";
            default:
                return "unknown";
        }
    };
    
    EXPECT_EQ(get_error_category(EXTSOCK_SUCCESS), "success");
    EXPECT_EQ(get_error_category(EXTSOCK_ERROR_JSON_PARSE), "parsing");
    EXPECT_EQ(get_error_category(EXTSOCK_ERROR_CONFIG_INVALID), "configuration");
    
    std::cout << "Enums work correctly in switch statements" << std::endl;
}

// 타입 안전성 테스트
TEST_F(BasicTypesTest, TypeSafety) {
    std::cout << "Testing type safety" << std::endl;
    
    // 서로 다른 enum 타입은 직접 비교할 수 없어야 함 (컴파일 시간에 검증됨)
    extsock_error_t error = EXTSOCK_ERROR_JSON_PARSE;
    extsock_event_type_t event = EXTSOCK_EVENT_TUNNEL_UP;
    
    // 이 테스트는 컴파일이 성공하면 타입이 제대로 정의되었음을 의미
    EXPECT_NE(static_cast<int>(error), static_cast<int>(event));
    
    std::cout << "Type safety is maintained" << std::endl;
}

// Range-based for loop에서의 enum 사용
TEST_F(BasicTypesTest, EnumsInRangeBasedFor) {
    std::cout << "Testing enums in range-based for loops" << std::endl;
    
    std::vector<extsock_error_t> all_errors = TestDataFactory::createErrorCodes();
    
    int count = 0;
    for (auto error : all_errors) {
        EXPECT_TRUE(error == EXTSOCK_SUCCESS || error > 0);
        count++;
    }
    
    EXPECT_GT(count, 0);
    EXPECT_EQ(count, static_cast<int>(all_errors.size()));
    
    std::cout << "Enums work correctly in range-based for loops" << std::endl;
}

// const와 static 사용 테스트
TEST_F(BasicTypesTest, ConstAndStaticUsage) {
    std::cout << "Testing const and static usage" << std::endl;
    
    // const enum 변수
    const extsock_error_t CONST_ERROR = EXTSOCK_ERROR_JSON_PARSE;
    EXPECT_EQ(CONST_ERROR, EXTSOCK_ERROR_JSON_PARSE);
    
    // static enum 변수 (함수 내)
    auto get_static_error = []() -> extsock_error_t {
        static extsock_error_t static_error = EXTSOCK_ERROR_CONFIG_INVALID;
        return static_error;
    };
    
    EXPECT_EQ(get_static_error(), EXTSOCK_ERROR_CONFIG_INVALID);
    EXPECT_EQ(get_static_error(), EXTSOCK_ERROR_CONFIG_INVALID); // 두 번째 호출도 같은 값
    
    std::cout << "Const and static usage works correctly" << std::endl;
}

// 함수 매개변수로서의 enum 사용
TEST_F(BasicTypesTest, EnumsAsFunctionParameters) {
    std::cout << "Testing enums as function parameters" << std::endl;
    
    auto process_error = [](extsock_error_t input_error) -> extsock_error_t {
        if (input_error == EXTSOCK_SUCCESS) {
            return EXTSOCK_SUCCESS;
        }
        return EXTSOCK_ERROR_MEMORY_ALLOCATION; // 다른 에러로 변환
    };
    
    auto classify_event = [](extsock_event_type_t event) -> bool {
        return event == EXTSOCK_EVENT_TUNNEL_UP || event == EXTSOCK_EVENT_TUNNEL_DOWN;
    };
    
    // 함수 호출 테스트
    EXPECT_EQ(process_error(EXTSOCK_SUCCESS), EXTSOCK_SUCCESS);
    EXPECT_EQ(process_error(EXTSOCK_ERROR_JSON_PARSE), EXTSOCK_ERROR_MEMORY_ALLOCATION);
    
    EXPECT_TRUE(classify_event(EXTSOCK_EVENT_TUNNEL_UP));
    EXPECT_TRUE(classify_event(EXTSOCK_EVENT_TUNNEL_DOWN));
    EXPECT_FALSE(classify_event(EXTSOCK_EVENT_CONFIG_APPLIED));
    
    std::cout << "Enums work correctly as function parameters" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "Starting Basic Types Tests (Week 1)" << std::endl;
    
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    
    if (result == 0) {
        std::cout << "All basic types tests passed!" << std::endl;
    } else {
        std::cout << "Some basic types tests failed!" << std::endl;
    }
    
    return result;
} 