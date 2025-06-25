#!/bin/bash

# strongSwan extsock Plugin 최종 커버리지 측정 스크립트

# 색깔 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# 설정
COVERAGE_DIR="coverage_final"
CC="gcc"
CFLAGS="-std=gnu99 -Wall -g --coverage -fprofile-arcs -ftest-coverage -include ../../../../../config.h -D_GNU_SOURCE -DHAVE_CONFIG_H -fgnu89-inline -Wno-unused-function -D__USE_GNU"
INCLUDES="-I.. -I../../../../../src/libstrongswan -I../../../../../src/libcharon -I/usr/include/cjson -DUNIT_TEST"
LIBS="-L../../../../../src/libstrongswan/.libs -L../../../../../src/libcharon/.libs -lstrongswan -lcharon -lcheck -lsubunit -lm -lrt -lpthread -lcjson -lgcov"

# 라이브러리 경로 설정
export LD_LIBRARY_PATH="../../../../../src/libstrongswan/.libs:../../../../../src/libcharon/.libs:$LD_LIBRARY_PATH"

# 정리 함수
cleanup_temp_files() {
    echo -e "${YELLOW}임시 파일 정리 중...${NC}"
    find . -name "test_final_*" -type f -executable -delete 2>/dev/null
    find . -maxdepth 1 -name "*.gcov" -delete 2>/dev/null
    echo -e "${GREEN}✓ 임시 파일 정리 완료${NC}"
}

# 스크립트 종료 시 정리
trap cleanup_temp_files EXIT

echo -e "${BLUE}=== strongSwan extsock Plugin 최종 커버리지 측정 ===${NC}"
echo

# 1. 정리
echo -e "${YELLOW}1. 이전 데이터 정리...${NC}"
rm -rf "$COVERAGE_DIR" 2>/dev/null
find . -name "test_*" -type f -executable -delete 2>/dev/null
find . -name "*.gcda" -delete 2>/dev/null
find . -name "*.gcno" -delete 2>/dev/null
find . -name "*.gcov" -delete 2>/dev/null
mkdir -p "$COVERAGE_DIR"
echo -e "${GREEN}✓ 정리 완료${NC}"

# 2. 테스트 실행
echo -e "${YELLOW}2. 커버리지 테스트 실행...${NC}"

declare -A COVERAGE_TESTS=(
    ["simple"]="unit/test_simple_unit.c:../common/extsock_errors.c"
    ["json_real"]="unit/test_json_parser_real.c:../common/extsock_errors.c:../adapters/json/extsock_json_parser.c"
    ["socket_real"]="unit/test_socket_adapter_real.c:../common/extsock_errors.c:../adapters/socket/extsock_socket_adapter.c"
    ["config_real"]="unit/test_config_usecase_real.c:../common/extsock_errors.c:../usecases/extsock_config_usecase.c:../adapters/strongswan/extsock_strongswan_adapter.c"
    ["event_real"]="unit/test_event_usecase_real.c:../common/extsock_errors.c:../usecases/extsock_event_usecase.c"
    ["domain_real"]="unit/test_domain_entity_real.c:../common/extsock_errors.c:../domain/extsock_config_entity.c"
)

