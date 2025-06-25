#!/bin/bash

# strongSwan extsock Plugin 개별 테스트 실행 스크립트
# 사용법: ./run_individual_test.sh [테스트명] [옵션]
# 예시: ./run_individual_test.sh simple_unit --verbose

set -e

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
CFLAGS="-std=c99 -Wall -Wextra -g -include ../../../../../config.h -D_GNU_SOURCE -DHAVE_CONFIG_H"
INCLUDES="-I.. -I../../../../../src/libstrongswan -I../../../../../src/libcharon -I/usr/include/cjson -DUNIT_TEST"
LIBS="-lcheck -lsubunit -lm -lrt -lpthread -lcjson"
COMMON_SRC="../common/extsock_errors.c"

# 라이브러리 경로 설정
export LD_LIBRARY_PATH="../../../../../src/libstrongswan/.libs:../../../../../src/libcharon/.libs:$LD_LIBRARY_PATH"

# 사용 가능한 테스트 목록
declare -A TESTS=(
    ["simple_unit"]="unit/test_simple_unit.c:Simple Unit Tests:7"
    ["json_parser_simple"]="unit/test_json_parser_simple.c:JSON Parser Simple Tests:7"
    ["socket_adapter_simple"]="unit/test_socket_adapter_simple.c:Socket Adapter Simple Tests:6"
    ["error_scenarios"]="unit/test_error_scenarios.c:Error Scenarios:4"
    ["plugin_simple"]="unit/test_plugin_simple.c:Plugin Simple Tests:8"
    ["json_parser_real"]="unit/test_json_parser_real.c:JSON Parser Real Implementation Tests:8"
    ["socket_adapter_real"]="unit/test_socket_adapter_real.c:Socket Adapter Real Implementation Tests:9"
    ["config_usecase_real"]="unit/test_config_usecase_real.c:Config Usecase Real Implementation Tests:8"
    ["event_usecase_real"]="unit/test_event_usecase_real.c:Event Usecase Real Implementation Tests:8"
    ["domain_entity_real"]="unit/test_domain_entity_real.c:Domain Entity Real Implementation Tests:8"
    ["complete_workflow"]="integration/test_complete_workflow.c:Complete Workflow Integration Tests:9"
)

# 도움말 출력
show_help() {
    echo -e "${BLUE}strongSwan extsock Plugin 개별 테스트 실행기${NC}"
    echo ""
    echo -e "${CYAN}사용법:${NC}"
    echo "  $0 [테스트명] [옵션]"
    echo ""
    echo -e "${CYAN}사용 가능한 테스트:${NC}"
    for test_name in "${!TESTS[@]}"; do
        IFS=':' read -r file desc checks <<< "${TESTS[$test_name]}"
        printf "  ${GREEN}%-20s${NC} - %s (%s checks)\n" "$test_name" "$desc" "$checks"
    done | sort
    echo ""
    echo -e "${CYAN}옵션:${NC}"
    echo "  --verbose, -v    자세한 출력"
    echo "  --help, -h       이 도움말 표시"
    echo "  --list, -l       사용 가능한 테스트 목록만 표시"
    echo ""
    echo -e "${CYAN}예시:${NC}"
    echo "  $0 simple_unit              # 간단한 단위 테스트 실행"
    echo "  $0 json_parser_real --verbose # JSON 파서 실제 테스트를 자세히 실행"
    echo "  $0 --list                   # 테스트 목록만 표시"
}

# 테스트 목록만 표시
show_list() {
    echo -e "${BLUE}사용 가능한 테스트 목록:${NC}"
    echo ""
    for test_name in "${!TESTS[@]}"; do
        IFS=':' read -r file desc checks <<< "${TESTS[$test_name]}"
        printf "  ${GREEN}%-20s${NC} - %s (%s checks)\n" "$test_name" "$desc" "$checks"
    done | sort
}

