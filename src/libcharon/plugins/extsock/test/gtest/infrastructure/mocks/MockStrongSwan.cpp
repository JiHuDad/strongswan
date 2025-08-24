/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Mock based strongSwan API Mock Implementation
 * TASK-M002: Mock Infrastructure Construction
 */

#include "MockStrongSwan.hpp"
#include <memory>

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;

namespace extsock_test {
namespace mocks {

// StrongSwanMockManager Implementation

StrongSwanMockManager::StrongSwanMockManager() 
    : linked_list_mock_(std::make_unique<MockLinkedList>())
    , ike_config_mock_(std::make_unique<MockIkeConfig>())
    , peer_config_mock_(std::make_unique<MockPeerConfig>())
    , child_config_mock_(std::make_unique<MockChildConfig>())
    , auth_config_mock_(std::make_unique<MockAuthConfig>())
    , ike_sa_mock_(std::make_unique<MockIkeSA>())
    , child_sa_mock_(std::make_unique<MockChildSA>())
{
    // Constructor - mocks are created but not configured
}

StrongSwanMockManager::~StrongSwanMockManager() {
    // Destructor - unique_ptrs will clean up automatically
}

std::unique_ptr<MockLinkedList> StrongSwanMockManager::createLinkedListMock() {
    auto mock = std::make_unique<MockLinkedList>();
    
    // Set up default behaviors
    ON_CALL(*mock, get_count(_))
        .WillByDefault(Return(0));
    
    ON_CALL(*mock, get_first(_))
        .WillByDefault(Return(nullptr));
        
    ON_CALL(*mock, remove_first(_))
        .WillByDefault(Return(nullptr));
        
    ON_CALL(*mock, create_enumerator(_))
        .WillByDefault(Return(nullptr));
    
    return mock;
}

std::unique_ptr<MockIkeConfig> StrongSwanMockManager::createIkeConfigMock() {
    auto mock = std::make_unique<MockIkeConfig>();
    
    // Set up default behaviors
    ON_CALL(*mock, get_my_port(_))
        .WillByDefault(Return(500));  // Default IKE port
    
    ON_CALL(*mock, get_other_port(_))
        .WillByDefault(Return(500));
    
    ON_CALL(*mock, get_version(_))
        .WillByDefault(Return(2));  // IKEv2
    
    ON_CALL(*mock, get_proposals(_))
        .WillByDefault(Return(nullptr));
    
    return mock;
}

std::unique_ptr<MockPeerConfig> StrongSwanMockManager::createPeerConfigMock() {
    auto mock = std::make_unique<MockPeerConfig>();
    
    // Set up default behaviors
    ON_CALL(*mock, get_name(_))
        .WillByDefault(Return("test_peer"));
    
    ON_CALL(*mock, get_ike_cfg(_))
        .WillByDefault(Return(nullptr));
    
    ON_CALL(*mock, get_child_cfgs(_))
        .WillByDefault(Return(nullptr));
        
    ON_CALL(*mock, get_auth_cfg(_, _))
        .WillByDefault(Return(nullptr));
    
    return mock;
}

std::unique_ptr<MockChildConfig> StrongSwanMockManager::createChildConfigMock() {
    auto mock = std::make_unique<MockChildConfig>();
    
    // Set up default behaviors
    ON_CALL(*mock, get_name(_))
        .WillByDefault(Return("test_child"));
    
    ON_CALL(*mock, get_proposals(_, _))
        .WillByDefault(Return(nullptr));
        
    ON_CALL(*mock, get_traffic_selectors(_, _, _))
        .WillByDefault(Return(nullptr));
    
    return mock;
}

std::unique_ptr<MockAuthConfig> StrongSwanMockManager::createAuthConfigMock() {
    auto mock = std::make_unique<MockAuthConfig>();
    
    // Set up default behaviors
    ON_CALL(*mock, get_id(_))
        .WillByDefault(Return(nullptr));
    
    ON_CALL(*mock, get_auth_class(_))
        .WillByDefault(Return("PSK"));
    
    return mock;
}

std::unique_ptr<MockIkeSA> StrongSwanMockManager::createIkeSAMock() {
    auto mock = std::make_unique<MockIkeSA>();
    
    // Set up default behaviors
    ON_CALL(*mock, get_unique_id(_))
        .WillByDefault(Return(1));
    
    ON_CALL(*mock, get_peer_cfg(_))
        .WillByDefault(Return(nullptr));
        
    ON_CALL(*mock, initiate(_, _, _, _, _))
        .WillByDefault(Return(0));  // SUCCESS
        
    ON_CALL(*mock, supports_extension(_, _))
        .WillByDefault(Return(false));
    
    return mock;
}

std::unique_ptr<MockChildSA> StrongSwanMockManager::createChildSAMock() {
    auto mock = std::make_unique<MockChildSA>();
    
    // Set up default behaviors
    ON_CALL(*mock, get_reqid(_))
        .WillByDefault(Return(1));
    
    ON_CALL(*mock, get_name(_))
        .WillByDefault(Return("test_child_sa"));
        
    ON_CALL(*mock, get_traffic_selectors(_, _))
        .WillByDefault(Return(nullptr));
    
    return mock;
}

void StrongSwanMockManager::setupBasicIkeScenario() {
    // Configure mocks for a basic IKE scenario
    
    // IKE Config setup
    EXPECT_CALL(*ike_config_mock_, get_version(_))
        .WillRepeatedly(Return(2));  // IKEv2
    EXPECT_CALL(*ike_config_mock_, get_my_port(_))
        .WillRepeatedly(Return(500));
    EXPECT_CALL(*ike_config_mock_, get_other_port(_))
        .WillRepeatedly(Return(500));
    
    // Peer Config setup
    EXPECT_CALL(*peer_config_mock_, get_name(_))
        .WillRepeatedly(Return("basic_peer"));
    EXPECT_CALL(*peer_config_mock_, get_ike_cfg(_))
        .WillRepeatedly(Return(reinterpret_cast<ike_cfg_t*>(0x1000))); // Fake pointer
    
    // IKE SA setup
    EXPECT_CALL(*ike_sa_mock_, get_unique_id(_))
        .WillRepeatedly(Return(100));
    EXPECT_CALL(*ike_sa_mock_, supports_extension(_, ::testing::StrEq("extsock")))
        .WillRepeatedly(Return(true));
}

void StrongSwanMockManager::setupChildSAScenario() {
    // Configure mocks for Child SA operations
    
    // Child Config setup
    EXPECT_CALL(*child_config_mock_, get_name(_))
        .WillRepeatedly(Return("child_sa_config"));
    
    // Child SA setup
    EXPECT_CALL(*child_sa_mock_, get_name(_))
        .WillRepeatedly(Return("active_child_sa"));
    EXPECT_CALL(*child_sa_mock_, get_reqid(_))
        .WillRepeatedly(Return(42));
}

void StrongSwanMockManager::setupFailoverScenario() {
    // Configure mocks for failover testing
    
    // First call fails, second succeeds
    EXPECT_CALL(*ike_sa_mock_, initiate(_, _, _, _, _))
        .WillOnce(Return(-1))  // First attempt fails
        .WillOnce(Return(0));  // Second attempt succeeds
        
    // Peer config returns different names for different gateways
    EXPECT_CALL(*peer_config_mock_, get_name(_))
        .WillOnce(Return("primary_gateway"))
        .WillOnce(Return("backup_gateway"));
}

void StrongSwanMockManager::resetAllMocks() {
    // Reset all mock expectations and behaviors
    ::testing::Mock::VerifyAndClearExpectations(linked_list_mock_.get());
    ::testing::Mock::VerifyAndClearExpectations(ike_config_mock_.get());
    ::testing::Mock::VerifyAndClearExpectations(peer_config_mock_.get());
    ::testing::Mock::VerifyAndClearExpectations(child_config_mock_.get());
    ::testing::Mock::VerifyAndClearExpectations(auth_config_mock_.get());
    ::testing::Mock::VerifyAndClearExpectations(ike_sa_mock_.get());
    ::testing::Mock::VerifyAndClearExpectations(child_sa_mock_.get());
}

} // namespace mocks
} // namespace extsock_test