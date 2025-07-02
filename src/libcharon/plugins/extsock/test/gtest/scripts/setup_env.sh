#!/bin/bash
# setup_env.sh - Google Test 환경 설정 스크립트

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GTEST_DIR="$SCRIPT_DIR/../"
PROJECT_ROOT="$SCRIPT_DIR/../../../"
EXTSOCK_ROOT="$PROJECT_ROOT"
STRONGSWAN_ROOT="$PROJECT_ROOT/../../../.."

echo "=== extsock Google Test Environment Setup ==="

# 1. 디렉토리 정보 출력
echo "Directory structure:"
echo "  GTEST_DIR: $GTEST_DIR"
echo "  EXTSOCK_ROOT: $EXTSOCK_ROOT"
echo "  STRONGSWAN_ROOT: $STRONGSWAN_ROOT"

# 2. 필요한 패키지 확인
echo ""
echo "Checking required packages..."

check_package() {
    if pkg-config --exists "$1" 2>/dev/null; then
        echo "  ✓ $1: $(pkg-config --modversion $1)"
        return 0
    else
        echo "  ✗ $1: not found"
        return 1
    fi
}

MISSING_PACKAGES=()

# Google Test 확인
if ! check_package "gtest"; then
    if [ -f "/usr/lib/$(uname -m)-linux-gnu/libgtest.a" ] || [ -f "/usr/lib/libgtest.a" ]; then
        echo "  ✓ gtest: found in system libraries"
    else
        MISSING_PACKAGES+=("libgtest-dev")
    fi
fi

# Google Mock 확인
if [ -f "/usr/lib/$(uname -m)-linux-gnu/libgmock.a" ] || [ -f "/usr/lib/libgmock.a" ] || [ -f "/usr/include/gmock/gmock.h" ]; then
    echo "  ✓ gmock: found in system"
else
    MISSING_PACKAGES+=("libgmock-dev")
fi

# cJSON 확인
if ! check_package "libcjson"; then
    if [ -f "/usr/include/cjson/cJSON.h" ]; then
        echo "  ✓ cjson: found in system headers"
    else
        MISSING_PACKAGES+=("libcjson-dev")
    fi
fi

# 누락된 패키지가 있으면 설치 안내
if [ ${#MISSING_PACKAGES[@]} -gt 0 ]; then
    echo ""
    echo "Missing packages detected. Please install:"
    echo "  sudo apt install ${MISSING_PACKAGES[*]}"
    exit 1
fi

# 3. 환경 변수 설정 파일 생성
echo ""
echo "Creating environment configuration..."

cat > "$GTEST_DIR/gtest_env.sh" << 'EOF'
#!/bin/bash
# Google Test 환경 변수 설정

# 컴파일러 설정
export CXX=g++
export CC=gcc

# Google Test/Mock 라이브러리 경로
if [ -d "/usr/lib/$(uname -m)-linux-gnu" ]; then
    export GTEST_LIB_PATH="/usr/lib/$(uname -m)-linux-gnu"
else
    export GTEST_LIB_PATH="/usr/lib"
fi

export GTEST_INCLUDE_PATH="/usr/include"

# strongSwan 라이브러리 경로 설정
STRONGSWAN_BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../../../../" && pwd)"
export STRONGSWAN_LIB_PATH="$STRONGSWAN_BASE/src/libstrongswan/.libs:$STRONGSWAN_BASE/src/libcharon/.libs"

# 런타임 라이브러리 경로
export LD_LIBRARY_PATH="$STRONGSWAN_LIB_PATH:$GTEST_LIB_PATH:$LD_LIBRARY_PATH"

# 디버그 정보
export GTEST_COLOR=1
export GTEST_PRINT_TIME=1

echo "Google Test environment loaded"
echo "  GTEST_LIB_PATH: $GTEST_LIB_PATH"
echo "  STRONGSWAN_LIB_PATH: $STRONGSWAN_LIB_PATH"
EOF

chmod +x "$GTEST_DIR/gtest_env.sh"

# 4. 의존성 확인 스크립트 생성
cat > "$GTEST_DIR/scripts/check_deps.sh" << 'EOF'
#!/bin/bash
# check_deps.sh - 의존성 확인 스크립트

echo "=== Dependency Check ==="

# strongSwan 라이브러리 확인
STRONGSWAN_BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../../../../" && pwd)"

echo "Checking strongSwan libraries:"
if [ -f "$STRONGSWAN_BASE/src/libstrongswan/.libs/libstrongswan.so" ]; then
    echo "  ✓ libstrongswan.so found"
else
    echo "  ✗ libstrongswan.so not found"
    echo "    Please build strongSwan first: ./autogen.sh && ./configure && make"
fi

if [ -f "$STRONGSWAN_BASE/src/libcharon/.libs/libcharon.so" ]; then
    echo "  ✓ libcharon.so found"
else
    echo "  ✗ libcharon.so not found"
    echo "    Please build strongSwan first: ./autogen.sh && ./configure && make"
fi

# extsock 소스 파일 확인
EXTSOCK_BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo ""
echo "Checking extsock source files:"

check_source() {
    if [ -f "$1" ]; then
        echo "  ✓ $(basename $1)"
        return 0
    else
        echo "  ✗ $(basename $1) not found"
        return 1
    fi
}

MISSING_SOURCES=0

check_source "$EXTSOCK_BASE/../common/extsock_errors.c" || ((MISSING_SOURCES++))
check_source "$EXTSOCK_BASE/../adapters/json/extsock_json_parser.c" || ((MISSING_SOURCES++))
check_source "$EXTSOCK_BASE/../adapters/socket/extsock_socket_adapter.c" || ((MISSING_SOURCES++))
check_source "$EXTSOCK_BASE/../usecases/extsock_config_usecase.c" || ((MISSING_SOURCES++))

if [ $MISSING_SOURCES -gt 0 ]; then
    echo ""
    echo "Warning: $MISSING_SOURCES source files are missing"
fi

echo ""
echo "Dependency check completed"
EOF

chmod +x "$GTEST_DIR/scripts/check_deps.sh"

# 5. 환경 설정 테스트
echo ""
echo "Testing environment setup..."
source "$GTEST_DIR/gtest_env.sh"

# 6. strongSwan 라이브러리 확인
echo ""
echo "Checking strongSwan build status..."
"$GTEST_DIR/scripts/check_deps.sh"

echo ""
echo "✓ Google Test environment setup completed!"
echo ""
echo "To use this environment:"
echo "  cd $GTEST_DIR"
echo "  source gtest_env.sh"
echo "  make"
echo "" 