# 개별 테스트 실행
run_test() {
    local test_name="$1"
    local verbose="$2"
    
    if [[ ! "${TESTS[$test_name]}" ]]; then
        echo -e "${RED}❌ 테스트 '$test_name'을 찾을 수 없습니다.${NC}"
        echo ""
        echo -e "${YELLOW}사용 가능한 테스트:${NC}"
        show_list
        return 1
    fi
    
    IFS=':' read -r source_file test_desc expected_checks <<< "${TESTS[$test_name]}"
    binary_name="test_${test_name}"
    
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}개별 테스트 실행: ${test_desc}${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    
    # 빌드
    echo -e "${YELLOW}Phase 1: 테스트 빌드 중...${NC}"
    if [[ "$verbose" == "true" ]]; then
        echo -e "${CYAN}컴파일 명령어:${NC}"
        echo "$CC $CFLAGS $INCLUDES \"$source_file\" $COMMON_SRC -o \"$binary_name\" $LIBS"
        echo ""
    fi
    
    if $CC $CFLAGS $INCLUDES "$source_file" $COMMON_SRC -o "$binary_name" $LIBS 2>/dev/null; then
        echo -e "${GREEN}✓ 빌드 성공: $binary_name${NC}"
    else
        echo -e "${RED}✗ 빌드 실패: $binary_name${NC}"
        if [[ "$verbose" == "true" ]]; then
            echo ""
            echo -e "${YELLOW}자세한 에러 정보:${NC}"
            $CC $CFLAGS $INCLUDES "$source_file" $COMMON_SRC -o "$binary_name" $LIBS
        fi
        return 1
    fi
    echo ""
    
    # 실행
    echo -e "${YELLOW}Phase 2: 테스트 실행 중...${NC}"
    if [[ "$verbose" == "true" ]]; then
        echo -e "${CYAN}실행 명령어: ./$binary_name${NC}"
        echo ""
    fi
    
    if output=$(timeout 30s ./"$binary_name" 2>&1); then
        # 결과 파싱
        if echo "$output" | grep -q "100%:"; then
            actual_checks=$(echo "$output" | grep "100%:" | sed 's/.*Checks: \([0-9]*\).*/\1/')
            echo -e "${GREEN}✓ 테스트 성공: $actual_checks/$expected_checks 체크 통과${NC}"
            
            if [[ "$verbose" == "true" ]]; then
                echo ""
                echo -e "${CYAN}상세 결과:${NC}"
                echo "$output"
            fi
        else
            echo -e "${RED}✗ 테스트 실패: 예상치 못한 출력${NC}"
            if [[ "$verbose" == "true" ]]; then
                echo ""
                echo -e "${YELLOW}출력 내용:${NC}"
                echo "$output"
            fi
            return 1
        fi
    else
        echo -e "${RED}✗ 테스트 실행 실패 또는 타임아웃${NC}"
        if [[ "$verbose" == "true" ]]; then
            echo ""
            echo -e "${YELLOW}에러 출력:${NC}"
            echo "$output"
        fi
        return 1
    fi
    echo ""
    
    # 정리
    echo -e "${YELLOW}Phase 3: 정리 중...${NC}"
    if [[ -f "$binary_name" ]]; then
        rm -f "$binary_name"
        echo -e "${GREEN}✓ 임시 파일 정리 완료${NC}"
    fi
    echo ""
    
    echo -e "${GREEN}🎉 테스트 '${test_name}' 완료!${NC}"
    return 0
}

# 파라미터 파싱
VERBOSE=false
TEST_NAME=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --help|-h)
            show_help
            exit 0
            ;;
        --list|-l)
            show_list
            exit 0
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        *)
            if [[ -z "$TEST_NAME" ]]; then
                TEST_NAME="$1"
            else
                echo -e "${RED}❌ 알 수 없는 옵션: $1${NC}"
                echo ""
                show_help
                exit 1
            fi
            shift
            ;;
    esac
done

# 테스트명이 없으면 도움말 표시
if [[ -z "$TEST_NAME" ]]; then
    show_help
    exit 1
fi

# 테스트 실행
run_test "$TEST_NAME" "$VERBOSE" 