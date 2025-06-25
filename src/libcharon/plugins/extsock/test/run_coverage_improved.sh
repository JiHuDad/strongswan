#!/bin/bash

# 🧪 개선된 strongSwan extsock Plugin 커버리지 측정 스크립트
# 실제 함수 호출을 통한 정확한 커버리지 측정

set -e

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m'

# 컴파일러 설정
CC="gcc"
CFLAGS="-Wall -Wextra -g -O0 --coverage -fprofile-arcs -ftest-coverage"
INCLUDES="-I. -I../domain -I../usecases -I../adapters -I../../ -I../../../ -I../../../../ -I../../../../../libstrongswan"
LIBS="-lcjson -lcheck -lm -lrt -lsubunit -lgcov --coverage -lpthread"

# 실제 함수 호출 테스트 목록 (Phase 기반)
declare -A REAL_FUNCTION_TESTS=(
    ["phase1_minimal"]="test_phases/phase1_minimal/test_minimal_real.c"
    ["phase2_source_inclusion"]="test_phases/phase2_source_inclusion/test_source_inclusion.c"
    ["phase3_linked_source"]="test_phases/phase3_linked/test_linked_source.c:test_phases/phase3_linked/standalone_extsock_errors.c"
    ["phase4_json_parser"]="test_phases/phase4_json_parser/test_json_parser_standalone.c"
    ["phase5_socket_adapter"]="test_phases/phase5_socket_adapter/test_socket_adapter_standalone.c"
)

print_header() {
    echo -e "${BLUE}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC} 🎯 ${CYAN}Improved Coverage Analysis - Real Functions${NC} ${BLUE}║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════════════════╝${NC}"
    echo ""
}

# 이전 커버리지 데이터 정리
cleanup_previous_data() {
    echo -e "${CYAN}🧹 Cleaning previous coverage data...${NC}"
    find . -name "*.gcda" -delete 2>/dev/null || true
    find . -name "*.gcno" -delete 2>/dev/null || true
    find . -name "*.gcov" -delete 2>/dev/null || true
    find . -name "test_coverage_*" -type f -executable -delete 2>/dev/null || true
    echo -e "${GREEN}✓ Previous data cleaned${NC}"
    echo ""
}

# 개별 테스트 실행 및 커버리지 수집
run_coverage_test() {
    local test_name="$1"
    local test_definition="$2"
    local test_number="$3"
    local total_tests="$4"
    
    echo -e "${YELLOW}[${test_number}/${total_tests}] ${MAGENTA}Testing: ${test_name}${NC}"
    
    # 소스 파일들 파싱
    IFS=':' read -r -a source_files <<< "$test_definition"
    test_file="${source_files[0]}"
    impl_files="${source_files[@]:1}"
    
    # 파일 존재 확인
    if [[ ! -f "$test_file" ]]; then
        echo -e "  ${RED}✗ Test file not found: $test_file${NC}"
        return 1
    fi
    
    binary_name="test_coverage_${test_name}"
    
    # 컴파일
    echo -e "  ${CYAN}🔨 Compiling with coverage...${NC}"
    if $CC $CFLAGS $INCLUDES "$test_file" $impl_files -o "$binary_name" $LIBS 2>/dev/null; then
        # 실행
        echo -e "  ${CYAN}🚀 Running tests...${NC}"
        if timeout 60s ./"$binary_name" >/dev/null 2>&1; then
            echo -e "  ${GREEN}✓ SUCCESS${NC}"
            
            # 즉시 커버리지 데이터 처리
            echo -e "  ${CYAN}📊 Processing coverage data...${NC}"
            local gcda_files=$(find . -name "${binary_name}*.gcda" 2>/dev/null)
            if [[ -n "$gcda_files" ]]; then
                local gcda_count=$(echo "$gcda_files" | wc -l)
                echo -e "  ${GREEN}→ ${gcda_count} coverage files found${NC}"
                
                # 각 gcda 파일에 대해 gcov 실행
                for gcda_file in $gcda_files; do
                    if gcov "$gcda_file" >/dev/null 2>&1; then
                        echo -e "  ${GREEN}→ Coverage processed: $(basename "$gcda_file")${NC}"
                    fi
                done
            else
                echo -e "  ${YELLOW}→ No coverage files generated${NC}"
            fi
            return 0
        else
            echo -e "  ${RED}✗ TEST EXECUTION FAILED${NC}"
            return 1
        fi
    else
        echo -e "  ${RED}✗ COMPILATION FAILED${NC}"
        return 1
    fi
}

