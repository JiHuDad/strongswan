# extsock_client

> **참고:** 이 클라이언트는 strongSwan 메인 빌드/설치에 포함되지 않는 참고/테스트용 도구입니다. 필요할 때 이 디렉터리에서 직접 빌드해서 사용하세요.

strongSwan extsock 플러그인과 연동하는 테스트/관리용 CLI 클라이언트입니다.

## 주요 기능
- extsock 플러그인 소켓(`/tmp/strongswan_extsock.sock`)에 연결
- JSON 파일로 IPsec/IKE 설정(APPLY_CONFIG) 전송
- DPD 트리거 명령(START_DPD) 전송
- 터널 up/down 등 이벤트 실시간 수신 및 출력
- 단일 프로세스에서 명령 전송과 이벤트 모니터링을 모두 지원

## 빌드 방법

```sh
# cJSON 라이브러리 필요 (예: Ubuntu)
sudo apt-get install libcjson-dev

# 빌드
cd src/libcharon/plugins/extsock/client
gcc -o extsock_client extsock_client.c -lcjson
```

## 사용법

```sh
# 설정 적용 (apply-config.json 파일 사용)
./extsock_client apply-config apply-config.json [--wait-events]

# DPD 트리거 (ike_sa_name 지정)
./extsock_client start-dpd <ike_sa_name> [--wait-events]

# 이벤트만 모니터링
./extsock_client monitor-events
```

- `--wait-events` 옵션을 주면 명령 전송 후 터널 이벤트를 계속 수신합니다.
- `monitor-events`는 명령 없이 이벤트만 수신합니다.

## 예시

```sh
./extsock_client apply-config myconfig.json --wait-events
# tunnel_up 이벤트 수신 후, 별도로 DPD 트리거 가능
./extsock_client start-dpd my-ike-sa-name
```

## 참고
- strongSwan extsock 플러그인과 함께 사용할 때만 동작합니다.
- 소켓 경로(`/tmp/strongswan_extsock.sock`)는 필요에 따라 수정 가능합니다.

## 예제 JSON 설정 파일 및 옵션 설명

아래는 apply-config 명령에 사용할 수 있는 예제 JSON 파일입니다. 실제 파일에는 주석이 들어갈 수 없으니 참고용으로만 활용하세요.

```json
{
  "auth": {
    "id": "client1",              // 로컬 ID
    "remote_id": "server1",       // 원격 ID
    "method": "psk",              // 인증 방식 (psk, cert 등)
    "psk": "secret123"            // PSK 비밀번호 (psk 방식일 때)
  },
  "ike": {
    "version": 2,                  // IKE 버전 (1 또는 2)
    "dscp": "000000",              // IKE 패킷 DSCP 값 (6비트 바이너리 문자열)
    "proposals": [
      "aes128-sha256-modp2048"      // IKE 암호/해시/그룹
    ]
  },
  "children": [
    {
      "name": "net1",              // CHILD_SA 이름
      "mode": "tunnel",            // 모드 (tunnel, transport)
      "copy_dscp": "out",          // DSCP 복사 모드 (out, in, yes, no)
      "proposals": [
        "aes128gcm16-prfsha256-modp2048" // ESP 암호/PRF/그룹
      ],
      "local_ts": ["10.0.0.1/32"], // 로컬 트래픽 선택자
      "remote_ts": ["10.0.0.2/32"] // 원격 트래픽 선택자
    }
  ],
  "dpd": {
    "delay": 10,                   // DPD 주기(초)
    "timeout": 30                  // DPD 타임아웃(초)
  }
}
```

### 주요 옵션 설명
- `auth`: 인증 관련 설정
  - `id`: 로컬 식별자(ID)
  - `remote_id`: 원격 식별자
  - `method`: 인증 방식 (`psk`, `cert` 등)
  - `psk`: 사전 공유 키(PSK, `method`가 `psk`일 때)
- `ike`: IKE SA 관련 설정
  - `version`: IKE 버전(1 또는 2)
  - `dscp`: IKE 패킷 DSCP 값(6비트 바이너리 문자열, 예: `"101110"` = EF)
  - `proposals`: IKE 암호/해시/그룹 조합(문자열 배열)
- `children`: CHILD_SA(터널) 설정 목록
  - `name`: CHILD_SA 이름
  - `mode`: 터널 모드(`tunnel`, `transport`)
  - `copy_dscp`: DSCP 복사 모드(`out`, `in`, `yes`, `no`)
    - `out`: 내부 → 외부 헤더로만 복사 (기본값)
    - `in`: 외부 → 내부 헤더로만 복사
    - `yes`: 양방향 복사
    - `no`: 복사 안함
  - `proposals`: ESP 암호/PRF/그룹 조합(문자열 배열)
  - `local_ts`: 로컬 트래픽 선택자(예: `10.0.0.1/32`)
  - `remote_ts`: 원격 트래픽 선택자
- `dpd`: Dead Peer Detection 설정
  - `delay`: DPD 체크 주기(초)
  - `timeout`: DPD 타임아웃(초)

실제 적용 시에는 환경에 맞게 값을 수정해서 사용하세요.

## DSCP 설정 예제

### QoS 기반 터널 설정
```json
{
  "auth": {
    "id": "client1",
    "remote_id": "server1", 
    "method": "psk",
    "psk": "secret123"
  },
  "ike": {
    "version": 2,
    "dscp": "101110",  // Expedited Forwarding (EF)
    "proposals": ["aes128-sha256-modp2048"]
  },
  "children": [
    {
      "name": "voice",
      "copy_dscp": "yes",  // 양방향 복사
      "local_ts": ["10.0.1.0/24"],
      "remote_ts": ["192.168.1.0/24"],
      "esp_proposals": ["aes128gcm16-prfsha256-modp2048"]
    },
    {
      "name": "data", 
      "copy_dscp": "out",  // 기본 복사
      "local_ts": ["10.0.2.0/24"],
      "remote_ts": ["192.168.2.0/24"],
      "esp_proposals": ["aes128gcm16-prfsha256-modp2048"]
    }
  ]
}
```

### 일반적인 DSCP 값
- `"000000"`: Best Effort (BE) - 기본값
- `"101110"`: Expedited Forwarding (EF) - 음성/실시간 트래픽
- `"101100"`: Assured Forwarding (AF41) - 우선순위 데이터
- `"100010"`: Class Selector (CS1) - 낮은 우선순위 