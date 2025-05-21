# extsock Plugin for strongSwan

## 개요

`extsock` 플러그인은 strongSwan의 charon 데몬과 외부 프로그램 간에 **유닉스 도메인 소켓**을 통해 동적으로 IPsec/IKE 설정을 주고받을 수 있도록 해주는 플러그인입니다. 외부 프로그램이 JSON 포맷의 설정을 소켓으로 전송하면, strongSwan이 이를 실시간으로 적용할 수 있습니다.

---

## 주요 기능
- 외부 프로그램이 유닉스 도메인 소켓(`/tmp/strongswan_extsock.sock`)으로 명령을 전송
- PSK/인증서 기반 인증, 여러 CHILD_SA, IKE/ESP proposal 등 다양한 설정 지원
- **터널(Child SA) up/down 이벤트를 외부로 JSON 포맷으로 알림** (SPD/SAD 이벤트는 지원하지 않음)
- DPD(Dead Peer Detection) 트리거 명령 지원

---

## 지원 명령 및 예시

### 1. IPsec 설정 적용
- **명령어:** `APPLY_CONFIG <json>`
- **설명:** JSON 포맷의 IPsec/IKE 설정을 strongSwan에 적용합니다.

#### 예시 JSON (auth, children, proposal 등)
```json
{
  "name": "vpn-conn1",
  "local": "192.168.1.10",
  "remote": "203.0.113.5",
  "auth": {
    "type": "psk",
    "id": "CN=myuser",
    "secret": "supersecret"
  },
  "ike_proposal": "aes256-sha256-modp2048",
  "esp_proposal": "aes256gcm16-modp2048",
  "children": [
    {
      "name": "child1",
      "local_ts": "10.0.0.0/24",
      "remote_ts": "10.1.0.0/24"
    },
    {
      "name": "child2",
      "local_ts": "10.0.1.0/24",
      "remote_ts": "10.1.1.0/24"
    }
  ]
}
```

### 2. DPD(Dead Peer Detection) 트리거
- **명령어:** `START_DPD <ike_sa_name>`
- **설명:** 지정한 IKE_SA 이름에 대해 DPD를 즉시 트리거합니다.
- **예시:**
```
START_DPD vpn-conn1
```

---

## 터널(Child SA) up/down 이벤트 알림 포맷 예시

플러그인은 strongSwan의 CHILD_SA(터널) 상태 변화(up/down)를 감지하여 외부 프로그램에 아래와 같은 JSON 포맷으로 알림을 전송합니다.

### 1. 터널(Child SA) up 이벤트
```json
{
  "event": "tunnel_up",
  "ike_sa_name": "vpn-conn1",
  "spi": 12345678,
  "proto": "esp",
  "mode": "tunnel",
  "enc_alg": "aes256",
  "integ_alg": "sha256",
  "src": "192.168.1.10",
  "dst": "203.0.113.5",
  "local_ts": "10.0.0.0/24",
  "remote_ts": "10.1.0.0/24",
  "direction": "out",
  "policy_action": "protect"
}
```

### 2. 터널(Child SA) down 이벤트
```json
{
  "event": "tunnel_down",
  "ike_sa_name": "vpn-conn1",
  "spi": 12345678,
  "proto": "esp",
  "mode": "tunnel",
  "enc_alg": "aes256",
  "integ_alg": "sha256",
  "src": "192.168.1.10",
  "dst": "203.0.113.5",
  "local_ts": "10.0.0.0/24",
  "remote_ts": "10.1.0.0/24",
  "direction": "out",
  "policy_action": "protect"
}
```

- `event`: 이벤트 종류(`tunnel_up`, `tunnel_down`)
- `ike_sa_name`: IKE_SA 이름
- `spi`: SA의 SPI 값
- `proto`: 프로토콜(예: "esp", "ah")
- `mode`: 터널 모드("tunnel"/"transport")
- `enc_alg`: 암호화 알고리즘
- `integ_alg`: 무결성 알고리즘
- `src`, `dst`: SA의 소스/목적지 주소
- `local_ts`, `remote_ts`: 트래픽 선택자(로컬/원격)
- `direction`: 방향(보통 "out")
- `policy_action`: 정책(보통 "protect")

---

## 외부 프로그램 통합 예제 (APPLY_CONFIG + tunnel_up 후 DPD)

아래 예제는 다음을 모두 포함합니다:
- 소켓 연결 및 APPLY_CONFIG 명령 전송
- tunnel_up 이벤트 수신 시 10초 후 START_DPD 자동 전송
- 모든 이벤트(tunnel_up, tunnel_down 등) 출력

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cJSON.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/strongswan_extsock.sock"

typedef struct {
    int fd;
    char ike_sa_name[128];
} dpd_args_t;

void* dpd_thread(void* arg) {
    dpd_args_t* args = (dpd_args_t*)arg;
    sleep(10); // 10초 대기
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "START_DPD %s", args->ike_sa_name);
    write(args->fd, cmd, strlen(cmd));
    printf("[cmd] Sent DPD trigger: %s\n", cmd);
    free(args);
    return NULL;
}

