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
