# strongSwan extsock Plugin - TDD 개발 가이드

이 문서는 strongSwan extsock plugin에서 Test-Driven Development(TDD) 방법론을 사용하여 코드를 개발하고 수정하는 방법에 대한 완전한 가이드입니다. 이 문서만 보고도 새로 개발하거나 수정하는 코드에 대해 완벽한 테스트를 작성할 수 있습니다.

## 📋 목차

- [1. TDD 개념과 원칙](#1-tdd-개념과-원칙)
- [2. extsock 플러그인 아키텍처 이해](#2-extsock-플러그인-아키텍처-이해)
- [3. 테스트 환경 설정](#3-테스트-환경-설정)
- [4. TDD 개발 프로세스](#4-tdd-개발-프로세스)
- [5. 계층별 테스트 작성 가이드](#5-계층별-테스트-작성-가이드)
- [6. 테스트 패턴과 베스트 프랙티스](#6-테스트-패턴과-베스트-프랙티스)
- [7. 실전 예제](#7-실전-예제)
- [8. 문제해결 및 팁](#8-문제해결-및-팁)

---

## 1. TDD 개념과 원칙

### 1.1 TDD 사이클 (Red-Green-Refactor)

```
🔴 RED    → 실패하는 테스트 작성
🟢 GREEN  → 테스트를 통과시키는 최소한의 코드 작성
🔵 REFACTOR → 코드 개선 (테스트 통과 유지)
```

### 1.2 TDD 원칙

1. **실패하는 테스트 먼저 작성**: 구현 전에 테스트를 작성
2. **최소한의 구현**: 테스트가 통과할 정도만 구현
3. **리팩토링**: 테스트 통과를 유지하면서 코드 개선
4. **빠른 피드백**: 작은 단위로 자주 실행
5. **문서화 효과**: 테스트가 곧 요구사항과 사용법

---

## 2. extsock 플러그인 아키텍처 이해

### 2.1 Clean Architecture 계층구조

```
┌─────────────────────────────────────┐
│        Plugin Layer                 │ ← 플러그인 생명주기 관리
├─────────────────────────────────────┤
│        Usecase Layer               │ ← 비즈니스 로직
│  - Config Usecase                  │
│  - Event Usecase                   │
│  - Failover Manager                │
├─────────────────────────────────────┤
│        Adapter Layer               │ ← 외부 인터페이스
│  - JSON Parser                     │
│  - Socket Adapter                  │
│  - strongSwan Adapter              │
├─────────────────────────────────────┤
│        Domain Layer                │ ← 핵심 비즈니스 엔터티
│  - Config Entity                   │
├─────────────────────────────────────┤
│        Common Layer                │ ← 공통 유틸리티
│  - Error Handling                  │
│  - Types and Constants             │
└─────────────────────────────────────┘
```

### 2.2 디렉토리 구조

```
src/libcharon/plugins/extsock/
├── common/           # 공통 유틸리티
├── domain/           # 도메인 엔터티
├── usecases/         # 비즈니스 로직
├── adapters/         # 외부 인터페이스 어댑터
├── interfaces/       # 인터페이스 정의
├── test/             # 테스트 코드
│   ├── unit/         # 단위 테스트
│   ├── integration/  # 통합 테스트
│   └── docs/         # 테스트 문서
└── extsock_plugin.c  # 플러그인 진입점
```

---

## 3. 테스트 환경 설정

### 3.1 필수 도구 설치

```bash
# Ubuntu/Debian
sudo apt-get install libcheck-dev libcjson-dev gcovr valgrind

# 또는 RHEL/CentOS
sudo yum install check-devel libcjson-devel gcovr valgrind
```

### 3.2 빌드 환경 확인

```bash
# strongSwan 루트 디렉토리에서
./configure
make

# 테스트 디렉토리에서
cd src/libcharon/plugins/extsock/test
chmod +x *.sh
```

### 3.3 테스트 실행 확인

```bash
# 빠른 테스트
./quick_test.sh

# 전체 테스트
./run_working_tests.sh
```

---

## 4. TDD 개발 프로세스

### 4.1 기본 TDD 워크플로우

```bash
# 1단계: RED - 실패하는 테스트 작성
echo "새 기능에 대한 실패 테스트 작성"

# 2단계: GREEN - 최소한의 구현으로 테스트 통과
echo "테스트를 통과시키는 최소 코드 작성"

# 3단계: REFACTOR - 코드 개선
echo "테스트 통과 유지하면서 코드 개선"

# 4단계: 반복
echo "다음 기능으로 사이클 반복"
```

### 4.2 실무 TDD 프로세스 (extsock 기준)

```bash
# Step 1: 요구사항 분석
# - 어떤 기능을 구현할 것인가?
# - 어떤 계층에 속하는가?
# - 의존성은 무엇인가?

# Step 2: 테스트 작성 (RED)
cd src/libcharon/plugins/extsock/test/unit
vim test_new_feature.c  # 실패하는 테스트 작성

# Step 3: 테스트 실행 (실패 확인)
./run_individual_test.sh new_feature

# Step 4: 최소 구현 (GREEN)
cd ../
vim new_feature.c       # 테스트가 통과할 최소 코드

# Step 5: 테스트 실행 (성공 확인)
cd test
./run_individual_test.sh new_feature

# Step 6: 리팩토링 (REFACTOR)
# 코드 개선 후 테스트 재실행

# Step 7: 통합 테스트 실행
./run_working_tests.sh
```

---

## 5. 계층별 테스트 작성 가이드

### 5.1 Common Layer 테스트

**목적**: 공통 유틸리티와 에러 처리 테스트

**테스트 파일**: `test/unit/test_[utility_name].c`

**예제**: Error Handling 테스트

```c
/*
 * test_error_handling.c - 에러 처리 테스트 예제
 */
#include <check.h>
#include "../common/extsock_errors.h"

// Setup/Teardown
void setup_error_test(void) {
    // 테스트 전 초기화
}

void teardown_error_test(void) {
    // 테스트 후 정리
}

// RED: 실패하는 테스트 먼저 작성
START_TEST(test_error_creation_should_return_valid_error)
{
    // Given: 에러 코드와 메시지
    extsock_error_code_t code = EXTSOCK_ERROR_JSON_PARSE;
    const char *message = "JSON parsing failed";
    
    // When: 에러 생성
    extsock_error_info_t *error = extsock_error_create(code, message);
    
    // Then: 유효한 에러 객체 반환
    ck_assert_ptr_nonnull(error);
    ck_assert_int_eq(error->code, code);
    ck_assert_str_eq(error->message, message);
    
    // Cleanup
    extsock_error_destroy(error);
}
END_TEST

// 테스트 스위트 생성
Suite *error_handling_suite(void) {
    Suite *s = suite_create("Error Handling Tests");
    TCase *tc_core = tcase_create("Core Error Tests");
    
    // Setup/Teardown 설정
    tcase_add_checked_fixture(tc_core, setup_error_test, teardown_error_test);
    
    // 테스트 추가
    tcase_add_test(tc_core, test_error_creation_should_return_valid_error);
    
    suite_add_tcase(s, tc_core);
    return s;
}
```

### 5.2 Domain Layer 테스트

**목적**: 핵심 비즈니스 엔터티 로직 테스트

**테스트 파일**: `test/unit/test_[entity_name]_entity.c`

**예제**: Config Entity 테스트

```c
/*
 * test_config_entity.c - 설정 엔터티 테스트
 */
#include <check.h>
#include "../domain/extsock_config_entity.h"

// RED: 실패 테스트
START_TEST(test_config_entity_creation_should_initialize_properly)
{
    // Given: 연결 설정 정보
    const char *name = "test_connection";
    const char *local_ip = "192.168.1.10";
    const char *remote_ip = "203.0.113.5";
    
    // When: 엔터티 생성
    config_entity_t *entity = config_entity_create(name, local_ip, remote_ip);
    
    // Then: 올바르게 초기화됨
    ck_assert_ptr_nonnull(entity);
    ck_assert_str_eq(config_entity_get_name(entity), name);
    ck_assert_str_eq(config_entity_get_local_ip(entity), local_ip);
    ck_assert_str_eq(config_entity_get_remote_ip(entity), remote_ip);
    
    // Cleanup
    config_entity_destroy(entity);
}
END_TEST

// GREEN: 최소 구현 가이드
/*
 * domain/extsock_config_entity.c 에 다음과 같은 최소 구현:
 * 
 * typedef struct {
 *     char *name;
 *     char *local_ip;
 *     char *remote_ip;
 * } config_entity_t;
 * 
 * config_entity_t *config_entity_create(const char *name, ...) {
 *     // 최소한의 구현
 * }
 */
```

### 5.3 Adapter Layer 테스트

**목적**: 외부 시스템과의 인터페이스 테스트

**테스트 파일**: `test/unit/test_[adapter_name]_adapter.c`

**예제**: JSON Parser Adapter 테스트

```c
/*
 * test_json_parser_adapter.c - JSON 파서 어댑터 테스트
 */
#include <check.h>
#include <cjson/cJSON.h>
#include "../adapters/json/extsock_json_parser.h"

static json_parser_t *parser;

void setup_json_parser_test(void) {
    parser = json_parser_create();
}

void teardown_json_parser_test(void) {
    if (parser) {
        parser->destroy(parser);
    }
}

// RED: JSON 파싱 실패 테스트
START_TEST(test_json_parser_should_parse_ipsec_config)
{
    // Given: IPsec 설정 JSON
    const char *json_str = "{"
        "\"connection_name\": \"tunnel1\","
        "\"local_ip\": \"192.168.1.10\","
        "\"remote_ip\": \"203.0.113.5\","
        "\"auth_method\": \"psk\""
    "}";
    
    // When: JSON 파싱
    ipsec_config_t *config = parser->parse_ipsec_config(parser, json_str);
    
    // Then: 올바르게 파싱됨
    ck_assert_ptr_nonnull(config);
    ck_assert_str_eq(config->connection_name, "tunnel1");
    ck_assert_str_eq(config->local_ip, "192.168.1.10");
    ck_assert_str_eq(config->remote_ip, "203.0.113.5");
    ck_assert_str_eq(config->auth_method, "psk");
    
    // Cleanup
    ipsec_config_destroy(config);
}
END_TEST

// 에러 케이스 테스트
START_TEST(test_json_parser_should_handle_invalid_json)
{
    // Given: 잘못된 JSON
    const char *invalid_json = "{invalid json";
    
    // When: 파싱 시도
    ipsec_config_t *config = parser->parse_ipsec_config(parser, invalid_json);
    
    // Then: NULL 반환되고 에러 설정
    ck_assert_ptr_null(config);
    ck_assert_ptr_nonnull(parser->get_last_error(parser));
}
END_TEST
```

### 5.4 Usecase Layer 테스트

**목적**: 비즈니스 로직과 워크플로우 테스트

**테스트 파일**: `test/unit/test_[usecase_name]_usecase.c`

**예제**: Config Usecase 테스트

```c
/*
 * test_config_usecase.c - 설정 유스케이스 테스트
 */
#include <check.h>
#include "../usecases/extsock_config_usecase.h"
#include "../interfaces/extsock_config_repository.h"

// Mock Repository (의존성 모킹)
static config_repository_t *mock_repo;
static config_usecase_t *usecase;

void setup_config_usecase_test(void) {
    mock_repo = create_mock_config_repository();
    usecase = config_usecase_create(mock_repo);
}

void teardown_config_usecase_test(void) {
    if (usecase) {
        usecase->destroy(usecase);
    }
    if (mock_repo) {
        mock_repo->destroy(mock_repo);
    }
}

// RED: 설정 저장 실패 테스트
START_TEST(test_config_usecase_should_save_valid_config)
{
    // Given: 유효한 설정
    config_data_t config = {
        .name = "test_conn",
        .local_ip = "192.168.1.10",
        .remote_ip = "203.0.113.5"
    };
    
    // When: 설정 저장
    bool result = usecase->save_config(usecase, &config);
    
    // Then: 성공적으로 저장됨
    ck_assert(result);
    
    // 리포지토리에 저장되었는지 확인
    config_data_t *saved = mock_repo->find_by_name(mock_repo, "test_conn");
    ck_assert_ptr_nonnull(saved);
    ck_assert_str_eq(saved->name, config.name);
}
END_TEST

// 비즈니스 규칙 테스트
START_TEST(test_config_usecase_should_reject_duplicate_names)
{
    // Given: 이미 존재하는 설정명
    config_data_t existing = { .name = "existing_conn" };
    mock_repo->save(mock_repo, &existing);
    
    config_data_t duplicate = { .name = "existing_conn" };
    
    // When: 동일한 이름으로 저장 시도
    bool result = usecase->save_config(usecase, &duplicate);
    
    // Then: 실패해야 함
    ck_assert(!result);
    
    // 에러 메시지 확인
    error_t *error = usecase->get_last_error(usecase);
    ck_assert_ptr_nonnull(error);
    ck_assert_int_eq(error->code, CONFIG_ERROR_DUPLICATE_NAME);
}
END_TEST
```

### 5.5 Plugin Layer 테스트

**목적**: 플러그인 생명주기와 인터페이스 테스트

**테스트 파일**: `test/unit/test_plugin_lifecycle.c`

```c
/*
 * test_plugin_lifecycle.c - 플러그인 생명주기 테스트
 */
#include <check.h>
#include "../extsock_plugin.h"

static plugin_t *plugin;

void setup_plugin_test(void) {
    plugin = NULL;
}

void teardown_plugin_test(void) {
    if (plugin) {
        plugin->destroy(plugin);
    }
}

// RED: 플러그인 생성 실패 테스트
START_TEST(test_plugin_should_initialize_successfully)
{
    // When: 플러그인 생성
    plugin = extsock_plugin_create();
    
    // Then: 유효한 플러그인 객체 반환
    ck_assert_ptr_nonnull(plugin);
    ck_assert_ptr_nonnull(plugin->get_name);
    ck_assert_ptr_nonnull(plugin->destroy);
    
    // 플러그인 이름 확인
    ck_assert_str_eq(plugin->get_name(plugin), "extsock");
}
END_TEST

START_TEST(test_plugin_should_provide_required_features)
{
    // Given: 초기화된 플러그인
    plugin = extsock_plugin_create();
    
    // When: 기능 리스트 조회
    enumerator_t *features = plugin->create_feature_enumerator(plugin);
    
    // Then: 필요한 기능들이 제공됨
    bool has_socket_feature = false;
    bool has_config_feature = false;
    
    feature_t *feature;
    while (features->enumerate(features, &feature)) {
        if (feature->type == FEATURE_SOCKET) {
            has_socket_feature = true;
        }
        if (feature->type == FEATURE_CONFIG) {
            has_config_feature = true;
        }
    }
    
    ck_assert(has_socket_feature);
    ck_assert(has_config_feature);
    
    features->destroy(features);
}
END_TEST
```

### 5.6 Integration Layer 테스트

**목적**: 계층 간 통합과 End-to-End 워크플로우 테스트

**테스트 파일**: `test/integration/test_complete_workflow.c`

```c
/*
 * test_complete_workflow.c - 통합 워크플로우 테스트
 */
#include <check.h>
#include "../extsock_plugin.h"

static plugin_t *plugin;

void setup_integration_test(void) {
    plugin = extsock_plugin_create();
}

void teardown_integration_test(void) {
    if (plugin) {
        plugin->destroy(plugin);
    }
}

// End-to-End 워크플로우 테스트
START_TEST(test_complete_config_to_connection_workflow)
{
    // Given: JSON 설정 데이터
    const char *config_json = "{"
        "\"connection_name\": \"integration_test\","
        "\"local_ip\": \"192.168.1.10\","
        "\"remote_ip\": \"203.0.113.5\","
        "\"auth_method\": \"psk\","
        "\"psk_secret\": \"test123\""
    "}";
    
    // When: 설정 적용 워크플로우 실행
    // 1. JSON 파싱
    json_parser_t *parser = plugin->get_json_parser(plugin);
    ipsec_config_t *config = parser->parse(parser, config_json);
    ck_assert_ptr_nonnull(config);
    
    // 2. 설정 유효성 검증
    config_validator_t *validator = plugin->get_config_validator(plugin);
    bool is_valid = validator->validate(validator, config);
    ck_assert(is_valid);
    
    // 3. strongSwan 설정 변환
    strongswan_adapter_t *adapter = plugin->get_strongswan_adapter(plugin);
    strongswan_config_t *sw_config = adapter->convert_config(adapter, config);
    ck_assert_ptr_nonnull(sw_config);
    
    // 4. 연결 설정 적용
    bool applied = adapter->apply_config(adapter, sw_config);
    ck_assert(applied);
    
    // Then: 전체 워크플로우 성공
    // 설정이 실제로 적용되었는지 확인
    connection_status_t status = adapter->get_connection_status(adapter, "integration_test");
    ck_assert_int_eq(status, CONNECTION_CONFIGURED);
    
    // Cleanup
    strongswan_config_destroy(sw_config);
    ipsec_config_destroy(config);
}
END_TEST

// 에러 시나리오 통합 테스트
START_TEST(test_workflow_should_handle_parsing_errors_gracefully)
{
    // Given: 잘못된 JSON
    const char *invalid_json = "{invalid json structure";
    
    // When: 워크플로우 실행 (파싱 단계에서 실패 예상)
    json_parser_t *parser = plugin->get_json_parser(plugin);
    ipsec_config_t *config = parser->parse(parser, invalid_json);
    
    // Then: 우아하게 에러 처리
    ck_assert_ptr_null(config);
    
    error_t *error = parser->get_last_error(parser);
    ck_assert_ptr_nonnull(error);
    ck_assert_int_eq(error->code, JSON_PARSE_ERROR);
    
    // 플러그인 상태는 안전해야 함
    plugin_status_t status = plugin->get_status(plugin);
    ck_assert_int_eq(status, PLUGIN_STATUS_READY);
}
END_TEST
```

---

## 6. 테스트 패턴과 베스트 프랙티스

### 6.1 테스트 명명 규칙

**패턴**: `test_[component]_should_[expected_behavior]_when_[condition]`

**예제들**:
```c
// Good
START_TEST(test_json_parser_should_return_null_when_invalid_json_provided)
START_TEST(test_config_entity_should_validate_ip_format_when_creating)
START_TEST(test_socket_adapter_should_retry_connection_when_initial_fails)

// Avoid
START_TEST(test1)  // 너무 모호함
START_TEST(test_json_works)  // 무엇을 테스트하는지 불분명
```

### 6.2 Given-When-Then 패턴

모든 테스트는 다음 구조를 따릅니다:

```c
START_TEST(test_example)
{
    // Given: 테스트 전제 조건 설정
    config_t *config = create_test_config();
    parser_t *parser = create_json_parser();
    
    // When: 테스트할 동작 실행
    result_t *result = parser->parse(parser, config);
    
    // Then: 결과 검증
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result->name, expected_name);
    
    // Cleanup: 리소스 정리
    destroy_config(config);
    destroy_parser(parser);
    destroy_result(result);
}
END_TEST
```

### 6.3 모킹 패턴

외부 의존성은 모킹하여 테스트를 격리합니다:

```c
// Mock 구조체 정의
typedef struct mock_socket_adapter {
    socket_adapter_t interface;  // 인터페이스 상속
    
    // Mock 상태
    bool connect_called;
    bool send_called;
    int send_call_count;
    
    // Mock 반환값 설정
    bool should_connect_succeed;
    bool should_send_succeed;
} mock_socket_adapter_t;

// Mock 메서드 구현
static bool mock_connect(socket_adapter_t *this, const char *address) {
    mock_socket_adapter_t *mock = (mock_socket_adapter_t*)this;
    mock->connect_called = true;
    return mock->should_connect_succeed;
}

// Mock 생성 함수
static socket_adapter_t *create_mock_socket_adapter(void) {
    mock_socket_adapter_t *mock = malloc(sizeof(mock_socket_adapter_t));
    mock->interface.connect = mock_connect;
    mock->interface.send = mock_send;
    mock->interface.destroy = mock_destroy;
    
    // 기본값 설정
    mock->should_connect_succeed = true;
    mock->should_send_succeed = true;
    
    return &mock->interface;
}
```

### 6.4 테스트 데이터 관리

```c
// 테스트 데이터 팩토리 패턴
static config_data_t *create_valid_test_config(void) {
    config_data_t *config = malloc(sizeof(config_data_t));
    config->name = strdup("test_connection");
    config->local_ip = strdup("192.168.1.10");
    config->remote_ip = strdup("203.0.113.5");
    config->auth_method = strdup("psk");
    config->psk_secret = strdup("test_secret");
    return config;
}

static config_data_t *create_invalid_test_config(void) {
    config_data_t *config = malloc(sizeof(config_data_t));
    config->name = NULL;  // 의도적으로 잘못된 데이터
    config->local_ip = strdup("invalid.ip");
    return config;
}

// 상수 정의
#define TEST_CONNECTION_NAME "test_conn"
#define TEST_LOCAL_IP "192.168.1.10"
#define TEST_REMOTE_IP "203.0.113.5"
#define TEST_PSK_SECRET "test_secret_123"
```

### 6.5 메모리 관리 테스트

```c
START_TEST(test_memory_management_should_not_leak)
{
    // Given: 메모리 추적 시작
    size_t initial_memory = get_memory_usage();
    
    // When: 메모리를 사용하는 동작들
    for (int i = 0; i < 100; i++) {
        config_t *config = config_create("test");
        config_set_ip(config, "192.168.1.10");
        config_destroy(config);  // 반드시 해제
    }
    
    // Then: 메모리 사용량이 초기와 동일해야 함
    size_t final_memory = get_memory_usage();
    ck_assert_int_eq(initial_memory, final_memory);
}
END_TEST
```

### 6.6 동시성 테스트

```c
START_TEST(test_thread_safety_should_handle_concurrent_access)
{
    // Given: 공유 리소스
    shared_resource_t *resource = create_shared_resource();
    pthread_t threads[10];
    
    // When: 여러 스레드에서 동시 접근
    for (int i = 0; i < 10; i++) {
        pthread_create(&threads[i], NULL, concurrent_access_thread, resource);
    }
    
    // 모든 스레드 완료 대기
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Then: 데이터 일관성 확인
    ck_assert_int_eq(resource->access_count, 10);
    ck_assert(resource->is_consistent);
    
    destroy_shared_resource(resource);
}
END_TEST
```

---

## 7. 실전 예제

### 7.1 새로운 기능 추가: Failover Manager

**요구사항**: 연결 실패 시 자동으로 백업 서버로 전환하는 Failover Manager 구현

**Step 1: RED - 실패하는 테스트 작성**

```c
/*
 * test_failover_manager.c
 */
#include <check.h>
#include "../usecases/extsock_failover_manager.h"

static failover_manager_t *manager;
static mock_connection_monitor_t *monitor;

void setup_failover_test(void) {
    monitor = create_mock_connection_monitor();
    manager = failover_manager_create(monitor);
}

void teardown_failover_test(void) {
    if (manager) {
        manager->destroy(manager);
    }
    if (monitor) {
        monitor->destroy(monitor);
    }
}

// RED: 이 테스트는 처음에 실패해야 함
START_TEST(test_failover_manager_should_switch_to_backup_when_primary_fails)
{
    // Given: Primary와 backup 서버 설정
    server_config_t primary = { 
        .ip = "203.0.113.5", 
        .port = 500, 
        .priority = 1 
    };
    server_config_t backup = { 
        .ip = "203.0.113.10", 
        .port = 500, 
        .priority = 2 
    };
    
    manager->add_server(manager, &primary);
    manager->add_server(manager, &backup);
    
    // When: Primary 서버 연결 실패 시뮬레이션
    monitor->simulate_connection_failure(monitor, "203.0.113.5");
    
    // Then: 자동으로 backup 서버로 전환되어야 함
    server_config_t *active = manager->get_active_server(manager);
    ck_assert_ptr_nonnull(active);
    ck_assert_str_eq(active->ip, "203.0.113.10");  // backup 서버
    
    // 이벤트가 발생했는지 확인
    ck_assert(manager->has_failover_occurred(manager));
}
END_TEST

Suite *failover_manager_suite(void) {
    Suite *s = suite_create("Failover Manager Tests");
    TCase *tc_core = tcase_create("Core Failover Tests");
    
    tcase_add_checked_fixture(tc_core, setup_failover_test, teardown_failover_test);
    tcase_add_test(tc_core, test_failover_manager_should_switch_to_backup_when_primary_fails);
    
    suite_add_tcase(s, tc_core);
    return s;
}
```

**Step 2: 테스트 실행 (실패 확인)**

```bash
cd test
./run_individual_test.sh failover_manager
# 예상 결과: 컴파일 에러 (아직 구현 안됨)
```

**Step 3: GREEN - 최소 구현**

```c
/*
 * usecases/extsock_failover_manager.h
 */
#ifndef EXTSOCK_FAILOVER_MANAGER_H
#define EXTSOCK_FAILOVER_MANAGER_H

#include <stdbool.h>

typedef struct server_config server_config_t;
typedef struct failover_manager failover_manager_t;
typedef struct connection_monitor connection_monitor_t;

struct server_config {
    char *ip;
    int port;
    int priority;
};

struct failover_manager {
    bool (*add_server)(failover_manager_t *this, server_config_t *server);
    server_config_t *(*get_active_server)(failover_manager_t *this);
    bool (*has_failover_occurred)(failover_manager_t *this);
    void (*destroy)(failover_manager_t *this);
};

failover_manager_t *failover_manager_create(connection_monitor_t *monitor);

#endif
```

```c
/*
 * usecases/extsock_failover_manager.c - 최소 구현
 */
#include "extsock_failover_manager.h"
#include <stdlib.h>

typedef struct private_failover_manager {
    failover_manager_t public;
    
    server_config_t *servers[10];  // 최대 10개 서버
    int server_count;
    int active_server_index;
    bool failover_occurred;
    connection_monitor_t *monitor;
} private_failover_manager_t;

static bool add_server(failover_manager_t *this, server_config_t *server) {
    private_failover_manager_t *private = (private_failover_manager_t*)this;
    
    if (private->server_count < 10) {
        private->servers[private->server_count] = server;
        private->server_count++;
        return true;
    }
    return false;
}

static server_config_t *get_active_server(failover_manager_t *this) {
    private_failover_manager_t *private = (private_failover_manager_t*)this;
    
    // 간단한 로직: backup 서버가 있으면 그것을 반환
    if (private->server_count > 1) {
        private->failover_occurred = true;  // 시뮬레이션을 위해
        return private->servers[1];  // backup 서버
    }
    return private->servers[0];  // primary 서버
}

static bool has_failover_occurred(failover_manager_t *this) {
    private_failover_manager_t *private = (private_failover_manager_t*)this;
    return private->failover_occurred;
}

static void destroy(failover_manager_t *this) {
    free(this);
}

failover_manager_t *failover_manager_create(connection_monitor_t *monitor) {
    private_failover_manager_t *private = malloc(sizeof(private_failover_manager_t));
    
    private->public.add_server = add_server;
    private->public.get_active_server = get_active_server;
    private->public.has_failover_occurred = has_failover_occurred;
    private->public.destroy = destroy;
    
    private->server_count = 0;
    private->active_server_index = 0;
    private->failover_occurred = false;
    private->monitor = monitor;
    
    return &private->public;
}
```

**Step 4: 테스트 실행 (성공 확인)**

```bash
./run_individual_test.sh failover_manager
# 예상 결과: 테스트 통과
```

**Step 5: REFACTOR - 코드 개선**

더 많은 테스트 추가하고 실제 로직 구현:

```c
// 추가 테스트들
START_TEST(test_failover_manager_should_restore_primary_when_available)
START_TEST(test_failover_manager_should_handle_multiple_failures)
START_TEST(test_failover_manager_should_prioritize_servers_correctly)
```

### 7.2 기존 기능 수정: JSON Parser Enhancement

**요구사항**: JSON Parser가 중첩된 설정도 파싱할 수 있도록 개선

**Step 1: RED - 새로운 요구사항 테스트**

```c
START_TEST(test_json_parser_should_parse_nested_child_configs)
{
    // Given: 중첩된 JSON 설정
    const char *nested_json = "{"
        "\"connection_name\": \"main_tunnel\","
        "\"children\": ["
            "{"
                "\"name\": \"child1\","
                "\"local_ts\": [\"10.0.0.0/24\"],"
                "\"remote_ts\": [\"10.0.1.0/24\"]"
            "},"
            "{"
                "\"name\": \"child2\","
                "\"local_ts\": [\"10.0.2.0/24\"],"
                "\"remote_ts\": [\"10.0.3.0/24\"]"
            "}"
        "]"
    "}";
    
    // When: 중첩 JSON 파싱
    ipsec_config_t *config = parser->parse(parser, nested_json);
    
    // Then: 중첩된 children도 올바르게 파싱됨
    ck_assert_ptr_nonnull(config);
    ck_assert_str_eq(config->connection_name, "main_tunnel");
    ck_assert_int_eq(config->child_count, 2);
    
    child_config_t *child1 = config->children[0];
    ck_assert_str_eq(child1->name, "child1");
    ck_assert_int_eq(child1->local_ts_count, 1);
    ck_assert_str_eq(child1->local_ts[0], "10.0.0.0/24");
    
    child_config_t *child2 = config->children[1];
    ck_assert_str_eq(child2->name, "child2");
    ck_assert_str_eq(child2->local_ts[0], "10.0.2.0/24");
}
END_TEST
```

**Step 2: 테스트 실행 (실패 확인)**

```bash
./run_individual_test.sh json_parser_enhanced
# 실패: 중첩 구조 파싱 지원 안됨
```

**Step 3: GREEN - 기능 구현**

기존 JSON Parser 수정:

```c
// adapters/json/extsock_json_parser.c에 추가
static bool parse_child_configs(cJSON *children_json, ipsec_config_t *config) {
    if (!cJSON_IsArray(children_json)) {
        return false;
    }
    
    int child_count = cJSON_GetArraySize(children_json);
    config->children = malloc(sizeof(child_config_t*) * child_count);
    config->child_count = child_count;
    
    cJSON *child_json;
    int index = 0;
    cJSON_ArrayForEach(child_json, children_json) {
        child_config_t *child = parse_single_child_config(child_json);
        if (child) {
            config->children[index++] = child;
        }
    }
    
    return true;
}

// 기존 parse 함수에서 children 처리 추가
static ipsec_config_t *parse(json_parser_t *this, const char *json_str) {
    // ... 기존 코드 ...
    
    // Children 파싱 추가
    cJSON *children = cJSON_GetObjectItem(root, "children");
    if (children) {
        parse_child_configs(children, config);
    }
    
    // ... 나머지 코드 ...
}
```

**Step 4: 테스트 실행 및 리팩토링**

```bash
./run_individual_test.sh json_parser_enhanced
# 성공 확인 후 코드 정리 및 최적화
```

---

## 8. 문제해결 및 팁

### 8.1 일반적인 문제들

**1. 컴파일 에러**
```bash
# 문제: 헤더 파일을 찾을 수 없음
# 해결: 인클루드 경로 확인
gcc -I../../../../../src/libstrongswan -I../../../../../src/libcharon ...

# 문제: 라이브러리 링크 에러  
# 해결: 라이브러리 경로와 LD_LIBRARY_PATH 확인
export LD_LIBRARY_PATH="../../../../../src/libstrongswan/.libs:$LD_LIBRARY_PATH"
```

**2. 테스트 실행 실패**
```bash
# Check 프레임워크 설치 확인
sudo apt-get install libcheck-dev

# cJSON 라이브러리 설치 확인
sudo apt-get install libcjson-dev

# 권한 확인
chmod +x run_individual_test.sh
```

**3. 메모리 누수**
```c
// 모든 malloc에 대응하는 free 확인
void cleanup_test(void) {
    if (config) {
        free(config->name);
        free(config->local_ip);
        free(config->remote_ip);
        free(config);
        config = NULL;
    }
}

// Valgrind로 메모리 누수 검사
valgrind --leak-check=full ./test_binary
```

### 8.2 디버깅 기법

**1. 단위 테스트 디버깅**
```c
#ifdef DEBUG
    printf("DEBUG: config->name = %s\n", config->name);
    printf("DEBUG: expected = %s, actual = %s\n", expected, actual);
#endif

// 또는 Check 프레임워크의 메시지 활용
ck_assert_msg(condition, "Expected %s but got %s", expected, actual);
```

**2. GDB 사용**
```bash
# 디버그 정보로 컴파일
gcc -g -O0 test.c -o test

# GDB로 디버깅
gdb ./test
(gdb) break test_function_name
(gdb) run
(gdb) print variable_name
(gdb) step
```

**3. 로그 추가**
```c
// 임시 로그 (개발 중에만)
#define DEBUG_LOG(fmt, ...) \
    printf("[DEBUG %s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

START_TEST(test_with_debug) {
    DEBUG_LOG("Starting test with config: %s", config->name);
    // ... 테스트 코드 ...
    DEBUG_LOG("Test completed successfully");
}
```

### 8.3 성능 최적화 팁

**1. 테스트 실행 속도**
```bash
# 병렬 테스트 실행
make -j$(nproc) all

# 필요한 테스트만 실행
./run_individual_test.sh specific_test
```

**2. 테스트 격리**
```c
// Setup/Teardown을 효율적으로 사용
void setup_once(void) {
    // 비용이 큰 초기화는 한 번만
}

void setup_each(void) {
    // 각 테스트마다 필요한 최소한의 설정
}
```

### 8.4 CI/CD 통합

**GitHub Actions 예제**:
```yaml
# .github/workflows/test.yml
name: extsock Plugin Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install libcheck-dev libcjson-dev gcovr
    
    - name: Configure strongSwan
      run: ./configure
    
    - name: Build strongSwan
      run: make -j$(nproc)
    
    - name: Run tests
      run: |
        cd src/libcharon/plugins/extsock/test
        ./run_working_tests.sh
    
    - name: Generate coverage
      run: |
        cd src/libcharon/plugins/extsock/test
        ./run_coverage_test.sh
    
    - name: Upload coverage
      uses: codecov/codecov-action@v1
```

### 8.5 팁과 권장사항

**1. TDD 마인드셋**
- 테스트는 문서다: 다른 개발자가 읽고 이해할 수 있게 작성
- 실패 먼저: RED 단계에서 테스트가 정말 실패하는지 확인
- 작은 단위: 한 번에 하나의 기능만 테스트
- 빠른 피드백: 테스트 실행이 빨라야 자주 실행한다

**2. 코드 품질**
- 메모리 관리: 모든 malloc에 대응하는 free
- 에러 처리: NULL 체크와 에러 케이스 커버
- 상수 사용: 매직 넘버 대신 의미있는 상수 정의
- 코드 중복 제거: 공통 테스트 유틸리티 활용

**3. 협업**
- 일관된 네이밍: 팀 내 테스트 명명 규칙 준수
- 문서화: 복잡한 테스트는 주석으로 설명
- 리뷰: 테스트 코드도 코드 리뷰 대상
- 지식 공유: TDD 경험과 노하우 공유

---

## 결론

이 가이드를 통해 strongSwan extsock plugin 개발 시 TDD 방법론을 적용할 수 있는 완전한 프레임워크를 제공했습니다. 

**핵심 포인트**:
1. **RED-GREEN-REFACTOR** 사이클을 엄격히 따름
2. **계층별 테스트 전략**으로 체계적 접근
3. **Mock과 격리**를 통한 신뢰성 있는 테스트
4. **지속적 개선**을 통한 코드 품질 향상

이 가이드를 참고하여 안정적이고 유지보수하기 쉬운 strongSwan extsock plugin을 개발하시기 바랍니다.

---

**문서 버전**: 1.0  
**최종 업데이트**: 2024년  
**작성자**: strongSwan extsock Plugin Development Team  
**라이선스**: strongSwan Project License