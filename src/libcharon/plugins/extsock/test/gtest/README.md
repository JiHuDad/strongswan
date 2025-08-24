# extsock Plugin Google Test Suite

**🚀 Google Test/Mock 기반 현대적 테스트 프레임워크**

## 📋 프로젝트 개요

이 디렉토리는 기존 Check 프레임워크를 Google Test/Google Mock으로 마이그레이션한 새로운 테스트 스위트입니다.

- **시작일**: 2024-08-24
- **상태**: 🚧 **개발 중** - 마이그레이션 진행 중
- **목표**: 현대적이고 강력한 C++ 테스트 프레임워크로 전환

---

## 🏗️ 디렉토리 구조

```
gtest/
├── 📄 README.md                   # 이 파일
├── 🔧 CMakeLists.txt              # CMake 빌드 시스템 (주)
├── 🔧 Makefile.gtest              # Makefile 빌드 시스템 (보조)
├── 🏗️ infrastructure/             # Level 0: 테스트 인프라
│   ├── mocks/                     # Google Mock 기반 Mock 클래스들
│   │   ├── MockStrongSwan.hpp     # strongSwan API Mock
│   │   ├── MockJsonParser.hpp     # JSON 파서 Mock
│   │   └── MockSocketAdapter.hpp  # 소켓 어댑터 Mock
│   ├── fixtures/                  # 테스트 Fixture 클래스들
│   │   ├── ExtsockTestBase.hpp    # 기본 테스트 Fixture
│   │   └── IntegrationTestFixture.hpp # 통합 테스트 Fixture
│   └── utils/                     # 테스트 유틸리티
│       ├── TestDataFactory.hpp    # 테스트 데이터 팩토리
│       └── MemoryTestUtils.hpp    # 메모리 테스트 유틸
├── 🧪 unit/                       # Level 1-2: 단위/어댑터 테스트
│   ├── common/                    # 공통 모듈 테스트
│   │   ├── ExtsockErrorsTest.cpp  # extsock_errors 테스트
│   │   └── ExtsockTypesTest.cpp   # extsock_types 테스트
│   ├── adapters/                  # 어댑터 레이어 테스트
│   │   ├── JsonParserTest.cpp     # JSON 파서 테스트
│   │   ├── SocketAdapterTest.cpp  # 소켓 어댑터 테스트
│   │   └── StrongswanAdapterTest.cpp # strongSwan 어댑터 테스트
│   └── domain/                    # 도메인 레이어 테스트
│       ├── ConfigEntityTest.cpp   # 설정 엔티티 테스트
│       └── ConfigUsecaseTest.cpp  # 설정 유스케이스 테스트
├── 🔗 integration/                # Level 3: 통합 테스트
│   ├── WorkflowIntegrationTest.cpp    # 워크플로우 통합 테스트
│   ├── PluginLifecycleTest.cpp        # 플러그인 라이프사이클 테스트
│   └── FailoverIntegrationTest.cpp    # 페일오버 통합 테스트
├── ⚡ performance/                 # 성능 테스트
│   ├── BenchmarkTests.cpp         # 벤치마크 테스트
│   └── StressTests.cpp            # 스트레스 테스트
├── 📜 scripts/                    # Google Test 실행 스크립트
│   ├── run_gtest_suite.sh         # 전체 테스트 스위트 실행
│   ├── run_gtest_with_coverage.sh # 커버리지 포함 실행
│   └── generate_gtest_report.sh   # 리포트 생성
└── 📚 docs/                       # Google Test 관련 문서
    ├── MIGRATION_GUIDE.md         # 마이그레이션 가이드
    ├── GTEST_CONVENTIONS.md       # Google Test 컨벤션
    └── MOCK_GUIDELINES.md         # Mock 사용 가이드라인
```

---

## 🚀 Quick Start

### 전제조건

```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev libgmock-dev cmake build-essential

# macOS
brew install googletest cmake
```

### 빌드 및 실행

#### CMake 방식 (권장)
```bash
cd gtest/
mkdir build && cd build
cmake ..
make
ctest
```

#### Makefile 방식
```bash
cd gtest/
make -f Makefile.gtest all
make -f Makefile.gtest test
```

---

## 🔄 Check vs Google Test 비교

| 특성 | Check Framework | Google Test | 장점 |
|------|----------------|-------------|------|
| **테스트 작성** | `START_TEST/END_TEST` | `TEST(TestSuite, TestName)` | 더 직관적 |
| **Assertion** | `ck_assert_int_eq(a, b)` | `EXPECT_EQ(a, b)` | 더 자연스러운 문법 |
| **Mock 지원** | 수동 Mock | `MOCK_METHOD()` | 자동 Mock 생성 |
| **출력 형식** | 텍스트 | XML/JSON + 색상 | CI/CD 친화적 |
| **테스트 필터** | 제한적 | `--gtest_filter=*` | 유연한 필터링 |

### 마이그레이션 예시

**기존 (Check)**:
```c
START_TEST(test_extsock_error_create)
{
    extsock_error_info_t *error = extsock_error_create(
        EXTSOCK_ERROR_INVALID_CONFIG, "test message"
    );
    ck_assert_ptr_nonnull(error);
    ck_assert_int_eq(error->code, EXTSOCK_ERROR_INVALID_CONFIG);
    extsock_error_destroy(error);
}
END_TEST
```

