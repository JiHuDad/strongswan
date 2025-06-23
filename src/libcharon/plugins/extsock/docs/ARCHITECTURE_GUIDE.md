# extsock Plugin Architecture Guide

## 📋 개요

이 문서는 extsock 플러그인의 아키텍처 설계 원칙, 구조, 그리고 각 계층간의 관계를 상세히 설명합니다.

---

## 🎯 설계 목표

### 주요 목표
1. **모듈화**: 단일 책임 원칙에 따른 기능별 모듈 분리
2. **테스트 가능성**: 의존성 주입을 통한 단위 테스트 지원
3. **확장성**: 새로운 기능 추가 시 기존 코드 수정 최소화
4. **유지보수성**: 코드 복잡도 감소 및 가독성 향상
5. **성능**: 모듈화로 인한 오버헤드 최소화

### 달성 결과
- ✅ **메인 파일 크기**: 868라인 → 184라인 (78.8% 감소)
- ✅ **Clean Architecture 적용**: 계층별 관심사 분리
- ✅ **완전한 모듈화**: 18개 모듈로 기능 분산

---

## 🏗️ 아키텍처 개요

### Clean Architecture 기반 설계

```
┌─────────────────────────────────────────────────────────────┐
│                        Plugin Entry Point                   │
│                     (Dependency Container)                  │
└─────────────────────────────────────────────────────────────┘
                                │
                ┌───────────────┼───────────────┐
                │               │               │
┌───────────────▼──┐   ┌────────▼────────┐   ┌─▼─────────────┐
│   Use Cases      │   │   Adapters      │   │   Domain      │
│  (Business Logic)│   │ (Infrastructure)│   │ (Core Models) │
│                  │   │                 │   │               │
│ • Config UseCase │   │ • JSON Adapter  │   │ • Config      │
│ • Event UseCase  │   │ • Socket        │   │   Entity      │
│                  │   │ • strongSwan    │   │               │
└──────────────────┘   └─────────────────┘   └───────────────┘
                                │
                ┌───────────────┼───────────────┐
                │               │               │
┌───────────────▼──┐   ┌────────▼────────┐   ┌─▼─────────────┐
│   Interfaces     │   │     Common      │   │   External    │
│  (Abstractions)  │   │   (Shared)      │   │   Systems     │
│                  │   │                 │   │               │
│ • Repository     │   │ • Types         │   │ • strongSwan  │
│ • Publisher      │   │ • Constants     │   │ • JSON Client │
│ • Handler        │   │ • Utilities     │   │ • Socket      │
└──────────────────┘   └─────────────────┘   └───────────────┘
```

---

## 🔄 계층별 상세 설명

### 1. Plugin Entry Point (플러그인 진입점)

**파일**: `extsock_plugin.c` (184라인)

**역할**:
- 의존성 주입 컨테이너 역할
- 컴포넌트 생명주기 관리
- strongSwan 플러그인 인터페이스 구현

**핵심 구조**:
```c
struct private_extsock_plugin_t {
    extsock_plugin_t public;               // 공개 인터페이스
    extsock_di_container_t container;      // 의존성 주입 컨테이너
    thread_t *socket_thread;               // 소켓 스레드
};

typedef struct extsock_di_container_t {
    extsock_json_parser_t *json_parser;
    extsock_socket_adapter_t *socket_adapter;
    extsock_config_usecase_t *config_usecase;
    extsock_event_usecase_t *event_usecase;
} extsock_di_container_t;
```

**의존성 주입 플로우**:
```
1. JSON 파서 생성
2. 이벤트 유스케이스 생성
3. 설정 유스케이스 생성 (JSON 파서 + 이벤트 발행자 주입)
4. 소켓 어댑터 생성 (명령 처리기 주입)
5. 순환 의존성 해결 (이벤트 유스케이스 ← 소켓 어댑터)
```

### 2. Use Cases Layer (비즈니스 로직 계층)

#### 2.1 Config Use Case
**파일**: `usecases/extsock_config_usecase.h/.c` (256라인)

**책임**:
- IPsec 설정 적용 비즈니스 로직
- 외부 명령 처리 워크플로우
- 설정 검증 및 변환 조정

