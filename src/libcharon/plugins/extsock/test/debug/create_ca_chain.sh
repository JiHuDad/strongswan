#!/bin/bash

# strongSwan extsock Multiple CA 체인 테스트용 인증서 생성 스크립트

set -e

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== strongSwan extsock Multiple CA 체인 생성 ===${NC}"

# 디렉토리 생성
CERT_DIR="test_ca_chain"
mkdir -p "$CERT_DIR"/{root,intermediate,client,server}

cd "$CERT_DIR"

# 1. Root CA 생성
echo -e "\n${BLUE}1. Root CA 생성${NC}"
echo -e "${YELLOW}Root CA 개인키 생성...${NC}"
openssl genpkey -algorithm RSA -pkcs8 -out root/root_ca.key 2>/dev/null

echo -e "${YELLOW}Root CA 인증서 생성...${NC}"
openssl req -new -x509 -key root/root_ca.key -sha256 -days 7300 -out root/root_ca.crt \
    -subj "/C=KR/ST=Seoul/L=Seoul/O=Test Company/OU=Root CA/CN=Test Root CA" 2>/dev/null

echo -e "${GREEN}✅ Root CA 생성 완료: root/root_ca.crt${NC}"

# 2. Intermediate CA 1 생성
echo -e "\n${BLUE}2. Intermediate CA 1 생성${NC}"
echo -e "${YELLOW}Intermediate CA 1 개인키 생성...${NC}"
openssl genpkey -algorithm RSA -pkcs8 -out intermediate/intermediate_ca_1.key 2>/dev/null

echo -e "${YELLOW}Intermediate CA 1 CSR 생성...${NC}"
openssl req -new -key intermediate/intermediate_ca_1.key -out intermediate/intermediate_ca_1.csr \
    -subj "/C=KR/ST=Seoul/L=Seoul/O=Test Company/OU=Intermediate CA 1/CN=Test Intermediate CA 1" 2>/dev/null

echo -e "${YELLOW}Intermediate CA 1 인증서 서명...${NC}"
openssl x509 -req -in intermediate/intermediate_ca_1.csr -CA root/root_ca.crt -CAkey root/root_ca.key \
    -CAcreateserial -out intermediate/intermediate_ca_1.crt -days 3650 -sha256 \
    -extensions v3_ca -extfile <(echo -e "basicConstraints=CA:true\nkeyUsage=keyCertSign,cRLSign") 2>/dev/null

echo -e "${GREEN}✅ Intermediate CA 1 생성 완료: intermediate/intermediate_ca_1.crt${NC}"

# 3. Intermediate CA 2 생성 (Sub-CA)
echo -e "\n${BLUE}3. Intermediate CA 2 생성 (Sub-CA)${NC}"
echo -e "${YELLOW}Intermediate CA 2 개인키 생성...${NC}"
openssl genpkey -algorithm RSA -pkcs8 -out intermediate/intermediate_ca_2.key 2>/dev/null

echo -e "${YELLOW}Intermediate CA 2 CSR 생성...${NC}"
openssl req -new -key intermediate/intermediate_ca_2.key -out intermediate/intermediate_ca_2.csr \
    -subj "/C=KR/ST=Seoul/L=Seoul/O=Test Company/OU=Intermediate CA 2/CN=Test Intermediate CA 2" 2>/dev/null

echo -e "${YELLOW}Intermediate CA 2 인증서 서명 (by Intermediate CA 1)...${NC}"
openssl x509 -req -in intermediate/intermediate_ca_2.csr -CA intermediate/intermediate_ca_1.crt -CAkey intermediate/intermediate_ca_1.key \
    -CAcreateserial -out intermediate/intermediate_ca_2.crt -days 3650 -sha256 \
    -extensions v3_ca -extfile <(echo -e "basicConstraints=CA:true\nkeyUsage=keyCertSign,cRLSign") 2>/dev/null

echo -e "${GREEN}✅ Intermediate CA 2 생성 완료: intermediate/intermediate_ca_2.crt${NC}"

# 4. 클라이언트 인증서 생성 (End Entity)
echo -e "\n${BLUE}4. 클라이언트 인증서 생성${NC}"
echo -e "${YELLOW}클라이언트 개인키 생성 (암호화)...${NC}"
openssl genpkey -algorithm RSA -pkcs8 -pass pass:client123 -out client/client.key 2>/dev/null

echo -e "${YELLOW}클라이언트 CSR 생성...${NC}"
openssl req -new -passin pass:client123 -key client/client.key -out client/client.csr \
    -subj "/C=KR/ST=Seoul/L=Seoul/O=Test Company/OU=VPN Client/CN=vpn.client.test" 2>/dev/null

echo -e "${YELLOW}클라이언트 인증서 서명 (by Intermediate CA 2)...${NC}"
openssl x509 -req -in client/client.csr -CA intermediate/intermediate_ca_2.crt -CAkey intermediate/intermediate_ca_2.key \
    -CAcreateserial -out client/client.crt -days 365 -sha256 \
    -extensions v3_req -extfile <(echo -e "basicConstraints=CA:false\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage=clientAuth") 2>/dev/null

