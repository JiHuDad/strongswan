# strongSwan 의존성 해결 및 테스트 커버리지 확장 계획

## 📊 현재 상황 분석

### 테스트 커버리지 현황
- **현재 커버리지**: 80% (핵심 비즈니스 로직)
- **커버된 함수**: 4/5 (select_next_segw, retry management 등)
- **미커버 함수**: create_failover_config, handle_connection_failure (strongSwan 통합 부분)

### strongSwan 의존성 복잡도
- **Core Data Structures**: 12개 타입 (ike_sa_t, peer_cfg_t, ike_cfg_t 등)
- **API 함수들**: 25개 함수 (configuration 관리, 메모리 관리, 암호화 등)
- **External Dependencies**: config_usecase, charon daemon

## 🎯 해결 방안 로드맵

### Phase 1: Mock Framework 구축 (우선순위 1)

#### 1.1 strongSwan Mock Objects 생성
```c
// File: src/libcharon/plugins/extsock/test/mocks/strongswan_mocks.h

typedef struct mock_ike_sa_t {
    char name[64];
    char remote_host[64];
    mock_peer_cfg_t *peer_cfg;
    
    // Mock methods
    const char* (*get_name)(mock_ike_sa_t *this);
    mock_peer_cfg_t* (*get_peer_cfg)(mock_ike_sa_t *this);
    mock_host_t* (*get_other_host)(mock_ike_sa_t *this);
    void (*destroy)(mock_ike_sa_t *this);
} mock_ike_sa_t;

typedef struct mock_peer_cfg_t {
    char name[64];
    mock_ike_cfg_t *ike_cfg;
    linked_list_t *auth_cfgs;
    linked_list_t *child_cfgs;
    
    // Mock methods
    const char* (*get_name)(mock_peer_cfg_t *this);
    mock_ike_cfg_t* (*get_ike_cfg)(mock_peer_cfg_t *this);
    void (*destroy)(mock_peer_cfg_t *this);
} mock_peer_cfg_t;

typedef struct mock_ike_cfg_t {
    char local_addr[64];
    char remote_addr[64];
    uint16_t local_port;
    uint16_t remote_port;
    ike_version_t version;
    
    // Mock methods
    char* (*get_other_addr)(mock_ike_cfg_t *this);
    char* (*get_my_addr)(mock_ike_cfg_t *this);
    uint16_t (*get_my_port)(mock_ike_cfg_t *this);
    uint16_t (*get_other_port)(mock_ike_cfg_t *this);
    ike_version_t (*get_version)(mock_ike_cfg_t *this);
    void (*destroy)(mock_ike_cfg_t *this);
} mock_ike_cfg_t;
```

#### 1.2 Mock Factory Functions
```c
// File: src/libcharon/plugins/extsock/test/mocks/strongswan_mock_factory.c

mock_ike_sa_t* create_mock_ike_sa(const char *name, const char *remote_addr);
mock_peer_cfg_t* create_mock_peer_cfg(const char *name, const char *segw_addresses);
mock_ike_cfg_t* create_mock_ike_cfg(const char *local_addr, const char *remote_addr);

// Mock behavior simulator
typedef struct mock_config_result_t {
    bool success;
    char error_message[256];
    mock_peer_cfg_t *created_config;
} mock_config_result_t;

mock_config_result_t simulate_config_creation(mock_peer_cfg_t *original, const char *new_addr);
```

#### 1.3 Mock 기반 create_failover_config 테스트
```cpp
// File: src/libcharon/plugins/extsock/test/gtest/src/mocks/test_create_failover_config_mock.cpp

class MockFailoverConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup mock objects
        mock_original_cfg = create_mock_peer_cfg("test-conn", "10.1.1.1,10.1.1.2,10.1.1.3");
        failover_manager = create_mock_failover_manager();
    }
    
    mock_peer_cfg_t *mock_original_cfg;
    mock_failover_manager_t *failover_manager;
};

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_ValidInput_Success) {
    // Given: Valid peer config and next SEGW address
    const char *next_segw = "10.1.1.2";
    
    // When: Create failover config
    extsock_error_t result = failover_manager->create_failover_config(
        failover_manager, (peer_cfg_t*)mock_original_cfg, next_segw);
    
    // Then: Should succeed
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    
    // Verify new config was created with correct address
    mock_ike_cfg_t *new_ike_cfg = mock_original_cfg->get_ike_cfg(mock_original_cfg);
    EXPECT_STREQ(new_ike_cfg->get_other_addr(new_ike_cfg), next_segw);
}

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_NullInput_Error) {
    extsock_error_t result = failover_manager->create_failover_config(
        failover_manager, nullptr, "10.1.1.2");
    
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
}

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_ConfigCreationFails_Error) {
    // Setup mock to simulate config creation failure
    setup_mock_config_creation_failure();
    
    extsock_error_t result = failover_manager->create_failover_config(
        failover_manager, (peer_cfg_t*)mock_original_cfg, "10.1.1.2");
    
    EXPECT_EQ(result, EXTSOCK_ERROR_CONFIG_CREATION_FAILED);
}
```

