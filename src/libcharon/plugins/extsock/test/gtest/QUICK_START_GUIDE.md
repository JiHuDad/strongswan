# Google Test Suite Quick Start Guide

## 🚀 빠른 시작

### 1. 빌드 및 실행
```bash
cd /home/finux/dev/plugin/strongswan/src/libcharon/plugins/extsock/test/gtest/build

# 빌드
make unit_tests

# 모든 테스트 실행
./unit_tests

# 특정 테스트만 실행
./unit_tests --gtest_filter="ExtsockErrorsTest*"
./unit_tests --gtest_filter="JsonParserTestSimple*"
./unit_tests --gtest_filter="SocketAdapterTest*"
./unit_tests --gtest_filter="StrongswanAdapterTest*"
```

### 2. 현재 테스트 현황 (2024-08-25)
```
총 116개 테스트 - 100% 통과 ✅

├── Level 1 Pure Tests (31개)
│   ├── ExtsockErrorsTest: 14개 테스트
│   └── ExtsockTypesTest: 17개 테스트
│
├── Level 2 Adapter Tests (65개)  
│   ├── JsonParserTestSimple: 22개 테스트
│   ├── SocketAdapterTest: 21개 테스트
│   └── StrongswanAdapterTest: 22개 테스트
│
└── Parameterized Tests (20개)
    ├── ErrorCodes: 6개 테스트
    ├── AllErrorCodes: 8개 테스트  
    └── AllSeverityLevels: 6개 테스트
```

## 📊 테스트 결과 예시

```bash
$ ./unit_tests
[==========] Running 116 tests from 8 test suites.
[----------] Global test environment set-up.
[----------] 14 tests from ExtsockErrorsTest
[ RUN      ] ExtsockErrorsTest.CreateValidInput
[       OK ] ExtsockErrorsTest.CreateValidInput (0 ms)
...
[----------] 14 tests from ExtsockErrorsTest (0 ms total)

[----------] Global test environment tear-down
[==========] 116 tests from 8 test suites ran. (17 ms total)
[  PASSED  ] 116 tests.
```

## 🔧 고급 사용법

### 테스트 필터링
```bash
# 실패한 테스트만 실행
./unit_tests --gtest_filter="*FAILED*"

# 특정 패턴 제외
./unit_tests --gtest_filter="-*Stress*"

# 여러 패턴 조합
./unit_tests --gtest_filter="*Error*:*Json*"
```

### 상세 출력
```bash
# XML 출력 (CI/CD용)
./unit_tests --gtest_output=xml:test_results.xml

# 테스트 목록만 보기
./unit_tests --gtest_list_tests

# 반복 실행
./unit_tests --gtest_repeat=10
```

### 성능 측정
```bash
# 시간 측정
time ./unit_tests

# 메모리 사용량 (valgrind 필요시)
valgrind --tool=memcheck ./unit_tests
```

## 📁 프로젝트 구조

```
gtest/
├── build/                      # 빌드 디렉토리
│   ├── CMakeCache.txt
│   ├── Makefile
│   └── unit_tests              # 실행 파일
├── CMakeLists.txt              # CMake 설정
├── include/                    # 헤더 파일
│   └── c_wrappers/
├── infrastructure/             # Mock 및 인프라
│   ├── fixtures/
│   └── mocks/
│       └── MockStrongSwan.hpp
└── src/                        # 테스트 소스
    ├── c_wrappers/
    └── unit/
        ├── ExtsockErrorsTest.cpp
        ├── ExtsockTypesTest.cpp
        ├── JsonParserTestSimple.cpp
        ├── SocketAdapterTest.cpp
        └── StrongswanAdapterTest.cpp
```

## 🏆 주요 특징

### 1. Google Mock 활용
```cpp
// 강력한 Mock 객체 지원
EXPECT_CALL(*mock_adapter, add_peer_config(test_peer))
    .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
```

### 2. Parameterized Testing
```cpp
// 데이터 드리븐 테스트
INSTANTIATE_TEST_SUITE_P(ErrorCodes, ExtsockErrorsParameterizedTest,
    ::testing::ValuesIn(error_test_data));
```

### 3. 현대적 C++17 지원
```cpp
// 스마트 포인터와 RAII 패턴
auto mock_adapter = std::make_unique<MockStrongSwanAdapter>();
```

### 4. 풍부한 Assertion
```cpp
// 다양한 검증 매크로
EXPECT_EQ(result, EXTSOCK_SUCCESS);
EXPECT_STREQ(name, "test_peer");
EXPECT_THAT(values, ::testing::ContainsRegex(".*pattern.*"));
```

## 🔍 디버깅 팁

### 실패한 테스트 분석
```bash
# 자세한 실패 정보
./unit_tests --gtest_break_on_failure

# 특정 테스트만 디버깅
./unit_tests --gtest_filter="ExtsockErrorsTest.CreateValidInput" --gtest_break_on_failure
```

### Mock 디버깅
```cpp
// Mock 호출 검증
EXPECT_CALL(*mock, method())
    .Times(::testing::Exactly(2))
    .WillRepeatedly(::testing::Return(value));
```

## 🚀 성능 정보

- **빌드 시간**: ~10초 (첫 빌드 시)
- **테스트 실행 시간**: ~17ms (116개 테스트)
- **메모리 사용량**: 최소한의 오버헤드
- **바이너리 크기**: ~2MB (디버그 포함)

## 📚 참고 자료

- [Google Test Documentation](https://google.github.io/googletest/)
- [Google Mock Cheat Sheet](https://github.com/google/googletest/blob/master/docs/gmock_cheat_sheet.md)
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)

---
**업데이트**: 2024-08-25  
**버전**: 1.0  
**상태**: 116개 테스트 100% 통과 🎉