#!/bin/bash
#
# Google Test Suite 실행 스크립트
# Google Test 마이그레이션 프로젝트용
#

set -e

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# 로그 파일
LOG_FILE="gtest_suite_$(date +%Y%m%d_%H%M%S).log"

echo "=======================================================" | tee -a "$LOG_FILE"
echo "🧪 extsock Plugin Google Test Suite" | tee -a "$LOG_FILE"  
echo "=======================================================" | tee -a "$LOG_FILE"
echo "시작 시간: $(date)" | tee -a "$LOG_FILE"
echo "작업 디렉토리: $(pwd)" | tee -a "$LOG_FILE"
echo "로그 파일: $LOG_FILE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# 의존성 확인
echo -e "${CYAN}🔍 의존성 확인${NC}" | tee -a "$LOG_FILE"

if ! command -v g++ &> /dev/null; then
    echo -e "${RED}❌ g++ 컴파일러를 찾을 수 없습니다${NC}" | tee -a "$LOG_FILE"
    exit 1
fi

if ! pkg-config --exists gtest 2>/dev/null; then
    echo -e "${YELLOW}⚠️  libgtest-dev가 설치되지 않았습니다${NC}" | tee -a "$LOG_FILE"
    echo "설치 방법:" | tee -a "$LOG_FILE" 
    echo "  Ubuntu/Debian: sudo apt-get install libgtest-dev libgmock-dev" | tee -a "$LOG_FILE"
    echo "  macOS: brew install googletest" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    echo "의존성 자동 설치를 시도합니다..." | tee -a "$LOG_FILE"
    make -f Makefile.gtest install-deps | tee -a "$LOG_FILE"
fi

echo "✅ 의존성 확인 완료" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# 빌드 시스템 선택
BUILD_SYSTEM="makefile"  # 기본값

if [ "$1" = "cmake" ] || [ "$1" = "CMAKE" ]; then
    BUILD_SYSTEM="cmake"
elif [ "$1" = "makefile" ] || [ "$1" = "MAKEFILE" ] || [ "$1" = "make" ]; then
    BUILD_SYSTEM="makefile"
fi

echo -e "${BLUE}🔨 빌드 시스템: $BUILD_SYSTEM${NC}" | tee -a "$LOG_FILE"

# 결과 추적 변수
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
BUILD_ERRORS=0

# Makefile 기반 빌드 및 실행
run_makefile_tests() {
    echo -e "${PURPLE}=== Makefile 기반 빌드 및 테스트 ===${NC}" | tee -a "$LOG_FILE"
    
    # 의존성 확인
    if make -f Makefile.gtest check-deps >> "$LOG_FILE" 2>&1; then
        echo "✅ Makefile 의존성 확인 완료" | tee -a "$LOG_FILE"
    else
        echo -e "${RED}❌ Makefile 의존성 확인 실패${NC}" | tee -a "$LOG_FILE"
        BUILD_ERRORS=$((BUILD_ERRORS + 1))
        return 1
    fi
    
    # 이전 빌드 정리
    echo "빌드 정리 중..." | tee -a "$LOG_FILE"
    make -f Makefile.gtest clean >> "$LOG_FILE" 2>&1
    
    # 빌드
    echo "Google Test 프로젝트 빌드 중..." | tee -a "$LOG_FILE"
    if make -f Makefile.gtest all >> "$LOG_FILE" 2>&1; then
        echo "✅ 빌드 성공" | tee -a "$LOG_FILE"
    else
        echo -e "${RED}❌ 빌드 실패${NC}" | tee -a "$LOG_FILE"
        BUILD_ERRORS=$((BUILD_ERRORS + 1))
        return 1
    fi
    
    # 테스트 실행
    echo "Google Test 실행 중..." | tee -a "$LOG_FILE"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if make -f Makefile.gtest test >> "$LOG_FILE" 2>&1; then
        echo -e "${GREEN}✅ 모든 테스트 통과${NC}" | tee -a "$LOG_FILE"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ 일부 테스트 실패${NC}" | tee -a "$LOG_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# CMake 기반 빌드 및 실행
run_cmake_tests() {
    echo -e "${PURPLE}=== CMake 기반 빌드 및 테스트 ===${NC}" | tee -a "$LOG_FILE"
    
    # CMake 확인
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}❌ CMake를 찾을 수 없습니다${NC}" | tee -a "$LOG_FILE"
        echo "CMake 설치 후 다시 시도하세요" | tee -a "$LOG_FILE"
        BUILD_ERRORS=$((BUILD_ERRORS + 1))
        return 1
    fi
    
    # 빌드 디렉토리 생성
    mkdir -p build
    cd build
    
    # CMake 설정
    echo "CMake 설정 중..." | tee -a "../$LOG_FILE"
    if cmake .. >> "../$LOG_FILE" 2>&1; then
        echo "✅ CMake 설정 완료" | tee -a "../$LOG_FILE"
    else
        echo -e "${RED}❌ CMake 설정 실패${NC}" | tee -a "../$LOG_FILE"
        BUILD_ERRORS=$((BUILD_ERRORS + 1))
        cd ..
        return 1
    fi
    
    # 빌드
    echo "CMake 빌드 중..." | tee -a "../$LOG_FILE"
    if make >> "../$LOG_FILE" 2>&1; then
        echo "✅ 빌드 성공" | tee -a "../$LOG_FILE"
    else
        echo -e "${RED}❌ 빌드 실패${NC}" | tee -a "../$LOG_FILE"
        BUILD_ERRORS=$((BUILD_ERRORS + 1))
        cd ..
        return 1
    fi
    
    # 테스트 실행
    echo "CTest 실행 중..." | tee -a "../$LOG_FILE"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if ctest --verbose >> "../$LOG_FILE" 2>&1; then
        echo -e "${GREEN}✅ 모든 테스트 통과${NC}" | tee -a "../$LOG_FILE"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ 일부 테스트 실패${NC}" | tee -a "../$LOG_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    cd ..
}

