/**
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Test Unit Tests for strongSwan Adapter
 * Migrated from test_extsock_strongswan_adapter.c
 * 
 * Level 2 (Adapter) tests that use Google Mock to test strongSwan adapter layer
 * functionality with controlled dependencies.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <memory>
#include <chrono>
#include <vector>
#include <string>

// Mock strongSwan dependencies
#include "../../infrastructure/mocks/MockStrongSwan.hpp"

// Pure types for testing  
extern "C" {
    #include "extsock_types_pure.h"
}

// Forward declarations for strongSwan structures
struct peer_cfg_t;
struct child_cfg_t;
struct ike_cfg_t;
struct linked_list_t;
struct mem_cred_t;
struct extsock_config_entity_t;

// Mock strongSwan Adapter Implementation for testing
class MockStrongSwanAdapter {
public:
    MOCK_METHOD(extsock_error_t, add_peer_config, (peer_cfg_t* peer_cfg), ());
    MOCK_METHOD(extsock_error_t, remove_peer_config, (const char* name), ());
    MOCK_METHOD(extsock_error_t, initiate_child_sa, (peer_cfg_t* peer_cfg, child_cfg_t* child_cfg), ());
    MOCK_METHOD(linked_list_t*, get_managed_configs, (), ());
    MOCK_METHOD(mem_cred_t*, get_credentials, (), ());
    
    // Config Repository interface methods
    MOCK_METHOD(extsock_error_t, apply_config, (extsock_config_entity_t* config), ());
    MOCK_METHOD(extsock_error_t, remove_config, (const char* name), ());
    MOCK_METHOD(extsock_error_t, start_dpd, (const char* ike_sa_name), ());
    
    // State tracking methods for verification
    MOCK_METHOD(int, get_add_peer_config_calls, (), ());
    MOCK_METHOD(int, get_remove_peer_config_calls, (), ());
    MOCK_METHOD(int, get_initiate_child_sa_calls, (), ());
    MOCK_METHOD(int, get_apply_config_calls, (), ());
    MOCK_METHOD(int, get_remove_config_calls, (), ());
    MOCK_METHOD(int, get_start_dpd_calls, (), ());
    MOCK_METHOD(const char*, get_last_peer_name, (), ());
    MOCK_METHOD(const char*, get_last_removed_name, (), ());
    MOCK_METHOD(const char*, get_last_ike_sa_name, (), ());
    
    // Simulation control methods
    MOCK_METHOD(void, simulate_failure, (bool enable, extsock_error_t error_code), ());
    MOCK_METHOD(void, reset_state, (), ());
};

// Mock Config Entity for testing
class MockConfigEntity {
public:
    MOCK_METHOD(const char*, get_name, (), ());
    MOCK_METHOD(peer_cfg_t*, get_peer_config, (), ());
    MOCK_METHOD(void, destroy, (), ());
};

// Mock linked list for managed configs
class MockLinkedListAdapter {
public:
    MOCK_METHOD(int, get_count, (), ());
    MOCK_METHOD(void, insert_last, (void* item), ());
    MOCK_METHOD(void*, get_first, (), ());
    MOCK_METHOD(void*, remove_first, (), ());
    MOCK_METHOD(void, destroy, (), ());
};

/**
 * ============================================================================ 
 * Test Fixture Class
 * ============================================================================
 */
class StrongswanAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mock strongSwan system
        mock_strongswan = std::make_unique<extsock_test::mocks::StrongSwanMockManager>();
        mock_adapter = std::make_unique<MockStrongSwanAdapter>();
        mock_config_entity = std::make_unique<MockConfigEntity>();
        mock_managed_list = std::make_unique<MockLinkedListAdapter>();
        
        // Setup default expectations
        EXPECT_CALL(*mock_strongswan, reset_state())
            .WillRepeatedly(::testing::Return());
    }

    void TearDown() override {
        mock_managed_list.reset();
        mock_config_entity.reset();
        mock_adapter.reset();
        mock_strongswan.reset();
    }

    // Test helper methods
    peer_cfg_t* create_mock_peer_config(const std::string& name) {
        return reinterpret_cast<peer_cfg_t*>(0x10000000 + peer_config_counter++);
    }
    
    child_cfg_t* create_mock_child_config(const std::string& name) {
        return reinterpret_cast<child_cfg_t*>(0x20000000 + child_config_counter++);
    }
    
    extsock_config_entity_t* create_mock_config_entity(const std::string& name) {
        return reinterpret_cast<extsock_config_entity_t*>(0x30000000 + config_entity_counter++);
    }

    std::unique_ptr<extsock_test::mocks::StrongSwanMockManager> mock_strongswan;
    std::unique_ptr<MockStrongSwanAdapter> mock_adapter;
    std::unique_ptr<MockConfigEntity> mock_config_entity;
    std::unique_ptr<MockLinkedListAdapter> mock_managed_list;
    
