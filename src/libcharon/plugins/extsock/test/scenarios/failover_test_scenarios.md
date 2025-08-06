# extsock 2nd SEGW Failover 실제 테스트 시나리오

## 📋 테스트 환경 구성

### 기본 네트워크 구성
```
[Client] <---> [1st SEGW: 10.0.0.1] <---> [Target Network]
           |
           \--> [2nd SEGW: 10.0.0.2] <---> [Target Network]
           |
           \--> [3rd SEGW: 10.0.0.3] <---> [Target Network]
```

---

## 🧪 테스트 시나리오

### Scenario 1: 기본 2nd SEGW Failover

#### 설정 예시
```json
{
    "ike_version": "ikev2",
    "local": "0.0.0.0",
    "remote_addrs": "10.0.0.1,10.0.0.2",
    "connection_name": "test-failover-basic",
    "ike_proposals": [
        {
            "encr": "aes256",
            "integ": "sha256",
            "prf": "sha256",
            "group": "modp2048"
        }
    ],
    "esp_proposals": [
        {
            "encr": "aes256",
            "integ": "sha256",
            "group": "modp2048"
        }
    ],
    "auth_local": {
        "auth": "psk",
        "id": "client@example.com"
    },
    "auth_remote": {
        "auth": "psk",
        "id": "server@example.com"
    },
    "children": [
        {
            "child_name": "test-child",
            "local_ts": "0.0.0.0/0",
            "remote_ts": "192.168.100.0/24",
            "dpd_action": "restart"
        }
    ]
}
```

#### 테스트 절차
1. **1st SEGW 연결 시도**: `10.0.0.1`로 연결
2. **연결 실패 시뮬레이션**: 1st SEGW 다운
3. **자동 Failover 확인**: `10.0.0.2`로 자동 전환
4. **연결 성공 확인**: 터널 설정 완료

#### 예상 로그
```
[extsock] IKE_DESTROYING detected for IKE SA 'test-failover-basic'
[extsock] Handling connection failure for IKE SA 'test-failover-basic'
[extsock] Initiating failover: 10.0.0.1 -> 10.0.0.2
[extsock] Failover to 10.0.0.2 initiated successfully
```

---

### Scenario 2: 3개 SEGW 순환 Failover

#### 설정 예시
```json
{
    "connection_name": "test-failover-multi",
    "remote_addrs": "192.168.1.1,192.168.1.2,192.168.1.3",
    "local": "0.0.0.0",
    "ike_version": "ikev2"
}
```

#### 테스트 절차
1. **1st SEGW**: `192.168.1.1` → 실패
2. **2nd SEGW**: `192.168.1.2` → 실패  
3. **3rd SEGW**: `192.168.1.3` → 성공
4. **순환 테스트**: `192.168.1.3` → `192.168.1.1`로 순환

---

### Scenario 3: 재시도 제한 테스트

#### 테스트 절차
1. **5회 연속 실패**: 모든 SEGW 다운 상황
2. **재시도 제한 확인**: 6번째 시도 시 차단
3. **제한 해제 테스트**: 연결 성공 후 카운터 리셋

#### 예상 동작
```
Attempt 1-5: Failover attempts (10.0.0.1 → 10.0.0.2 → 10.0.0.1 ...)
Attempt 6+: "Max retry count exceeded for connection 'test-conn'"
Success: Reset retry count, resume normal operation
```

---

### Scenario 4: 동시 다중 연결 Failover

#### 설정
- Connection A: `10.1.1.1,10.1.1.2`
- Connection B: `10.2.2.1,10.2.2.2`
- Connection C: `10.3.3.1,10.3.3.2`

#### 테스트 절차
1. **동시 연결**: 3개 연결 모두 1st SEGW 사용
2. **동시 실패**: 모든 1st SEGW 다운
3. **독립적 Failover**: 각각 2nd SEGW로 전환
4. **상태 격리 확인**: 연결별 독립적 재시도 카운터

---

## 🔧 실제 환경 테스트 도구

### 테스트 스크립트 예시