**새로운 (Google Test)**:
```cpp
TEST(ExtsockErrorsTest, CreateValidError) {
    extsock_error_info_t *error = extsock_error_create(
        EXTSOCK_ERROR_INVALID_CONFIG, "test message"
    );
    EXPECT_NE(error, nullptr);
    EXPECT_EQ(error->code, EXTSOCK_ERROR_INVALID_CONFIG);
    extsock_error_destroy(error);
}
```

---

## 📊 마이그레이션 현황

### 전체 진행률
```
Google Test 마이그레이션: [□□□□□□□□□□] 0% (0 완료 / 19+ 파일)
├── Infrastructure ⏳ 대기중 (0/3 파일)
├── Unit Tests ⏳ 대기중 (0/10+ 파일)
└── Integration ⏳ 대기중 (0/6 파일)
```

### 우선순위 마이그레이션 순서
1. **Phase 1**: Infrastructure & Mocks 구축
2. **Phase 2**: Pure Unit Tests (Level 1) 마이그레이션
3. **Phase 3**: Adapter Tests (Level 2) 마이그레이션  
4. **Phase 4**: Integration Tests (Level 3) 마이그레이션
5. **Phase 5**: 고급 기능 활용 (Parameterized, Performance)

---

## 🎯 Google Test 고급 기능

### Parameterized Tests 예시
```cpp
class ExtsockErrorParameterizedTest : 
    public ::testing::TestWithParam<extsock_error_t> {};

TEST_P(ExtsockErrorParameterizedTest, CreateDifferentErrorCodes) {
    extsock_error_t code = GetParam();
    extsock_error_info_t *error = extsock_error_create(code, "test");
    
    EXPECT_NE(error, nullptr);
    EXPECT_EQ(error->code, code);
    
    extsock_error_destroy(error);
}

INSTANTIATE_TEST_SUITE_P(
    AllErrorCodes,
    ExtsockErrorParameterizedTest,
    ::testing::Values(
        EXTSOCK_ERROR_INVALID_CONFIG,
        EXTSOCK_ERROR_CONNECTION_FAILED,
        EXTSOCK_ERROR_PARSE_ERROR
    )
);
```

### Google Mock 예시
```cpp
class MockJsonParser : public ExtsockJsonParserInterface {
public:
    MOCK_METHOD(bool, parse_config, (const char* json_str), (override));
    MOCK_METHOD(void, destroy, (), (override));
};

TEST(JsonParserTest, ParseConfigSuccess) {
    MockJsonParser mock_parser;
    const char* test_json = "{\"ike\": {}}";
    
    EXPECT_CALL(mock_parser, parse_config(test_json))
        .WillOnce(Return(true));
    
    bool result = mock_parser.parse_config(test_json);
    EXPECT_TRUE(result);
}
```

---

## 🔧 개발 가이드라인

### 네이밍 컨벤션
- **테스트 클래스**: `[Component]Test` (예: `ExtsockErrorsTest`)
- **테스트 케이스**: `[Action][Expected]` (예: `CreateValidError`)
- **Mock 클래스**: `Mock[Interface]` (예: `MockJsonParser`)
- **Fixture 클래스**: `[Component]TestFixture` (예: `JsonParserTestFixture`)

### 코딩 스타일
- **C++ 표준**: C++17 사용
- **Assertion**: `EXPECT_*` 우선, 치명적인 경우만 `ASSERT_*`
- **Mock**: Interface 기반으로 Mock 클래스 생성
- **Fixture**: 공통 설정/정리 로직은 Test Fixture 활용

---

## 📚 참고 문서

### 내부 문서
- [마이그레이션 계획서](../docs/GTEST_MIGRATION_PLAN.md) - 전체 마이그레이션 계획
- [마이그레이션 가이드](docs/MIGRATION_GUIDE.md) - 실제 변환 가이드
- [컨벤션 가이드](docs/GTEST_CONVENTIONS.md) - 코딩 스타일 및 컨벤션
- [Mock 가이드라인](docs/MOCK_GUIDELINES.md) - Mock 사용 베스트 프랙티스

### 외부 참고자료
- [Google Test 공식 문서](https://google.github.io/googletest/)
- [Google Mock 가이드](https://google.github.io/googletest/gmock_cook_book.html)
- [CMake + Google Test 통합](https://cmake.org/cmake/help/latest/module/GoogleTest.html)

---

## 🤝 기여 가이드

### 테스트 추가 프로세스
1. 해당 카테고리의 디렉토리에 테스트 파일 생성
2. Google Test 컨벤션에 맞춰 테스트 작성
3. CMakeLists.txt에 새 테스트 파일 추가
4. 빌드 및 실행 테스트
5. 문서 업데이트

### 코드 리뷰 체크리스트
- [ ] Google Test 네이밍 컨벤션 준수
- [ ] 적절한 Assertion 매크로 사용
- [ ] Mock 사용 시 Interface 기반 설계
- [ ] 메모리 누수 없음 (Valgrind 검증)
- [ ] 테스트 격리 보장 (독립성)

---

**프로젝트 상태**: 🚧 **개발 중** - Google Test 마이그레이션 초기 단계  
**마지막 업데이트**: 2024-08-24  
**버전**: 0.1.0 (Alpha)  
**라이선스**: strongSwan Project License