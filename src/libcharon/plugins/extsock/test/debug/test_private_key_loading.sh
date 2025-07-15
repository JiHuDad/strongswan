#!/bin/bash

# strongSwan extsock 플러그인 개인키 로딩 테스트 스크립트

set -e

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== strongSwan extsock 개인키 로딩 테스트 ===${NC}"

# 테스트용 암호화 키 생성
KEY_FILE="debug_encrypted.key"
KEY_PASSWORD="testpassword123"

if [ ! -f "$KEY_FILE" ]; then
    echo -e "${YELLOW}테스트용 암호화 개인키 생성 중...${NC}"
    openssl genpkey -algorithm RSA -pkcs8 \
        -pass pass:"$KEY_PASSWORD" \
        -out "$KEY_FILE" 2>/dev/null
    echo -e "${GREEN}✅ 암호화된 개인키 생성: $KEY_FILE${NC}"
fi

# 키 파일 검증
echo -e "\n${BLUE}1. OpenSSL 직접 테스트${NC}"
echo -n "올바른 패스워드: "
if echo "$KEY_PASSWORD" | openssl rsa -in "$KEY_FILE" -passin stdin -noout 2>/dev/null; then
    echo -e "${GREEN}✅ 성공${NC}"
else
    echo -e "${RED}❌ 실패${NC}"
fi

echo -n "잘못된 패스워드: "
if echo "wrongpassword" | openssl rsa -in "$KEY_FILE" -passin stdin -noout 2>/dev/null; then
    echo -e "${RED}❌ 예상치 못한 성공${NC}"
else
    echo -e "${GREEN}✅ 예상대로 실패${NC}"
fi

# JSON 설정 파일 생성
JSON_CONFIG="debug_config.json"
cat > "$JSON_CONFIG" << EOF
{
    "name": "debug_connection",
    "ike_cfg": {
        "local_addrs": ["0.0.0.0"],
        "remote_addrs": ["192.168.1.1"],
        "version": 2
    },
    "local_auth": {
        "auth": "cert",
        "cert": "nonexistent.crt",
        "private_key": "$KEY_FILE",
        "private_key_passphrase": "$KEY_PASSWORD"
    },
    "remote_auth": {
        "auth": "cert"
    }
}
EOF

echo -e "\n${BLUE}2. JSON 설정 파일 생성${NC}"
echo -e "${GREEN}✅ 설정 파일: $JSON_CONFIG${NC}"
echo "패스워드: $KEY_PASSWORD"

# 환경변수 테스트
echo -e "\n${BLUE}3. 환경변수 테스트${NC}"
export STRONGSWAN_PRIVATE_KEY_PASS="$KEY_PASSWORD"
echo -e "${GREEN}✅ STRONGSWAN_PRIVATE_KEY_PASS 설정됨${NC}"

# strongSwan 컴파일 확인
echo -e "\n${BLUE}4. strongSwan 컴파일 확인${NC}"
if make -C /home/finux/dev/plugin/strongswan/src/libcharon/plugins/extsock/ >/dev/null 2>&1; then
    echo -e "${GREEN}✅ extsock 플러그인 컴파일 성공${NC}"
else
    echo -e "${YELLOW}⚠️  컴파일 필요 또는 오류 발생${NC}"
fi

# 테스트 안내
echo -e "\n${BLUE}5. 테스트 방법${NC}"
echo "다음과 같이 strongSwan을 실행하여 로그를 확인하세요:"
echo ""
echo -e "${YELLOW}# 환경변수로 테스트${NC}"
echo "export STRONGSWAN_PRIVATE_KEY_PASS=\"$KEY_PASSWORD\""
echo "sudo ipsec start"
echo "# JSON 설정으로 연결 시도"
echo ""
echo -e "${YELLOW}# 로그 모니터링${NC}"
echo "sudo tail -f /var/log/daemon.log | grep -i 'extsock\\|private.*key\\|password'"
echo ""
echo -e "${YELLOW}# 디버그 레벨 증가${NC}"
echo "sudo ipsec stroke loglevel cfg 2"
echo ""

# 클린업 함수
cleanup() {
    echo -e "\n${BLUE}클린업${NC}"
    rm -f "$JSON_CONFIG"
    unset STRONGSWAN_PRIVATE_KEY_PASS
    echo -e "${GREEN}✅ 완료${NC}"
}

# 스크립트 종료 시 클린업
trap cleanup EXIT

echo -e "\n${GREEN}테스트 준비 완료! 위 안내를 따라 테스트하세요.${NC}" 