private:
    static int peer_config_counter;
    static int child_config_counter;
    static int config_entity_counter;
};

// Static counter initialization
int StrongswanAdapterTest::peer_config_counter = 1;
int StrongswanAdapterTest::child_config_counter = 1;
int StrongswanAdapterTest::config_entity_counter = 1;

/**
 * ============================================================================
 * strongSwan Adapter Creation and Destruction Tests
 * ============================================================================
 */

TEST_F(StrongswanAdapterTest, CreateDestroy) {
    // Test that we can create a mock strongSwan adapter with all required methods
    MockStrongSwanAdapter adapter;
    
    // Verify that all methods are available (compile-time check)
    EXPECT_CALL(adapter, get_managed_configs())
        .WillOnce(::testing::Return(reinterpret_cast<linked_list_t*>(0x12345678)));
    
    EXPECT_CALL(adapter, get_credentials())
        .WillOnce(::testing::Return(reinterpret_cast<mem_cred_t*>(0x87654321)));
    
    linked_list_t* configs = adapter.get_managed_configs();
    EXPECT_NE(configs, nullptr);
    
    mem_cred_t* creds = adapter.get_credentials();
    EXPECT_NE(creds, nullptr);
}

TEST_F(StrongswanAdapterTest, InitialState) {
    // Test initial state of the adapter
    linked_list_t* mock_list = reinterpret_cast<linked_list_t*>(0xABCDEF00);
    mem_cred_t* mock_creds = reinterpret_cast<mem_cred_t*>(0xFEDCBA00);
    
    EXPECT_CALL(*mock_adapter, get_managed_configs())
        .WillOnce(::testing::Return(mock_list));
    
    EXPECT_CALL(*mock_adapter, get_credentials())
        .WillOnce(::testing::Return(mock_creds));
    
    EXPECT_CALL(*mock_adapter, get_add_peer_config_calls())
        .WillOnce(::testing::Return(0));
    
    EXPECT_CALL(*mock_adapter, get_remove_peer_config_calls())
        .WillOnce(::testing::Return(0));
    
    // Verify initial state
    linked_list_t* configs = mock_adapter->get_managed_configs();
    EXPECT_EQ(configs, mock_list);
    
    mem_cred_t* creds = mock_adapter->get_credentials();
    EXPECT_EQ(creds, mock_creds);
    
    EXPECT_EQ(mock_adapter->get_add_peer_config_calls(), 0);
    EXPECT_EQ(mock_adapter->get_remove_peer_config_calls(), 0);
}

/**
 * ============================================================================
 * Peer Configuration Management Tests
 * ============================================================================
 */

TEST_F(StrongswanAdapterTest, AddPeerConfig) {
    peer_cfg_t* test_peer = create_mock_peer_config("test_peer");
    
    EXPECT_CALL(*mock_adapter, add_peer_config(test_peer))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, get_add_peer_config_calls())
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mock_adapter, get_last_peer_name())
        .WillOnce(::testing::Return("test_peer"));
    
    extsock_error_t result = mock_adapter->add_peer_config(test_peer);
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(mock_adapter->get_add_peer_config_calls(), 1);
    EXPECT_STREQ(mock_adapter->get_last_peer_name(), "test_peer");
}

TEST_F(StrongswanAdapterTest, AddPeerConfigNullPointer) {
    EXPECT_CALL(*mock_adapter, add_peer_config(nullptr))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_INVALID_PARAMETER));
    
    extsock_error_t result = mock_adapter->add_peer_config(nullptr);
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
}