echo -e "${GREEN}✅ 클라이언트 인증서 생성 완료: client/client.crt${NC}"

# 5. 서버 인증서 생성 (End Entity)
echo -e "\n${BLUE}5. 서버 인증서 생성${NC}"
echo -e "${YELLOW}서버 개인키 생성...${NC}"
openssl genpkey -algorithm RSA -pkcs8 -out server/server.key 2>/dev/null

echo -e "${YELLOW}서버 CSR 생성...${NC}"
openssl req -new -key server/server.key -out server/server.csr \
    -subj "/C=KR/ST=Seoul/L=Seoul/O=Test Company/OU=VPN Server/CN=vpn.server.test" 2>/dev/null

echo -e "${YELLOW}서버 인증서 서명 (by Intermediate CA 1)...${NC}"
openssl x509 -req -in server/server.csr -CA intermediate/intermediate_ca_1.crt -CAkey intermediate/intermediate_ca_1.key \
    -CAcreateserial -out server/server.crt -days 365 -sha256 \
    -extensions v3_req -extfile <(echo -e "basicConstraints=CA:false\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth") 2>/dev/null

echo -e "${GREEN}✅ 서버 인증서 생성 완료: server/server.crt${NC}"

# 6. 인증서 체인 검증
echo -e "\n${BLUE}6. 인증서 체인 검증${NC}"

echo -e "${YELLOW}클라이언트 인증서 체인 검증...${NC}"
cat intermediate/intermediate_ca_2.crt intermediate/intermediate_ca_1.crt root/root_ca.crt > client/client_ca_chain.crt
if openssl verify -CAfile client/client_ca_chain.crt client/client.crt >/dev/null 2>&1; then
    echo -e "${GREEN}✅ 클라이언트 인증서 체인 검증 성공${NC}"
else
    echo -e "${RED}❌ 클라이언트 인증서 체인 검증 실패${NC}"
fi

echo -e "${YELLOW}서버 인증서 체인 검증...${NC}"
cat intermediate/intermediate_ca_1.crt root/root_ca.crt > server/server_ca_chain.crt
if openssl verify -CAfile server/server_ca_chain.crt server/server.crt >/dev/null 2>&1; then
    echo -e "${GREEN}✅ 서버 인증서 체인 검증 성공${NC}"
else
    echo -e "${RED}❌ 서버 인증서 체인 검증 실패${NC}"
fi

# 7. strongSwan 설정 파일 생성
echo -e "\n${BLUE}7. strongSwan 설정 파일 생성${NC}"

cat > test_client_config.json << EOF
{
    "name": "test_multiple_ca_client",
    "ike_cfg": {
        "local_addrs": ["0.0.0.0"],
        "remote_addrs": ["vpn.server.test"],
        "version": 2
    },
    "local_auth": {
        "auth": "cert",
        "cert": "$(pwd)/client/client.crt",
        "private_key": "$(pwd)/client/client.key",
        "private_key_passphrase": "client123",
        "ca_chain": {
            "root_ca": "$(pwd)/root/root_ca.crt",
            "intermediate_cas": [
                "$(pwd)/intermediate/intermediate_ca_1.crt",
                "$(pwd)/intermediate/intermediate_ca_2.crt"
            ]
        },
        "enable_ocsp": false,
        "enable_crl": false
    },
    "remote_auth": {
        "auth": "cert",
        "ca_chain": {
            "root_ca": "$(pwd)/root/root_ca.crt",
            "intermediate_cas": [
                "$(pwd)/intermediate/intermediate_ca_1.crt"
            ]
        }
    }
}
EOF

echo -e "${GREEN}✅ 클라이언트 설정 파일 생성: test_client_config.json${NC}"

# 8. 파일 구조 출력
echo -e "\n${BLUE}8. 생성된 파일 구조${NC}"
echo -e "${YELLOW}📁 $CERT_DIR/${NC}"
find . -type f -name "*.crt" -o -name "*.key" -o -name "*.json" | sort | sed 's/^/  /'

echo -e "\n${BLUE}9. 인증서 체인 다이어그램${NC}"
echo -e "${YELLOW}"
echo "    Root CA"
echo "       │"
echo "   Intermediate CA 1"
echo "       │         │"
echo "  Server Cert   Intermediate CA 2"
echo "                    │"
echo "               Client Cert"
echo -e "${NC}"

echo -e "\n${GREEN}🎉 Multiple CA 체인 생성 완료!${NC}"
echo -e "${BLUE}테스트 방법:${NC}"
echo "1. extsock 플러그인 컴파일"
echo "2. test_client_config.json 사용하여 연결 테스트"
echo "3. 로그에서 Multiple CA 로딩 확인:"
echo "   - 📋 Total CA certificates loaded: 3"
echo "   - 🔐 Root CA loaded from: ..."
echo "   - 🔗 Intermediate CA #1 loaded from: ..."
echo "   - 🔗 Intermediate CA #2 loaded from: ..." 