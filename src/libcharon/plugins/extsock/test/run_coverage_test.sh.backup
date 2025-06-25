#!/bin/bash

# strongSwan extsock Plugin 코드 커버리지 측정 스크립트
# 사용법: ./run_coverage_test.sh [옵션]

# set -e는 커버리지 측정에서 문제를 일으킬 수 있으므로 제거

# 색깔 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 커버리지 디렉토리 설정
COVERAGE_DIR="coverage_output"
SOURCE_DIR=".."

# 컴파일 설정 (커버리지 플래그 추가)
CC="gcc"
CFLAGS="-std=c99 -Wall -Wextra -g --coverage -fprofile-arcs -ftest-coverage -include ../../../../../config.h -D_GNU_SOURCE -DHAVE_CONFIG_H"
INCLUDES="-I.. -I../../../../../src/libstrongswan -I../../../../../src/libcharon -I/usr/include/cjson -DUNIT_TEST"
LIBS="-lcheck -lsubunit -lm -lrt -lpthread -lcjson -lgcov"
COMMON_SRC="../common/extsock_errors.c"

# 라이브러리 경로 설정
export LD_LIBRARY_PATH="../../../../../src/libstrongswan/.libs:../../../../../src/libcharon/.libs:$LD_LIBRARY_PATH"

# 테스트 목록 (실제 소스 코드를 포함하는 테스트들만)
declare -A COVERAGE_TESTS=(
    ["simple_unit"]="unit/test_simple_unit.c:../common/extsock_errors.c"
    ["json_parser_simple"]="unit/test_json_parser_simple.c:../common/extsock_errors.c"
    ["json_parser_real"]="unit/test_json_parser_real.c:../common/extsock_errors.c:../adapters/json/extsock_json_parser.c"
    ["socket_adapter_simple"]="unit/test_socket_adapter_simple.c:../common/extsock_errors.c"
    ["socket_adapter_real"]="unit/test_socket_adapter_real.c:../common/extsock_errors.c:../adapters/socket/extsock_socket_adapter.c"
    ["config_usecase_real"]="unit/test_config_usecase_real.c:../common/extsock_errors.c:../usecases/extsock_config_usecase.c"
    ["event_usecase_real"]="unit/test_event_usecase_real.c:../common/extsock_errors.c:../usecases/extsock_event_usecase.c"
    ["domain_entity_real"]="unit/test_domain_entity_real.c:../common/extsock_errors.c:../domain/extsock_config_entity.c"
)

# 도움말 출력
show_help() {
    echo -e "${BLUE}strongSwan extsock Plugin 코드 커버리지 측정기${NC}"
    echo ""
    echo -e "${CYAN}사용법:${NC}"
    echo "  $0 [옵션]"
    echo ""
    echo -e "${CYAN}옵션:${NC}"
    echo "  --html, -h       HTML 리포트 생성"
    echo "  --text, -t       텍스트 리포트만 출력"
    echo "  --clean, -c      이전 커버리지 데이터 정리"
    echo "  --help           이 도움말 표시"
    echo ""
    echo -e "${CYAN}예시:${NC}"
    echo "  $0               # 기본 커버리지 측정"
    echo "  $0 --html        # HTML 리포트와 함께"
    echo "  $0 --clean       # 정리 후 측정"
}

# 이전 커버리지 데이터 정리
clean_coverage() {
    echo -e "${YELLOW}이전 커버리지 데이터 정리 중...${NC}"
    
    # .gcno, .gcda 파일들 제거
    find . -name "*.gcno" -delete 2>/dev/null || true
    find . -name "*.gcda" -delete 2>/dev/null || true
    find .. -name "*.gcno" -delete 2>/dev/null || true
    find .. -name "*.gcda" -delete 2>/dev/null || true
    
    # 커버리지 디렉토리 제거
    rm -rf "$COVERAGE_DIR" 2>/dev/null || true
    
    # 테스트 바이너리들 제거
    rm -f test_coverage_* 2>/dev/null || true
    
    echo -e "${GREEN}✓ 정리 완료${NC}"
}

