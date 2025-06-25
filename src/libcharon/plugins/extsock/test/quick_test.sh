#!/bin/bash

# strongSwan extsock Plugin 빠른 테스트 스크립트
# 사용법: ./quick_test.sh

# 컴파일 설정
CFLAGS="-std=c99 -Wall -Wextra -g -include ../../../../../config.h -D_GNU_SOURCE -DHAVE_CONFIG_H"
INCLUDES="-I.. -I../../../../../src/libstrongswan -I../../../../../src/libcharon -I/usr/include/cjson -DUNIT_TEST"
LIBS="-lcheck -lsubunit -lm -lrt -lpthread -lcjson"
COMMON_SRC="../common/extsock_errors.c"

# 라이브러리 경로 설정
export LD_LIBRARY_PATH="../../../../../src/libstrongswan/.libs:../../../../../src/libcharon/.libs:$LD_LIBRARY_PATH"

echo "strongSwan extsock Plugin 빠른 테스트"
echo "====================================="

# 핵심 테스트만 실행
declare -a QUICK_TESTS=(
    "unit/test_simple_unit.c:Simple Unit:7"
    "unit/test_json_parser_simple.c:JSON Parser:7"
    "unit/test_socket_adapter_simple.c:Socket Adapter:6"
    "unit/test_error_scenarios.c:Error Scenarios:4"
)

total_checks=0
passed_checks=0

for test_info in "${QUICK_TESTS[@]}"; do
    IFS=':' read -r source_file test_name expected_checks <<< "$test_info"
    binary_name=$(basename "$source_file" .c)
    
    echo -n "Testing $test_name... "
    
    # 빌드 및 실행
    if gcc $CFLAGS $INCLUDES "$source_file" $COMMON_SRC -o "$binary_name" $LIBS 2>/dev/null; then
        if output=$(./"$binary_name" 2>/dev/null); then
            if echo "$output" | grep -q "100%:"; then
                actual_checks=$(echo "$output" | grep "100%:" | sed 's/.*Checks: \([0-9]*\).*/\1/')
                echo "✓ ($actual_checks/$expected_checks)"
                passed_checks=$((passed_checks + actual_checks))
            else
                echo "✗ (실패)"
            fi
        else
            echo "✗ (실행 오류)"
        fi
        rm -f "$binary_name" 2>/dev/null
    else
        echo "✗ (빌드 실패)"
    fi
    
    total_checks=$((total_checks + expected_checks))
done

echo ""
echo "결과: $passed_checks/$total_checks 체크 통과"
if [ $passed_checks -eq $total_checks ]; then
    echo "🎉 모든 핵심 테스트 성공!"
    exit 0
else
    echo "❌ 일부 테스트 실패"
    exit 1
fi 