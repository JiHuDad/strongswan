/*
 * Hello Google Test - 초기 설정 검증용 테스트
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Test 마이그레이션의 첫 번째 단계로 
 * 기본적인 Google Test 환경 설정을 검증합니다.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// 기본 Google Test 기능 테스트
TEST(HelloGoogleTest, BasicAssertions) {
    // 기본 비교 테스트
    EXPECT_EQ(7 * 6, 42);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
    
    // 문자열 테스트
    EXPECT_STREQ("hello", "hello");
    EXPECT_STRNE("hello", "world");
    
    // 포인터 테스트
    int value = 42;
    int* ptr = &value;
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 42);
}

// C 스타일 함수 테스트 (extsock은 C로 작성됨)
extern "C" {
    // 간단한 C 함수 시뮬레이션
    int add_numbers(int a, int b) {
        return a + b;
    }
    
    const char* get_test_message() {
        return "Google Test is working!";
    }
}

TEST(HelloGoogleTest, CLanguageCompatibility) {
    // C 함수 호출 테스트
    EXPECT_EQ(add_numbers(2, 3), 5);
    EXPECT_EQ(add_numbers(-1, 1), 0);
    
    // C 문자열 함수 테스트
    const char* message = get_test_message();
    EXPECT_STREQ(message, "Google Test is working!");
}

// Google Mock 기본 테스트
class MockInterface {
public:
    virtual ~MockInterface() = default;
    virtual int getValue() = 0;
    virtual bool processData(const char* data) = 0;
};

class MockImplementation : public MockInterface {
public:
    MOCK_METHOD(int, getValue, (), (override));
    MOCK_METHOD(bool, processData, (const char* data), (override));
};

TEST(HelloGoogleTest, GoogleMockBasics) {
    MockImplementation mock;
    
    // Mock 동작 설정
    EXPECT_CALL(mock, getValue())
        .WillOnce(::testing::Return(42));
        
    EXPECT_CALL(mock, processData(::testing::_))
        .WillOnce(::testing::Return(true));
    
    // Mock 호출 및 검증
    EXPECT_EQ(mock.getValue(), 42);
    EXPECT_TRUE(mock.processData("test data"));
}

// Test Fixture 예시
class HelloGoogleTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트 시작 전 설정
        test_data = new int[5]{1, 2, 3, 4, 5};
        test_string = "fixture test";
    }
    
    void TearDown() override {
        // 테스트 완료 후 정리
        delete[] test_data;
        test_data = nullptr;
    }
    
    int* test_data = nullptr;
    const char* test_string = nullptr;
};

TEST_F(HelloGoogleTestFixture, UseFixtureData) {
    // Fixture에서 설정한 데이터 사용
    ASSERT_NE(test_data, nullptr);
    EXPECT_EQ(test_data[0], 1);
    EXPECT_EQ(test_data[4], 5);
    EXPECT_STREQ(test_string, "fixture test");
}

// Parameterized Test 예시
class ParameterizedTestExample : 
    public ::testing::TestWithParam<std::pair<int, int>> {
};

TEST_P(ParameterizedTestExample, AdditionTest) {
    auto [a, b] = GetParam();
    int result = add_numbers(a, b);
    EXPECT_EQ(result, a + b);
}

INSTANTIATE_TEST_SUITE_P(
    AdditionTests,
    ParameterizedTestExample,
    ::testing::Values(
        std::make_pair(1, 1),
        std::make_pair(2, 3),
        std::make_pair(-1, 5),
        std::make_pair(0, 0),
        std::make_pair(10, -5)
    )
);

// 실패하는 테스트 예시 (비활성화됨)
TEST(HelloGoogleTest, DISABLED_FailureExample) {
    // 이 테스트는 실행되지 않음 (DISABLED_ 접두사)
    EXPECT_EQ(1, 2) << "This test is disabled and won't run";
}

// 조건부 테스트 예시
#ifdef ENABLE_EXPERIMENTAL_TESTS
TEST(HelloGoogleTest, ExperimentalFeature) {
    // 실험적 기능 테스트
    EXPECT_TRUE(true) << "Experimental feature test";
}
#endif

// Death Test 예시 (크래시 테스트)
TEST(HelloGoogleTest, DeathTest) {
    // 정상적인 경우
    EXPECT_EXIT(exit(0), ::testing::ExitedWithCode(0), "");
    
    // 비정상 종료 테스트 (실제로는 실행하지 않음)
    // EXPECT_DEATH(abort(), "");
}

// 성능 관련 간단한 테스트
TEST(HelloGoogleTest, SimplePerformanceTest) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // 간단한 연산 수행
    volatile int sum = 0;
    for (int i = 0; i < 10000; ++i) {
        sum += i;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start
    );
    
    // 너무 오래 걸리지 않는지 확인 (10ms 이하)
    EXPECT_LT(duration.count(), 10000) 
        << "Simple loop took too long: " << duration.count() << " microseconds";
}

// 메모리 관련 기본 테스트
TEST(HelloGoogleTest, MemoryTest) {
    // 동적 할당/해제 테스트
    int* ptr = new int(42);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 42);
    delete ptr;
    
    // 배열 할당/해제 테스트  
    int* array = new int[10];
    ASSERT_NE(array, nullptr);
    array[0] = 1;
    array[9] = 10;
    EXPECT_EQ(array[0], 1);
    EXPECT_EQ(array[9], 10);
    delete[] array;
}

// 에러 처리 테스트
TEST(HelloGoogleTest, ErrorHandlingTest) {
    // 예외 테스트
    EXPECT_NO_THROW({
        int result = add_numbers(1, 2);
        (void)result; // 컴파일러 경고 방지
    });
    
    // 조건부 실행
    if (true) {
        EXPECT_TRUE(true);
    } else {
        FAIL() << "This should not be reached";
    }
}

// 메인 함수 (Google Test 기본 메인 사용)
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }