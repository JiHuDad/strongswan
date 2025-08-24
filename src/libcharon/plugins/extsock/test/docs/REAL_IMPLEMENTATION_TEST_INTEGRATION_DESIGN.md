# extsock Plugin - 실제 구현 코드 테스트 통합 설계 문서

## 📋 문서 개요

본 문서는 현재 Mock 기반으로 작성된 extsock plugin 테스트를 실제 구현 코드와 통합하여 진정한 단위 테스트 및 통합 테스트를 수행할 수 있도록 하는 설계 방안을 제시합니다.

**문서 버전**: 1.0  
**작성일**: 2024년  
**대상**: extsock Plugin 개발팀

---

## 🔍 현재 상황 분석

### 현재 테스트 구조의 문제점

1. **Mock 기반 테스트**: 대부분의 테스트가 실제 구현체가 아닌 Mock 객체를 테스트
2. **빌드 실패**: config.h 및 strongSwan 라이브러리 의존성 문제로 모든 테스트 빌드 실패
3. **구현체 분리**: 실제 구현 코드와 테스트 코드 간 연결 부재
4. **의존성 이슈**: strongSwan 복잡한 내부 API 의존성으로 인한 테스트 복잡도 증가

### 실제 구현 현황

```
src/libcharon/plugins/extsock/
├── common/
│   ├── extsock_errors.c/h         ✅ 구현됨
│   └── extsock_types.h            ✅ 구현됨
├── adapters/
│   ├── json/
│   │   └── extsock_json_parser.c/h ✅ 구현됨
│   ├── socket/
│   │   └── extsock_socket_adapter.c/h ✅ 구현됨
│   └── strongswan/
│       └── extsock_strongswan_adapter.c/h ✅ 구현됨
├── domain/
│   └── extsock_config_entity.c/h  ✅ 구현됨
├── usecases/
│   ├── extsock_config_usecase.c/h ✅ 구현됨
│   ├── extsock_event_usecase.c/h  ✅ 구현됨
│   └── extsock_failover_manager.c ✅ 구현됨
└── extsock_plugin.c/h             ✅ 구현됨
```

---

## 🎯 통합 목표

### 주요 목표

1. **실제 코드 테스트**: Mock이 아닌 실제 구현체 테스트
2. **빌드 환경 수정**: strongSwan 환경에서 테스트 빌드 성공
3. **의존성 관리**: 복잡한 strongSwan API 의존성을 효율적으로 처리
4. **테스트 격리**: 각 계층별 독립적인 테스트 가능
5. **CI/CD 통합**: 자동화된 테스트 실행 환경 구축

### 성공 지표

- [ ] 모든 테스트가 빌드 성공
- [ ] 실제 구현 코드의 80% 이상 커버리지
- [ ] 각 계층별 독립적 테스트 실행
- [ ] 통합 테스트에서 End-to-End 워크플로우 검증
- [ ] 메모리 누수 없는 안정적인 테스트 실행

---

## 🏗️ 아키텍처 설계

### 테스트 통합 아키텍처

```
┌─────────────────────────────────────────────────────────┐
│                Test Integration Layer                   │
├─────────────────────────────────────────────────────────┤
│  Real Implementation Tests          │  Mock Tests        │
│  ├── Unit Tests (Real)             │  ├── Unit Tests    │
│  ├── Integration Tests             │  ├── Stub Tests    │
│  └── End-to-End Tests             │  └── Isolated Tests│
├─────────────────────────────────────────────────────────┤
│                Test Infrastructure                      │
│  ├── Dependency Injection Container                    │
│  ├── strongSwan Mock/Wrapper Layer                     │
│  ├── Test Utilities and Helpers                       │
│  └── Build System Integration                         │
├─────────────────────────────────────────────────────────┤
│              Actual Implementation                      │
│  ├── Plugin Layer                                     │
│  ├── Usecase Layer                                    │
│  ├── Adapter Layer                                    │
│  ├── Domain Layer                                     │
│  └── Common Layer                                     │
└─────────────────────────────────────────────────────────┘
```

### 의존성 관리 전략

#### 1. Layered Testing Approach

```c
// Level 1: Pure Unit Tests (No strongSwan dependencies)
// - Common Layer (errors, types, utilities)
// - Domain Layer (business entities)

// Level 2: Adapter Tests (Mock strongSwan APIs)
// - JSON Parser (with mocked strongSwan types)
// - Socket Adapter (with system call mocks)

// Level 3: Integration Tests (Real strongSwan integration)
// - Full workflow with actual strongSwan libraries
// - Real IKE/IPsec configuration
```

#### 2. Dependency Injection Container

```c
typedef struct test_container {
    // Real implementations
    extsock_json_parser_t *json_parser;
    extsock_config_usecase_t *config_usecase;
    extsock_event_usecase_t *event_usecase;
    
    // Mock implementations
    mock_strongswan_adapter_t *mock_strongswan;
    mock_socket_adapter_t *mock_socket;
    
    // Test utilities
    test_data_factory_t *data_factory;
    memory_tracker_t *memory_tracker;
} test_container_t;
```

---

## 📁 디렉토리 구조 재설계

### 새로운 테스트 디렉토리 구조

```
test/
├── infrastructure/           # 테스트 인프라
│   ├── test_container.c/h   # DI 컨테이너
│   ├── strongswan_mocks.c/h # strongSwan API 목킹
│   ├── test_utilities.c/h   # 공통 테스트 유틸리티
│   └── build_config.h       # 빌드 설정
├── unit_real/               # 실제 구현 단위 테스트
│   ├── common/
│   │   ├── test_extsock_errors_real.c
│   │   └── test_extsock_types_real.c
│   ├── domain/
│   │   └── test_config_entity_real.c
│   ├── adapters/
│   │   ├── test_json_parser_real.c
│   │   ├── test_socket_adapter_real.c
│   │   └── test_strongswan_adapter_real.c
│   ├── usecases/
│   │   ├── test_config_usecase_real.c
│   │   ├── test_event_usecase_real.c
│   │   └── test_failover_manager_real.c
│   └── plugin/
│       └── test_plugin_lifecycle_real.c
├── integration_real/        # 실제 구현 통합 테스트
│   ├── test_json_to_config_flow.c
│   ├── test_config_to_strongswan_flow.c
│   └── test_end_to_end_workflow.c
├── mock/                    # Mock 기반 테스트 (현재 유지)
│   └── [existing mock tests]
├── data/                    # 테스트 데이터
│   ├── json_configs/
│   ├── certificates/
│   └── test_cases/
└── scripts/                 # 테스트 스크립트
    ├── build_real_tests.sh
    ├── run_real_tests.sh
    └── run_all_tests.sh
```

---

## 🔧 빌드 시스템 설계

### 빌드 환경 구분

#### 1. 순수 단위 테스트 (Level 1)

```makefile
# Makefile.unit_pure
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -O0 --coverage
CFLAGS += -DUNIT_TEST_PURE
INCLUDES = -I.. -I../common
LIBS = -lcheck -lsubunit -lm -lrt -lpthread -lcjson

# 순수 단위 테스트 (strongSwan 의존성 없음)
PURE_TESTS = test_extsock_errors_real test_extsock_types_real

test_extsock_errors_real: unit_real/common/test_extsock_errors_real.c
	$(CC) $(CFLAGS) $(INCLUDES) $< ../common/extsock_errors.c -o $@ $(LIBS)
```

#### 2. 어댑터 테스트 (Level 2)

```makefile
# Makefile.unit_adapter  
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -O0 --coverage
CFLAGS += -DUNIT_TEST_ADAPTER
INCLUDES = -I.. -I../common -Iinfrastructure
LIBS = -lcheck -lsubunit -lm -lrt -lpthread -lcjson

# strongSwan 타입을 모킹
ADAPTER_TESTS = test_json_parser_real test_socket_adapter_real

test_json_parser_real: unit_real/adapters/test_json_parser_real.c
	$(CC) $(CFLAGS) $(INCLUDES) $< \
		../adapters/json/extsock_json_parser.c \
		../common/extsock_errors.c \
		infrastructure/strongswan_mocks.c \
		-o $@ $(LIBS)
```

#### 3. 통합 테스트 (Level 3)

```makefile
# Makefile.integration
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -O0 --coverage
CFLAGS += -DINTEGRATION_TEST
INCLUDES = -I.. -I../../../../../../src/libstrongswan -I../../../../../../src/libcharon
LIBS = -lcheck -lsubunit -lm -lrt -lpthread -lcjson
LIBS += -L../../../../../../src/libstrongswan/.libs -lstrongswan
LIBS += -L../../../../../../src/libcharon/.libs -lcharon

export LD_LIBRARY_PATH=../../../../../../src/libstrongswan/.libs:../../../../../../src/libcharon/.libs:$$LD_LIBRARY_PATH

INTEGRATION_TESTS = test_end_to_end_workflow

test_end_to_end_workflow: integration_real/test_end_to_end_workflow.c
	$(CC) $(CFLAGS) $(INCLUDES) $< \
		../extsock_plugin.c \
		../adapters/json/extsock_json_parser.c \
		../adapters/strongswan/extsock_strongswan_adapter.c \
		../common/extsock_errors.c \
		-o $@ $(LIBS)
```

---

## 🧪 테스트 구현 전략

### 1. Common Layer 실제 테스트

**파일**: `unit_real/common/test_extsock_errors_real.c`

```c
/*
 * 실제 extsock_errors.c 구현 테스트
 */
#include <check.h>
#include "../../common/extsock_errors.h"

START_TEST(test_real_error_create_with_actual_implementation)
{
    // Given: 실제 에러 코드
    extsock_error_t code = EXTSOCK_ERROR_JSON_PARSE;
    const char *message = "Real JSON parsing error";
    
    // When: 실제 구현 함수 호출
    extsock_error_info_t *error = extsock_error_create(code, message);
    
    // Then: 실제 구현 결과 검증
    ck_assert_ptr_nonnull(error);
    ck_assert_int_eq(error->code, code);
    ck_assert_str_eq(error->message, message);
    ck_assert(error->timestamp > 0); // 실제 timestamp 설정 확인
    ck_assert(error->thread_id > 0); // 실제 thread ID 확인
    
    // Cleanup
    extsock_error_destroy(error);
}
END_TEST

// strongSwan 의존성 없는 순수 테스트
START_TEST(test_real_error_string_conversion)
{
    // When & Then: 실제 구현된 에러 문자열 변환 테스트
    ck_assert_str_eq(extsock_error_to_string(EXTSOCK_SUCCESS), "Success");
    ck_assert_str_eq(extsock_error_to_string(EXTSOCK_ERROR_JSON_PARSE), "JSON Parse Error");
    ck_assert_str_eq(extsock_error_to_string(EXTSOCK_ERROR_CONFIG_INVALID), "Invalid Configuration");
}
END_TEST
```

### 2. JSON Parser 실제 테스트

**파일**: `unit_real/adapters/test_json_parser_real.c`

```c
/*
 * 실제 extsock_json_parser.c 구현 테스트
 */
#include <check.h>
#include "../../adapters/json/extsock_json_parser.h"
#include "../infrastructure/strongswan_mocks.h"

static extsock_json_parser_t *parser;

void setup_real_json_parser_test(void)
{
    // strongSwan 환경 모킹 초기화
    init_strongswan_mocks();
    
    // 실제 JSON parser 생성
    parser = extsock_json_parser_create();
}

void teardown_real_json_parser_test(void)
{
    if (parser) {
        parser->destroy(parser);
    }
    cleanup_strongswan_mocks();
}

START_TEST(test_real_json_parser_parse_ike_config)
{
    // Given: 실제 IKE 설정 JSON
    cJSON *ike_json = cJSON_CreateObject();
    cJSON *local_addrs = cJSON_CreateArray();
    cJSON_AddItemToArray(local_addrs, cJSON_CreateString("192.168.1.10"));
    cJSON_AddItemToObject(ike_json, "local_addrs", local_addrs);
    
    // When: 실제 구현 함수 호출
    ike_cfg_t *ike_cfg = parser->parse_ike_config(parser, ike_json);
    
    // Then: 실제 strongSwan ike_cfg_t 객체 생성 확인
    ck_assert_ptr_nonnull(ike_cfg);
    
    // Mock을 통한 내부 호출 검증
    ck_assert(mock_ike_cfg_create_called());
    ck_assert_str_eq(mock_get_last_local_addr(), "192.168.1.10");
    
    // Cleanup
    cJSON_Delete(ike_json);
    if (ike_cfg) {
        ike_cfg->destroy(ike_cfg);
    }
}
END_TEST
```

### 3. strongSwan Mock Infrastructure

**파일**: `infrastructure/strongswan_mocks.h`

```c
/*
 * strongSwan API 모킹 인프라
 */
#ifndef STRONGSWAN_MOCKS_H
#define STRONGSWAN_MOCKS_H

#include <stdbool.h>
#include <collections/linked_list.h>
#include <config/ike_cfg.h>
#include <config/peer_cfg.h>

// Mock 초기화/정리
void init_strongswan_mocks(void);
void cleanup_strongswan_mocks(void);

// Mock 함수들
ike_cfg_t *mock_ike_cfg_create(char *name, bool encap, 
                              certs_t certs, bool force_encap);
peer_cfg_t *mock_peer_cfg_create(char *name, ike_cfg_t *ike_cfg,
                                cert_policy_t cert_policy, 
                                unique_policy_t unique, 
                                u_int keyingtries, u_int rekeytime);

// Mock 호출 검증
bool mock_ike_cfg_create_called(void);
bool mock_peer_cfg_create_called(void);
const char *mock_get_last_local_addr(void);
const char *mock_get_last_remote_addr(void);

// Mock 상태 리셋
void reset_mock_state(void);

#endif
```

**파일**: `infrastructure/strongswan_mocks.c`

```c
/*
 * strongSwan API 모킹 구현
 */
#include "strongswan_mocks.h"
#include <stdlib.h>
#include <string.h>

// Mock 상태 추적
static bool ike_cfg_create_called = false;
static bool peer_cfg_create_called = false;
static char *last_local_addr = NULL;
static char *last_remote_addr = NULL;

// Mock ike_cfg_t 구조체
typedef struct mock_ike_cfg {
    ike_cfg_t public;
    char *name;
} mock_ike_cfg_t;

static void mock_ike_cfg_destroy(ike_cfg_t *this)
{
    mock_ike_cfg_t *mock = (mock_ike_cfg_t*)this;
    free(mock->name);
    free(mock);
}

ike_cfg_t *mock_ike_cfg_create(char *name, bool encap, 
                              certs_t certs, bool force_encap)
{
    ike_cfg_create_called = true;
    
    mock_ike_cfg_t *mock = malloc(sizeof(mock_ike_cfg_t));
    mock->public.destroy = mock_ike_cfg_destroy;
    mock->name = strdup(name);
    
    return &mock->public;
}

void init_strongswan_mocks(void)
{
    reset_mock_state();
    
    // 실제 strongSwan 함수를 Mock 함수로 교체
    // (매크로나 링커 기법 사용)
    #define ike_cfg_create mock_ike_cfg_create
    #define peer_cfg_create mock_peer_cfg_create
}

bool mock_ike_cfg_create_called(void)
{
    return ike_cfg_create_called;
}

void reset_mock_state(void)
{
    ike_cfg_create_called = false;
    peer_cfg_create_called = false;
    
    if (last_local_addr) {
        free(last_local_addr);
        last_local_addr = NULL;
    }
    if (last_remote_addr) {
        free(last_remote_addr);  
        last_remote_addr = NULL;
    }
}
```

### 4. 통합 테스트

**파일**: `integration_real/test_end_to_end_workflow.c`

```c
/*
 * End-to-End 워크플로우 실제 구현 테스트
 */
#include <check.h>
#include "../../extsock_plugin.h"
#include "../infrastructure/test_container.h"

static test_container_t *container;

void setup_integration_test(void)
{
    container = test_container_create();
    
    // 실제 strongSwan 라이브러리 초기화
    library_init(NULL, "extsock-test");
    lib->plugins->load(lib->plugins, PLUGINS);
    
    // 실제 extsock plugin 초기화
    plugin_t *plugin = extsock_plugin_create();
    ck_assert_ptr_nonnull(plugin);
}

void teardown_integration_test(void)
{
    if (container) {
        container->destroy(container);
    }
    
    library_deinit();
}

START_TEST(test_real_end_to_end_json_to_ipsec_connection)
{
    // Given: 실제 IPsec 설정 JSON
    const char *config_json = 
        "{"
        "  \"connection_name\": \"integration_test\","
        "  \"ike\": {"
        "    \"local_addrs\": [\"192.168.1.10\"],"
        "    \"remote_addrs\": [\"203.0.113.5\"],"
        "    \"version\": 2,"
        "    \"proposals\": [\"aes256-sha256-modp2048\"]"
        "  },"
        "  \"local\": {"
        "    \"auth\": \"psk\","
        "    \"id\": \"client@test.com\","
        "    \"secret\": \"test_secret\""
        "  },"
        "  \"remote\": {"
        "    \"auth\": \"psk\","
        "    \"id\": \"server@test.com\""
        "  },"
        "  \"children\": ["
        "    {"
        "      \"name\": \"child1\","
        "      \"local_ts\": [\"10.0.0.0/24\"],"
        "      \"remote_ts\": [\"10.0.1.0/24\"],"
        "      \"esp_proposals\": [\"aes128gcm16\"]"
        "    }"
        "  ]"
        "}";
    
    // When: 실제 워크플로우 실행
    extsock_config_usecase_t *config_usecase = container->get_config_usecase(container);
    extsock_error_t result = config_usecase->apply_json_config(config_usecase, config_json);
    
    // Then: 전체 워크플로우 성공
    ck_assert_int_eq(result, EXTSOCK_SUCCESS);
    
    // strongSwan backend에서 설정 확인
    peer_cfg_t *peer_cfg = charon->backends->get_peer_cfg_by_name(
        charon->backends, "integration_test");
    ck_assert_ptr_nonnull(peer_cfg);
    
    // IKE 설정 검증
    ike_cfg_t *ike_cfg = peer_cfg->get_ike_cfg(peer_cfg);
    ck_assert_ptr_nonnull(ike_cfg);
    
    // Child SA 설정 검증
    enumerator_t *children = peer_cfg->create_child_cfg_enumerator(peer_cfg);
    child_cfg_t *child_cfg;
    bool found_child = children->enumerate(children, &child_cfg);
    ck_assert(found_child);
    ck_assert_str_eq(child_cfg->get_name(child_cfg), "child1");
    
    children->destroy(children);
    peer_cfg->destroy(peer_cfg);
}
END_TEST
```

---

## 📋 단계별 구현 계획

### Phase 1: 테스트 인프라 구축 (1-2주)

#### Week 1: 기본 인프라
- [ ] strongSwan Mock 인프라 구축
- [ ] 테스트 컨테이너 DI 시스템 구축
- [ ] 빌드 시스템 분리 (Pure/Adapter/Integration)
- [ ] 테스트 유틸리티 함수 개발

#### Week 2: Common Layer 테스트
- [ ] `test_extsock_errors_real.c` 구현
- [ ] `test_extsock_types_real.c` 구현
- [ ] 메모리 추적 시스템 구축
- [ ] CI/CD 파이프라인 기초 설정

### Phase 2: 어댑터 계층 테스트 (2-3주)

#### Week 3-4: JSON Parser 실제 테스트
- [ ] `test_json_parser_real.c` 구현
- [ ] strongSwan 타입 모킹 완성
- [ ] IKE 설정 파싱 실제 테스트
- [ ] 인증 설정 파싱 실제 테스트

#### Week 5: Socket/strongSwan Adapter 테스트
- [ ] `test_socket_adapter_real.c` 구현
- [ ] `test_strongswan_adapter_real.c` 구현
- [ ] 시스템 콜 모킹
- [ ] 네트워크 시뮬레이션

### Phase 3: 비즈니스 로직 테스트 (2주)

#### Week 6-7: Usecase Layer 테스트
- [ ] `test_config_usecase_real.c` 구현
- [ ] `test_event_usecase_real.c` 구현
- [ ] `test_failover_manager_real.c` 구현
- [ ] 도메인 엔터티 실제 테스트

### Phase 4: 통합 테스트 (2주)

#### Week 8-9: End-to-End 테스트
- [ ] `test_end_to_end_workflow.c` 구현
- [ ] 실제 strongSwan 환경에서 통합 테스트
- [ ] 성능 테스트 및 메모리 누수 검증
- [ ] 문서화 완성

---

## 📝 상세 Task List

### Task 1: strongSwan Mock 인프라 구축

**우선순위**: 🔴 HIGH  
**예상 기간**: 3-5일  

**하위 작업**:
1. `infrastructure/strongswan_mocks.h` 인터페이스 설계
2. `infrastructure/strongswan_mocks.c` 구현
   - `ike_cfg_t`, `peer_cfg_t`, `child_cfg_t` 모킹
   - `linked_list_t`, `enumerator_t` 모킹
   - `identification_t`, `traffic_selector_t` 모킹
3. Mock 상태 추적 및 검증 시스템
4. 초기화/정리 함수 구현
5. 단위 테스트로 Mock 시스템 검증

**완료 조건**:
- [ ] Mock 객체 생성/소멸 정상 동작
- [ ] Mock 호출 추적 정상 동작
- [ ] 메모리 누수 없음 (Valgrind 검증)

### Task 2: 테스트 컨테이너 DI 시스템

**우선순위**: 🔴 HIGH  
**예상 기간**: 2-3일

**하위 작업**:
1. `infrastructure/test_container.h` 인터페이스 설계
2. `infrastructure/test_container.c` 구현
   - 실제 객체 생성 관리
   - Mock 객체 생성 관리
   - 의존성 주입 시스템
3. 테스트 데이터 팩토리 구현
4. 설정별 컨테이너 생성 (Unit/Integration)

**완료 조건**:
- [ ] 실제 객체 생성/주입 정상 동작
- [ ] Mock 객체 생성/주입 정상 동작
- [ ] 메모리 관리 정상 동작

### Task 3: 빌드 시스템 분리

**우선순위**: 🟡 MEDIUM  
**예상 기간**: 2-3일

**하위 작업**:
1. `Makefile.unit_pure` 작성 (strongSwan 의존성 없음)
2. `Makefile.unit_adapter` 작성 (Mock strongSwan)  
3. `Makefile.integration` 작성 (실제 strongSwan)
4. `scripts/build_real_tests.sh` 스크립트 작성
5. `scripts/run_real_tests.sh` 스크립트 작성

**완료 조건**:
- [ ] 각 레벨별 독립적 빌드 성공
- [ ] 스크립트를 통한 자동화 빌드
- [ ] CI 환경에서 빌드 성공

### Task 4: Common Layer 실제 테스트

**우선순위**: 🔴 HIGH  
**예상 기간**: 3-4일

**하위 작업**:
1. `unit_real/common/test_extsock_errors_real.c` 구현
   - 실제 `extsock_error_create` 테스트
   - 실제 `extsock_error_destroy` 테스트  
   - 에러 문자열 변환 테스트
2. `unit_real/common/test_extsock_types_real.c` 구현
   - 타입 정의 검증
   - 열거형 값 검증
3. 메모리 추적 시스템 통합

**완료 조건**:
- [ ] 모든 Common Layer 함수 테스트
- [ ] 100% 코드 커버리지
- [ ] 메모리 누수 없음

### Task 5: JSON Parser 실제 테스트

**우선순위**: 🔴 HIGH  
**예상 기간**: 5-7일

**하위 작업**:
1. `unit_real/adapters/test_json_parser_real.c` 구현
   - 실제 `extsock_json_parser_create` 테스트
   - 실제 `parse_ike_config` 테스트
   - 실제 `parse_auth_config` 테스트
   - 실제 `parse_proposals` 테스트
   - 실제 `parse_traffic_selectors` 테스트
2. JSON 에러 케이스 테스트
3. 복잡한 중첩 JSON 테스트

**완료 조건**:
- [ ] 모든 JSON Parser 메서드 테스트
- [ ] 에러 케이스 100% 커버
- [ ] 실제 strongSwan 객체 생성 확인

### Task 6: Usecase Layer 실제 테스트

**우선순위**: 🟡 MEDIUM  
**예상 기간**: 5-7일

**하위 작업**:
1. `unit_real/usecases/test_config_usecase_real.c` 구현
   - 실제 `apply_json_config` 테스트
   - 실제 `remove_config` 테스트
   - 실제 `start_dpd` 테스트
2. `unit_real/usecases/test_event_usecase_real.c` 구현
3. `unit_real/usecases/test_failover_manager_real.c` 구현
4. 비즈니스 로직 검증

**완료 조건**:
- [ ] 모든 Usecase 메서드 테스트
- [ ] 비즈니스 규칙 검증
- [ ] 에러 처리 검증

### Task 7: End-to-End 통합 테스트

**우선순위**: 🟡 MEDIUM  
**예상 기간**: 7-10일

**하위 작업**:
1. `integration_real/test_end_to_end_workflow.c` 구현
   - JSON → strongSwan 설정 전체 플로우
   - 실제 strongSwan backend 연동
   - IKE SA/Child SA 생성 검증
2. 성능 테스트 추가
3. 동시성 테스트 추가
4. 장기 실행 안정성 테스트

**완료 조건**:
- [ ] 전체 워크플로우 성공
- [ ] strongSwan backend 설정 확인
- [ ] 성능 기준 만족
- [ ] 메모리/리소스 안정성 확인

### Task 8: CI/CD 파이프라인 구축

**우선순위**: 🟢 LOW  
**예상 기간**: 3-5일

**하위 작업**:
1. GitHub Actions 설정
2. 자동화 테스트 실행
3. 커버리지 리포팅
4. 테스트 결과 알림
5. 문서 자동 생성

**완료 조건**:
- [ ] PR 생성 시 자동 테스트
- [ ] 커버리지 80% 이상
- [ ] 테스트 실패 시 알림
- [ ] 문서 자동 업데이트

---

## 🎯 성공 기준 및 검증 방법

### 정량적 성공 기준

1. **빌드 성공률**: 100% (모든 테스트 빌드 성공)
2. **테스트 통과율**: 95% 이상
3. **코드 커버리지**: 80% 이상
4. **메모리 안전성**: Valgrind 검증 통과
5. **성능**: 전체 테스트 실행 시간 5분 이내

### 정성적 성공 기준

1. **유지보수성**: 새로운 기능 추가 시 테스트 추가 용이
2. **가독성**: 테스트 코드가 문서 역할 수행
3. **안정성**: CI/CD 환경에서 일관된 결과
4. **확장성**: 다른 strongSwan 플러그인에 적용 가능

### 검증 방법

```bash
# 1. 빌드 검증
make -f Makefile.unit_pure all
make -f Makefile.unit_adapter all  
make -f Makefile.integration all

# 2. 테스트 실행 검증
./scripts/run_real_tests.sh --coverage

# 3. 메모리 검증  
./scripts/run_real_tests.sh --valgrind

# 4. 성능 검증
./scripts/run_real_tests.sh --benchmark
```

---

## 🚀 기대 효과

### 개발 품질 향상

1. **버그 조기 발견**: 실제 코드 테스트로 런타임 버그 사전 차단
2. **리팩토링 안전성**: 변경 시 기존 기능 보장
3. **문서화 효과**: 테스트가 코드 사용법 명시

### 개발 효율성 증대

1. **빠른 피드백**: 코드 변경 즉시 영향도 확인
2. **자동화**: 수동 테스트 부담 감소
3. **신뢰성**: 배포 전 품질 보장

### 협업 개선

1. **코드 이해도**: 새로운 팀원의 빠른 적응
2. **변경 영향도**: 코드 변경 시 영향 범위 명확화
3. **품질 표준**: 일관된 코드 품질 유지

---

## 📞 결론 및 다음 단계

본 설계 문서에 따라 extsock plugin의 테스트를 실제 구현 코드와 통합하면, Mock 기반의 허상 테스트에서 벗어나 진정한 품질 보장이 가능합니다.

### 즉시 시작 가능한 작업

1. **Task 1**: strongSwan Mock 인프라 구축
2. **Task 2**: 테스트 컨테이너 DI 시스템  
3. **Task 3**: 빌드 시스템 분리

### 성공을 위한 핵심 요소

1. **단계별 접근**: 한 번에 모든 것을 하지 말고 단계적 진행
2. **지속적 검증**: 각 단계에서 품질 검증
3. **문서 업데이트**: 진행상황에 따른 문서 지속 갱신

이 설계안을 바탕으로 실제 구현 코드를 검증하는 견고한 테스트 시스템을 구축할 수 있을 것입니다.

---

**문서 끝**