successful_tests=0
total_tests=${#COVERAGE_TESTS[@]}

for test_name in "${!COVERAGE_TESTS[@]}"; do
    echo -e "${CYAN}테스트: ${test_name}${NC}"
    
    IFS=':' read -r -a source_files <<< "${COVERAGE_TESTS[$test_name]}"
    test_file="${source_files[0]}"
    impl_files="${source_files[@]:1}"
    
    binary_name="test_final_${test_name}"
    
    echo -e "  컴파일 중..."
    if $CC $CFLAGS $INCLUDES "$test_file" $impl_files -o "$binary_name" $LIBS 2>/dev/null; then
        echo -e "  실행 중..."
        if ./"$binary_name" >/dev/null 2>&1; then
            echo -e "  ${GREEN}✓ 성공${NC}"
            ((successful_tests++))
            
            gcda_count=$(find . -name "${binary_name}*.gcda" | wc -l)
            echo -e "  → ${gcda_count}개 커버리지 파일 생성"
        else
            echo -e "  ${RED}✗ 실행 실패${NC}"
        fi
    else
        echo -e "  ${RED}✗ 컴파일 실패${NC}"
    fi
    echo
done

echo -e "${CYAN}테스트 실행 완료: ${successful_tests}/${total_tests} 성공${NC}"
echo

# 3. 커버리지 분석
echo -e "${YELLOW}3. 커버리지 분석...${NC}"

echo -e "${CYAN}전체 커버리지 요약:${NC}"
gcovr --root .. --exclude '.*/test/.*' . 2>/dev/null || echo "gcovr 실행 실패"

echo
echo -e "${CYAN}개별 파일 커버리지:${NC}"

coverage_summary=""
impl_files=(
    "../common/extsock_errors.c"
    "../adapters/json/extsock_json_parser.c"
    "../adapters/socket/extsock_socket_adapter.c"
    "../usecases/extsock_config_usecase.c"
    "../usecases/extsock_event_usecase.c"
    "../domain/extsock_config_entity.c"
)

for impl_file in "${impl_files[@]}"; do
    basename_file=$(basename "$impl_file")
    echo -e "${YELLOW}📁 ${basename_file}${NC}"
    
    gcda_files=$(find . -name "*$(basename "$impl_file" .c).gcda" 2>/dev/null)
    
    if [[ -n "$gcda_files" ]]; then
        coverage_found=false
        for gcda_file in $gcda_files; do
            if gcov "$gcda_file" >/dev/null 2>&1; then
                gcov_file=$(basename "$impl_file").gcov
                if [[ -f "$gcov_file" ]]; then
                    # 간단하고 안전한 커버리지 계산
                    total=$(grep -c "^ *[0-9#-].*:" "$gcov_file" 2>/dev/null)
                    covered=$(grep -c "^ *[1-9].*:" "$gcov_file" 2>/dev/null)
                    
                    if [[ "$total" -gt 0 ]] 2>/dev/null; then
                        percent=$(( covered * 100 / total ))
                        echo -e "  라인 커버리지: ${covered}/${total} (${percent}%)"
                        coverage_summary="${coverage_summary}\n  ${basename_file}: ${percent}% (${covered}/${total})"
                    else
                        echo -e "  ${YELLOW}실행되지 않음 (0%)${NC}"
                        coverage_summary="${coverage_summary}\n  ${basename_file}: 0% (실행 안됨)"
                    fi
                    
                    cp "$gcov_file" "$COVERAGE_DIR/" 2>/dev/null
                    rm -f "$gcov_file" 2>/dev/null
                    coverage_found=true
                    break
                fi
            fi
        done
        
        if [[ "$coverage_found" == "false" ]]; then
            echo -e "  ${YELLOW}커버리지 처리 실패${NC}"
            coverage_summary="${coverage_summary}\n  ${basename_file}: 처리 실패"
        fi
    else
        echo -e "  ${RED}커버리지 데이터 없음${NC}"
        coverage_summary="${coverage_summary}\n  ${basename_file}: 데이터 없음"
    fi
done

# 4. HTML 리포트
echo
echo -e "${YELLOW}4. HTML 리포트 생성...${NC}"
if gcovr --root .. --exclude '.*/test/.*' --html-details "$COVERAGE_DIR/coverage_report.html" . 2>/dev/null; then
    echo -e "${GREEN}✓ HTML 리포트: $COVERAGE_DIR/coverage_report.html${NC}"
else
    echo -e "${YELLOW}HTML 리포트 생성 실패${NC}"
fi

# 5. 최종 요약
echo
echo -e "${BLUE}================================================================${NC}"
echo -e "${BLUE}                    커버리지 측정 완료 요약${NC}"
echo -e "${BLUE}================================================================${NC}"
echo -e "📊 성공한 테스트: ${GREEN}${successful_tests}/${total_tests}${NC}"
echo -e "📁 커버리지 데이터: ${CYAN}$COVERAGE_DIR/${NC}"

gcov_files=$(ls "$COVERAGE_DIR"/*.gcov 2>/dev/null | wc -l)
echo -e "🔍 생성된 .gcov 파일: ${GREEN}${gcov_files}개${NC}"

echo
echo -e "${CYAN}📋 파일별 커버리지 요약:${NC}"
echo -e "$coverage_summary"

echo
echo -e "${CYAN}📈 전체 통계:${NC}"
total_gcda=$(find . -name "*.gcda" | wc -l)
echo -e "  생성된 커버리지 파일: ${total_gcda}개"

if [[ -f "$COVERAGE_DIR/coverage_report.html" ]]; then
    echo -e "  HTML 리포트: $COVERAGE_DIR/coverage_report.html"
    echo
    echo -e "${GREEN}🌐 HTML 리포트 보기:${NC}"
    echo -e "  file://$(pwd)/$COVERAGE_DIR/coverage_report.html"
fi

echo
echo -e "${GREEN}🎉 커버리지 분석 완료!${NC}"
echo -e "${YELLOW}💡 팁: Mock 테스트를 실제 구현 호출로 바꾸면 커버리지가 크게 향상됩니다.${NC}"