**주요 메서드**:
```c
// IPsec 설정 적용
extsock_error_t (*apply_config)(extsock_config_usecase_t *this,
                                const char *config_json);

// DPD 트리거
extsock_error_t (*start_dpd)(extsock_config_usecase_t *this,
                             const char *ike_sa_name);

// 명령 처리기 인터페이스 제공
extsock_command_handler_t *(*get_command_handler)(extsock_config_usecase_t *this);
```

**워크플로우**:
```
1. JSON 수신 → 2. 파싱 → 3. 검증 → 4. strongSwan 설정 → 5. 이벤트 발행
```

#### 2.2 Event Use Case
**파일**: `usecases/extsock_event_usecase.h/.c` (197라인)

**책임**:
- strongSwan 이벤트 처리
- JSON 이벤트 생성
- 외부 시스템으로 이벤트 전송

**주요 기능**:
```c
// strongSwan 버스 리스너 구현
listener_t listener;

// Child SA 상태 변화 처리
void (*handle_child_updown)(extsock_event_usecase_t *this,
                            ike_sa_t *ike_sa, child_sa_t *child_sa, bool up);

// 이벤트 발행자 인터페이스 제공
extsock_event_publisher_t *(*get_event_publisher)(extsock_event_usecase_t *this);
```

### 3. Adapters Layer (인프라스트럭처 계층)

#### 3.1 JSON Parser Adapter
**파일**: `adapters/json/extsock_json_parser.h/.c` (432라인)

**책임**:
- JSON ↔ strongSwan 객체 변환
- 설정 파싱 및 검증
- 기본값 적용

**변환 매핑**:
```
JSON Array → linked_list_t (proposals)
JSON Object → ike_cfg_t (IKE 설정)
JSON Object → auth_cfg_t (인증 설정)
JSON Array → linked_list_t (traffic selectors)
```

#### 3.2 Socket Adapter
**파일**: `adapters/socket/extsock_socket_adapter.h/.c` (288라인)

**책임**:
- 유닉스 도메인 소켓 서버 관리
- 멀티 클라이언트 지원
- 비동기 이벤트 전송

**아키텍처**:
```
Socket Server Thread
├── Accept Loop
├── Client Handler Threads
└── Event Broadcast Queue
```

#### 3.3 strongSwan Adapter
**파일**: `adapters/strongswan/extsock_strongswan_adapter.h/.c` (337라인)

**책임**:
- strongSwan API 래핑
- peer_cfg/child_cfg 관리
- DPD 작업 처리

### 4. Domain Layer (도메인 계층)

#### Config Entity
**파일**: `domain/extsock_config_entity.h/.c` (203라인)

**책임**:
- IPsec 설정의 도메인 모델
- 비즈니스 규칙 검증
- 설정 불변성 보장

**도메인 규칙**:
```c
// 필수 필드 검증
bool validate_required_fields(extsock_config_entity_t *this);

// 네트워크 주소 검증  
bool validate_network_addresses(extsock_config_entity_t *this);

// 인증 설정 검증
bool validate_auth_config(extsock_config_entity_t *this);
```

### 5. Interfaces Layer (추상화 계층)

#### 5.1 Config Repository Interface
**파일**: `interfaces/extsock_config_repository.h` (50라인)

**목적**: strongSwan 설정 관리 추상화

#### 5.2 Event Publisher Interface  
**파일**: `interfaces/extsock_event_publisher.h` (43라인)

**목적**: 이벤트 발행 추상화

#### 5.3 Command Handler Interface
**파일**: `interfaces/extsock_command_handler.h` (48라인)

**목적**: 명령 처리 추상화

### 6. Common Layer (공통 계층)

#### Types & Constants
**파일**: `common/extsock_types.h` (58라인), `common/extsock_common.h` (42라인)

**포함 내용**:
- 공통 타입 정의
- 에러 코드 정의
- 로깅 매크로
- 상수 정의

---

## 🔀 데이터 플로우

### 1. 설정 적용 플로우

