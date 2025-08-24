# Google Test 마이그레이션 계획

## 📋 프로젝트 개요

**목표**: 기존 Check 프레임워크 기반 테스트를 Google Test/Google Mock으로 마이그레이션  
**시작일**: 2024-08-24  
**예상 완료일**: 2024-09-07 (약 2주)  
**현재 상태**: 🚀 **계획 수립 완료** - 마이그레이션 준비  

---

## 🎯 마이그레이션 목표 및 이유

### 📈 개선 목표
1. **더 나은 테스트 프레임워크**: 현대적이고 강력한 C++ 테스트 프레임워크
2. **향상된 Mock 지원**: Google Mock을 통한 정교한 모킹 시스템
3. **더 나은 출력 포맷**: XML/JSON 출력, 색상 지원, 상세한 실패 정보
4. **크로스 플랫폼 호환성**: 더 넓은 플랫폼 지원 및 CI/CD 통합
5. **현대적 C++ 지원**: 람다, auto 등 현대 C++ 기능 활용

### ⚖️ Check vs Google Test 비교

| 특성 | Check Framework | Google Test | 개선점 |
|------|----------------|-------------|---------|
| **언어 지원** | C Only | C/C++ | C++ 호환성 |
| **Assertion 매크로** | 기본적 | 풍부함 | `EXPECT_THAT`, `ASSERT_THAT` 등 |
| **Mock 지원** | 없음 | Google Mock | 강력한 모킹 시스템 |
| **출력 형식** | 텍스트 | XML/JSON/색상 | CI/CD 통합성 |
| **테스트 조직화** | Suite/Runner | Class/Fixture | 더 체계적 |
| **Parameterized Tests** | 없음 | 지원 | 데이터 드리븐 테스트 |

---

## 🗓️ 3단계 마이그레이션 일정

### Phase 1: 기초 설정 및 인프라 구축 (Week 1)
- **목표**: Google Test 환경 설정 및 기본 구조 구축
- **기간**: Day 1-5
- **상태**: ⏳ **대기 중**

### Phase 2: 테스트 마이그레이션 (Week 1-2)  
- **목표**: 기존 테스트를 Google Test로 단계별 변환
- **기간**: Day 3-10
- **상태**: ⏳ **대기 중**

### Phase 3: 고급 기능 활용 및 최적화 (Week 2)
- **목표**: Google Mock, Fixtures, Parameterized Tests 활용
- **기간**: Day 8-14
- **상태**: ⏳ **대기 중**

---

## 📁 새로운 디렉토리 구조

```
extsock/test/
├── check/                          # 기존 Check 기반 테스트 (보존)
│   ├── infrastructure/
│   ├── unit/
│   ├── integration/
│   └── scripts/
├── gtest/                          # 새로운 Google Test 기반 테스트
│   ├── README.md                   # Google Test 테스트 가이드
│   ├── CMakeLists.txt             # CMake 빌드 시스템
│   ├── Makefile.gtest             # Makefile 빌드 시스템  
│   ├── infrastructure/            # Level 0: Mock 및 테스트 인프라
│   │   ├── mocks/                 # Google Mock 기반 Mock 클래스들
│   │   │   ├── MockStrongSwan.hpp
│   │   │   ├── MockJsonParser.hpp
│   │   │   └── MockSocketAdapter.hpp
│   │   ├── fixtures/              # 테스트 Fixture 클래스들
│   │   │   ├── ExtsockTestBase.hpp
│   │   │   └── IntegrationTestFixture.hpp
│   │   └── utils/                 # 테스트 유틸리티
│   │       ├── TestDataFactory.hpp
│   │       └── MemoryTestUtils.hpp
│   ├── unit/                      # Level 1-2: 단위/어댑터 테스트
│   │   ├── common/                # 공통 모듈 테스트
│   │   │   ├── ExtsockErrorsTest.cpp
│   │   │   └── ExtsockTypesTest.cpp
│   │   ├── adapters/              # 어댑터 레이어 테스트
│   │   │   ├── JsonParserTest.cpp
│   │   │   ├── SocketAdapterTest.cpp
│   │   │   └── StrongswanAdapterTest.cpp
│   │   └── domain/                # 도메인 레이어 테스트
│   │       ├── ConfigEntityTest.cpp
│   │       └── ConfigUsecaseTest.cpp
│   ├── integration/               # Level 3: 통합 테스트
│   │   ├── WorkflowIntegrationTest.cpp
│   │   ├── PluginLifecycleTest.cpp
│   │   └── FailoverIntegrationTest.cpp
│   ├── performance/               # 성능 테스트
│   │   ├── BenchmarkTests.cpp
│   │   └── StressTests.cpp
│   ├── scripts/                   # Google Test 실행 스크립트
│   │   ├── run_gtest_suite.sh
│   │   ├── run_gtest_with_coverage.sh
│   │   └── generate_gtest_report.sh
│   └── docs/                      # Google Test 관련 문서
│       ├── MIGRATION_GUIDE.md
│       ├── GTEST_CONVENTIONS.md
│       └── MOCK_GUIDELINES.md
└── docs/
    ├── GTEST_MIGRATION_PLAN.md    # 이 문서
    └── TESTING_COMPARISON.md      # Check vs Google Test 비교
```

