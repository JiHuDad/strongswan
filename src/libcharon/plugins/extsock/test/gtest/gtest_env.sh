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