# Hello World 테스트만 실행 (빠른 검증)
run_hello_test_only() {
    echo -e "${BLUE}=== Hello World 테스트 실행 ===${NC}" | tee -a "$LOG_FILE"
    
    if make -f Makefile.gtest run-hello >> "$LOG_FILE" 2>&1; then
        echo -e "${GREEN}✅ Hello Google Test 성공${NC}" | tee -a "$LOG_FILE"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ Hello Google Test 실패${NC}" | tee -a "$LOG_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

# 메인 실행 로직
main() {
    case "$BUILD_SYSTEM" in
        "cmake")
            run_cmake_tests
            ;;
        "makefile")
            run_makefile_tests
            ;;
        *)
            echo -e "${RED}❌ 알 수 없는 빌드 시스템: $BUILD_SYSTEM${NC}" | tee -a "$LOG_FILE"
            exit 1
            ;;
    esac
}

# 특별 모드 처리
if [ "$1" = "hello" ] || [ "$1" = "quick" ]; then
    echo "빠른 검증 모드: Hello World 테스트만 실행" | tee -a "$LOG_FILE"
    run_hello_test_only
else
    main
fi

# XML 출력 생성 (옵션)
if [ "$2" = "xml" ] || [ "$XML_OUTPUT" = "true" ]; then
    echo -e "${CYAN}📄 XML 출력 생성${NC}" | tee -a "$LOG_FILE"
    if [ "$BUILD_SYSTEM" = "makefile" ]; then
        make -f Makefile.gtest test-xml >> "$LOG_FILE" 2>&1 || true
    fi
fi

# 최종 결과 리포트
echo "" | tee -a "$LOG_FILE"
echo "=======================================================" | tee -a "$LOG_FILE"
echo "🎯 Google Test Suite 실행 완료" | tee -a "$LOG_FILE"
echo "=======================================================" | tee -a "$LOG_FILE"

echo "실행 결과:" | tee -a "$LOG_FILE"
echo "  총 테스트 그룹: $TOTAL_TESTS" | tee -a "$LOG_FILE"
echo -e "  ${GREEN}✅ 성공: $PASSED_TESTS${NC}" | tee -a "$LOG_FILE"
echo -e "  ${RED}❌ 실패: $FAILED_TESTS${NC}" | tee -a "$LOG_FILE"
echo -e "  ${YELLOW}🔧 빌드 오류: $BUILD_ERRORS${NC}" | tee -a "$LOG_FILE"

if [ $TOTAL_TESTS -gt 0 ]; then
    success_rate=$(( (PASSED_TESTS * 100) / TOTAL_TESTS ))
    echo "  성공률: $success_rate%" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "종료 시간: $(date)" | tee -a "$LOG_FILE"
echo "로그 위치: $LOG_FILE" | tee -a "$LOG_FILE"

# 종료 상태
if [ $FAILED_TESTS -gt 0 ] || [ $BUILD_ERRORS -gt 0 ]; then
    echo -e "${RED}❌ 테스트 스위트 실패${NC}" | tee -a "$LOG_FILE"
    exit 1
else
    echo -e "${GREEN}🎉 테스트 스위트 성공${NC}" | tee -a "$LOG_FILE"
    exit 0
fi