# 모든 Phase 테스트 실행
run_all_phase_tests() {
    echo -e "${BLUE}🎯 Running Real Function Tests (Phase 1-5)${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════${NC}"
    echo ""
    
    local total_tests=${#REAL_FUNCTION_TESTS[@]}
    local passed_tests=0
    local test_number=0
    
    for test_name in "${!REAL_FUNCTION_TESTS[@]}"; do
        ((test_number++))
        if run_coverage_test "$test_name" "${REAL_FUNCTION_TESTS[$test_name]}" "$test_number" "$total_tests"; then
            ((passed_tests++))
        fi
        echo ""
    done
    
    echo -e "${CYAN}Phase Tests Complete: ${passed_tests}/${total_tests} successful${NC}"
    echo ""
    return $((total_tests - passed_tests))
}

# 통합 커버리지 리포트 생성
generate_integrated_report() {
    echo -e "${BLUE}📈 Generating Integrated Coverage Report${NC}"
    echo -e "${BLUE}══════════════════════════════════════════${NC}"
    echo ""
    
    # 모든 gcov 파일 수집
    local gcov_files=$(find . -name "*.gcov" | wc -l)
    if [[ $gcov_files -eq 0 ]]; then
        echo -e "${RED}❌ No coverage files found!${NC}"
        return 1
    fi
    
    echo -e "${GREEN}📊 Found ${gcov_files} coverage files${NC}"
    
    # 커버리지 요약 생성
    echo -e "${CYAN}🔍 Analyzing coverage data...${NC}"
    
    # 실제 소스 파일별 커버리지 분석
    echo ""
    echo -e "${YELLOW}📋 Coverage Summary by Source File:${NC}"
    echo "=================================================="
    
    # 각 주요 소스 파일별 커버리지 확인
    for source_pattern in "extsock_errors" "extsock_json_parser" "extsock_socket_adapter" "extsock_config" "test_"; do
        local matching_gcov=$(find . -name "*${source_pattern}*.gcov" | head -1)
        if [[ -n "$matching_gcov" ]]; then
            echo ""
            echo -e "${CYAN}📁 ${source_pattern}*:${NC}"
            local coverage_info=$(grep "Lines executed:" "$matching_gcov" | head -1)
            if [[ -n "$coverage_info" ]]; then
                echo "  $coverage_info"
                
                # 실행된 라인 수 추출
                local executed_lines=$(echo "$coverage_info" | grep -o '[0-9]\+' | head -1)
                local total_lines=$(echo "$coverage_info" | grep -o 'of [0-9]\+' | grep -o '[0-9]\+')
                
                if [[ "$executed_lines" -gt 0 ]]; then
                    echo -e "  ${GREEN}✓ Real functions executed!${NC}"
                else
                    echo -e "  ${YELLOW}⚠ No real functions executed (Mock only)${NC}"
                fi
            else
                echo -e "  ${RED}✗ No coverage data${NC}"
            fi
        fi
    done
    
    echo ""
    echo "=================================================="
    
    # HTML 리포트 생성 (lcov 사용 가능한 경우)
    if command -v lcov >/dev/null 2>&1; then
        echo -e "${CYAN}📊 Generating HTML report...${NC}"
        mkdir -p coverage_improved
        
        if lcov --capture --directory . --output-file coverage_improved/coverage.info 2>/dev/null; then
            if genhtml coverage_improved/coverage.info --output-directory coverage_improved 2>/dev/null; then
                echo -e "${GREEN}✓ HTML report generated: coverage_improved/index.html${NC}"
            fi
        fi
    fi
    
    echo ""
}

# 정리 작업
cleanup_build_files() {
    echo -e "${CYAN}🧹 Organizing build artifacts...${NC}"
    
    # build_artifacts 디렉토리로 이동
    mkdir -p build_artifacts/improved_coverage
    find . -maxdepth 1 \( -name "*.gcda" -o -name "*.gcno" -o -name "test_coverage_*" ! -name "*.c" \) \
        -exec mv {} build_artifacts/improved_coverage/ \; 2>/dev/null || true
    
    echo -e "${GREEN}✓ Build artifacts organized${NC}"
}

# 메인 실행 함수
main() {
    print_header
    
    cleanup_previous_data
    
    run_all_phase_tests
    phase_result=$?
    
    generate_integrated_report
    
    cleanup_build_files
    
    echo ""
    if [[ $phase_result -eq 0 ]]; then
        echo -e "${GREEN}🎉 All real function tests completed successfully!${NC}"
        echo -e "${GREEN}📊 Improved coverage analysis complete${NC}"
        echo -e "${CYAN}📁 Results available in coverage_improved/index.html${NC}"
    else
        echo -e "${YELLOW}⚠ Some tests failed, but coverage data collected${NC}"
        echo -e "${CYAN}📁 Check coverage_improved/ for detailed results${NC}"
    fi
    
    echo ""
    echo -e "${BLUE}💡 Key difference: This script focuses on REAL function execution${NC}"
    echo -e "${BLUE}   instead of Mock functions for accurate coverage measurement${NC}"
}

# 스크립트 실행
main "$@" 