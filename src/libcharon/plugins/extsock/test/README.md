# strongSwan extsock Plugin Test Suite

이 디렉토리는 strongSwan extsock 플러그인의 테스트 스위트를 포함합니다.

## 📋 개요

extsock 플러그인은 외부 애플리케이션이 strongSwan과 Unix Domain Socket을 통해 통신할 수 있게 해주는 플러그인입니다. 이 테스트 스위트는 플러그인의 모든 기능을 검증합니다.

## 🏗️ 아키텍처

### Clean Architecture 기반 구조
```
Plugin Layer        (플러그인 생명주기 관리)
├── Usecase Layer   (비즈니스 로직: Config, Event)
├── Adapter Layer   (외부 인터페이스: JSON, Socket, strongSwan)
├── Domain Layer    (핵심 비즈니스 엔터티)
└── Common Layer    (공통 유틸리티, 에러 처리)
```

## 🧪 HOW TO TEST

### 빠른 시작

1. **전체 테스트 실행**
   ```bash
   ./run_working_tests.sh
   ```

2. **개별 테스트 실행**
   ```bash
   ./run_individual_test.sh <test_name>
   ```

3. **커버리지 측정**
   ```bash
   ./run_coverage_test.sh
   ```

### 사용 가능한 테스트 목록

#### 1. 기본 단위 테스트
- `simple_unit` - 기본 단위 테스트 (7개 체크)
- `error_scenarios` - 에러 시나리오 테스트 (4개 체크)

#### 2. JSON 파서 테스트
- `json_parser_simple` - 기본 JSON 파싱 (7개 체크)
- `json_parser_real` - 실제 JSON 파서 구현 (8개 체크)

#### 3. 소켓 어댑터 테스트
- `socket_adapter_simple` - 기본 소켓 통신 (6개 체크)
- `socket_adapter_real` - 실제 소켓 어댑터 구현 (9개 체크)

#### 4. 플러그인 테스트
- `plugin_simple` - 플러그인 기본 기능 (8개 체크)

#### 5. Usecase 테스트
- `config_usecase_real` - Config Usecase 구현 (8개 체크)
- `event_usecase_real` - Event Usecase 구현 (8개 체크)

#### 6. 도메인 엔터티 테스트
- `domain_entity_real` - 도메인 엔터티 (8개 체크)

#### 7. 통합 테스트
- `complete_workflow` - 전체 워크플로우 (9개 체크)

### Phase 1 테스트 (고급)

Phase 1 테스트는 4주간에 걸쳐 개발된 종합적인 테스트입니다:

```bash
# Week별 실행
./run_phase1_tests.sh week1   # 플러그인 생명주기 + 에러 처리
./run_phase1_tests.sh week2   # JSON 파싱 완성
./run_phase1_tests.sh week3   # 소켓 통신 완성  
./run_phase1_tests.sh week4   # usecase 통합 완성

# 커버리지 포함
./run_phase1_tests.sh week1 --coverage --verbose

# 상태 확인
./run_phase1_tests.sh --status
```

### 개별 테스트 실행

```bash
# 기본 실행
./run_individual_test.sh json_parser_simple

# 상세 출력
./run_individual_test.sh socket_adapter_real --verbose

# 도움말
./run_individual_test.sh --help

# 테스트 목록 조회
./run_individual_test.sh --list
```

### 커버리지 측정

```bash
# 전체 커버리지
./run_coverage_test.sh

# 특정 파일 커버리지
./run_coverage_test.sh --file extsock_json_parser

# HTML 리포트 생성
./run_coverage_test.sh --html

# 상세 출력
./run_coverage_test.sh --verbose
```

## 📊 테스트 현황

### 전체 통계
- **총 테스트 수**: 82개
- **성공률**: 100% (82/82)
- **라인 커버리지**: 67% (548/809 라인)
- **브랜치 커버리지**: 38% (304/792 브랜치)

### Phase 1 달성 사항 (80개 테스트)
- **Week 1**: 플러그인 생명주기 + 에러 처리 (11개)
- **Week 2**: JSON 파싱 완성 (22개)
- **Week 3**: 소켓 통신 완성 (23개)
- **Week 4**: usecase 통합 완성 (24개)