```
External Client
      │
      │ JSON Config
      ▼
Socket Adapter ────────┐
      │                │
      │ Command        │
      ▼                │
Config UseCase         │
      │                │
      │ JSON Parse     │
      ▼                │
JSON Parser ───────────┘
      │
      │ strongSwan Objects
      ▼
strongSwan Adapter
      │
      │ peer_cfg/child_cfg
      ▼
strongSwan Core
```

### 2. 이벤트 발행 플로우

```
strongSwan Core
      │
      │ IKE/Child SA Events
      ▼
Event UseCase (Bus Listener)
      │
      │ JSON Event
      ▼
Socket Adapter
      │
      │ JSON String
      ▼
External Clients
```

---

## 🔄 의존성 관계

### 의존성 그래프

```
Plugin Entry Point
├── depends on → Config UseCase
├── depends on → Event UseCase  
├── depends on → Socket Adapter
└── depends on → JSON Parser

Config UseCase
├── depends on → JSON Parser (Interface)
├── depends on → Event Publisher (Interface)
└── depends on → strongSwan Adapter

Event UseCase
├── depends on → Socket Adapter (Injected)
└── implements → Event Publisher (Interface)

Socket Adapter
├── depends on → Command Handler (Interface)
└── implements → Socket Communication

JSON Parser
└── implements → JSON ↔ strongSwan conversion

strongSwan Adapter
├── depends on → strongSwan Core APIs
└── implements → Config Repository (Interface)
```

### 의존성 역전 예시

**Before (강한 결합)**:
```c
// 직접 의존성 - 테스트 어려움
void apply_config(const char *json) {
    cJSON *parsed = cJSON_Parse(json);  // JSON 라이브러리에 직접 의존
    peer_cfg_t *cfg = /* strongSwan API 직접 호출 */;
}
```

**After (의존성 역전)**:
```c
// 인터페이스 의존성 - 테스트 용이
typedef struct config_usecase_t {
    extsock_json_parser_t *json_parser;     // 추상화된 인터페이스
    extsock_config_repository_t *repository; // 추상화된 인터페이스
} config_usecase_t;

extsock_error_t apply_config(config_usecase_t *this, const char *json) {
    // 인터페이스를 통한 호출 - 구현체 교체 가능
    extsock_config_entity_t *config = this->json_parser->parse(this->json_parser, json);
    return this->repository->create_peer_config(this->repository, config);
}
```

---

## 🧪 테스트 전략

### 1. 단위 테스트 (Unit Tests)

**각 계층별 독립 테스트**:
```c
// Mock 의존성을 사용한 테스트
void test_config_usecase_apply_config() {
    // Given
    mock_json_parser_t *mock_parser = create_mock_json_parser();
    mock_repository_t *mock_repo = create_mock_repository();
    
    config_usecase_t *usecase = config_usecase_create(
        (extsock_json_parser_t*)mock_parser,
        (extsock_config_repository_t*)mock_repo
    );
    
    // When
    extsock_error_t result = usecase->apply_config(usecase, valid_json);
    
    // Then
    assert_equal(EXTSOCK_ERROR_NONE, result);
    assert_called(mock_parser, parse_method);
    assert_called(mock_repo, create_peer_config);
}
```

### 2. 통합 테스트 (Integration Tests)

**전체 워크플로우 테스트**:
```c
void test_full_config_workflow() {
    // 실제 컴포넌트들을 연결한 완전한 워크플로우 테스트
    // Socket → UseCase → Parser → strongSwan
}
```

### 3. 계약 테스트 (Contract Tests)

**인터페이스 준수 검증**:
```c
void test_json_parser_contract() {
    // 모든 JSON 파서 구현체가 동일한 인터페이스 계약을 준수하는지 검증
}
```

---

## 🚀 확장성 고려사항

### 1. 새로운 Adapter 추가

**예시: REST API Adapter 추가**
```c
// 1. 인터페이스 정의 (이미 존재)
extsock_command_handler_t *command_handler;

// 2. REST API Adapter 구현
typedef struct rest_api_adapter_t {
    extsock_command_handler_t *command_handler;
    http_server_t *server;
} rest_api_adapter_t;

// 3. Plugin Entry Point에서 주입
rest_api_adapter_t *rest_adapter = rest_api_adapter_create(command_handler);
```

