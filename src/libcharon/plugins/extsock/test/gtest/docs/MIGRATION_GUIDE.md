# Check Framework → Google Test 마이그레이션 가이드

## 📋 개요

이 문서는 기존 Check 프레임워크 기반 테스트를 Google Test/Google Mock으로 마이그레이션하는 실제적인 가이드입니다.

---

## 🔄 기본 문법 변환

### 1. 테스트 함수 정의

**Check Framework**:
```c
START_TEST(test_function_name)
{
    // 테스트 로직
}
END_TEST
```

**Google Test**:
```cpp
TEST(TestSuiteName, TestName) {
    // 테스트 로직
}
```

### 2. Assertion 매크로 변환

| Check Framework | Google Test | 설명 |
|----------------|-------------|------|
| `ck_assert(expr)` | `EXPECT_TRUE(expr)` | 불리언 참 검증 |
| `ck_assert_int_eq(a, b)` | `EXPECT_EQ(a, b)` | 정수 동등성 |
| `ck_assert_int_ne(a, b)` | `EXPECT_NE(a, b)` | 정수 비동등성 |
| `ck_assert_int_lt(a, b)` | `EXPECT_LT(a, b)` | 미만 비교 |
| `ck_assert_int_le(a, b)` | `EXPECT_LE(a, b)` | 이하 비교 |
| `ck_assert_int_gt(a, b)` | `EXPECT_GT(a, b)` | 초과 비교 |
| `ck_assert_int_ge(a, b)` | `EXPECT_GE(a, b)` | 이상 비교 |
| `ck_assert_str_eq(a, b)` | `EXPECT_STREQ(a, b)` | 문자열 동등성 |
| `ck_assert_str_ne(a, b)` | `EXPECT_STRNE(a, b)` | 문자열 비동등성 |
| `ck_assert_ptr_nonnull(p)` | `EXPECT_NE(p, nullptr)` | NULL 포인터 검사 |
| `ck_assert_ptr_null(p)` | `EXPECT_EQ(p, nullptr)` | NULL 포인터 검사 |
| `ck_assert_ptr_eq(a, b)` | `EXPECT_EQ(a, b)` | 포인터 동등성 |

### 3. Suite 및 Runner 변환

**Check Framework**:
```c
Suite *create_test_suite(void) {
    Suite *s;
    TCase *tc_core;
    
    s = suite_create("TestSuite");
    tc_core = tcase_create("Core");
    
    tcase_add_test(tc_core, test_function_name);
    suite_add_tcase(s, tc_core);
    
    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    s = create_test_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

**Google Test**:
```cpp
// 메인 함수는 필요 없음 (gtest_main 링크 시)
// 또는 커스텀 메인:
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

---

## 🎯 실제 변환 예시

### 예시 1: 간단한 함수 테스트

**기존 (Check)**:
```c
#include <check.h>
#include "../common/extsock_errors.h"

START_TEST(test_extsock_error_create_valid_input)
{
    extsock_error_t code = EXTSOCK_ERROR_INVALID_CONFIG;
    const char *message = "Test error message";
    
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    ck_assert_ptr_nonnull(error_info);
    ck_assert_int_eq(error_info->code, code);
    ck_assert_str_eq(error_info->message, message);
    ck_assert_ptr_nonnull(error_info->timestamp);
    
    extsock_error_destroy(error_info);
}
END_TEST

START_TEST(test_extsock_error_create_null_message)
{
    extsock_error_t code = EXTSOCK_ERROR_CONNECTION_FAILED;
    const char *message = NULL;
    
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    ck_assert_ptr_nonnull(error_info);
    ck_assert_int_eq(error_info->code, code);
    ck_assert_str_eq(error_info->message, "Unknown error");
    
    extsock_error_destroy(error_info);
}
END_TEST
```

**새로운 (Google Test)**:
```cpp
#include <gtest/gtest.h>
extern "C" {
#include "../common/extsock_errors.h"
}

class ExtsockErrorsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트 전 설정 (필요시)
    }
    
    void TearDown() override {
        // 테스트 후 정리 (필요시)
    }
};

TEST_F(ExtsockErrorsTest, CreateValidInput) {
    extsock_error_t code = EXTSOCK_ERROR_INVALID_CONFIG;
    const char *message = "Test error message";
    
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, code);
    EXPECT_STREQ(error_info->message, message);
    EXPECT_NE(error_info->timestamp, nullptr);
    
    extsock_error_destroy(error_info);
}

TEST_F(ExtsockErrorsTest, CreateNullMessage) {
    extsock_error_t code = EXTSOCK_ERROR_CONNECTION_FAILED;
    const char *message = nullptr;
    
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, code);
    EXPECT_STREQ(error_info->message, "Unknown error");
    
    extsock_error_destroy(error_info);
}
```

