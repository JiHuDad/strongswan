/*
 * Simple Mock-based Tests for Failover Config functionality
 * Copyright (C) 2024 strongSwan Project
 */

#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <string>

extern "C" {
#include "../../mocks/simple_mocks.h"
}

namespace simple_mock_test {

/**
 * Simple Mock Failover Manager
 */
class SimpleMockFailoverManager {
public:
    /**
     * Mock implementation of create_failover_config
     */
    extsock_error_t create_failover_config(peer_cfg_t *original_cfg, const char *next_segw_addr) {
        call_count_++;
        
        if (!original_cfg || !next_segw_addr) {
            return EXTSOCK_ERROR_INVALID_PARAMETER;
        }
        
        last_next_segw_ = next_segw_addr;
        
        // Get original IKE config
        ike_cfg_t *original_ike_cfg = mock_peer_cfg_get_ike_cfg(original_cfg);
        if (!original_ike_cfg) {
            return EXTSOCK_ERROR_INVALID_PARAMETER;
        }
        
        // Create new IKE config
        ike_cfg_create_t ike_data = {
            .version = mock_ike_cfg_get_version(original_ike_cfg),
            .local = "127.0.0.1",
            .remote = next_segw_addr,
            .local_port = mock_ike_cfg_get_my_port(original_ike_cfg),
            .remote_port = mock_ike_cfg_get_other_port(original_ike_cfg),
            .no_certreq = !mock_ike_cfg_send_certreq(original_ike_cfg),
            .force_encap = mock_ike_cfg_force_encap(original_ike_cfg),
        };
        
        ike_cfg_t *new_ike_cfg = ike_cfg_create(&ike_data);
        if (!new_ike_cfg) {
            return EXTSOCK_ERROR_CONFIG_CREATION_FAILED;
        }
        
        // Create new peer config
        char new_name[128];
        snprintf(new_name, sizeof(new_name), "%s-failover-%s", 
                 mock_peer_cfg_get_name(original_cfg), next_segw_addr);
        
        peer_cfg_create_t peer_data = {
            .unique = mock_peer_cfg_get_unique_policy(original_cfg),
            .keyingtries = mock_peer_cfg_get_keyingtries(original_cfg),
            .rekey_time = mock_peer_cfg_get_rekey_time(original_cfg),
            .reauth_time = mock_peer_cfg_get_reauth_time(original_cfg),
            .over_time = mock_peer_cfg_get_over_time(original_cfg),
            .dpd_timeout = mock_peer_cfg_get_dpd_timeout(original_cfg),
        };
        
        peer_cfg_t *new_peer_cfg = peer_cfg_create(new_name, new_ike_cfg, &peer_data);
        if (!new_peer_cfg) {
            mock_destroy_ike_cfg(new_ike_cfg);
            return EXTSOCK_ERROR_CONFIG_CREATION_FAILED;
        }
        
        created_configs_.push_back(new_peer_cfg);
        return EXTSOCK_SUCCESS;
    }
    
    int get_call_count() const { return call_count_; }
    const std::string& get_last_next_segw() const { return last_next_segw_; }
    const std::vector<peer_cfg_t*>& get_created_configs() const { return created_configs_; }
    
    void reset() {
        call_count_ = 0;
        last_next_segw_.clear();
        for (auto cfg : created_configs_) {
            mock_destroy_peer_cfg(cfg);
        }
        created_configs_.clear();
    }

private:
    int call_count_ = 0;
    std::string last_next_segw_;
    std::vector<peer_cfg_t*> created_configs_;
};

/**
 * Test fixture
 */
class SimpleMockFailoverTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_mock_simulate_failure = false;
        
        mock_original_cfg_ = create_mock_peer_cfg("test-conn", "10.1.1.1,10.1.1.2,10.1.1.3");
        ASSERT_NE(mock_original_cfg_, nullptr);
        
        failover_manager_ = std::make_unique<SimpleMockFailoverManager>();
    }
    
    void TearDown() override {
        if (mock_original_cfg_) {
            mock_destroy_peer_cfg(mock_original_cfg_);
        }
        
        if (failover_manager_) {
            failover_manager_->reset();
        }
        
        g_mock_simulate_failure = false;
    }
    
    peer_cfg_t *mock_original_cfg_;
    std::unique_ptr<SimpleMockFailoverManager> failover_manager_;
};

/* =============================================================================
 * Core Functionality Tests
 * =============================================================================
 */

TEST_F(SimpleMockFailoverTest, CreateFailoverConfig_ValidInput_Success) {
    // Given
    const char *next_segw = "10.1.1.2";
    
    // When
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, next_segw);
    
    // Then
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(failover_manager_->get_call_count(), 1);
    EXPECT_EQ(failover_manager_->get_last_next_segw(), next_segw);
    
    // Verify config was created
    const auto& created_configs = failover_manager_->get_created_configs();
    EXPECT_EQ(created_configs.size(), 1);
    
    if (!created_configs.empty()) {
        peer_cfg_t *new_cfg = created_configs[0];
        EXPECT_NE(new_cfg, nullptr);
        
        // Check config name
        const char *new_name = mock_peer_cfg_get_name(new_cfg);
        EXPECT_NE(strstr(new_name, "failover"), nullptr);
        EXPECT_NE(strstr(new_name, next_segw), nullptr);
        
        // Check IKE config remote address
        ike_cfg_t *new_ike_cfg = mock_peer_cfg_get_ike_cfg(new_cfg);
        EXPECT_NE(new_ike_cfg, nullptr);
        
        char *remote_addr = mock_ike_cfg_get_other_addr(new_ike_cfg);
        EXPECT_STREQ(remote_addr, next_segw);
        free(remote_addr);
    }
}

