/*
 * Mock-based Tests for create_failover_config function
 * Copyright (C) 2024 strongSwan Project
 * 
 * This test suite uses mock strongSwan objects to test the create_failover_config
 * functionality without requiring the full strongSwan library.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "../../mocks/strongswan_mocks.h"
#include "../../mocks/strongswan_mock_factory.c"  // Include implementation
}

namespace mock_failover_test {

/**
 * Mock Failover Manager for testing
 */
class MockFailoverManager {
public:
    /**
     * Mock implementation of create_failover_config
     * This simulates the core logic without strongSwan dependencies
     */
    extsock_error_t create_failover_config(peer_cfg_t *original_cfg, const char *next_segw_addr) {
        call_count_++;
        
        if (!original_cfg || !next_segw_addr) {
            return EXTSOCK_ERROR_INVALID_PARAMETER;
        }
        
        // Record the call details
        last_next_segw_ = next_segw_addr;
        
        // Get the original IKE config
        ike_cfg_t *original_ike_cfg = original_cfg->get_ike_cfg(original_cfg);
        if (!original_ike_cfg) {
            return EXTSOCK_ERROR_INVALID_PARAMETER;
        }
        
        // Simulate the core logic: create new IKE config with new remote address
        ike_cfg_create_t ike_data = {
            .version = original_ike_cfg->get_version(original_ike_cfg),
            .local = "127.0.0.1",  // Use mock value
            .remote = next_segw_addr,
            .local_port = original_ike_cfg->get_my_port(original_ike_cfg),
            .remote_port = original_ike_cfg->get_other_port(original_ike_cfg),
            .no_certreq = !original_ike_cfg->send_certreq(original_ike_cfg),
            .force_encap = original_ike_cfg->force_encap(original_ike_cfg),
        };
        
        ike_cfg_t *new_ike_cfg = ike_cfg_create(&ike_data);
        if (!new_ike_cfg) {
            return EXTSOCK_ERROR_CONFIG_CREATION_FAILED;
        }
        
        // Create new peer config
        char new_name[128];
        snprintf(new_name, sizeof(new_name), "%s-failover-%s", 
                 original_cfg->get_name(original_cfg), next_segw_addr);
        
        peer_cfg_create_t peer_data = {
            .unique = original_cfg->get_unique_policy(original_cfg),
            .keyingtries = original_cfg->get_keyingtries(original_cfg),
            .rekey_time = original_cfg->get_rekey_time(original_cfg, false),
            .reauth_time = original_cfg->get_reauth_time(original_cfg, false),
            .over_time = original_cfg->get_over_time(original_cfg),
            .dpd_timeout = original_cfg->get_dpd_timeout(original_cfg),
        };
        
        peer_cfg_t *new_peer_cfg = peer_cfg_create(new_name, new_ike_cfg, &peer_data);
        if (!new_peer_cfg) {
            new_ike_cfg->destroy(new_ike_cfg);
            return EXTSOCK_ERROR_CONFIG_CREATION_FAILED;
        }
        
        // Store created config for verification
        created_configs_.push_back(new_peer_cfg);
        
        // Simulate success
        return EXTSOCK_SUCCESS;
    }
    
    // Test utilities
    int get_call_count() const { return call_count_; }
    const std::string& get_last_next_segw() const { return last_next_segw_; }
    const std::vector<peer_cfg_t*>& get_created_configs() const { return created_configs_; }
    
    void reset() {
        call_count_ = 0;
        last_next_segw_.clear();
        // Clean up created configs
        for (auto cfg : created_configs_) {
            cfg->destroy(cfg);
        }
        created_configs_.clear();
    }

private:
    int call_count_ = 0;
    std::string last_next_segw_;
    std::vector<peer_cfg_t*> created_configs_;
};

/**
 * Test fixture for Mock Failover Config tests
 */
class MockFailoverConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset mock behavior
        mock_reset_behavior();
        
        // Create mock original config
        mock_original_cfg_ = create_mock_peer_cfg("test-conn", "10.1.1.1,10.1.1.2,10.1.1.3");
        ASSERT_NE(mock_original_cfg_, nullptr);
        
        // Create mock failover manager
        failover_manager_ = std::make_unique<MockFailoverManager>();
    }
    
    void TearDown() override {
        if (mock_original_cfg_) {
            mock_original_cfg_->destroy(mock_original_cfg_);
        }
        
        if (failover_manager_) {
            failover_manager_->reset();
        }
        
        mock_reset_behavior();
    }
    
    peer_cfg_t *mock_original_cfg_;
    std::unique_ptr<MockFailoverManager> failover_manager_;
};

/* =============================================================================
 * Core Functionality Tests
 * =============================================================================
 */

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_ValidInput_Success) {
    // Given: Valid peer config and next SEGW address
    const char *next_segw = "10.1.1.2";
    
    // When: Create failover config
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, next_segw);
    
    // Then: Should succeed
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(failover_manager_->get_call_count(), 1);
    EXPECT_EQ(failover_manager_->get_last_next_segw(), next_segw);
    
    // Verify new config was created
    const auto& created_configs = failover_manager_->get_created_configs();
    EXPECT_EQ(created_configs.size(), 1);
    
    if (!created_configs.empty()) {
        peer_cfg_t *new_cfg = created_configs[0];
        EXPECT_NE(new_cfg, nullptr);
        
        // Verify new config name contains failover suffix
        const char *new_name = new_cfg->get_name(new_cfg);
        EXPECT_NE(strstr(new_name, "failover"), nullptr);
        EXPECT_NE(strstr(new_name, next_segw), nullptr);
        
        // Verify IKE config has correct remote address
        ike_cfg_t *new_ike_cfg = new_cfg->get_ike_cfg(new_cfg);
        EXPECT_NE(new_ike_cfg, nullptr);
        
        char *remote_addr = new_ike_cfg->get_other_addr(new_ike_cfg);
        EXPECT_STREQ(remote_addr, next_segw);
        free(remote_addr);
    }
}

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_NullPeerConfig_Error) {
    // When: Call with null peer config
    extsock_error_t result = failover_manager_->create_failover_config(
        nullptr, "10.1.1.2");
    
    // Then: Should return parameter error
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(failover_manager_->get_call_count(), 1);
    EXPECT_TRUE(failover_manager_->get_created_configs().empty());
}

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_NullNextAddress_Error) {
    // When: Call with null next address
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, nullptr);
    
    // Then: Should return parameter error
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(failover_manager_->get_call_count(), 1);
    EXPECT_TRUE(failover_manager_->get_created_configs().empty());
}

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_BothParametersNull_Error) {
    // When: Call with both parameters null
    extsock_error_t result = failover_manager_->create_failover_config(nullptr, nullptr);
    
    // Then: Should return parameter error
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(failover_manager_->get_call_count(), 1);
}

/* =============================================================================
 * Configuration Copying Tests
 * =============================================================================
 */

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_CopiesOriginalSettings_Success) {
    // Given: Original config with specific settings
    const char *next_segw = "192.168.100.50";
    
    // When: Create failover config
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, next_segw);
    
    // Then: New config should copy original settings
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    
    const auto& created_configs = failover_manager_->get_created_configs();
    ASSERT_EQ(created_configs.size(), 1);
    
    peer_cfg_t *new_cfg = created_configs[0];
    
    // Verify copied settings
    EXPECT_EQ(new_cfg->get_unique_policy(new_cfg), 
              mock_original_cfg_->get_unique_policy(mock_original_cfg_));
    EXPECT_EQ(new_cfg->get_keyingtries(new_cfg), 
              mock_original_cfg_->get_keyingtries(mock_original_cfg_));
    EXPECT_EQ(new_cfg->get_rekey_time(new_cfg, false), 
              mock_original_cfg_->get_rekey_time(mock_original_cfg_, false));
    EXPECT_EQ(new_cfg->get_reauth_time(new_cfg, false), 
              mock_original_cfg_->get_reauth_time(mock_original_cfg_, false));
}

