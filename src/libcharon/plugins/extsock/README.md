# extsock Plugin for strongSwan

## 개요

`extsock` 플러그인은 strongSwan의 charon 데몬과 외부 프로그램 간에 **유닉스 도메인 소켓**을 통해 동적으로 IPsec/IKE 설정을 주고받을 수 있도록 해주는 플러그인입니다. 외부 프로그램이 JSON 포맷의 설정을 소켓으로 전송하면, strongSwan이 이를 실시간으로 적용할 수 있습니다.

---

## 주요 기능
- 외부 프로그램이 유닉스 도메인 소켓(`/tmp/strongswan_extsock.sock`)으로 명령을 전송
- PSK/인증서 기반 인증, 여러 CHILD_SA, IKE/ESP proposal 등 다양한 설정 지원
- SAD/SPD 이벤트를 외부로 JSON 포맷으로 알림 가능

---

## 지원 JSON 포맷 예시

### 1. 인증서 기반, 여러 CHILD_SA, proposal 지정
```json
{
  "name": "vpn-conn1",
  "local": "192.168.1.10",
  "remote": "203.0.113.5",
  "auth": {
    "type": "cert",
    "local_id": "CN=myuser",
    "remote_id": "CN=server",
    "cert_file": "/etc/ipsec.d/certs/myuser.pem",
    "key_file": "/etc/ipsec.d/private/myuser.key"
  },
  "ike_proposal": "aes256-sha256-modp2048",
  "esp_proposal": "aes256gcm16-modp2048",
  "dpd": 30,
  "rekey_time": 3600,
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

### 2. PSK 기반, 단일 CHILD_SA
```json
{
  "name": "office-vpn",
  "local": "10.0.0.2",
  "remote": "198.51.100.10",
  "psk": "supersecret",
  "child": {
    "name": "office-tunnel",
    "local_ts": "192.168.100.0/24",
    "remote_ts": "192.168.200.0/24"
  }
}
```

---

## 외부 프로그램 예시 (C, cJSON 사용)

아래는 cJSON 라이브러리를 이용해 JSON 메시지를 만들고, extsock 소켓으로 전송하는 예시입니다.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "cJSON.h"

#define SOCKET_PATH "/tmp/strongswan_extsock.sock"

int main() {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", "vpn-conn1");
    cJSON_AddStringToObject(root, "local", "192.168.1.10");
    cJSON_AddStringToObject(root, "remote", "203.0.113.5");
    cJSON_AddStringToObject(root, "ike_proposal", "aes256-sha256-modp2048");
    cJSON_AddStringToObject(root, "esp_proposal", "aes256gcm16-modp2048");

    cJSON *auth = cJSON_CreateObject();
    cJSON_AddStringToObject(auth, "type", "cert");
    cJSON_AddStringToObject(auth, "local_id", "CN=myuser");
    cJSON_AddStringToObject(auth, "remote_id", "CN=server");
    cJSON_AddStringToObject(auth, "cert_file", "/etc/ipsec.d/certs/myuser.pem");
    cJSON_AddStringToObject(auth, "key_file", "/etc/ipsec.d/private/myuser.key");
    cJSON_AddItemToObject(root, "auth", auth);

    cJSON *children = cJSON_CreateArray();
    cJSON *child1 = cJSON_CreateObject();
    cJSON_AddStringToObject(child1, "name", "child1");
    cJSON_AddStringToObject(child1, "local_ts", "10.0.0.0/24");
    cJSON_AddStringToObject(child1, "remote_ts", "10.1.0.0/24");
    cJSON_AddItemToArray(children, child1);

    cJSON *child2 = cJSON_CreateObject();
    cJSON_AddStringToObject(child2, "name", "child2");
    cJSON_AddStringToObject(child2, "local_ts", "10.0.1.0/24");
    cJSON_AddStringToObject(child2, "remote_ts", "10.1.1.0/24");
    cJSON_AddItemToArray(children, child2);

    cJSON_AddItemToObject(root, "children", children);

    char *json_str = cJSON_PrintUnformatted(root);
    char *cmd;
    size_t cmd_len = strlen("APPLY_CONFIG ") + strlen(json_str) + 1;
    cmd = malloc(cmd_len);
    snprintf(cmd, cmd_len, "APPLY_CONFIG %s", json_str);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        return 1;
    }

    write(fd, cmd, strlen(cmd));

    close(fd);
    free(cmd);
    cJSON_Delete(root);
    free(json_str);

    printf("Sent config to extsock plugin.\n");
    return 0;
}
```

---

## 사용 방법 요약
1. strongSwan을 extsock 플러그인과 함께 빌드 및 실행
2. 외부 프로그램에서 위와 같은 방식으로 JSON 메시지를 생성
3. `APPLY_CONFIG <json>` 형태로 유닉스 도메인 소켓(`/tmp/strongswan_extsock.sock`)에 write
4. strongSwan이 해당 설정을 실시간으로 적용

---

## 참고
- cJSON 외에도 Python, Go 등 다양한 언어에서 JSON 문자열을 만들어 동일하게 전송할 수 있습니다.
- 소켓 경로, JSON 포맷 등은 플러그인 코드와 일치해야 합니다. 