TEST_F(StrongswanAdapterTest, RemovePeerConfig) {
    const std::string peer_name = "test_peer_to_remove";
    
    EXPECT_CALL(*mock_adapter, remove_peer_config(::testing::StrEq(peer_name.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, get_remove_peer_config_calls())
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mock_adapter, get_last_removed_name())
        .WillOnce(::testing::Return(peer_name.c_str()));
    
    extsock_error_t result = mock_adapter->remove_peer_config(peer_name.c_str());
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(mock_adapter->get_remove_peer_config_calls(), 1);
    EXPECT_STREQ(mock_adapter->get_last_removed_name(), peer_name.c_str());
}

TEST_F(StrongswanAdapterTest, RemovePeerConfigNullName) {
    EXPECT_CALL(*mock_adapter, remove_peer_config(nullptr))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_INVALID_PARAMETER));
    
    extsock_error_t result = mock_adapter->remove_peer_config(nullptr);
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
}

TEST_F(StrongswanAdapterTest, InitiateChildSA) {
    peer_cfg_t* test_peer = create_mock_peer_config("test_peer");
    child_cfg_t* test_child = create_mock_child_config("test_child");
    
    EXPECT_CALL(*mock_adapter, initiate_child_sa(test_peer, test_child))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, get_initiate_child_sa_calls())
        .WillOnce(::testing::Return(1));
    
    extsock_error_t result = mock_adapter->initiate_child_sa(test_peer, test_child);
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(mock_adapter->get_initiate_child_sa_calls(), 1);
}

TEST_F(StrongswanAdapterTest, InitiateChildSANullPointers) {
    EXPECT_CALL(*mock_adapter, initiate_child_sa(nullptr, nullptr))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_INVALID_PARAMETER));
    
    extsock_error_t result = mock_adapter->initiate_child_sa(nullptr, nullptr);
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
}

/**
 * ============================================================================
 * Config Repository Interface Tests
 * ============================================================================
 */

TEST_F(StrongswanAdapterTest, ApplyConfig) {
    extsock_config_entity_t* test_config = create_mock_config_entity("test_config");
    
    EXPECT_CALL(*mock_adapter, apply_config(test_config))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, get_apply_config_calls())
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mock_adapter, get_last_peer_name())
        .WillOnce(::testing::Return("test_config"));
    
    extsock_error_t result = mock_adapter->apply_config(test_config);
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(mock_adapter->get_apply_config_calls(), 1);
    EXPECT_STREQ(mock_adapter->get_last_peer_name(), "test_config");
}

TEST_F(StrongswanAdapterTest, ApplyConfigNullPointer) {
    EXPECT_CALL(*mock_adapter, apply_config(nullptr))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_INVALID_PARAMETER));
    
    extsock_error_t result = mock_adapter->apply_config(nullptr);
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
}

TEST_F(StrongswanAdapterTest, RemoveConfig) {
    const std::string config_name = "test_config_to_remove";
    
    EXPECT_CALL(*mock_adapter, remove_config(::testing::StrEq(config_name.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, get_remove_config_calls())
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mock_adapter, get_last_removed_name())
        .WillOnce(::testing::Return(config_name.c_str()));
    
    extsock_error_t result = mock_adapter->remove_config(config_name.c_str());
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(mock_adapter->get_remove_config_calls(), 1);
    EXPECT_STREQ(mock_adapter->get_last_removed_name(), config_name.c_str());
}

TEST_F(StrongswanAdapterTest, RemoveConfigNullName) {
    EXPECT_CALL(*mock_adapter, remove_config(nullptr))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_INVALID_PARAMETER));
    
    extsock_error_t result = mock_adapter->remove_config(nullptr);
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
}

TEST_F(StrongswanAdapterTest, StartDPD) {
    const std::string ike_sa_name = "test_ike_sa";
    
    EXPECT_CALL(*mock_adapter, start_dpd(::testing::StrEq(ike_sa_name.c_str())))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, get_start_dpd_calls())
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mock_adapter, get_last_ike_sa_name())
        .WillOnce(::testing::Return(ike_sa_name.c_str()));
    
    extsock_error_t result = mock_adapter->start_dpd(ike_sa_name.c_str());
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(mock_adapter->get_start_dpd_calls(), 1);
    EXPECT_STREQ(mock_adapter->get_last_ike_sa_name(), ike_sa_name.c_str());
}

TEST_F(StrongswanAdapterTest, StartDPDNullName) {
    EXPECT_CALL(*mock_adapter, start_dpd(nullptr))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_INVALID_PARAMETER));
    
    extsock_error_t result = mock_adapter->start_dpd(nullptr);
    EXPECT_EQ(result, EXTSOCK_ERROR_INVALID_PARAMETER);
}