---

## 🔧 Phase 1: 기초 설정 및 인프라 구축

### TASK-M001: Google Test 환경 설정 🔴 HIGH
**예상 소요 시간**: 1일  
**담당자**: Claude Assistant  

#### 📋 세부 작업:
- [ ] **Day 1**: Google Test/Mock 의존성 설정
  - [ ] Google Test 라이브러리 다운로드 및 설치
  - [ ] Google Mock 통합 설정
  - [ ] CMakeLists.txt 생성 (primary build system)
  - [ ] Makefile.gtest 생성 (alternative build system)
  
- [ ] **Day 1**: 디렉토리 구조 생성
  - [ ] `/gtest` 루트 디렉토리 생성
  - [ ] 서브디렉토리 구조 생성
  - [ ] 기본 README.md 및 가이드 문서 생성

#### 🎯 완료 기준:
- Google Test 기본 프로젝트가 빌드되고 실행됨
- "Hello World" 스타일 테스트가 통과함
- CI/CD 파이프라인에서 빌드 확인됨

### TASK-M002: Mock 인프라 구축 🔴 HIGH  
**예상 소요 시간**: 2일  
**담당자**: Claude Assistant  

#### 📋 세부 작업:
- [ ] **Day 2-3**: Google Mock 기반 Mock 클래스 생성
  - [ ] `MockStrongSwan.hpp` - strongSwan API Mock
  - [ ] `MockJsonParser.hpp` - JSON 파서 Mock  
  - [ ] `MockSocketAdapter.hpp` - 소켓 어댑터 Mock
  - [ ] Mock 클래스 통합 테스트
  
- [ ] **Day 3**: 테스트 Fixture 기반 클래스 생성
  - [ ] `ExtsockTestBase.hpp` - 기본 테스트 Fixture
  - [ ] `IntegrationTestFixture.hpp` - 통합 테스트용 Fixture
  - [ ] 메모리 관리 및 설정/정리 로직 구현

#### 🎯 완료 기준:
- 모든 Mock 클래스가 컴파일되고 기본 동작 확인됨
- Fixture 클래스를 사용한 테스트가 정상 실행됨
- 기존 strongSwan Mock과 호환성 확인됨

---

## 🧪 Phase 2: 테스트 마이그레이션

### TASK-M003: Level 1 Pure 테스트 마이그레이션 🟡 MEDIUM
**예상 소요 시간**: 2일  
**우선순위**: 첫 번째 마이그레이션 대상  

#### 📊 마이그레이션 대상:
| 기존 Check 파일 | 새로운 Google Test 파일 | 테스트 수 |
|----------------|------------------------|----------|
| `test_extsock_errors_pure.c` | `ExtsockErrorsTest.cpp` | 13개 |
| `test_extsock_types_pure.c` | `ExtsockTypesTest.cpp` | 14개 |

#### 🔄 변환 예시:
**기존 (Check)**:
```c
START_TEST(test_extsock_error_create_valid_input)
{
    extsock_error_t code = EXTSOCK_ERROR_INVALID_CONFIG;
    const char *message = "Test error message";
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    ck_assert_ptr_nonnull(error_info);
    ck_assert_int_eq(error_info->code, code);
    ck_assert_str_eq(error_info->message, message);
    
    extsock_error_destroy(error_info);
}
END_TEST
```

**새로운 (Google Test)**:
```cpp
TEST(ExtsockErrorsTest, CreateValidInput) {
    extsock_error_t code = EXTSOCK_ERROR_INVALID_CONFIG;
    const char *message = "Test error message";
    extsock_error_info_t *error_info = extsock_error_create(code, message);
    
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, code);
    EXPECT_STREQ(error_info->message, message);
    
    extsock_error_destroy(error_info);
}
```