### 예시 2: Parameterized Test 활용

**Check에서는 불가능 → Google Test에서 가능**:
```cpp
class ExtsockErrorParameterizedTest : 
    public ::testing::TestWithParam<std::tuple<extsock_error_t, const char*, const char*>> {
};

TEST_P(ExtsockErrorParameterizedTest, CreateWithDifferentErrorCodes) {
    auto [error_code, input_message, expected_message] = GetParam();
    
    extsock_error_info_t *error_info = extsock_error_create(error_code, input_message);
    
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, error_code);
    EXPECT_STREQ(error_info->message, expected_message);
    
    extsock_error_destroy(error_info);
}

INSTANTIATE_TEST_SUITE_P(
    AllErrorCodes,
    ExtsockErrorParameterizedTest,
    ::testing::Values(
        std::make_tuple(EXTSOCK_ERROR_INVALID_CONFIG, "Config error", "Config error"),
        std::make_tuple(EXTSOCK_ERROR_CONNECTION_FAILED, "Connection failed", "Connection failed"),
        std::make_tuple(EXTSOCK_ERROR_PARSE_ERROR, nullptr, "Unknown error"),
        std::make_tuple(EXTSOCK_ERROR_MEMORY_ALLOCATION, "", "Unknown error")
    )
);
```

---

## 🎭 Google Mock 활용

### Mock 클래스 생성

**기존 (수동 Mock)**:
```c
// Check에서는 수동으로 Mock 구현
typedef struct mock_json_parser_t {
    bool parse_called;
    bool parse_return_value;
    char* last_input;
} mock_json_parser_t;

bool mock_parse_config(const char* json_str) {
    // 수동 Mock 로직
    return true;
}
```

**새로운 (Google Mock)**:
```cpp
class MockJsonParser : public ExtsockJsonParserInterface {
public:
    MOCK_METHOD(bool, parse_config, (const char* json_str), (override));
    MOCK_METHOD(extsock_config_entity_t*, parse_config_entity, 
                (const char* json_str), (override));
    MOCK_METHOD(void, destroy, (), (override));
};

TEST(JsonParserTest, ParseConfigSuccess) {
    MockJsonParser mock_parser;
    const char* test_json = "{\"ike\": {\"version\": 2}}";
    
    // Mock 동작 설정
    EXPECT_CALL(mock_parser, parse_config(test_json))
        .WillOnce(::testing::Return(true));
    
    // 실제 호출 및 검증
    bool result = mock_parser.parse_config(test_json);
    EXPECT_TRUE(result);
}
```

---

## 🏗️ Test Fixture 패턴

### 기본 Fixture

```cpp
class ExtsockTestBase : public ::testing::Test {
protected:
    void SetUp() override {
        // 모든 테스트에서 공통으로 사용할 설정
        test_config = create_default_config();
        memory_tracker = create_memory_tracker();
    }
    
    void TearDown() override {
        // 정리 작업
        destroy_config(test_config);
        destroy_memory_tracker(memory_tracker);
    }
    
    // 보호된 멤버 (모든 테스트에서 접근 가능)
    extsock_config_t* test_config = nullptr;
    memory_tracker_t* memory_tracker = nullptr;
};

TEST_F(ExtsockTestBase, UseCommonSetup) {
    // test_config와 memory_tracker 사용 가능
    EXPECT_NE(test_config, nullptr);
    EXPECT_NE(memory_tracker, nullptr);
}
```

### 통합 테스트 Fixture

```cpp
class IntegrationTestFixture : public ExtsockTestBase {
protected:
    void SetUp() override {
        ExtsockTestBase::SetUp();  // 부모 설정 호출
        
        // 통합 테스트 전용 설정
        mock_strongswan = std::make_unique<MockStrongSwan>();
        plugin = create_extsock_plugin();
    }
    
    void TearDown() override {
        // 통합 테스트 정리
        destroy_plugin(plugin);
        mock_strongswan.reset();
        
        ExtsockTestBase::TearDown();  // 부모 정리 호출
    }
    
    std::unique_ptr<MockStrongSwan> mock_strongswan;
    extsock_plugin_t* plugin = nullptr;
};
```

---

## 📊 마이그레이션 단계별 접근법

### Phase 1: 인프라 및 Mock 구축

1. **Mock 인터페이스 정의**:
```cpp
// infrastructure/mocks/MockStrongSwan.hpp
class MockStrongSwan {
public:
    MOCK_METHOD(ike_sa_t*, create_ike_sa, (), ());
    MOCK_METHOD(void, destroy_ike_sa, (ike_sa_t* sa), ());
    // ... 기타 strongSwan API Mock
};
```