### Phase 2: Dependency Injection 확장

#### 2.1 strongSwan Adapter Interface
```c
// File: src/libcharon/plugins/extsock/adapters/strongswan_adapter.h

typedef struct strongswan_adapter_t strongswan_adapter_t;

struct strongswan_adapter_t {
    // Configuration management
    ike_cfg_t* (*create_ike_config)(strongswan_adapter_t *this, 
                                    const char *remote_addr, 
                                    ike_cfg_t *original);
    
    peer_cfg_t* (*create_peer_config)(strongswan_adapter_t *this,
                                      const char *name, 
                                      ike_cfg_t *ike_cfg,
                                      peer_cfg_create_t *data);
    
    bool (*copy_auth_configs)(strongswan_adapter_t *this,
                             peer_cfg_t *src, 
                             peer_cfg_t *dst);
    
    bool (*copy_child_configs)(strongswan_adapter_t *this,
                              peer_cfg_t *src, 
                              peer_cfg_t *dst);
    
    // Connection management
    extsock_error_t (*initiate_connection)(strongswan_adapter_t *this,
                                          peer_cfg_t *cfg);
    
    void (*destroy)(strongswan_adapter_t *this);
};

// Factory functions
strongswan_adapter_t *strongswan_adapter_create();
strongswan_adapter_t *strongswan_test_adapter_create(); // For testing
```

#### 2.2 Test Double Implementation
```c
// File: src/libcharon/plugins/extsock/test/adapters/strongswan_test_adapter.c

typedef struct test_strongswan_adapter_t {
    strongswan_adapter_t public;
    
    // Test state
    int config_creation_count;
    int connection_attempts;
    bool simulate_failure;
    char last_remote_addr[64];
} test_strongswan_adapter_t;

static ike_cfg_t* test_create_ike_config(strongswan_adapter_t *this, 
                                        const char *remote_addr, 
                                        ike_cfg_t *original) {
    test_strongswan_adapter_t *test_this = (test_strongswan_adapter_t*)this;
    
    // Record call for verification
    test_this->config_creation_count++;
    strncpy(test_this->last_remote_addr, remote_addr, sizeof(test_this->last_remote_addr));
    
    if (test_this->simulate_failure) {
        return NULL;
    }
    
    // Return mock config
    return (ike_cfg_t*)create_mock_ike_cfg(NULL, remote_addr);
}

static extsock_error_t test_initiate_connection(strongswan_adapter_t *this,
                                               peer_cfg_t *cfg) {
    test_strongswan_adapter_t *test_this = (test_strongswan_adapter_t*)this;
    test_this->connection_attempts++;
    
    return test_this->simulate_failure ? EXTSOCK_ERROR_CONNECTION_FAILED : EXTSOCK_SUCCESS;
}
```

#### 2.3 Failover Manager 리팩토링
```c
// File: src/libcharon/plugins/extsock/usecases/extsock_failover_manager.c

// Constructor에 adapter 주입
extsock_failover_manager_t *extsock_failover_manager_create(
    extsock_config_usecase_t *config_usecase,
    strongswan_adapter_t *strongswan_adapter) {
    
    private_extsock_failover_manager_t *this = malloc(sizeof(*this));
    
    this->config_usecase = config_usecase;
    this->strongswan_adapter = strongswan_adapter;  // 의존성 주입
    
    // ... rest of initialization
}

// create_failover_config에서 adapter 사용
METHOD(extsock_failover_manager_t, create_failover_config, extsock_error_t,
    private_extsock_failover_manager_t *this, peer_cfg_t *original_cfg, const char *next_segw_addr) {
    
    // strongSwan adapter를 통해 config 생성
    ike_cfg_t *new_ike_cfg = this->strongswan_adapter->create_ike_config(
        this->strongswan_adapter, next_segw_addr, original_ike_cfg);
    
    // ... rest of implementation using adapter
}
```

### Phase 3: Integration Test 환경