### TASK-M004: Level 2 Adapter 테스트 마이그레이션 🟡 MEDIUM
**예상 소요 시간**: 3일  

#### 📊 마이그레이션 대상:
| 컴포넌트 | 기존 파일 | 새로운 파일 | 특징 |
|----------|-----------|-------------|------|
| JSON Parser | `test_extsock_json_parser_adapter.c` | `JsonParserTest.cpp` | Mock cJSON 활용 |
| Socket Adapter | `test_socket_adapter_*.c` | `SocketAdapterTest.cpp` | Mock 소켓 시스템 |
| strongSwan Adapter | `test_extsock_strongswan_adapter.c` | `StrongswanAdapterTest.cpp` | Mock strongSwan API |

#### 🎯 Google Mock 활용 예시:
```cpp
class MockJsonParser : public ExtsockJsonParserInterface {
public:
    MOCK_METHOD(ike_cfg_t*, parse_ike_config, (cJSON* ike_json), (override));
    MOCK_METHOD(auth_cfg_t*, parse_auth_config, (cJSON* auth_json), (override));
    MOCK_METHOD(bool, parse_child_configs, (cJSON* children_json), (override));
};

TEST_F(JsonParserTest, ParseIkeConfigSuccess) {
    MockJsonParser mock_parser;
    cJSON *test_json = cJSON_CreateObject();
    ike_cfg_t *expected_config = create_test_ike_config();
    
    EXPECT_CALL(mock_parser, parse_ike_config(test_json))
        .WillOnce(Return(expected_config));
    
    ike_cfg_t *result = mock_parser.parse_ike_config(test_json);
    EXPECT_EQ(result, expected_config);
    
    cJSON_Delete(test_json);
}
```

### TASK-M005: Level 3 Integration 테스트 마이그레이션 🟠 LOW
**예상 소요 시간**: 3일  

#### 📊 마이그레이션 대상:
| 테스트 카테고리 | 기존 파일 | 새로운 파일 | 복잡도 |
|---------------|-----------|-------------|--------|
| End-to-End | `test_end_to_end_workflow.c` | `WorkflowIntegrationTest.cpp` | High |
| Plugin Lifecycle | `test_plugin_lifecycle_real.c` | `PluginLifecycleTest.cpp` | Medium |
| Failover | `test_failover_manager_real.c` | `FailoverIntegrationTest.cpp` | Medium |

---

## ⚡ Phase 3: 고급 기능 활용 및 최적화

### TASK-M006: Parameterized Tests 도입 🟢 ENHANCEMENT
**예상 소요 시간**: 2일  

#### 🎯 활용 예시:
```cpp
class ExtsockErrorParameterizedTest : 
    public ::testing::TestWithParam<std::pair<extsock_error_t, const char*>> {};

TEST_P(ExtsockErrorParameterizedTest, ErrorCreationWithDifferentCodes) {
    auto [error_code, error_message] = GetParam();
    
    extsock_error_info_t *error_info = extsock_error_create(error_code, error_message);
    
    EXPECT_NE(error_info, nullptr);
    EXPECT_EQ(error_info->code, error_code);
    EXPECT_STREQ(error_info->message, error_message);
    
    extsock_error_destroy(error_info);
}

INSTANTIATE_TEST_SUITE_P(
    ErrorCodes,
    ExtsockErrorParameterizedTest,
    ::testing::Values(
        std::make_pair(EXTSOCK_ERROR_INVALID_CONFIG, "Invalid config"),
        std::make_pair(EXTSOCK_ERROR_CONNECTION_FAILED, "Connection failed"),
        std::make_pair(EXTSOCK_ERROR_PARSE_ERROR, "Parse error")
    )
);
```

### TASK-M007: 성능 테스트 프레임워크 구축 🟢 ENHANCEMENT
**예상 소요 시간**: 2일  

#### 🎯 Google Benchmark 통합:
```cpp
#include <benchmark/benchmark.h>

static void BM_ExtsockJsonParsing(benchmark::State& state) {
    cJSON *test_json = create_large_test_config();
    extsock_json_parser_t *parser = extsock_json_parser_create();
    
    for (auto _ : state) {
        extsock_config_entity_t *config = 
            parser->parse_config_entity(parser, test_json);
        benchmark::DoNotOptimize(config);
        // cleanup
    }
    
    cJSON_Delete(test_json);
    parser->destroy(parser);
}
BENCHMARK(BM_ExtsockJsonParsing);
```

---

## 🔨 빌드 시스템

