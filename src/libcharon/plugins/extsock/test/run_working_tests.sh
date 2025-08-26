#!/bin/bash

# strongSwan extsock Plugin 테스트 자동화 스크립트
# 사용법: ./run_working_tests.sh

set -e  # 오류 시 스크립트 중단

# 색깔 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 컴파일 설정
CC="gcc"
CFLAGS="-std=c99 -Wall -Wextra -g -include ../../../../../config.h -D_GNU_SOURCE -DHAVE_CONFIG_H -Wno-macro-redefined"
INCLUDES="-I.. -I../../../../../src/libstrongswan -I../../../../../src/libcharon -I/usr/include/cjson -DUNIT_TEST"
LIBS="-lcheck -lsubunit -lm -lrt -lpthread -lcjson"
COMMON_SRC="../common/extsock_errors.c"

# 라이브러리 경로 설정
export LD_LIBRARY_PATH="../../../../../src/libstrongswan/.libs:../../../../../src/libcharon/.libs:$LD_LIBRARY_PATH"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}strongSwan extsock Plugin 테스트 실행${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 테스트 목록 (파일명, 출력명, 체크 수)
declare -a TESTS=(
    "unit/test_simple_unit.c:test_simple_unit:Simple Unit Tests:7"
    "unit/test_json_parser_simple.c:test_json_parser_simple:JSON Parser Simple Tests:7"
    "unit/test_socket_adapter_simple.c:test_socket_adapter_simple:Socket Adapter Simple Tests:6"
    "unit/test_error_scenarios.c:test_error_scenarios:Error Scenarios:4"
    "unit/test_plugin_simple.c:test_plugin_simple:Plugin Simple Tests:8"
    "unit/test_json_parser_real.c:test_json_parser_real:JSON Parser Real Implementation Tests:8"
    "unit/test_socket_adapter_real.c:test_socket_adapter_real:Socket Adapter Real Implementation Tests:9"
    "unit/test_config_usecase_real.c:test_config_usecase_real:Config Usecase Real Implementation Tests:8"
    "unit/test_event_usecase_real.c:test_event_usecase_real:Event Usecase Real Implementation Tests:8"
    "unit/test_domain_entity_real.c:test_domain_entity_real:Domain Entity Real Implementation Tests:8"
    "integration/test_complete_workflow.c:test_complete_workflow:Complete Workflow Integration Tests:9"
)

total_tests=0
passed_tests=0
total_checks=0
passed_checks=0
failed_tests=()

echo -e "${CYAN}Phase 1: 테스트 빌드 중...${NC}"
echo ""

# 테스트 빌드 및 실행
for test_info in "${TESTS[@]}"; do
    IFS=':' read -r source_file binary_name test_name expected_checks <<< "$test_info"
    
    echo -e "${YELLOW}빌드 중: ${test_name}${NC}"
    
    # 빌드
    if $CC $CFLAGS $INCLUDES "$source_file" $COMMON_SRC -o "$binary_name" $LIBS 2>/dev/null; then
        echo -e "${GREEN}✓ 빌드 성공: $binary_name${NC}"
    else
        echo -e "${RED}✗ 빌드 실패: $binary_name${NC}"
        failed_tests+=("$test_name (빌드 실패)")
        continue
    fi
    
    total_tests=$((total_tests + 1))
done

echo ""
echo -e "${CYAN}Phase 2: 테스트 실행 중...${NC}"
echo ""

# 테스트 실행
for test_info in "${TESTS[@]}"; do
    IFS=':' read -r source_file binary_name test_name expected_checks <<< "$test_info"
    
    if [ ! -f "$binary_name" ]; then
        continue
    fi
    
    echo -e "${PURPLE}실행 중: ${test_name}${NC}"
    
    # 테스트 실행
    if output=$(./"$binary_name" 2>&1); then
        # 결과 파싱
        if echo "$output" | grep -q "100%:"; then
            actual_checks=$(echo "$output" | grep "100%:" | sed 's/.*Checks: \([0-9]*\).*/\1/')
            echo -e "${GREEN}✓ 성공: $actual_checks/$expected_checks 체크 통과${NC}"
            passed_tests=$((passed_tests + 1))
            passed_checks=$((passed_checks + actual_checks))
            total_checks=$((total_checks + expected_checks))
        else
            echo -e "${RED}✗ 실패: 예상치 못한 출력${NC}"
            failed_tests+=("$test_name (실행 실패)")
            total_checks=$((total_checks + expected_checks))
        fi
    else
        echo -e "${RED}✗ 실패: 실행 오류${NC}"
        echo "$output"
        failed_tests+=("$test_name (실행 오류)")
        total_checks=$((total_checks + expected_checks))
    fi
    echo ""
done

# 정리
echo -e "${CYAN}Phase 3: 정리 중...${NC}"
for test_info in "${TESTS[@]}"; do
    IFS=':' read -r source_file binary_name test_name expected_checks <<< "$test_info"
    if [ -f "$binary_name" ]; then
        rm -f "$binary_name"
    fi
done

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}테스트 결과 요약${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 결과 요약
if [ ${#failed_tests[@]} -eq 0 ]; then
    echo -e "${GREEN}🎉 모든 테스트 성공!${NC}"
else
    echo -e "${RED}⚠️  일부 테스트 실패${NC}"
    echo ""
    echo -e "${RED}실패한 테스트들:${NC}"
    for failed in "${failed_tests[@]}"; do
        echo -e "${RED}  - $failed${NC}"
    done
fi

echo ""
echo -e "${CYAN}통계:${NC}"
echo -e "  총 테스트 스위트: $total_tests"
echo -e "  성공한 테스트: ${GREEN}$passed_tests${NC}"
echo -e "  실패한 테스트: ${RED}$((total_tests - passed_tests))${NC}"
echo -e "  총 체크 수: $total_checks"
echo -e "  성공한 체크: ${GREEN}$passed_checks${NC}"
if [ $total_checks -gt 0 ]; then
    success_rate=$((passed_checks * 100 / total_checks))
    echo -e "  성공률: ${GREEN}$success_rate%${NC}"
fi

echo ""
if [ $passed_tests -eq $total_tests ]; then
    echo -e "${GREEN}✨ strongSwan extsock Plugin 테스트 완료! ✨${NC}"
    exit 0
else
    echo -e "${RED}❌ 일부 테스트가 실패했습니다.${NC}"
    exit 1
fi 