2. **기본 Fixture 생성**:
```cpp
// infrastructure/fixtures/ExtsockTestBase.hpp  
class ExtsockTestBase : public ::testing::Test {
    // 공통 설정/정리 로직
};
```

### Phase 2: Pure Unit Tests 마이그레이션

1. **extsock_errors 테스트 변환**:
   - `unit/test_extsock_errors_pure.c` → `unit/common/ExtsockErrorsTest.cpp`

2. **extsock_types 테스트 변환**:
   - `unit/test_extsock_types_pure.c` → `unit/common/ExtsockTypesTest.cpp`

### Phase 3: Adapter Tests 마이그레이션

1. **JSON Parser 테스트**:
   - Mock cJSON 라이브러리
   - 복잡한 JSON 파싱 시나리오 테스트

2. **Socket Adapter 테스트**:
   - Mock 소켓 API
   - 네트워크 시뮬레이션

### Phase 4: Integration Tests 마이그레이션

1. **End-to-End 워크플로우**:
   - 전체 플러그인 라이프사이클 테스트
   - 실제 strongSwan 컴포넌트와의 통합

---

## 🛠️ 빌드 시스템 통합

### CMake 통합

```cmake
# 기본 설정
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# 테스트 타겟 생성
add_executable(extsock_gtest
    ${TEST_SOURCES}
)

target_link_libraries(extsock_gtest
    GTest::gtest_main
    GMock::gmock
    # extsock 라이브러리들
)

# 테스트 등록
gtest_discover_tests(extsock_gtest)
```

### Makefile 통합

```makefile
# Google Test 설정
GTEST_LIBS = -lgtest -lgtest_main -lgmock -lgmock_main -lpthread

# 빌드 규칙
%_test: %_test.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(GTEST_LIBS)

# 테스트 실행
test: $(ALL_TESTS)
	@for test in $(ALL_TESTS); do ./$$test; done
```

---

## 🎯 마이그레이션 체크리스트

### ✅ 테스트 변환 체크리스트

- [ ] **테스트 함수명 변환**: `START_TEST` → `TEST`
- [ ] **Assertion 변환**: `ck_assert_*` → `EXPECT_*`
- [ ] **Setup/Teardown**: 함수 → Test Fixture
- [ ] **Suite 구조**: Check Suite → Google Test Class
- [ ] **메인 함수**: Check Runner → Google Test Main
- [ ] **포함 파일**: `#include <check.h>` → `#include <gtest/gtest.h>`
- [ ] **C++ 호환성**: C 헤더 → `extern "C" { }` 래핑

### ✅ 품질 검증 체크리스트

- [ ] **컴파일 성공**: 모든 테스트가 오류 없이 컴파일됨
- [ ] **테스트 통과**: 기존 테스트와 동일한 결과
- [ ] **메모리 안전성**: Valgrind 검사 통과
- [ ] **성능 검증**: 실행 시간이 허용 범위 내
- [ ] **커버리지 유지**: 코드 커버리지 동일 이상

---

## 🚀 고급 기능 활용

### 1. Parameterized Tests

```cpp
class ConfigParsingTest : public ::testing::TestWithParam<const char*> {
};

TEST_P(ConfigParsingTest, ParseDifferentConfigs) {
    const char* config_json = GetParam();
    // 다양한 설정으로 테스트
}

INSTANTIATE_TEST_SUITE_P(
    AllConfigs, 
    ConfigParsingTest,
    ::testing::ValuesIn(config_files)
);
```

### 2. Custom Matchers

```cpp
MATCHER_P(ConfigHasVersion, version, "") {
    return arg != nullptr && arg->version == version;
}

TEST(ConfigTest, ParsedConfigHasCorrectVersion) {
    auto config = parse_config(test_json);
    EXPECT_THAT(config, ConfigHasVersion(2));
}
```

### 3. Death Tests

```cpp
TEST(ConfigTest, InvalidConfigCausesCrash) {
    EXPECT_DEATH(parse_invalid_config(), "Invalid configuration");
}
```

---

## 📚 참고 자료

- [Google Test 공식 문서](https://google.github.io/googletest/)
- [Google Mock 가이드](https://google.github.io/googletest/gmock_cook_book.html)
- [Check Framework 문서](https://libcheck.github.io/check/)
- [C++에서 C 코드 테스트하기](https://google.github.io/googletest/faq.html#can-i-use-google-test-to-test-c-code)

---

**업데이트**: 2024-08-24  
**버전**: 1.0  
**작성자**: Claude Assistant