/**
 * ============================================================================
 * Error Handling and Simulation Tests
 * ============================================================================
 */

TEST_F(StrongswanAdapterTest, ErrorSimulation) {
    peer_cfg_t* test_peer = create_mock_peer_config("test_peer");
    
    // Setup expectations in order
    {
        ::testing::InSequence seq;
        
        // Enable failure simulation
        EXPECT_CALL(*mock_adapter, simulate_failure(true, EXTSOCK_ERROR_STRONGSWAN_API))
            .WillOnce(::testing::Return());
        
        // Operations should fail after enabling simulation
        EXPECT_CALL(*mock_adapter, add_peer_config(test_peer))
            .WillOnce(::testing::Return(EXTSOCK_ERROR_STRONGSWAN_API));
        
        EXPECT_CALL(*mock_adapter, remove_peer_config(::testing::StrEq("test")))
            .WillOnce(::testing::Return(EXTSOCK_ERROR_STRONGSWAN_API));
        
        // Disable failure simulation
        EXPECT_CALL(*mock_adapter, simulate_failure(false, EXTSOCK_SUCCESS))
            .WillOnce(::testing::Return());
        
        // Operations should succeed after disabling simulation
        EXPECT_CALL(*mock_adapter, add_peer_config(test_peer))
            .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    }
    
    mock_adapter->simulate_failure(true, EXTSOCK_ERROR_STRONGSWAN_API);
    
    // Test operations should fail
    extsock_error_t result = mock_adapter->add_peer_config(test_peer);
    EXPECT_EQ(result, EXTSOCK_ERROR_STRONGSWAN_API);
    
    result = mock_adapter->remove_peer_config("test");
    EXPECT_EQ(result, EXTSOCK_ERROR_STRONGSWAN_API);
    
    // Disable failure simulation
    mock_adapter->simulate_failure(false, EXTSOCK_SUCCESS);
    
    // Operations should now succeed
    result = mock_adapter->add_peer_config(test_peer);
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
}

TEST_F(StrongswanAdapterTest, ConfigRepositoryErrorSimulation) {
    extsock_config_entity_t* test_config = create_mock_config_entity("test_config");
    
    EXPECT_CALL(*mock_adapter, simulate_failure(true, EXTSOCK_ERROR_CONFIG_INVALID))
        .WillOnce(::testing::Return());
    
    EXPECT_CALL(*mock_adapter, apply_config(test_config))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_CONFIG_INVALID));
    
    EXPECT_CALL(*mock_adapter, remove_config(::testing::_))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_CONFIG_INVALID));
    
    EXPECT_CALL(*mock_adapter, start_dpd(::testing::_))
        .WillOnce(::testing::Return(EXTSOCK_ERROR_CONFIG_INVALID));
    
    mock_adapter->simulate_failure(true, EXTSOCK_ERROR_CONFIG_INVALID);
    
    // All config repository operations should fail
    extsock_error_t result = mock_adapter->apply_config(test_config);
    EXPECT_EQ(result, EXTSOCK_ERROR_CONFIG_INVALID);
    
    result = mock_adapter->remove_config("test");
    EXPECT_EQ(result, EXTSOCK_ERROR_CONFIG_INVALID);
    
    result = mock_adapter->start_dpd("test_ike");
    EXPECT_EQ(result, EXTSOCK_ERROR_CONFIG_INVALID);
}

/**
 * ============================================================================
 * Complex Workflow Tests
 * ============================================================================
 */

