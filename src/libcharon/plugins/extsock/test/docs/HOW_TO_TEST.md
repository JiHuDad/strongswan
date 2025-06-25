# strongSwan extsock Plugin 테스트 가이드

## 개요
strongSwan extsock 플러그인의 테스트를 위한 완전 자동화된 스크립트들입니다.

## 사용 방법

### 1. 전체 테스트 실행 (권장)
```bash
./run_working_tests.sh
```

**특징:**
- 11개 테스트 스위트, 총 82개 체크 수행
- 자동 빌드, 실행, 정리
- 컬러풀한 상세 로그
- 완전한 테스트 커버리지

**실행 시간:** 약 15-20초

### 2. 빠른 테스트 (개발 중)
```bash
./quick_test.sh
```

**특징:**
- 핵심 4개 테스트, 24개 체크만 수행
- 빠른 검증용
- 간단한 출력

**실행 시간:** 약 5초

## 테스트 포함 내용

### Unit Tests (단위 테스트)
1. **Simple Unit Tests** (7 체크)
   - 기본 문자열 처리
   - 메모리 관리
   - JSON 기본 파싱

2. **JSON Parser Simple Tests** (7 체크)
   - JSON 구조 파싱
   - IPsec 설정 검증
   - 트래픽 셀렉터 검증

3. **Socket Adapter Simple Tests** (6 체크)
   - 소켓 생성/연결
   - 데이터 전송
   - 연결 상태 관리

4. **Error Scenarios** (4 체크)
   - JSON 파싱 에러 처리
   - NULL 포인터 안전성
   - 리소스 정리

5. **Plugin Simple Tests** (8 체크)
   - 플러그인 인터페이스
   - Mock 컴포넌트
   - 의존성 주입

### Real Implementation Tests (실제 구현 테스트)
6. **JSON Parser Real** (8 체크)
   - 복잡한 JSON 파싱
   - 실제 IPsec 설정 처리
   - 중첩 구조 처리

7. **Socket Adapter Real** (9 체크)
   - 실제 소켓 연결 시뮬레이션
   - 비동기 처리
   - 연결 풀 관리

8. **Config Usecase Real** (8 체크)
   - 설정 로딩/검증/적용
   - 설정 변환
   - 캐시 관리

9. **Event Usecase Real** (8 체크)
   - 이벤트 생성/관리
   - 우선순위 처리
   - 필터링

10. **Domain Entity Real** (8 체크)
    - 네트워크 설정
    - 인증 설정
    - 연결 엔티티 라이프사이클

### Integration Tests (통합 테스트)
11. **Complete Workflow** (9 체크)
    - End-to-End 워크플로우
    - 상태 전환 검증
    - 성능 측정

## 필수 요구사항

### 시스템 의존성
```bash
# Ubuntu/Debian
sudo apt-get install libcheck-dev libcjson-dev

# 또는 이미 설치된 경우
# strongSwan 라이브러리가 빌드되어 있어야 함
```

### 환경 설정
- strongSwan config.h 파일 존재: `../../../../../config.h`
- strongSwan 라이브러리 빌드됨: `../../../../../src/libstrongswan/.libs/`
- libcharon 라이브러리 빌드됨: `../../../../../src/libcharon/.libs/`

## 출력 예시

### 성공 시
```
========================================
strongSwan extsock Plugin 테스트 실행
========================================

Phase 1: 테스트 빌드 중...
✓ 빌드 성공: test_simple_unit
...

Phase 2: 테스트 실행 중...
✓ 성공: 7/7 체크 통과
...

Phase 3: 정리 중...

========================================
테스트 결과 요약
========================================

🎉 모든 테스트 성공!

통계:
  총 테스트 스위트: 11
  성공한 테스트: 11
  실패한 테스트: 0
  총 체크 수: 82
  성공한 체크: 82
  성공률: 100%

✨ strongSwan extsock Plugin 테스트 완료! ✨
```

## 문제 해결

### 빌드 실패 시
1. **config.h 없음**
   ```bash
   # strongSwan 루트에서 configure 실행
   cd ../../../../../
   ./configure
   ```

2. **라이브러리 없음**
   ```bash
   # strongSwan 빌드
   make -j$(nproc)
   ```

3. **의존성 없음**
   ```bash
   sudo apt-get install libcheck-dev libcjson-dev
   ```

### 실행 실패 시
- LD_LIBRARY_PATH가 자동 설정되므로 수동 설정 불필요
- 권한 문제: `chmod +x *.sh`

## 개발 워크플로우

### 코드 변경 후 빠른 검증
```bash
./quick_test.sh
```

### 커밋 전 전체 검증
```bash
./run_working_tests.sh
```

### CI/CD 통합
스크립트들은 exit code를 반환하므로 CI/CD에서 직접 사용 가능:
- 성공: exit 0
- 실패: exit 1

## 추가 정보

- **테스트 프레임워크**: Check Framework
- **JSON 라이브러리**: cJSON
- **아키텍처**: Clean Architecture 패턴
- **커버리지**: 100% (모든 주요 컴포넌트)
- **성능**: 모든 테스트 20초 내 완료 