# strongSwan extsock Plugin - Google Test Rules & Commands

이 파일은 extsock plugin의 Google Test 실행을 위한 모든 규칙, 명령어, 설정을 포함합니다.

## 📋 Quick Commands Reference

### 기본 테스트 실행
```bash
# 작업 디렉토리로 이동
cd src/libcharon/plugins/extsock/test/gtest

# 모든 테스트 빌드 및 실행
make clean && make && make test

# 특정 테스트 스위트만 실행
./build/simple_mock_test
./build/final_integration_test
./build/real_plugin_tests
```

### Phase별 Real Plugin Tests ⭐ **UPDATED (2025-08-26)**
```bash
# Phase 1 실행 (Mock 환경, 인프라 테스트)
cd build
cmake .. -DREAL_PLUGIN_PHASE=1 && make real_plugin_tests && ./real_plugin_tests

# Phase 2 실행 ✅ **WORKING** (실제 strongSwan 연동)  
cmake .. -DREAL_PLUGIN_PHASE=2 && make real_plugin_tests && ./real_plugin_tests
# 결과: 4/5 테스트 통과 (strongSwan API Integration 성공)

# Phase 3 실행 🚧 **READY** (완전한 End-to-End 테스트)
cmake .. -DREAL_PLUGIN_PHASE=3 && make real_plugin_tests && ./real_plugin_tests
```

### ✅ Phase 2 검증된 명령어 (2025-08-26 23:37)
```bash
# Phase 2 성공적 실행 시퀀스
cd src/libcharon/plugins/extsock/test/gtest
mkdir -p build && cd build
cmake .. -DREAL_PLUGIN_PHASE=2
make clean && make real_plugin_tests
./real_plugin_tests

# 예상 결과:
# ✅ Phase 2 Execution Result: SUCCESS  
# 🎉 All tests passed!
# ✅ Tests: 4/5 PASSED, 1 SKIPPED
```

### 테스트 필터링 및 디버깅
```bash
# 특정 테스트만 실행
./real_plugin_tests --gtest_filter="*Environment*"
./real_plugin_tests --gtest_filter="RealExtsockErrorsTest.*"

# 상세한 출력
./real_plugin_tests --gtest_brief=1
./real_plugin_tests --gtest_verbose

# 실패한 테스트만 재실행
./real_plugin_tests --gtest_repeat=-1 --gtest_break_on_failure
```

## 🏗️ Build System Rules

### CMake 설정
```bash
# 기본 빌드 (Phase 1)
mkdir -p build && cd build
cmake .. && make

# Phase별 빌드
cmake .. -DREAL_PLUGIN_PHASE=1  # Mock 환경
cmake .. -DREAL_PLUGIN_PHASE=2  # Real strongSwan 연동
cmake .. -DREAL_PLUGIN_PHASE=3  # Full integration

# 디버그 빌드
cmake .. -DCMAKE_BUILD_TYPE=Debug -DREAL_PLUGIN_PHASE=1
cmake .. -DCMAKE_BUILD_TYPE=Release -DREAL_PLUGIN_PHASE=2
```

### Makefile 규칙
```bash
# 전체 빌드
make all

# 개별 타겟 빌드
make simple_mock_test
make final_integration_test  
make real_plugin_tests

# 정리
make clean
make distclean
```

## 🧪 Test Categories & Organization

### 1. Pure Tests (의존성 없음)
```bash
# 위치: src/unit/
# 실행: ./build/simple_mock_test
# 특징: strongSwan API 없이 순수 로직만 테스트
```

### 2. Mock Tests (Mock API 사용)
```bash
# 위치: src/mocks/, src/integration/
# 실행: ./build/final_integration_test
# 특징: strongSwan Mock을 통한 통합 테스트
```

### 3. Real Plugin Tests (실제 API 연동)
```bash
# 위치: src/real_integration/
# 실행: ./build/real_plugin_tests
# 특징: 실제 libstrongswan-extsock.la와 연동
```

## 🔧 Environment & Configuration

### 필수 환경 변수
```bash
export STRONGSWAN_TEST_MODE=1
export USE_REAL_PLUGIN=1
export REAL_PLUGIN_PHASE=1  # 1, 2, 3 중 선택
```

### 라이브러리 경로 설정
```bash
# extsock plugin 라이브러리 자동 탐지 경로:
# 1. ../../libstrongswan-extsock.la
# 2. ../../../libstrongswan-extsock.la  
# 3. ../../.libs/libstrongswan-extsock.so
```

### strongSwan 의존성 확인
```bash
# strongSwan 설치 확인
pkg-config --exists strongswan && echo "strongSwan found" || echo "strongSwan not found"

# 플러그인 디렉토리 확인
pkg-config --variable=plugindir strongswan
```

## 📊 Test Execution Patterns

### 개발 단계별 실행 순서
```bash
# 1. 개발 초기: Pure Tests 먼저 실행
cd build && ./simple_mock_test

# 2. 통합 테스트: Mock Tests 실행
./final_integration_test

# 3. 실제 환경: Real Plugin Tests 실행 (Phase 1)
./real_plugin_tests

# 4. 완전 검증: Real Plugin Tests (Phase 2+)
cmake .. -DREAL_PLUGIN_PHASE=2 && make && ./real_plugin_tests
```

