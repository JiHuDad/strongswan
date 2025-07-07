# extsock 플러그인 모듈화 계획서

## 📋 개요

현재 868라인의 단일 파일로 구성된 extsock 플러그인을 Clean Architecture + Layered Architecture 기반으로 모듈화하여 메인 플러그인 파일을 120라인으로 축소하고 유지보수성을 향상시킵니다.

## 🎯 목표

- **메인 파일 크기**: 868라인 → 120라인 (86% 감소)
- **모듈화**: 기능별 관심사 분리
- **테스트 용이성**: 의존성 주입을 통한 모킹 가능
- **확장성**: 새로운 기능 추가 시 기존 코드 영향 최소화

## 🏗️ 아키텍처 설계

### 계층 구조
```
┌─────────────────────────────────┐
│     Plugin Entry Point         │  ← 플러그인 생명주기 관리
├─────────────────────────────────┤
│     Use Cases Layer            │  ← 비즈니스 로직
├─────────────────────────────────┤
│     Domain Layer               │  ← 핵심 엔티티
├─────────────────────────────────┤
│     Adapters Layer             │  ← 외부 시스템 연동
├─────────────────────────────────┤
│     Interfaces Layer           │  ← 추상화 계층
└─────────────────────────────────┘
```

### 디렉토리 구조
```
src/libcharon/plugins/extsock/
├── extsock_plugin.h                    # 공개 인터페이스
├── extsock_plugin.c                    # 플러그인 진입점 (120라인)
├── common/                             # 공통 모듈
│   ├── extsock_types.h                 # 공통 타입 정의
│   └── extsock_common.h                # 공통 상수/매크로
├── domain/                             # 도메인 로직
│   ├── extsock_config_entity.h/.c      # IPsec 설정 엔티티
│   └── extsock_validator.h/.c          # 설정 검증 로직
├── usecases/                           # 애플리케이션 로직
│   ├── extsock_config_usecase.h/.c     # 설정 관리 유스케이스
│   ├── extsock_event_usecase.h/.c      # 이벤트 처리 유스케이스
│   └── extsock_dpd_usecase.h/.c        # DPD 관리 유스케이스
├── adapters/                           # 어댑터 계층
│   ├── json/
│   │   ├── extsock_json_parser.h/.c    # JSON 파싱 어댑터
│   │   └── extsock_json_serializer.h/.c # JSON 직렬화 어댑터
│   ├── strongswan/
│   │   └── extsock_strongswan_adapter.h/.c # strongSwan API 어댑터
│   └── socket/
│       └── extsock_socket_adapter.h/.c  # 소켓 통신 어댑터
└── interfaces/                         # 추상 인터페이스
    ├── extsock_config_repository.h     # 설정 저장소 인터페이스
    ├── extsock_event_publisher.h       # 이벤트 발행 인터페이스
    └── extsock_command_handler.h       # 명령 처리 인터페이스
```

## 📊 예상 라인 수 분포

| 모듈 | 라인 수 | 책임 |
|------|---------|------|
| extsock_plugin.c | **120** | 플러그인 생명주기, 의존성 조립 |
| JSON 어댑터 | 280 | JSON 파싱/직렬화 |
| 소켓 어댑터 | 150 | 소켓 통신 |
| strongSwan 어댑터 | 100 | strongSwan API 연동 |
| 설정 유스케이스 | 150 | 설정 관리 비즈니스 로직 |
| 이벤트 유스케이스 | 100 | 이벤트 처리 비즈니스 로직 |
| 도메인 엔티티 | 180 | 핵심 도메인 로직 |
| 인터페이스/타입 | 100 | 추상화 계층 |
| **총합** | **1,180** | **기존 868라인에서 35% 증가하지만 모듈화 달성** |

## 🚀 실행 계획

### Phase 1: 공통 모듈 및 인터페이스 정의 (Week 1)
1. 공통 타입 및 상수 정의
2. 추상 인터페이스 정의
3. 기본 디렉토리 구조 생성

### Phase 2: Infrastructure Layer 분리 (Week 2)
1. 소켓 통신 어댑터 분리
2. JSON 파싱 어댑터 분리
3. strongSwan API 어댑터 분리

### Phase 3: Domain Layer 분리 (Week 3)
1. 설정 엔티티 분리
2. 검증 로직 분리
3. 도메인 서비스 분리

### Phase 4: Use Cases 분리 (Week 4)
1. 설정 관리 유스케이스 분리
2. 이벤트 처리 유스케이스 분리
3. DPD 관리 유스케이스 분리

### Phase 5: 플러그인 진입점 리팩토링 (Week 5)
1. 의존성 주입 구현
2. 플러그인 생명주기 관리
3. 통합 테스트 및 검증

## 🔧 기술적 고려사항

### 의존성 주입 패턴
```c
// 생성자 의존성 주입
extsock_config_usecase_t *extsock_config_usecase_create(
    extsock_json_parser_t *parser,
    extsock_event_usecase_t *event_usecase);
```

### 메모리 관리
- RAII 패턴 적용
- 명확한 소유권 정의
- 자동 해제 메커니즘

### 에러 처리
- 일관된 에러 코드 체계
- 에러 전파 메커니즘
- 로깅 표준화

## 📝 검증 기준

### 기능적 요구사항
- [ ] 기존 모든 기능 동작 확인
- [ ] JSON 설정 파싱 정상 동작
- [ ] 소켓 통신 정상 동작
- [ ] strongSwan 이벤트 처리 정상 동작

### 비기능적 요구사항
- [ ] 메인 플러그인 파일 120라인 이하
- [ ] 모듈 간 순환 의존성 없음
- [ ] 메모리 누수 없음
- [ ] 컴파일 경고 없음

## 🎉 기대 효과

1. **유지보수성 향상**: 모듈별 독립적 수정 가능
2. **테스트 용이성**: 단위 테스트 작성 가능
3. **확장성**: 새로운 기능 추가 시 영향도 최소화
4. **가독성**: 코드 이해도 향상
5. **재사용성**: 모듈별 재사용 가능

---

**작성일**: 2024년
**작성자**: AI Assistant
**버전**: 1.0 