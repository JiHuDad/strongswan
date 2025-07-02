#!/bin/bash
# run_tests.sh - Google Test 실행 스크립트

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GTEST_DIR="$SCRIPT_DIR/.."

echo "🧪 Starting Google Test Execution"

# 1. 환경 로드
if [ -f "$GTEST_DIR/gtest_env.sh" ]; then
    echo "📋 Loading environment..."
    source "$GTEST_DIR/gtest_env.sh"
else
    echo "❌ Environment file not found. Run setup_env.sh first."
    exit 1
fi

# 2. 디렉토리로 이동
cd "$GTEST_DIR"

# 3. 빌드 상태 확인
if [ ! -d "build/bin" ] || [ -z "$(ls -A build/bin 2>/dev/null)" ]; then
    echo "❌ No test binaries found. Please build first:"
    echo "   ./scripts/build.sh"
    exit 1
fi

# 4. 테스트 실행 모드 결정
TEST_MODE=""
SPECIFIC_TEST=""
VERBOSE=false
COVERAGE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --coverage)
            COVERAGE=true
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --test=*)
            SPECIFIC_TEST="${1#*=}"
            shift
            ;;
        week1)
            TEST_MODE="week1"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [week1] [--test=test_name] [--verbose] [--coverage]"
            exit 1
            ;;
    esac
done

# 5. 환경 변수 설정
export LD_LIBRARY_PATH="$STRONGSWAN_LIB_PATH:$GTEST_LIB_PATH:$LD_LIBRARY_PATH"

# 6. 테스트 실행
echo "🎯 Running tests..."

if [ -n "$SPECIFIC_TEST" ]; then
    # 특정 테스트 실행
    echo "Running specific test: $SPECIFIC_TEST"
    make "test-$SPECIFIC_TEST"
elif [ "$TEST_MODE" = "week1" ]; then
    # Week 1 테스트만 실행
    echo "Running Week 1 tests..."
    
    WEEK1_TESTS=("test_common" "test_error_handling" "test_types_basic")
    PASSED=0
    FAILED=0
    
    for test_name in "${WEEK1_TESTS[@]}"; do
        echo ""
        echo "🧪 Running $test_name..."
        echo "=================================="
        
        test_binary="build/bin/$test_name"
        
        if [ -x "$test_binary" ]; then
            if $VERBOSE; then
                if $test_binary --gtest_color=yes; then
                    echo "✅ PASSED: $test_name"
                    ((PASSED++))
                else
                    echo "❌ FAILED: $test_name"
                    ((FAILED++))
                fi
            else
                if $test_binary --gtest_color=yes --gtest_brief=yes 2>/dev/null; then
                    echo "✅ PASSED: $test_name"
                    ((PASSED++))
                else
                    echo "❌ FAILED: $test_name"
                    ((FAILED++))
                fi
            fi
        else
            echo "❌ BINARY NOT FOUND: $test_name"
            ((FAILED++))
        fi
    done
    
    # Week 1 결과 요약
    echo ""
    echo "📊 Week 1 Test Summary"
    echo "======================"
    echo "Total Tests: $((PASSED + FAILED))"
    echo "✅ Passed: $PASSED"
    echo "❌ Failed: $FAILED"
    
    if [ $FAILED -eq 0 ]; then
        echo "🎉 All Week 1 tests passed!"
        TEST_SUCCESS=true
    else
        echo "💥 Some Week 1 tests failed!"
        TEST_SUCCESS=false
    fi
else
    # 모든 테스트 실행
    echo "Running all tests..."
    if make test; then
        TEST_SUCCESS=true
    else
        TEST_SUCCESS=false
    fi
fi

# 7. 커버리지 생성 (요청된 경우)
if [ "$COVERAGE" = true ]; then
    echo ""
    echo "📈 Generating coverage report..."
    make coverage
    
    if [ -f "build/reports/coverage.html" ]; then
        echo "📊 Coverage report generated: build/reports/coverage.html"
        echo "   Open it in a browser to view detailed coverage information"
    fi
fi

# 8. 테스트 리포트 확인
echo ""
echo "📋 Test Reports:"
if [ -d "build/reports" ]; then
    ls -la build/reports/ | grep -E "\.(xml|html)$" || echo "  No report files found"
else
    echo "  No reports directory found"
fi

# 9. 최종 결과
echo ""
if [ "$TEST_SUCCESS" = true ]; then
    echo "🎉 Test execution completed successfully!"
    exit 0
else
    echo "💥 Test execution failed!"
    exit 1
fi 