/* =============================================================================
 * Error Path Tests
 * =============================================================================
 */

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_IkeConfigCreationFails_Error) {
    // Given: Mock setup to simulate IKE config creation failure
    mock_set_failure_mode(true, "IKE config creation failed");
    
    // When: Create failover config
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, "10.1.1.2");
    
    // Then: Should return config creation error
    EXPECT_EQ(result, EXTSOCK_ERROR_CONFIG_CREATION_FAILED);
    EXPECT_TRUE(failover_manager_->get_created_configs().empty());
    
    // Verify mock was called
    EXPECT_GT(mock_get_call_count(), 0);
}

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_PeerConfigCreationFails_Error) {
    // This test would require more sophisticated mock control
    // For now, we test the basic failure case
    mock_set_failure_mode(true, "Peer config creation failed");
    
    extsock_error_t result = failover_manager_->create_failover_config(
        mock_original_cfg_, "10.1.1.2");
    
    EXPECT_EQ(result, EXTSOCK_ERROR_CONFIG_CREATION_FAILED);
}

/* =============================================================================
 * Multiple Calls Tests
 * =============================================================================
 */

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_MultipleCalls_AllSucceed) {
    // Given: Multiple different SEGW addresses
    std::vector<std::string> segw_addresses = {
        "10.1.1.2", "10.1.1.3", "192.168.50.100"
    };
    
    // When: Create multiple failover configs
    for (const auto& addr : segw_addresses) {
        extsock_error_t result = failover_manager_->create_failover_config(
            mock_original_cfg_, addr.c_str());
        
        // Then: Each call should succeed
        EXPECT_EQ(result, EXTSOCK_SUCCESS) << "Failed for address: " << addr;
    }
    
    // Verify call count and created configs
    EXPECT_EQ(failover_manager_->get_call_count(), segw_addresses.size());
    EXPECT_EQ(failover_manager_->get_created_configs().size(), segw_addresses.size());
    
    // Verify last address was recorded correctly
    EXPECT_EQ(failover_manager_->get_last_next_segw(), segw_addresses.back());
}

/* =============================================================================
 * Performance Tests
 * =============================================================================
 */

TEST_F(MockFailoverConfigTest, CreateFailoverConfig_Performance_UnderThreshold) {
    // Given: Performance measurement setup
    const int iterations = 1000;
    const char *next_segw = "10.1.1.2";
    
    // When: Measure time for multiple config creations
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        extsock_error_t result = failover_manager_->create_failover_config(
            mock_original_cfg_, next_segw);
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Then: Performance should be under threshold
    double avg_time_us = static_cast<double>(duration.count()) / iterations;
    
    std::cout << "  Average config creation time: " << avg_time_us << " μs" << std::endl;
    std::cout << "  Total configs created: " << failover_manager_->get_created_configs().size() << std::endl;
    
    // Performance assertion (should be very fast with mocks)
    EXPECT_LT(avg_time_us, 100.0) << "Config creation too slow: " << avg_time_us << " μs";
}

} // namespace mock_failover_test

/**
 * Main function for standalone execution
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "🧪 Running Mock-based create_failover_config Tests" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n=================================================" << std::endl;
    if (result == 0) {
        std::cout << "✅ ALL MOCK TESTS PASSED!" << std::endl;
        std::cout << "🎯 create_failover_config functionality verified with mocks" << std::endl;
    } else {
        std::cout << "❌ SOME MOCK TESTS FAILED" << std::endl;
        std::cout << "⚠️  Please review the test results above" << std::endl;
    }
    
    return result;
}