int main() {
    int fd;
    struct sockaddr_un addr;
    char buf[2048];

    // 1. 소켓 생성 및 연결
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect"); close(fd); return 1;
    }

    // 2. APPLY_CONFIG 명령 전송
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", "vpn-conn1");
    cJSON_AddStringToObject(root, "local", "192.168.1.10");
    cJSON_AddStringToObject(root, "remote", "203.0.113.5");
    cJSON *auth = cJSON_CreateObject();
    cJSON_AddStringToObject(auth, "type", "psk");
    cJSON_AddStringToObject(auth, "id", "CN=myuser");
    cJSON_AddStringToObject(auth, "secret", "supersecret");
    cJSON_AddItemToObject(root, "auth", auth);
    cJSON_AddStringToObject(root, "ike_proposal", "aes256-sha256-modp2048");
    cJSON_AddStringToObject(root, "esp_proposal", "aes256gcm16-modp2048");
    cJSON *children = cJSON_CreateArray();
    cJSON *child1 = cJSON_CreateObject();
    cJSON_AddStringToObject(child1, "name", "child1");
    cJSON_AddStringToObject(child1, "local_ts", "10.0.0.0/24");
    cJSON_AddStringToObject(child1, "remote_ts", "10.1.0.0/24");
    cJSON_AddItemToArray(children, child1);
    cJSON_AddItemToObject(root, "children", children);
    char *json_str = cJSON_PrintUnformatted(root);
    char *cmd;
    size_t cmd_len = strlen("APPLY_CONFIG ") + strlen(json_str) + 1;
    cmd = malloc(cmd_len);
    snprintf(cmd, cmd_len, "APPLY_CONFIG %s", json_str);
    if (write(fd, cmd, strlen(cmd)) < 0) {
        perror("write");
        close(fd);
        free(cmd);
        cJSON_Delete(root);
        free(json_str);
        return 1;
    }
    printf("[cmd] Sent config to extsock plugin.\n");
    free(cmd);
    cJSON_Delete(root);
    free(json_str);

    // 3. 이벤트 수신 및 tunnel_up 시 DPD 트리거
    while (1) {
        ssize_t len = read(fd, buf, sizeof(buf)-1);
        if (len > 0) {
            buf[len] = '\0';
            cJSON *json = cJSON_Parse(buf);
            if (json) {
                cJSON *event = cJSON_GetObjectItem(json, "event");
                if (event && cJSON_IsString(event)) {
                    printf("[event] Received event: %s\n", event->valuestring);
                    printf("[event] Full JSON: %s\n", buf);
                    if (strcmp(event->valuestring, "tunnel_up") == 0) {
                        cJSON *name = cJSON_GetObjectItem(json, "ike_sa_name");
                        if (name && cJSON_IsString(name)) {
                            dpd_args_t* args = malloc(sizeof(dpd_args_t));
                            args->fd = fd;
                            strncpy(args->ike_sa_name, name->valuestring, sizeof(args->ike_sa_name)-1);
                            args->ike_sa_name[sizeof(args->ike_sa_name)-1] = '\0';
                            pthread_t tid;
                            pthread_create(&tid, NULL, dpd_thread, args);
                            pthread_detach(tid);
                        }
                    }
                } else {
                    printf("[event] Received non-event JSON: %s\n", buf);
                }
                cJSON_Delete(json);
            } else {
                printf("[event] Received non-JSON data: %s\n", buf);
            }
        } else if (len == 0) {
            printf("[event] Connection closed by server.\n");
            break;
        } else {
            perror("read");
            break;
        }
    }
    close(fd);
    return 0;
}

**DPD 동작 확인 방법:**
- tunnel_up 이벤트 수신 → 10초 후 DPD 트리거
- strongSwan이 DPD를 수행, 상대방이 응답하지 않으면 SA가 내려가고 tunnel_down 이벤트가 다시 수신됨
- 즉, DPD가 제대로 동작하면 tunnel_down 이벤트가 자동으로 도착합니다.

---

## 사용 방법 요약
1. strongSwan을 extsock 플러그인과 함께 빌드 및 실행
2. 외부 프로그램에서 위와 같은 방식으로 JSON 메시지를 생성
3. `APPLY_CONFIG <json>` 또는 `START_DPD <ike_sa_name>` 형태로 유닉스 도메인 소켓(`/tmp/strongswan_extsock.sock`)에 write
4. strongSwan이 해당 설정을 실시간으로 적용하거나, DPD를 트리거함
5. 터널(Child SA) up/down 이벤트가 발생하면 외부 프로그램으로 JSON 알림이 전송됨

---

## 참고
- cJSON 외에도 Python, Go 등 다양한 언어에서 JSON 문자열을 만들어 동일하게 전송할 수 있습니다.
- 소켓 경로, JSON 포맷 등은 플러그인 코드와 일치해야 합니다.
- 실제 strongSwan 설정 적용은 향후 구현 예정입니다. 