#### 3.1 strongSwan 개발 환경 설정
```bash
# File: scripts/setup_strongswan_dev.sh

#!/bin/bash
# strongSwan 개발 환경 설정

# 의존성 설치
sudo apt-get update
sudo apt-get install -y \
    strongswan strongswan-dev \
    libstrongswan-dev libcharon-extra-plugins \
    build-essential autotools-dev \
    libgtest-dev libgmock-dev

# strongSwan 헤더 경로 확인
STRONGSWAN_INCLUDE="/usr/include/strongswan"
if [ ! -d "$STRONGSWAN_INCLUDE" ]; then
    echo "Error: strongSwan headers not found"
    exit 1
fi

# 테스트 빌드용 Makefile 생성
cat > Makefile.integration << 'EOF'
STRONGSWAN_CFLAGS = -I/usr/include/strongswan -DWITH_STRONGSWAN
STRONGSWAN_LIBS = -lstrongswan -lcharon

integration_tests: test_with_strongswan.cpp
	g++ -std=c++17 $(STRONGSWAN_CFLAGS) \
	    test_with_strongswan.cpp -o integration_tests \
	    $(STRONGSWAN_LIBS) -lgtest -lgtest_main -lpthread

.PHONY: run-integration
run-integration: integration_tests
	sudo ./integration_tests
EOF

echo "strongSwan development environment setup complete"
```

#### 3.2 Integration Test Suite
```cpp
// File: src/libcharon/plugins/extsock/test/integration/test_strongswan_integration.cpp

#include <gtest/gtest.h>

#ifdef WITH_STRONGSWAN
#include <library.h>
#include <daemon.h>
#include <sa/ike_sa.h>
#include <config/peer_cfg.h>
#endif

class StrongSwanIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef WITH_STRONGSWAN
        // Initialize strongSwan library
        library_init(NULL, "test");
        
        // Create real failover manager with strongSwan
        real_failover_manager = extsock_failover_manager_create(
            config_usecase, strongswan_adapter_create());
#else
        GTEST_SKIP() << "strongSwan integration not available";
#endif
    }
    
    void TearDown() override {
#ifdef WITH_STRONGSWAN
        if (real_failover_manager) {
            real_failover_manager->destroy(real_failover_manager);
        }
        library_deinit();
#endif
    }
    
#ifdef WITH_STRONGSWAN
    extsock_failover_manager_t *real_failover_manager;
#endif
};

#ifdef WITH_STRONGSWAN
TEST_F(StrongSwanIntegrationTest, CreateFailoverConfig_RealStrongSwan_Success) {
    // Create real peer_cfg using strongSwan APIs
    ike_cfg_create_t ike_data = {
        .version = IKEV2,
        .local = "127.0.0.1",
        .remote = "10.1.1.1,10.1.1.2,10.1.1.3",
        .local_port = 500,
        .remote_port = 500,
    };
    
    ike_cfg_t *ike_cfg = ike_cfg_create(&ike_data);
    ASSERT_NE(ike_cfg, nullptr);
    
    peer_cfg_create_t peer_data = {
        .unique = UNIQUE_REPLACE,
        .keyingtries = 3,
        .rekey_time = 3600,
        .reauth_time = 14400,
    };
    
    peer_cfg_t *peer_cfg = peer_cfg_create("test-integration", ike_cfg, &peer_data);
    ASSERT_NE(peer_cfg, nullptr);
    
    // Test failover config creation
    extsock_error_t result = real_failover_manager->create_failover_config(
        real_failover_manager, peer_cfg, "10.1.1.2");
    
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    
    // Cleanup
    peer_cfg->destroy(peer_cfg);
}

TEST_F(StrongSwanIntegrationTest, HandleConnectionFailure_RealIkeSa_PerformsFailover) {
    // This test requires a more complex setup with real IKE SA
    // May need daemon integration or mock IKE SA creation
    GTEST_SKIP() << "Requires daemon integration - implement in Phase 4";
}
#endif
```

#### 3.3 Docker 기반 테스트 환경
```dockerfile
# File: docker/Dockerfile.integration

FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    strongswan strongswan-dev \
    libstrongswan-dev libcharon-extra-plugins \
    build-essential cmake \
    libgtest-dev libgmock-dev \
    && rm -rf /var/lib/apt/lists/*

# Build Google Test
RUN cd /usr/src/gtest && \
    cmake . && make && \
    cp lib/*.a /usr/lib/

# Set up workspace
WORKDIR /workspace
COPY . .

# Build integration tests
RUN make -f Makefile.integration

# Run tests
CMD ["./scripts/run_integration_tests.sh"]
```

```bash
# File: scripts/run_integration_tests.sh

#!/bin/bash
set -e

echo "🚀 Running strongSwan Integration Tests"

# Run unit tests first
echo "📋 Phase 1: Unit Tests (Mock-based)"
./build/final_integration_test

# Run integration tests
echo "📋 Phase 2: Integration Tests (strongSwan-based)"
if [ -f "./integration_tests" ]; then
    sudo ./integration_tests
else
    echo "⚠️  Integration tests not built - run 'make integration_tests' first"
fi

# Generate coverage report if available
if command -v gcov &> /dev/null; then
    echo "📊 Generating coverage report"
    gcov src/libcharon/plugins/extsock/usecases/extsock_failover_manager.c
fi

echo "✅ All tests completed"
```

### Phase 4: System Test (고급)