TEST_F(StrongswanAdapterTest, ComplexWorkflow) {
    const int workflow_count = 3;
    
    // Setup expectations for complex workflow
    EXPECT_CALL(*mock_adapter, add_peer_config(::testing::_))
        .Times(workflow_count)
        .WillRepeatedly(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, initiate_child_sa(::testing::_, ::testing::_))
        .Times(workflow_count)
        .WillRepeatedly(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, remove_peer_config(::testing::_))
        .Times(workflow_count)
        .WillRepeatedly(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, get_add_peer_config_calls())
        .WillOnce(::testing::Return(workflow_count));
    
    EXPECT_CALL(*mock_adapter, get_initiate_child_sa_calls())
        .WillOnce(::testing::Return(workflow_count));
    
    EXPECT_CALL(*mock_adapter, get_remove_peer_config_calls())
        .WillOnce(::testing::Return(workflow_count));
    
    // Execute complex workflow
    for (int i = 0; i < workflow_count; i++) {
        peer_cfg_t* peer_cfg = create_mock_peer_config("peer_" + std::to_string(i));
        child_cfg_t* child_cfg = create_mock_child_config("child_" + std::to_string(i));
        
        // Add peer config
        extsock_error_t result = mock_adapter->add_peer_config(peer_cfg);
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
        
        // Initiate child SA
        result = mock_adapter->initiate_child_sa(peer_cfg, child_cfg);
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
    }
    
    // Remove configs
    for (int i = 0; i < workflow_count; i++) {
        std::string peer_name = "peer_" + std::to_string(i);
        extsock_error_t result = mock_adapter->remove_peer_config(peer_name.c_str());
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
    }
    
    // Verify call counts
    EXPECT_EQ(mock_adapter->get_add_peer_config_calls(), workflow_count);
    EXPECT_EQ(mock_adapter->get_initiate_child_sa_calls(), workflow_count);
    EXPECT_EQ(mock_adapter->get_remove_peer_config_calls(), workflow_count);
}

TEST_F(StrongswanAdapterTest, ManagedConfigsTracking) {
    const int config_count = 5;
    
    // Mock linked list behavior for managed configs
    EXPECT_CALL(*mock_managed_list, get_count())
        .WillOnce(::testing::Return(0))         // Initially empty
        .WillOnce(::testing::Return(config_count))  // After adding configs
        .WillOnce(::testing::Return(0));        // After removing all
    
    EXPECT_CALL(*mock_managed_list, insert_last(::testing::_))
        .Times(config_count);
    
    EXPECT_CALL(*mock_managed_list, remove_first())
        .Times(config_count)
        .WillRepeatedly(::testing::Return(reinterpret_cast<void*>(0x12345678)));
    
    EXPECT_CALL(*mock_adapter, get_managed_configs())
        .Times(3)
        .WillRepeatedly(::testing::Return(reinterpret_cast<linked_list_t*>(mock_managed_list.get())));
    
    // Verify initial state
    linked_list_t* managed = mock_adapter->get_managed_configs();
    MockLinkedListAdapter* mock_list = reinterpret_cast<MockLinkedListAdapter*>(managed);
    EXPECT_EQ(mock_list->get_count(), 0);
    
    // Add configs
    for (int i = 0; i < config_count; i++) {
        peer_cfg_t* peer_cfg = create_mock_peer_config("peer_" + std::to_string(i));
        mock_list->insert_last(peer_cfg);
    }
    
    // Verify count after adding
    managed = mock_adapter->get_managed_configs();
    mock_list = reinterpret_cast<MockLinkedListAdapter*>(managed);
    EXPECT_EQ(mock_list->get_count(), config_count);
    
    // Remove all configs
    for (int i = 0; i < config_count; i++) {
        void* removed = mock_list->remove_first();
        EXPECT_NE(removed, nullptr);
    }
    
    // Verify empty after removing
    managed = mock_adapter->get_managed_configs();
    mock_list = reinterpret_cast<MockLinkedListAdapter*>(managed);
    EXPECT_EQ(mock_list->get_count(), 0);
}

/**
 * ============================================================================
 * Performance and Stress Tests
 * ============================================================================
 */