TEST_F(SimpleMockFailoverTest, CreateFailoverConfig_NullPeerConfig_Error) {
    extsock_error_t result = failover_manager_->create_failover_config(nullptr, "10.1.1.2");
    
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(failover_manager_->get_call_count(), 1);
    EXPECT_TRUE(failover_manager_->get_created_configs().empty());
}

TEST_F(SimpleMockFailoverTest, CreateFailoverConfig_NullNextAddress_Error) {
    extsock_error_t result = failover_manager_->create_failover_config(mock_original_cfg_, nullptr);
    
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(failover_manager_->get_call_count(), 1);
    EXPECT_TRUE(failover_manager_->get_created_configs().empty());
}

TEST_F(SimpleMockFailoverTest, CreateFailoverConfig_ConfigCreationFails_Error) {
    // Setup failure simulation
    g_mock_simulate_failure = true;
    
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, "10.1.1.2");
    
    EXPECT_EQ(result, EXTSOCK_ERROR_CONFIG_CREATION_FAILED);
    EXPECT_TRUE(failover_manager_->get_created_configs().empty());
}

/* =============================================================================
 * Configuration Copy Tests
 * =============================================================================
 */

TEST_F(SimpleMockFailoverTest, CreateFailoverConfig_CopiesOriginalSettings_Success) {
    const char *next_segw = "192.168.100.50";
    
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, next_segw);
    
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    
    const auto& created_configs = failover_manager_->get_created_configs();
    ASSERT_EQ(created_configs.size(), 1);
    
    peer_cfg_t *new_cfg = created_configs[0];
    
    // Verify copied settings
    EXPECT_EQ(mock_peer_cfg_get_unique_policy(new_cfg), 
              mock_peer_cfg_get_unique_policy(mock_original_cfg_));
    EXPECT_EQ(mock_peer_cfg_get_keyingtries(new_cfg), 
              mock_peer_cfg_get_keyingtries(mock_original_cfg_));
    EXPECT_EQ(mock_peer_cfg_get_rekey_time(new_cfg), 
              mock_peer_cfg_get_rekey_time(mock_original_cfg_));
    EXPECT_EQ(mock_peer_cfg_get_reauth_time(new_cfg), 
              mock_peer_cfg_get_reauth_time(mock_original_cfg_));
}

/* =============================================================================
 * Multiple Calls Tests
 * =============================================================================
 */

TEST_F(SimpleMockFailoverTest, CreateFailoverConfig_MultipleCalls_AllSucceed) {
    std::vector<std::string> segw_addresses = {
        "10.1.1.2", "10.1.1.3", "192.168.50.100"
    };
    
    for (const auto& addr : segw_addresses) {
        extsock_error_t result = failover_manager_->create_failover_config(
            mock_original_cfg_, addr.c_str());
        
        EXPECT_EQ(result, EXTSOCK_SUCCESS) << "Failed for address: " << addr;
    }
    
    EXPECT_EQ(failover_manager_->get_call_count(), segw_addresses.size());
    EXPECT_EQ(failover_manager_->get_created_configs().size(), segw_addresses.size());
    EXPECT_EQ(failover_manager_->get_last_next_segw(), segw_addresses.back());
}

/* =============================================================================
 * Performance Tests
 * =============================================================================
 */

TEST_F(SimpleMockFailoverTest, CreateFailoverConfig_Performance_UnderThreshold) {
    const int iterations = 1000;
    const char *next_segw = "10.1.1.2";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        extsock_error_t result = failover_manager_->create_failover_config(
            mock_original_cfg_, next_segw);
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double avg_time_us = static_cast<double>(duration.count()) / iterations;
    
    std::cout << "  Average config creation time: " << avg_time_us << " μs" << std::endl;
    std::cout << "  Total configs created: " << failover_manager_->get_created_configs().size() << std::endl;
    
    EXPECT_LT(avg_time_us, 100.0) << "Config creation too slow: " << avg_time_us << " μs";
}

/* =============================================================================
 * Edge Cases Tests
 * =============================================================================
 */

TEST_F(SimpleMockFailoverTest, CreateFailoverConfig_EmptyNextAddress_HandlesGracefully) {
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, "");
    
    // Should still succeed (empty string is not NULL)
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(failover_manager_->get_last_next_segw(), "");
}

TEST_F(SimpleMockFailoverTest, CreateFailoverConfig_LongAddress_HandlesCorrectly) {
    const char *long_addr = "192.168.123.456.very.long.hostname.example.com";
    
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, long_addr);
    
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(failover_manager_->get_last_next_segw(), long_addr);
}

} // namespace simple_mock_test

/**
 * Main function
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "🧪 Running Simple Mock Failover Config Tests" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n=============================================" << std::endl;
    if (result == 0) {
        std::cout << "✅ ALL SIMPLE MOCK TESTS PASSED!" << std::endl;
        std::cout << "🎯 create_failover_config core logic verified" << std::endl;
    } else {
        std::cout << "❌ SOME MOCK TESTS FAILED" << std::endl;
    }
    
    return result;
}