# 커버리지와 함께 테스트 실행
run_coverage_tests() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}코드 커버리지 측정 시작${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    
    local total_tests=0
    local passed_tests=0
    
    for test_name in "${!COVERAGE_TESTS[@]}"; do
        echo -e "${YELLOW}[$((++total_tests))/${#COVERAGE_TESTS[@]}] 테스트: ${test_name}${NC}"
        
        # 소스 파일들 파싱
        IFS=':' read -r -a source_files <<< "${COVERAGE_TESTS[$test_name]}"
        test_file="${source_files[0]}"
        impl_files="${source_files[@]:1}"
        
        # 파일 존재 확인
        if [[ ! -f "$test_file" ]]; then
            echo -e "  ${RED}✗ 테스트 파일 없음: $test_file${NC}"
            continue
        fi
        
        # 구현 파일들 존재 확인
        missing_files=()
        for impl_file in $impl_files; do
            if [[ ! -f "$impl_file" ]]; then
                missing_files+=("$impl_file")
            fi
        done
        
        if [[ ${#missing_files[@]} -gt 0 ]]; then
            echo -e "  ${YELLOW}⚠ 일부 구현 파일 없음: ${missing_files[*]}${NC}"
            echo -e "  ${CYAN}→ 테스트 파일만으로 진행${NC}"
            impl_files=""
        fi
        
        binary_name="test_coverage_${test_name}"
        
        # 컴파일 (에러 출력 표시)
        echo -e "  ${CYAN}컴파일 중...${NC}"
        if $CC $CFLAGS $INCLUDES "$test_file" $impl_files -o "$binary_name" $LIBS 2>&1; then
            # 실행 (타임아웃 증가, 에러 출력 표시)
            echo -e "  ${CYAN}실행 중...${NC}"
            if timeout 60s ./"$binary_name" 2>&1; then
                echo -e "  ${GREEN}✓ 성공${NC}"
                ((passed_tests++))
            else
                echo -e "  ${RED}✗ 실행 실패 (종료 코드: $?)${NC}"
            fi
        else
            echo -e "  ${RED}✗ 컴파일 실패${NC}"
        fi
        
        # 바이너리 정리
        rm -f "$binary_name" 2>/dev/null || true
        echo ""
    done
    
    echo ""
    echo -e "${CYAN}테스트 실행 완료: ${passed_tests}/${total_tests} 성공${NC}"
    echo ""
}

# 커버리지 리포트 생성
generate_coverage_report() {
    local format="$1"
    
    echo -e "${YELLOW}커버리지 리포트 생성 중...${NC}"
    
    # 커버리지 디렉토리 생성
    mkdir -p "$COVERAGE_DIR"
    
    # gcovr을 사용하여 리포트 생성
    if [[ "$format" == "html" ]]; then
        echo -e "${CYAN}HTML 리포트 생성 중...${NC}"
        
        gcovr \
            --root "$SOURCE_DIR" \
            --exclude-unreachable-branches \
            --exclude-throw-branches \
            --exclude '.*/test/.*' \
            --exclude '.*/tests/.*' \
            --html-details "$COVERAGE_DIR/coverage_report.html" \
            --html-title "strongSwan extsock Plugin Coverage Report" \
            . \
            2>/dev/null || true
            
        if [[ -f "$COVERAGE_DIR/coverage_report.html" ]]; then
            echo -e "${GREEN}✓ HTML 리포트 생성 완료: $COVERAGE_DIR/coverage_report.html${NC}"
        else
            echo -e "${RED}✗ HTML 리포트 생성 실패${NC}"
        fi
    fi
    
    # 텍스트 요약 출력
    echo -e "${CYAN}커버리지 요약:${NC}"
    echo ""
    
    if command -v gcovr >/dev/null 2>&1; then
        gcovr \
            --root "$SOURCE_DIR" \
            --exclude-unreachable-branches \
            --exclude-throw-branches \
            --exclude '.*/test/.*' \
            --exclude '.*/tests/.*' \
            . \
            2>/dev/null || echo "gcovr 실행 중 오류 발생"
    else
        echo "gcovr이 설치되지 않음"
    fi
    
    echo ""
}

# 파일별 상세 커버리지 표시
show_detailed_coverage() {
    echo -e "${CYAN}파일별 상세 커버리지:${NC}"
    echo ""
    
    # 주요 소스 파일들의 커버리지 확인
    local source_files=(
        "../common/extsock_errors.c"
        "../adapters/json/extsock_json_parser.c"
        "../adapters/socket/extsock_socket_adapter.c"
        "../usecases/extsock_config_usecase.c"
        "../usecases/extsock_event_usecase.c"
        "../domain/extsock_config_entity.c"
    )
    
    for file in "${source_files[@]}"; do
        if [[ -f "$file" ]]; then
            basename_file=$(basename "$file")
            echo -e "${YELLOW}📁 $basename_file${NC}"
            
            # gcov로 개별 파일 분석
            gcov_file=$(echo "$basename_file" | sed 's/\.c$/.c.gcov/')
            
            if gcov "$file" >/dev/null 2>&1 && [[ -f "$gcov_file" ]]; then
                # 라인 커버리지 계산
                total_lines=$(grep -c "^ *[0-9#-].*:" "$gcov_file" || echo "0")
                covered_lines=$(grep -c "^ *[1-9].*:" "$gcov_file" || echo "0")
                
                if [[ $total_lines -gt 0 ]]; then
                    coverage_percent=$(( covered_lines * 100 / total_lines ))
                    echo -e "  라인 커버리지: ${covered_lines}/${total_lines} (${coverage_percent}%)"
                else
                    echo -e "  ${RED}커버리지 데이터 없음${NC}"
                fi
                
                # gcov 파일 정리
                rm -f "$gcov_file" 2>/dev/null || true
            else
                echo -e "  ${RED}커버리지 데이터 생성 실패${NC}"
            fi
            echo ""
        fi
    done
}

# 파라미터 파싱
GENERATE_HTML=false
TEXT_ONLY=false
CLEAN_FIRST=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --html|-h)
            GENERATE_HTML=true
            shift
            ;;
        --text|-t)
            TEXT_ONLY=true
            shift
            ;;
        --clean|-c)
            CLEAN_FIRST=true
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}❌ 알 수 없는 옵션: $1${NC}"
            echo ""
            show_help
            exit 1
            ;;
    esac
done

# 메인 실행
echo -e "${BLUE}strongSwan extsock Plugin 코드 커버리지 측정${NC}"
echo ""

# 정리 (필요시)
if [[ "$CLEAN_FIRST" == "true" ]]; then
    clean_coverage
    echo ""
fi

# 커버리지 테스트 실행
run_coverage_tests

# 리포트 생성
if [[ "$TEXT_ONLY" != "true" ]]; then
    if [[ "$GENERATE_HTML" == "true" ]]; then
        generate_coverage_report "html"
    else
        generate_coverage_report "text"
    fi
    
    # 상세 커버리지 표시
    show_detailed_coverage
fi

# 정리
echo -e "${YELLOW}임시 파일 정리 중...${NC}"
rm -f test_coverage_* 2>/dev/null || true
find . -name "*.gcov" -delete 2>/dev/null || true

echo -e "${GREEN}🎉 커버리지 측정 완료!${NC}"

if [[ "$GENERATE_HTML" == "true" ]] && [[ -f "$COVERAGE_DIR/coverage_report.html" ]]; then
    echo ""
    echo -e "${CYAN}HTML 리포트 확인:${NC}"
    echo "  file://$(pwd)/$COVERAGE_DIR/coverage_report.html"
fi 