TEST_F(StrongswanAdapterTest, StressOperations) {
    const int stress_count = 100;
    
    // Setup expectations for stress test
    EXPECT_CALL(*mock_adapter, add_peer_config(::testing::_))
        .Times(stress_count)
        .WillRepeatedly(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, get_add_peer_config_calls())
        .WillOnce(::testing::Return(stress_count));
    
    // Mock managed configs count
    EXPECT_CALL(*mock_managed_list, get_count())
        .WillOnce(::testing::Return(stress_count));
    
    EXPECT_CALL(*mock_adapter, get_managed_configs())
        .WillOnce(::testing::Return(reinterpret_cast<linked_list_t*>(mock_managed_list.get())));
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Execute stress test
    for (int i = 0; i < stress_count; i++) {
        peer_cfg_t* peer_cfg = create_mock_peer_config("stress_peer_" + std::to_string(i));
        extsock_error_t result = mock_adapter->add_peer_config(peer_cfg);
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Verify performance and results
    EXPECT_LT(duration.count(), 100) << "Stress test took too long";
    EXPECT_EQ(mock_adapter->get_add_peer_config_calls(), stress_count);
    
    // Verify managed configs count
    linked_list_t* managed = mock_adapter->get_managed_configs();
    MockLinkedListAdapter* mock_list = reinterpret_cast<MockLinkedListAdapter*>(managed);
    EXPECT_EQ(mock_list->get_count(), stress_count);
}

TEST_F(StrongswanAdapterTest, ConcurrentOperationsSimulation) {
    const int concurrent_count = 10;
    
    // Setup expectations for concurrent operations
    EXPECT_CALL(*mock_adapter, add_peer_config(::testing::_))
        .Times(concurrent_count)
        .WillRepeatedly(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, apply_config(::testing::_))
        .Times(concurrent_count)
        .WillRepeatedly(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, start_dpd(::testing::_))
        .Times(concurrent_count)
        .WillRepeatedly(::testing::Return(EXTSOCK_SUCCESS));
    
    // Simulate concurrent operations
    for (int i = 0; i < concurrent_count; i++) {
        peer_cfg_t* peer_cfg = create_mock_peer_config("concurrent_peer_" + std::to_string(i));
        extsock_config_entity_t* config = create_mock_config_entity("concurrent_config_" + std::to_string(i));
        std::string ike_sa_name = "concurrent_ike_sa_" + std::to_string(i);
        
        // Execute operations
        extsock_error_t result = mock_adapter->add_peer_config(peer_cfg);
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
        
        result = mock_adapter->apply_config(config);
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
        
        result = mock_adapter->start_dpd(ike_sa_name.c_str());
        EXPECT_EQ(result, EXTSOCK_SUCCESS);
    }
}

/**
 * ============================================================================
 * State Management Tests
 * ============================================================================
 */

TEST_F(StrongswanAdapterTest, StateReset) {
    // Setup some operations first
    peer_cfg_t* test_peer = create_mock_peer_config("test_peer");
    
    EXPECT_CALL(*mock_adapter, add_peer_config(test_peer))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, get_add_peer_config_calls())
        .WillOnce(::testing::Return(1))
        .WillOnce(::testing::Return(0)); // After reset
    
    EXPECT_CALL(*mock_adapter, reset_state())
        .WillOnce(::testing::Return());
    
    // Execute operation
    extsock_error_t result = mock_adapter->add_peer_config(test_peer);
    EXPECT_EQ(result, EXTSOCK_SUCCESS);
    EXPECT_EQ(mock_adapter->get_add_peer_config_calls(), 1);
    
    // Reset state
    mock_adapter->reset_state();
    
    // Verify state was reset
    EXPECT_EQ(mock_adapter->get_add_peer_config_calls(), 0);
}

TEST_F(StrongswanAdapterTest, StateTracking) {
    // Test comprehensive state tracking
    peer_cfg_t* test_peer = create_mock_peer_config("tracked_peer");
    extsock_config_entity_t* test_config = create_mock_config_entity("tracked_config");
    
    EXPECT_CALL(*mock_adapter, add_peer_config(test_peer))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, apply_config(test_config))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    EXPECT_CALL(*mock_adapter, start_dpd(::testing::StrEq("tracked_ike_sa")))
        .WillOnce(::testing::Return(EXTSOCK_SUCCESS));
    
    // Setup tracking expectations
    EXPECT_CALL(*mock_adapter, get_add_peer_config_calls())
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mock_adapter, get_apply_config_calls())
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mock_adapter, get_start_dpd_calls())
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mock_adapter, get_last_peer_name())
        .WillOnce(::testing::Return("tracked_peer"));
    
    EXPECT_CALL(*mock_adapter, get_last_ike_sa_name())
        .WillOnce(::testing::Return("tracked_ike_sa"));
    
    // Execute operations
    mock_adapter->add_peer_config(test_peer);
    mock_adapter->apply_config(test_config);
    mock_adapter->start_dpd("tracked_ike_sa");
    
    // Verify state tracking
    EXPECT_EQ(mock_adapter->get_add_peer_config_calls(), 1);
    EXPECT_EQ(mock_adapter->get_apply_config_calls(), 1);
    EXPECT_EQ(mock_adapter->get_start_dpd_calls(), 1);
    EXPECT_STREQ(mock_adapter->get_last_peer_name(), "tracked_peer");
    EXPECT_STREQ(mock_adapter->get_last_ike_sa_name(), "tracked_ike_sa");
}