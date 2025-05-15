1. Flowchart

```mermaid
flowchart LR
    subgraph strongSwan
        direction TB
        A["charon (IKE/IPsec 엔진)"]
        B["extsock 플러그인"]
    end
    subgraph UserSpace
        C["외부 프로그램 (DPDK)"]
    end
    D["유닉스 도메인 소켓\n/tmp/strongswan_extsock.sock"]

    A -- "IKE/CHILD SA up/down 이벤트" --> B
    B -- "SA/TS 정보(JSON)" --> D
    C -- "명령/설정(JSON)" --> D
    D -- "설정/명령(JSON)" --> B
    B -- "strongSwan API" --> A
```

2. SequenceDiagram
```mermaid
sequenceDiagram
    participant ExtProg as 외부 프로그램(DPDK)
    participant extsock as extsock 플러그인
    participant charon as charon (IKE/IPsec)
    ExtProg->>extsock: APPLY_CONFIG {json}
    extsock->>charon: strongSwan API로 설정 적용 (TODO)
    charon-->>extsock: IKE/CHILD SA up 이벤트
    extsock-->>ExtProg: {event: "tunnel_up", spi, proto, local_ts, remote_ts}
    Note over ExtProg: DPDK가 SA/TS 정보로 데이터플레인 구성
    ExtProg->>extsock: START_DPD <ike_sa_name>
    extsock->>charon: DPD 트리거
    charon-->>extsock: IKE/CHILD SA down 이벤트
    extsock-->>ExtProg: {event: "tunnel_down", ...}
```

3.ClassDiagram
```mermaid
classDiagram
    class TunnelEvent {
        +string event
        +uint32 spi
        +string proto
        +string local_ts
        +string remote_ts
    }
    class ApplyConfig {
        +string name
        +string local
        +string remote
        +object auth
        +string ike_proposal
        +string esp_proposal
        +array children
    }
    TunnelEvent <|-- tunnel_up
    TunnelEvent <|-- tunnel_down
    ApplyConfig <|-- APPLY_CONFIG
```