#### 4.1 strongSwan Daemon 통합 테스트
```bash
# File: scripts/system_test.sh

#!/bin/bash
# 실제 strongSwan daemon과 통합 테스트

# Start strongSwan daemon in test mode
sudo systemctl start strongswan
sleep 2

# Load test configuration
sudo swanctl --load-all

# Run system-level failover test
python3 scripts/test_real_failover.py

# Cleanup
sudo systemctl stop strongswan
```

#### 4.2 네트워크 환경 시뮬레이션
```python
# File: scripts/test_real_failover.py

import subprocess
import time
import json

def test_real_segw_failover():
    """실제 네트워크 환경에서 SEGW failover 테스트"""
    
    # 1. 초기 연결 설정
    config = {
        "connections": {
            "test-failover": {
                "remote_addrs": "192.168.1.100,192.168.1.101",
                "local_addrs": "192.168.1.10",
                "proposals": ["aes256-sha256-modp2048"]
            }
        }
    }
    
    # 2. 연결 시작
    result = subprocess.run(["swanctl", "--initiate", "--child", "test-failover"], 
                          capture_output=True, text=True)
    
    # 3. 첫 번째 SEGW 실패 시뮬레이션
    # (iptables로 첫 번째 주소 차단)
    subprocess.run(["iptables", "-A", "OUTPUT", "-d", "192.168.1.100", "-j", "DROP"])
    
    # 4. Failover 발생 대기 및 확인
    time.sleep(10)
    
    # 5. 두 번째 SEGW로 연결 확인
    status = subprocess.run(["swanctl", "--list-sas"], 
                          capture_output=True, text=True)
    
    assert "192.168.1.101" in status.stdout, "Failover to second SEGW failed"
    
    # Cleanup
    subprocess.run(["iptables", "-D", "OUTPUT", "-d", "192.168.1.100", "-j", "DROP"])
    
    print("✅ Real SEGW failover test passed")

if __name__ == "__main__":
    test_real_segw_failover()
```

## 📈 예상 커버리지 향상 로드맵

| Phase | 커버리지 | 소요 시간 | 완료 조건 |
|-------|----------|-----------|-----------|
| **현재** | 80% | - | 핵심 로직 완료 |
| **Phase 1** | 95% | 6-8시간 | Mock Framework 완료 |
| **Phase 2** | 98% | 4-6시간 | DI 리팩토링 완료 |  
| **Phase 3** | 99% | 3-4시간 | Integration 환경 구축 |
| **Phase 4** | 100% | 8-12시간 | System Test 완료 |

## 🎯 즉시 실행 가능한 액션 아이템

### 우선순위 1 (즉시 실행)
1. **Mock Framework 구현** (2-3시간)
   - strongswan_mocks.h 생성
   - Mock factory functions 구현
   - create_failover_config Mock 테스트 작성

2. **기존 테스트 확장** (1-2시간)
   - handle_connection_failure Mock 테스트 추가
   - Error path 테스트 케이스 보강

### 우선순위 2 (단기간 내)
1. **Dependency Injection 리팩토링** (3-4시간)
   - strongswan_adapter 인터페이스 생성
   - Failover Manager 리팩토링
   - Test Double 구현

2. **Integration 환경 구축** (2-3시간)
   - strongSwan 개발 환경 설정
   - Docker 기반 테스트 환경 구축
   - CI/CD 파이프라인 통합

### 우선순위 3 (장기)
1. **System Test 구현** (8-12시간)
   - strongSwan daemon 통합
   - 실제 네트워크 환경 테스트
   - 성능 및 안정성 테스트

## 📋 체크리스트

### Phase 1 완료 조건
- [ ] Mock Objects 구현 완료
- [ ] create_failover_config Mock 테스트 통과
- [ ] handle_connection_failure Mock 테스트 통과
- [ ] Error path 커버리지 95% 이상

### Phase 2 완료 조건  
- [ ] strongSwan Adapter 인터페이스 구현
- [ ] Dependency Injection 리팩토링 완료
- [ ] Test Double 모든 케이스 커버
- [ ] 기존 테스트 모두 통과

### Phase 3 완료 조건
- [ ] strongSwan 개발 환경 정상 동작
- [ ] Integration 테스트 통과
- [ ] Docker 환경에서 자동 테스트 실행
- [ ] CI/CD 파이프라인 통합

### Phase 4 완료 조건
- [ ] strongSwan daemon 통합 테스트 통과
- [ ] 실제 네트워크 failover 테스트 성공
- [ ] 성능 benchmarking 완료
- [ ] 메모리 leak 검증 완료

---

이 계획서를 통해 strongSwan 의존성 문제를 단계적으로 해결하고 100% 테스트 커버리지를 달성할 수 있습니다. 각 Phase는 독립적으로 실행 가능하며, 프로젝트 요구사항에 따라 우선순위를 조정할 수 있습니다.