## 📁 디렉토리 구조

```
test/
├── README.md                  # 이 파일
├── run_working_tests.sh       # 전체 테스트 실행
├── run_individual_test.sh     # 개별 테스트 실행
├── run_coverage_test.sh       # 커버리지 측정
├── run_phase1_tests.sh        # Phase 1 테스트 실행
├── quick_test.sh              # 빠른 테스트
├── docs/                      # 문서 디렉토리
│   ├── HOW_TO_TEST.md        # 상세 테스트 가이드
│   ├── PHASE1_WEEK1_REPORT.md # Week 1 리포트
│   ├── PHASE1_WEEK2_REPORT.md # Week 2 리포트
│   ├── PHASE1_WEEK3_REPORT.md # Week 3 리포트
│   ├── PHASE1_WEEK4_REPORT.md # Week 4 리포트
│   └── TEST_COMPLETION_REPORT.md # 테스트 완료 리포트
├── unit/                      # 단위 테스트
│   ├── core/                 # 핵심 기능 테스트
│   ├── adapters/             # 어댑터 테스트
│   ├── usecases/             # usecase 테스트
│   └── domain/               # 도메인 테스트
├── integration/              # 통합 테스트
├── phase1/                   # Phase 1 테스트 결과
│   ├── week1/
│   ├── week2/
│   ├── week3/
│   └── week4/
└── Makefile.tests            # 테스트 빌드 설정
```

## 🛠️ 개발자 가이드

### 새 테스트 추가

1. **단위 테스트 추가**
   ```bash
   # unit/ 하위에 테스트 파일 생성
   # test_[module_name].c 형식 사용
   ```

2. **run_individual_test.sh에 등록**
   ```bash
   # 스크립트의 test_configs 배열에 추가
   ```

3. **테스트 실행 확인**
   ```bash
   ./run_individual_test.sh your_new_test
   ```

### 디버깅

1. **컴파일 에러**
   ```bash
   # verbose 모드로 실행
   ./run_individual_test.sh test_name --verbose
   ```

2. **런타임 에러**
   ```bash
   # gdb로 디버깅
   gdb ./test_binary
   ```

3. **메모리 리크**
   ```bash
   # valgrind 사용
   valgrind --leak-check=full ./test_binary
   ```

## 🔧 의존성

### 필수 패키지
- `libcheck-dev` - Check 유닛 테스트 프레임워크
- `libcjson-dev` - JSON 라이브러리
- `gcovr` - 커버리지 리포트 (선택사항)

### 설치 (Ubuntu/Debian)
```bash
sudo apt-get install libcheck-dev libcjson-dev gcovr
```

## 🚀 CI/CD 통합

### GitHub Actions
```yaml
- name: Run Tests
  run: |
    cd src/libcharon/plugins/extsock/test
    ./run_working_tests.sh
    ./run_coverage_test.sh
```

### 자동화된 테스트
```bash
# 빠른 검증
./quick_test.sh

# 풀 테스트 (CI 환경)
./run_working_tests.sh --ci
```

## 📖 상세 문서

- **[HOW_TO_TEST.md](docs/HOW_TO_TEST.md)** - 상세한 테스트 실행 가이드
- **[Phase 1 리포트](docs/)** - 각 Week별 개발 및 테스트 리포트
- **[TEST_COMPLETION_REPORT.md](docs/TEST_COMPLETION_REPORT.md)** - 전체 테스트 완료 보고서

## 💡 팁

1. **빠른 테스트**: 개발 중에는 `quick_test.sh` 사용
2. **커버리지 확인**: 주기적으로 `run_coverage_test.sh` 실행
3. **Phase 1 테스트**: 종합적인 검증이 필요할 때 사용
4. **문서 참조**: 자세한 내용은 `docs/` 디렉토리 참조

## 🤝 기여하기

1. 새로운 기능 추가 시 해당 테스트도 함께 작성
2. 커버리지 감소 없이 코드 변경
3. 모든 테스트 통과 확인 후 커밋
4. 테스트 관련 문서 업데이트

---

**strongSwan extsock Plugin Test Suite** - 안정적이고 신뢰할 수 있는 VPN 플러그인을 위한 종합 테스트 🛡️ 