# extsock Plugin API Reference

## 개요

extsock 플러그인의 모든 공개 API와 인터페이스에 대한 상세한 레퍼런스입니다.

---

## 📋 목차

1. [Core Interfaces](#core-interfaces)
2. [Adapters Layer](#adapters-layer)
3. [Use Cases Layer](#use-cases-layer)
4. [Domain Layer](#domain-layer)
5. [Common Types](#common-types)
6. [Error Handling](#error-handling)

---

## Core Interfaces

### 1. extsock_config_repository_t

IPsec 설정 저장소 인터페이스로, strongSwan과의 설정 관리를 추상화합니다.

```c
typedef struct extsock_config_repository_t {
    /**
     * peer_cfg 생성 및 저장
     */
    extsock_error_t (*create_peer_config)(extsock_config_repository_t *this,
                                          const extsock_config_entity_t *config,
                                          peer_cfg_t **peer_cfg);
    
    /**
     * child_cfg 생성 및 추가
     */
    extsock_error_t (*add_child_config)(extsock_config_repository_t *this,
                                        peer_cfg_t *peer_cfg,
                                        const extsock_child_config_t *child_config);
    
    /**
     * DPD 시작
     */
    extsock_error_t (*start_dpd)(extsock_config_repository_t *this,
                                 const char *ike_sa_name);
    
    /**
     * 설정 검증
     */
    extsock_error_t (*validate_config)(extsock_config_repository_t *this,
                                       const extsock_config_entity_t *config);
    
    /**
     * 리소스 해제
     */
    void (*destroy)(extsock_config_repository_t *this);
} extsock_config_repository_t;
```

#### 메서드 설명

##### create_peer_config()
- **목적**: IPsec peer 설정을 생성하고 strongSwan에 등록
- **매개변수**:
  - `config`: 생성할 설정 엔티티
  - `peer_cfg`: 생성된 peer_cfg 객체 반환
- **반환값**: `EXTSOCK_ERROR_NONE` 성공, 기타 에러 코드
- **예제**:
```c
extsock_config_entity_t *config = /* 설정 생성 */;
peer_cfg_t *peer_cfg;
extsock_error_t result = repository->create_peer_config(repository, config, &peer_cfg);
if (result != EXTSOCK_ERROR_NONE) {
    // 에러 처리
}
```

##### add_child_config()
- **목적**: child SA 설정을 peer 설정에 추가
- **매개변수**:
  - `peer_cfg`: 대상 peer 설정
  - `child_config`: 추가할 child 설정
- **반환값**: `EXTSOCK_ERROR_NONE` 성공, 기타 에러 코드

##### start_dpd()
- **목적**: Dead Peer Detection 시작
- **매개변수**:
  - `ike_sa_name`: 대상 IKE SA 이름
- **반환값**: `EXTSOCK_ERROR_NONE` 성공, 기타 에러 코드

### 2. extsock_event_publisher_t

이벤트 발행 인터페이스로, strongSwan 이벤트를 외부 시스템에 전달합니다.

```c
typedef struct extsock_event_publisher_t {
    /**
     * 일반 이벤트 발행
     */
    extsock_error_t (*publish_event)(extsock_event_publisher_t *this,
                                     const char *event_json);
    
    /**
     * 터널 이벤트 발행
     */
    extsock_error_t (*publish_tunnel_event)(extsock_event_publisher_t *this,
                                            const char *tunnel_event_json);
    
    /**
     * 리소스 해제
     */
    void (*destroy)(extsock_event_publisher_t *this);
} extsock_event_publisher_t;
```

#### 메서드 설명

##### publish_event()
- **목적**: JSON 형태의 이벤트를 외부로 발행
- **매개변수**:
  - `event_json`: JSON 형태의 이벤트 데이터
- **반환값**: `EXTSOCK_ERROR_NONE` 성공, 기타 에러 코드
- **예제**:
```c
const char *event = "{\"event\":\"ike_sa_up\",\"name\":\"conn1\"}";
extsock_error_t result = publisher->publish_event(publisher, event);
```

### 3. extsock_command_handler_t

외부 명령 처리 인터페이스입니다.

```c
typedef struct extsock_command_handler_t {
    /**
     * 일반 명령 처리
     */
    extsock_error_t (*handle_command)(extsock_command_handler_t *this,
                                      const char *command);
    
    /**
     * DPD 명령 처리
     */
    extsock_error_t (*handle_dpd_command)(extsock_command_handler_t *this,
                                          const char *ike_sa_name);
    
    /**
     * 리소스 해제
     */
    void (*destroy)(extsock_command_handler_t *this);
} extsock_command_handler_t;
```

---

## Adapters Layer

### 1. extsock_json_parser_t

JSON 파싱 어댑터로, JSON 설정을 strongSwan 객체로 변환합니다.

```c
typedef struct extsock_json_parser_t {
    /**
     * JSON 배열을 제안 목록으로 변환
     */
    linked_list_t *(*parse_proposals_from_json_array)(extsock_json_parser_t *this,
                                                      cJSON *json_array,
                                                      protocol_id_t proto,
                                                      bool is_ike);
    
    /**
     * JSON 배열을 트래픽 셀렉터 목록으로 변환
     */
    linked_list_t *(*parse_ts_from_json_array)(extsock_json_parser_t *this,
                                               cJSON *json_array);
    
    /**
     * IKE 설정 JSON 파싱
     */
    ike_cfg_t *(*parse_ike_cfg_from_json)(extsock_json_parser_t *this,
                                          cJSON *ike_json);
    
    /**
     * 인증 설정 JSON 파싱
     */
    auth_cfg_t *(*parse_auth_cfg_from_json)(extsock_json_parser_t *this,
                                            cJSON *auth_json,
                                            bool is_local);
    
    /**
     * JSON 배열을 쉼표 구분 문자열로 변환
     */
    char *(*json_array_to_comma_separated_string)(extsock_json_parser_t *this,
                                                  cJSON *json_array);
    
    /**
     * 리소스 해제
     */
    void (*destroy)(extsock_json_parser_t *this);
} extsock_json_parser_t;
```

#### 사용 예제

```c
// JSON 파서 생성
extsock_json_parser_t *parser = extsock_json_parser_create();

// JSON에서 제안 파싱
cJSON *proposals_json = cJSON_CreateArray();
cJSON_AddItemToArray(proposals_json, cJSON_CreateString("aes256-sha256-modp2048"));

linked_list_t *proposals = parser->parse_proposals_from_json_array(
    parser, proposals_json, PROTO_IKE, TRUE);

// 리소스 정리
proposals->destroy_offset(proposals, offsetof(proposal_t, destroy));
cJSON_Delete(proposals_json);
parser->destroy(parser);
```

### 2. extsock_socket_adapter_t

소켓 통신 어댑터입니다.

```c
typedef struct extsock_socket_adapter_t {
    /**
     * 소켓 서버 시작
     */
    thread_t *(*start_listening)(extsock_socket_adapter_t *this);
    
    /**
     * 이벤트 전송
     */
    extsock_error_t (*send_event)(extsock_socket_adapter_t *this,
                                  const char *event_json);
    
    /**
     * 클라이언트 연결 수 조회
     */
    int (*get_client_count)(extsock_socket_adapter_t *this);
    
    /**
     * 리소스 해제
     */
    void (*destroy)(extsock_socket_adapter_t *this);
} extsock_socket_adapter_t;
```

---

## Use Cases Layer

### 1. extsock_config_usecase_t

설정 관리 비즈니스 로직을 담당합니다.

```c
typedef struct extsock_config_usecase_t {
    /**
     * IPsec 설정 적용
     */
    extsock_error_t (*apply_config)(extsock_config_usecase_t *this,
                                    const char *config_json);
    
    /**
     * DPD 시작
     */
    extsock_error_t (*start_dpd)(extsock_config_usecase_t *this,
                                 const char *ike_sa_name);
    
    /**
     * 명령 처리기 인터페이스 반환
     */
    extsock_command_handler_t *(*get_command_handler)(extsock_config_usecase_t *this);
    
    /**
     * 리소스 해제
     */
    void (*destroy)(extsock_config_usecase_t *this);
} extsock_config_usecase_t;
```

### 2. extsock_event_usecase_t

이벤트 처리 비즈니스 로직을 담당합니다.

```c
typedef struct extsock_event_usecase_t {
    /**
     * strongSwan 버스 리스너
     */
    listener_t listener;
    
    /**
     * Child SA 상태 변화 처리
     */
    void (*handle_child_updown)(extsock_event_usecase_t *this,
                                ike_sa_t *ike_sa,
                                child_sa_t *child_sa,
                                bool up);
    
    /**
     * 이벤트 발행자 인터페이스 반환
     */
    extsock_event_publisher_t *(*get_event_publisher)(extsock_event_usecase_t *this);
    
    /**
     * 소켓 어댑터 설정 (의존성 주입)
     */
    void (*set_socket_adapter)(extsock_event_usecase_t *this,
                               extsock_socket_adapter_t *socket_adapter);
    
    /**
     * 리소스 해제
     */
    void (*destroy)(extsock_event_usecase_t *this);
} extsock_event_usecase_t;
```

---

## Domain Layer

### extsock_config_entity_t

IPsec 설정의 도메인 모델입니다.

```c
typedef struct extsock_config_entity_t {
    /**
     * 설정 검증
     */
    extsock_error_t (*validate)(extsock_config_entity_t *this);
    
    /**
     * JSON에서 설정 파싱
     */
    extsock_error_t (*parse_from_json)(extsock_config_entity_t *this,
                                       const char *json_str);
    
    /**
     * 설정을 JSON으로 변환
     */
    char *(*to_json)(extsock_config_entity_t *this);
    
    /**
     * 리소스 해제
     */
    void (*destroy)(extsock_config_entity_t *this);
    
    /* 설정 데이터 */
    char *name;
    char *local_addr;
    char *remote_addr;
    extsock_auth_config_t auth;
    linked_list_t *children; // extsock_child_config_t 목록
    /* ... 기타 설정 필드들 */
} extsock_config_entity_t;
```

---

## Common Types

### extsock_error_t

플러그인 전체에서 사용하는 에러 타입입니다.

```c
typedef enum {
    EXTSOCK_ERROR_NONE = 0,              // 성공
    EXTSOCK_ERROR_MEMORY,                // 메모리 부족
    EXTSOCK_ERROR_CONFIG_INVALID,        // 잘못된 설정
    EXTSOCK_ERROR_JSON_PARSE,            // JSON 파싱 오류
    EXTSOCK_ERROR_STRONGSWAN_API,        // strongSwan API 오류
    EXTSOCK_ERROR_NETWORK,               // 네트워크 오류
    EXTSOCK_ERROR_NOT_FOUND,             // 리소스 없음
    EXTSOCK_ERROR_PERMISSION,            // 권한 부족
    EXTSOCK_ERROR_TIMEOUT,               // 타임아웃
    EXTSOCK_ERROR_UNKNOWN                // 알 수 없는 오류
} extsock_error_t;
```

### extsock_auth_config_t

인증 설정 구조체입니다.

```c
typedef struct extsock_auth_config_t {
    extsock_auth_type_t type;     // PSK, CERT, EAP 등
    char *identity;               // 인증 ID
    char *secret;                 // PSK 또는 키 파일 경로
    char *ca_cert;                // CA 인증서 (인증서 인증 시)
    char *cert;                   // 클라이언트 인증서
    char *private_key;            // 개인키
} extsock_auth_config_t;
```

### extsock_child_config_t

Child SA 설정 구조체입니다.

```c
typedef struct extsock_child_config_t {
    char *name;                   // Child SA 이름
    char *local_ts;               // 로컬 트래픽 셀렉터
    char *remote_ts;              // 원격 트래픽 셀렉터
    action_t dpd_action;          // DPD 액션
    uint32_t rekey_time;          // 재키잉 시간
    linked_list_t *esp_proposals; // ESP 제안 목록
} extsock_child_config_t;
```

---

## Error Handling

### 에러 처리 패턴

모든 API는 일관된 에러 처리 패턴을 따릅니다:

```c
extsock_error_t result = function_call(params);
switch (result) {
    case EXTSOCK_ERROR_NONE:
        // 성공 처리
        break;
    case EXTSOCK_ERROR_MEMORY:
        // 메모리 부족 처리
        break;
    case EXTSOCK_ERROR_CONFIG_INVALID:
        // 설정 오류 처리
        break;
    default:
        // 기타 오류 처리
        break;
}
```

### 에러 메시지 조회

```c
const char *extsock_error_to_string(extsock_error_t error);
```

---

## 생성자 함수들

### 어댑터 생성자

```c
// JSON 파서 생성
extsock_json_parser_t *extsock_json_parser_create();

// 소켓 어댑터 생성
extsock_socket_adapter_t *extsock_socket_adapter_create(
    extsock_command_handler_t *command_handler);

// strongSwan 어댑터 생성
extsock_strongswan_adapter_t *extsock_strongswan_adapter_create();
```

### 유스케이스 생성자

```c
// 설정 유스케이스 생성
extsock_config_usecase_t *extsock_config_usecase_create(
    extsock_json_parser_t *json_parser,
    extsock_event_publisher_t *event_publisher);

// 이벤트 유스케이스 생성
extsock_event_usecase_t *extsock_event_usecase_create();
```

### 도메인 엔티티 생성자

```c
// 설정 엔티티 생성
extsock_config_entity_t *extsock_config_entity_create();
```

---

## 의존성 주입 패턴

모든 컴포넌트는 의존성 주입 패턴을 사용합니다:

```c
// 1. 어댑터 생성
extsock_json_parser_t *json_parser = extsock_json_parser_create();
extsock_event_usecase_t *event_usecase = extsock_event_usecase_create();

// 2. 유스케이스 생성 (의존성 주입)
extsock_config_usecase_t *config_usecase = extsock_config_usecase_create(
    json_parser,
    event_usecase->get_event_publisher(event_usecase)
);

// 3. 소켓 어댑터 생성 (의존성 주입)
extsock_socket_adapter_t *socket_adapter = extsock_socket_adapter_create(
    config_usecase->get_command_handler(config_usecase)
);

// 4. 순환 의존성 해결
event_usecase->set_socket_adapter(event_usecase, socket_adapter);
```

---

## 스레드 안전성

- 모든 공개 API는 스레드 안전합니다
- 내부적으로 뮤텍스를 사용하여 동시성 제어
- 리소스 해제 시 모든 스레드가 안전하게 종료됨

## 메모리 관리

- 모든 객체는 `destroy()` 메서드를 통해 해제
- 반환된 문자열은 호출자가 `free()` 해야 함
- linked_list는 `destroy_offset()` 사용 권장

---

이 API 레퍼런스는 extsock 플러그인의 모든 공개 인터페이스를 다룹니다. 추가 정보나 예제는 개발자 가이드를 참조하세요. 