### CMakeLists.txt 구조
```cmake
cmake_minimum_required(VERSION 3.14)
project(extsock_gtest)

# Google Test 설정
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50b6d95c7ee0c67a0a434b84dd0cfe.zip
)
FetchContent_MakeAvailable(googletest)

# 컴파일 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 파일 및 테스트 설정
add_subdirectory(infrastructure)
add_subdirectory(unit)
add_subdirectory(integration)

# 테스트 실행 설정
include(GoogleTest)
gtest_discover_tests(all_tests)
```

### Makefile.gtest 구조
```makefile
# Google Test 기반 빌드 시스템
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0
INCLUDES = -I. -I../.. -I$(GTEST_DIR)/include
LIBS = -lgtest -lgtest_main -lgmock -lgmock_main -lpthread

# 타겟 정의
UNIT_TESTS = $(wildcard unit/*Test.cpp)
INTEGRATION_TESTS = $(wildcard integration/*Test.cpp)
ALL_TESTS = $(UNIT_TESTS) $(INTEGRATION_TESTS)

# 빌드 타겟
all: build_tests

build_tests: $(ALL_TESTS:.cpp=.test)

%.test: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LIBS)

# 테스트 실행
test: build_tests
	@for test in $(ALL_TESTS:.cpp=.test); do \
		echo "Running $$test..."; \
		./$$test --gtest_output=xml:$$test.xml; \
	done

# 정리
clean:
	find . -name "*.test" -delete
	find . -name "*.xml" -delete

.PHONY: all build_tests test clean
```

---

## 📊 마이그레이션 진행 상황 추적

### 전체 진행률 추적
```
마이그레이션 진행률: [□□□□□□□□□□] 0% (0 완료 / 7 전체 작업)
├── Phase 1: 기초 설정 ⏳ 대기중 (0/2 작업)
├── Phase 2: 테스트 마이그레이션 ⏳ 대기중 (0/3 작업)  
└── Phase 3: 고급 기능 활용 ⏳ 대기중 (0/2 작업)
```

### 테스트 파일 마이그레이션 진행률
| 테스트 레벨 | Check 파일 수 | 마이그레이션 완료 | 진행률 |
|-------------|---------------|-------------------|--------|
| Level 0 (Infrastructure) | 3 | 0 | 0% |
| Level 1 (Pure) | 2 | 0 | 0% |
| Level 2 (Adapter) | 8+ | 0 | 0% |
| Level 3 (Integration) | 6 | 0 | 0% |
| **전체** | **19+** | **0** | **0%** |

---

## 🎯 성공 기준 및 품질 게이트

### ✅ Phase별 완료 기준

#### Phase 1 완료 기준:
- [ ] Google Test 환경이 정상적으로 설치되고 빌드됨
- [ ] Mock 인프라가 구축되어 기본 테스트가 실행됨  
- [ ] CI/CD 파이프라인에서 Google Test 빌드 확인됨

#### Phase 2 완료 기준:
- [ ] 모든 기존 테스트가 Google Test로 성공적으로 변환됨
- [ ] 테스트 커버리지가 기존과 동일하거나 향상됨
- [ ] 모든 테스트가 통과하며 false positive 없음

#### Phase 3 완료 기준:
- [ ] Parameterized Tests가 적절히 활용됨
- [ ] 성능 테스트 프레임워크가 구축됨
- [ ] 문서화 및 사용자 가이드 완비됨

### 🔍 품질 메트릭
- **테스트 실행 시간**: 기존 대비 120% 이내
- **메모리 사용량**: 기존 대비 150% 이내  
- **코드 커버리지**: 기존 대비 동일 이상
- **빌드 시간**: 기존 대비 200% 이내

---

## 🚀 다음 단계 액션 아이템

### 즉시 실행 가능한 작업:
1. **환경 설정 시작**: Google Test/Mock 라이브러리 설치
2. **디렉토리 구조 생성**: `/gtest` 폴더 및 하위 구조 생성  
3. **기본 CMakeLists.txt 작성**: 첫 번째 "Hello World" 테스트 작성

### 의사결정 필요 사항:
1. **빌드 시스템 선택**: CMake vs Makefile vs 둘 다 지원
2. **C++ 표준 버전**: C++11, C++14, C++17 중 선택
3. **Mock 전략**: 전체 Mock vs 부분 Mock
4. **성능 테스트 포함 여부**: Google Benchmark 통합 여부

---

**마지막 업데이트**: 2024-08-24  
**다음 업데이트 예정**: Phase 1 시작 후  
**문서 버전**: 1.0  
**작성자**: Claude Assistant