#### 1. 기본 설정 테스트
```bash
#!/bin/bash
# test_basic_failover.sh

# 1st SEGW로 연결 설정
curl -X POST http://localhost:8080/api/config \
  -H "Content-Type: application/json" \
  -d '{
    "connection_name": "test-basic",
    "remote_addrs": "10.0.0.1,10.0.0.2",
    "local": "0.0.0.0"
  }'

# 연결 시도
curl -X POST http://localhost:8080/api/connect/test-basic

# 1st SEGW 차단 (iptables 등)
sudo iptables -A OUTPUT -d 10.0.0.1 -j DROP

# Failover 확인 (로그 모니터링)
tail -f /var/log/syslog | grep extsock
```

#### 2. 스트레스 테스트
```bash
#!/bin/bash
# stress_test_failover.sh

for i in {1..100}; do
    # 연결 생성
    curl -X POST http://localhost:8080/api/config \
      -d "{\"connection_name\": \"stress-test-$i\", \"remote_addrs\": \"10.0.0.1,10.0.0.2\"}"
    
    # 연결 실패 시뮬레이션
    curl -X POST http://localhost:8080/api/disconnect/stress-test-$i
    
    sleep 0.1
done
```

---

## 📊 성능 측정 기준

### 측정 항목
1. **Failover 지연 시간**: IKE_DESTROYING → 2nd SEGW 연결 완료
2. **메모리 사용량**: 다중 연결 시 메모리 증가량
3. **CPU 사용률**: Failover 과정 중 CPU 부하
4. **성공률**: 100회 Failover 중 성공 횟수

### 목표 기준
- **Failover 지연**: < 5초
- **메모리 증가**: < 100KB per connection
- **CPU 사용률**: < 10% during failover
- **성공률**: > 95%

---

## 🐳 Docker 기반 테스트 환경

### Docker Compose 예시
```yaml
version: '3.8'
services:
  client:
    image: strongswan-extsock:latest
    volumes:
      - ./config:/etc/strongswan
    networks:
      - test-network
  
  segw1:
    image: strongswan-server:latest
    environment:
      - SEGW_ID=1
      - SEGW_IP=10.0.0.1
    networks:
      test-network:
        ipv4_address: 10.0.0.1
  
  segw2:
    image: strongswan-server:latest
    environment:
      - SEGW_ID=2
      - SEGW_IP=10.0.0.2
    networks:
      test-network:
        ipv4_address: 10.0.0.2

networks:
  test-network:
    ipam:
      config:
        - subnet: 10.0.0.0/24
```

### 실행 명령
```bash
# 환경 시작
docker-compose up -d

# 테스트 실행
docker exec client /test/run_failover_tests.sh

# SEGW1 다운 시뮬레이션
docker stop segw1

# Failover 확인
docker exec client swanctl --list-sas
```

---

## 📝 수동 테스트 체크리스트

### ✅ 기본 기능
- [ ] 단일 주소 설정 (Failover 비활성화)
- [ ] 2개 주소 순환 Failover
- [ ] 3개 이상 주소 순환 Failover
- [ ] 공백 포함 주소 파싱

### ✅ 예외 상황
- [ ] NULL 주소 입력 처리
- [ ] 빈 문자열 입력 처리
- [ ] 잘못된 형식 주소 처리
- [ ] 재시도 횟수 초과 처리

### ✅ 동시성
- [ ] 다중 연결 동시 Failover
- [ ] 스레드 안전성
- [ ] 메모리 누수 방지

### ✅ 성능
- [ ] 대량 주소 목록 처리
- [ ] 반복적 Failover 성능
- [ ] 장시간 운영 안정성

---

## 🔧 디버깅 도구

### 로그 분석 명령
```bash
# Failover 관련 로그만 필터링
journalctl -f | grep -E "(extsock|IKE_DESTROYING|failover)"

# 상세 디버그 로그 활성화
echo "charon.plugins.extsock.debug_level = 3" >> /etc/strongswan/strongswan.conf

# 메모리 사용량 모니터링
while true; do
    ps aux | grep charon | awk '{print $6}' | head -1
    sleep 5
done
```

### 설정 검증 도구
```bash
# 현재 연결 상태 확인
swanctl --list-sas

# 설정 파일 문법 검사
swanctl --load-conns --debug

# IKE SA 수동 시작
swanctl --initiate --child test-child
```

이러한 실제 환경 테스트 시나리오를 통해 2nd SEGW Failover 기능의 실용성과 안정성을 검증할 수 있습니다. 