### CI/CD 파이프라인용 명령어
```bash
#!/bin/bash
# 전체 테스트 스위트 실행 (CI용)

set -e  # 실패 시 중단

cd src/libcharon/plugins/extsock/test/gtest

# 1단계: Pure/Mock 테스트
make clean && make
./build/simple_mock_test --gtest_output=xml:pure_test_results.xml
./build/final_integration_test --gtest_output=xml:integration_test_results.xml

# 2단계: Real Plugin Tests (Phase 1)
./build/real_plugin_tests --gtest_output=xml:real_phase1_results.xml

# 3단계: Real Plugin Tests (Phase 2) - 선택적
if [ "$ENABLE_PHASE2" = "1" ]; then
    cmake build -DREAL_PLUGIN_PHASE=2 && make -C build
    ./build/real_plugin_tests --gtest_output=xml:real_phase2_results.xml
fi

echo "✅ All tests completed successfully"
```

## 🐛 Debugging & Troubleshooting

### 일반적인 문제 해결
```bash
# 1. 빌드 실패
make clean && rm -rf build/* && cmake .. && make

# 2. 라이브러리 찾을 수 없음
find ../.. -name "*.la" -o -name "*.so" | grep extsock

# 3. strongSwan 초기화 실패 (Phase 2+)
export LD_LIBRARY_PATH=/usr/local/lib/ipsec:$LD_LIBRARY_PATH
ldd ./build/real_plugin_tests

# 4. 테스트 환경 문제
./build/real_plugin_tests --gtest_list_tests
```

### 로그 및 디버그 출력
```bash
# 상세한 Phase 로그
./build/real_plugin_tests 2>&1 | grep -E "(Phase|ERROR|WARNING|SUCCESS)"

# 메모리 검사 (Valgrind)
valgrind --leak-check=full ./build/real_plugin_tests

# GDB 디버깅
gdb --args ./build/real_plugin_tests --gtest_filter="*EnvironmentCheck*"
```

## 📈 Performance & Coverage

### 테스트 성능 측정
```bash
# 실행 시간 측정
time ./build/real_plugin_tests

# 각 테스트별 시간
./build/real_plugin_tests --gtest_print_time=1
```

### 코드 커버리지 (gcov)
```bash
# 커버리지 빌드
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON
make

# 커버리지 실행
./build/real_plugin_tests
gcov src/real_integration/*.cpp

# 커버리지 리포트
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## 🚀 Advanced Usage

### 병렬 테스트 실행
```bash
# 여러 Phase 동시 실행 (별도 터미널)
# Terminal 1:
./build/real_plugin_tests --gtest_filter="*Phase1*"

# Terminal 2: 
cmake .. -DREAL_PLUGIN_PHASE=2 && make && ./build/real_plugin_tests --gtest_filter="*Phase2*"
```

### 테스트 데이터 및 결과 관리
```bash
# 테스트 결과 저장
./build/real_plugin_tests --gtest_output=json:test_results.json

# 테스트 비교 (이전 결과와)
diff previous_results.json test_results.json

# 실패한 테스트 목록 추출
./build/real_plugin_tests --gtest_list_tests | grep DISABLED
```

## 📝 Development Guidelines

### 새로운 테스트 추가 시
1. 적절한 카테고리 선택 (Pure/Mock/Real)
2. Phase별 조건부 컴파일 사용
3. 명확한 테스트 이름 규칙 준수
4. Setup/TearDown에서 환경 검증

### 코드 스타일
- Google Test 표준 매크로 사용
- REAL_PLUGIN_* 매크로로 일관된 로깅
- Phase별 조건부 실행 구현
- 실패 시 명확한 오류 메시지 제공

---

## 📚 Related Documentation

- [REAL_PLUGIN_TEST_DESIGN.md](docs/REAL_PLUGIN_TEST_DESIGN.md) - 설계 상세사항
- [REAL_PLUGIN_IMPLEMENTATION_PLAN.md](docs/REAL_PLUGIN_IMPLEMENTATION_PLAN.md) - 구현 계획
- [QUICK_START_GUIDE.md](QUICK_START_GUIDE.md) - 빠른 시작 가이드
- [README.md](README.md) - 전체 프로젝트 개요

---

## 🚀 Phase 3 준비사항 (Next Steps)

### Phase 3 실행을 위한 요구사항
```bash
# Phase 3 환경 체크
cd src/libcharon/plugins/extsock/test/gtest/build

# Phase 3 빌드 테스트
cmake .. -DREAL_PLUGIN_PHASE=3 && make real_plugin_tests

# Phase 3 실행 (실제 extsock 함수 호출)
./real_plugin_tests --gtest_filter="*RealExtsockFunction*"
```

### Phase 3 새로운 테스트 타입들
- `RealExtsockFunctionTest`: 실제 extsock plugin 함수 직접 호출
- `RealEndToEndTest`: 완전한 strongSwan + extsock 통합 시나리오
- `RealPluginLifecycleTest`: Plugin 생명주기 전체 테스트

### 성능 및 안정성 테스트
```bash
# 스트레스 테스트 (Phase 3에서 활성화)
./real_plugin_tests --gtest_repeat=100 --gtest_filter="*Performance*"

# 메모리 누수 체크
valgrind --tool=memcheck --leak-check=full ./real_plugin_tests
```

---

**마지막 업데이트**: 2025-08-26 23:43 ✅ **Phase 2 완료**  
**관리자**: Claude Assistant  
**현재 상태**: Phase 3 Ready 🚧  
**버전**: 2.0