### 2. 새로운 Protocol Support

**예시: MQTT 이벤트 발행**
```c
// 1. MQTT Publisher 구현
typedef struct mqtt_publisher_t {
    extsock_event_publisher_t public;  // 인터페이스 상속
    mqtt_client_t *client;
} mqtt_publisher_t;

// 2. Event UseCase에 주입 가능
event_usecase->set_additional_publisher(event_usecase, mqtt_publisher);
```

### 3. 새로운 설정 형식

**예시: YAML 지원**
```c
// 1. YAML Parser 구현
extsock_yaml_parser_t implements extsock_config_parser_t;

// 2. Config UseCase에서 Parser 교체
config_usecase_set_parser(config_usecase, yaml_parser);
```

---

## 🔧 성능 고려사항

### 1. 메모리 효율성

**객체 풀링**:
```c
// 자주 생성/해제되는 객체들의 풀링
typedef struct object_pool_t {
    linked_list_t *available_configs;
    linked_list_t *available_events;
} object_pool_t;
```

**지연 초기화**:
```c
// 필요할 때만 컴포넌트 생성
lazy_initialization_t lazy_strongswan_adapter;
```

### 2. 처리량 최적화

**이벤트 배치 처리**:
```c
// 이벤트들을 모아서 배치로 전송
typedef struct event_batch_t {
    cJSON *events_array;
    size_t batch_size;
    uint32_t flush_interval_ms;
} event_batch_t;
```

**비동기 처리**:
```c
// 논블로킹 소켓 I/O
// 이벤트 루프 기반 처리
// 스레드 풀 활용
```

---

## 🛡️ 보안 고려사항

### 1. 입력 검증

**다층 검증**:
```
1. Socket Layer: 크기 제한, 형식 검증
2. Parser Layer: JSON 스키마 검증  
3. Domain Layer: 비즈니스 규칙 검증
4. strongSwan Layer: API 레벨 검증
```

### 2. 권한 제어

**소켓 권한**:
```c
// 유닉스 도메인 소켓 파일 권한 설정
chmod(socket_path, 0600);  // 소유자만 접근
```

### 3. 데이터 무결성

**설정 체크섬**:
```c
// 중요 설정에 대한 체크섬 검증
uint32_t calculate_config_checksum(extsock_config_entity_t *config);
```

---

## 📊 메트릭스 및 모니터링

### 1. 성능 메트릭스

```c
typedef struct performance_metrics_t {
    uint64_t configs_applied;
    uint64_t events_published;
    uint32_t avg_config_apply_time_ms;
    uint32_t avg_event_publish_time_ms;
    size_t memory_usage_bytes;
} performance_metrics_t;
```

### 2. 헬스 체크

```c
typedef struct health_check_t {
    bool socket_server_running;
    bool strongswan_api_accessible;
    uint32_t active_connections;
    time_t last_event_time;
} health_check_t;
```

---

## 🔄 마이그레이션 가이드

### Legacy Code에서 New Architecture로

**1단계: 인터페이스 도입**
```c
// 기존 함수를 인터페이스로 래핑
extsock_legacy_wrapper_t *wrapper = create_legacy_wrapper();
config_usecase_set_parser(config_usecase, wrapper);
```

**2단계: 점진적 교체**  
```c
// 하나씩 새로운 구현체로 교체
config_usecase_set_parser(config_usecase, new_json_parser);
```

**3단계: Legacy 코드 제거**
```c
// 모든 교체 완료 후 legacy 코드 삭제
```

---

## 📚 추가 자료

- **[API Reference](API_REFERENCE.md)**: 상세 API 문서
- **[Development Guide](DEVELOPMENT_GUIDE.md)**: 개발자 가이드  
- **[Integration Examples](INTEGRATION_EXAMPLES.md)**: 통합 예제
- **[Troubleshooting](TROUBLESHOOTING.md)**: 문제 해결 가이드

---

이 아키텍처 가이드는 extsock 플러그인의 설계 철학과 구현 세부사항을 포괄적으로 다룹니다. 새로운 기능 개발이나 유지보수 시 이 가이드를 참조하여 아키텍처 일관성을 유지하시기 바랍니다. 