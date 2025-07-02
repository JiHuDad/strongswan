#!/bin/bash
# build.sh - Google Test 빌드 스크립트

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GTEST_DIR="$SCRIPT_DIR/.."

echo "🚀 Starting Google Test Build Process"

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

# 3. clean build를 원하는 경우
if [ "$1" = "clean" ]; then
    echo "🧹 Cleaning previous build..."
    make clean
fi

# 4. 환경 검증
echo "🔍 Checking environment..."
make env-check

# 5. 디버그 정보 출력 (옵션)
if [ "$1" = "debug" ] || [ "$2" = "debug" ]; then
    echo "🐛 Debug information:"
    make debug-info
fi

# 6. Week 1 빌드
echo "🔨 Building Week 1 tests..."
if make week1; then
    echo "✅ Week 1 build completed successfully!"
else
    echo "❌ Week 1 build failed!"
    exit 1
fi

# 7. 빌드 결과 확인
echo ""
echo "📊 Build Summary:"
echo "=================="

if [ -d build/bin ]; then
    echo "Built test binaries:"
    ls -la build/bin/ | grep -E "test_" || echo "  No test binaries found"
else
    echo "  No build/bin directory found"
fi

echo ""
echo "🎉 Build process completed!"
echo ""
echo "Next steps:"
echo "  1. Run tests: make test"
echo "  2. Run specific test: make test-common"
echo "  3. Generate coverage: make coverage" 