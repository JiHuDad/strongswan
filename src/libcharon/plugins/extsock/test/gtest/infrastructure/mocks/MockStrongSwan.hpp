/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Mock based strongSwan API Mock Classes
 * TASK-M002: Mock Infrastructure Construction
 * 
 * This file provides Google Mock-based mock implementations of strongSwan's 
 * complex API to enable sophisticated unit testing with automatic verification.
 */

#ifndef MOCK_STRONGSWAN_HPP_
#define MOCK_STRONGSWAN_HPP_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

extern "C" {
// Include necessary strongSwan headers with C linkage
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Forward declarations for strongSwan types
typedef struct linked_list_t linked_list_t;
typedef struct enumerator_t enumerator_t;
typedef struct ike_cfg_t ike_cfg_t;
typedef struct peer_cfg_t peer_cfg_t;
typedef struct child_cfg_t child_cfg_t;
typedef struct auth_cfg_t auth_cfg_t;
typedef struct identification_t identification_t;
typedef struct traffic_selector_t traffic_selector_t;
typedef struct proposal_t proposal_t;
typedef struct ike_sa_t ike_sa_t;
typedef struct child_sa_t child_sa_t;
}

namespace extsock_test {
namespace mocks {

/**
 * Abstract interface for strongSwan linked_list_t
 */
class LinkedListInterface {
public:
    virtual ~LinkedListInterface() = default;
    virtual void destroy(linked_list_t* this_list) = 0;
    virtual int get_count(linked_list_t* this_list) = 0;
    virtual void insert_last(linked_list_t* this_list, void* item) = 0;
    virtual void* get_first(linked_list_t* this_list) = 0;
    virtual void* remove_first(linked_list_t* this_list) = 0;
    virtual enumerator_t* create_enumerator(linked_list_t* this_list) = 0;
};

/**
 * Mock implementation of strongSwan linked_list_t
 */
class MockLinkedList : public LinkedListInterface {
public:
    MOCK_METHOD(void, destroy, (linked_list_t* this_list), (override));
    MOCK_METHOD(int, get_count, (linked_list_t* this_list), (override));
    MOCK_METHOD(void, insert_last, (linked_list_t* this_list, void* item), (override));
    MOCK_METHOD(void*, get_first, (linked_list_t* this_list), (override));
    MOCK_METHOD(void*, remove_first, (linked_list_t* this_list), (override));
    MOCK_METHOD(enumerator_t*, create_enumerator, (linked_list_t* this_list), (override));
};

/**
 * Abstract interface for strongSwan IKE configuration
 */
class IkeConfigInterface {
public:
    virtual ~IkeConfigInterface() = default;
    virtual void destroy(ike_cfg_t* this_cfg) = 0;
    virtual uint16_t get_my_port(ike_cfg_t* this_cfg) = 0;
    virtual uint16_t get_other_port(ike_cfg_t* this_cfg) = 0;
    virtual int get_version(ike_cfg_t* this_cfg) = 0;
    virtual linked_list_t* get_proposals(ike_cfg_t* this_cfg) = 0;
};

/**
 * Mock implementation of strongSwan ike_cfg_t
 */
class MockIkeConfig : public IkeConfigInterface {
public:
    MOCK_METHOD(void, destroy, (ike_cfg_t* this_cfg), (override));
    MOCK_METHOD(uint16_t, get_my_port, (ike_cfg_t* this_cfg), (override));
    MOCK_METHOD(uint16_t, get_other_port, (ike_cfg_t* this_cfg), (override));
    MOCK_METHOD(int, get_version, (ike_cfg_t* this_cfg), (override));
    MOCK_METHOD(linked_list_t*, get_proposals, (ike_cfg_t* this_cfg), (override));
};

/**
 * Abstract interface for strongSwan Peer configuration
 */
class PeerConfigInterface {
public:
    virtual ~PeerConfigInterface() = default;
    virtual void destroy(peer_cfg_t* this_cfg) = 0;
    virtual const char* get_name(peer_cfg_t* this_cfg) = 0;
    virtual ike_cfg_t* get_ike_cfg(peer_cfg_t* this_cfg) = 0;
    virtual linked_list_t* get_child_cfgs(peer_cfg_t* this_cfg) = 0;
    virtual auth_cfg_t* get_auth_cfg(peer_cfg_t* this_cfg, bool local) = 0;
};

/**
 * Mock implementation of strongSwan peer_cfg_t
 */
class MockPeerConfig : public PeerConfigInterface {
public:
    MOCK_METHOD(void, destroy, (peer_cfg_t* this_cfg), (override));
    MOCK_METHOD(const char*, get_name, (peer_cfg_t* this_cfg), (override));
    MOCK_METHOD(ike_cfg_t*, get_ike_cfg, (peer_cfg_t* this_cfg), (override));
    MOCK_METHOD(linked_list_t*, get_child_cfgs, (peer_cfg_t* this_cfg), (override));
    MOCK_METHOD(auth_cfg_t*, get_auth_cfg, (peer_cfg_t* this_cfg, bool local), (override));
};

/**
 * Abstract interface for strongSwan Child configuration
 */
class ChildConfigInterface {
public:
    virtual ~ChildConfigInterface() = default;
    virtual void destroy(child_cfg_t* this_cfg) = 0;
    virtual const char* get_name(child_cfg_t* this_cfg) = 0;
    virtual linked_list_t* get_proposals(child_cfg_t* this_cfg, bool inbound) = 0;
    virtual linked_list_t* get_traffic_selectors(child_cfg_t* this_cfg, 
                                                  bool local, 
                                                  bool dynamic_ts) = 0;
};

/**
 * Mock implementation of strongSwan child_cfg_t
 */
class MockChildConfig : public ChildConfigInterface {
public:
    MOCK_METHOD(void, destroy, (child_cfg_t* this_cfg), (override));
    MOCK_METHOD(const char*, get_name, (child_cfg_t* this_cfg), (override));
    MOCK_METHOD(linked_list_t*, get_proposals, (child_cfg_t* this_cfg, bool inbound), (override));
    MOCK_METHOD(linked_list_t*, get_traffic_selectors, 
                (child_cfg_t* this_cfg, bool local, bool dynamic_ts), (override));
};

/**
 * Abstract interface for strongSwan Authentication configuration
 */
class AuthConfigInterface {
public:
    virtual ~AuthConfigInterface() = default;
    virtual void destroy(auth_cfg_t* this_cfg) = 0;
    virtual identification_t* get_id(auth_cfg_t* this_cfg) = 0;
    virtual const char* get_auth_class(auth_cfg_t* this_cfg) = 0;
};

/**
 * Mock implementation of strongSwan auth_cfg_t
 */
class MockAuthConfig : public AuthConfigInterface {
public:
    MOCK_METHOD(void, destroy, (auth_cfg_t* this_cfg), (override));
    MOCK_METHOD(identification_t*, get_id, (auth_cfg_t* this_cfg), (override));
    MOCK_METHOD(const char*, get_auth_class, (auth_cfg_t* this_cfg), (override));
};

/**
 * Abstract interface for strongSwan IKE SA
 */
class IkeSAInterface {
public:
    virtual ~IkeSAInterface() = default;
    virtual void destroy(ike_sa_t* this_sa) = 0;
    virtual uint32_t get_unique_id(ike_sa_t* this_sa) = 0;
    virtual peer_cfg_t* get_peer_cfg(ike_sa_t* this_sa) = 0;
    virtual int initiate(ike_sa_t* this_sa, child_cfg_t* child_cfg, 
                        uint32_t reqid, traffic_selector_t* tsi, 
                        traffic_selector_t* tsr) = 0;
    virtual bool supports_extension(ike_sa_t* this_sa, const char* extension) = 0;
};

/**
 * Mock implementation of strongSwan ike_sa_t
 */
class MockIkeSA : public IkeSAInterface {
public:
    MOCK_METHOD(void, destroy, (ike_sa_t* this_sa), (override));
    MOCK_METHOD(uint32_t, get_unique_id, (ike_sa_t* this_sa), (override));
    MOCK_METHOD(peer_cfg_t*, get_peer_cfg, (ike_sa_t* this_sa), (override));
    MOCK_METHOD(int, initiate, (ike_sa_t* this_sa, child_cfg_t* child_cfg,
                               uint32_t reqid, traffic_selector_t* tsi,
                               traffic_selector_t* tsr), (override));
    MOCK_METHOD(bool, supports_extension, (ike_sa_t* this_sa, const char* extension), (override));
};

/**
 * Abstract interface for strongSwan Child SA
 */
class ChildSAInterface {
public:
    virtual ~ChildSAInterface() = default;
    virtual void destroy(child_sa_t* this_sa) = 0;
    virtual uint32_t get_reqid(child_sa_t* this_sa) = 0;
    virtual const char* get_name(child_sa_t* this_sa) = 0;
    virtual linked_list_t* get_traffic_selectors(child_sa_t* this_sa, bool inbound) = 0;
};

/**
 * Mock implementation of strongSwan child_sa_t
 */
class MockChildSA : public ChildSAInterface {
public:
    MOCK_METHOD(void, destroy, (child_sa_t* this_sa), (override));
    MOCK_METHOD(uint32_t, get_reqid, (child_sa_t* this_sa), (override));
    MOCK_METHOD(const char*, get_name, (child_sa_t* this_sa), (override));
    MOCK_METHOD(linked_list_t*, get_traffic_selectors, 
                (child_sa_t* this_sa, bool inbound), (override));
};

/**
 * Comprehensive strongSwan API Mock Manager
 * 
 * This class provides a centralized way to manage all strongSwan mocks
 * and provides factory methods for creating configured mock instances.
 */
class StrongSwanMockManager {
public:
    StrongSwanMockManager();
    virtual ~StrongSwanMockManager();

    // Factory methods for creating configured mocks
    std::unique_ptr<MockLinkedList> createLinkedListMock();
    std::unique_ptr<MockIkeConfig> createIkeConfigMock();
    std::unique_ptr<MockPeerConfig> createPeerConfigMock();
    std::unique_ptr<MockChildConfig> createChildConfigMock();
    std::unique_ptr<MockAuthConfig> createAuthConfigMock();
    std::unique_ptr<MockIkeSA> createIkeSAMock();
    std::unique_ptr<MockChildSA> createChildSAMock();

    // Common test scenarios
    void setupBasicIkeScenario();
    void setupChildSAScenario();
    void setupFailoverScenario();
    void resetAllMocks();

    // Mock instance access
    MockLinkedList* getLinkedListMock() { return linked_list_mock_.get(); }
    MockIkeConfig* getIkeConfigMock() { return ike_config_mock_.get(); }
    MockPeerConfig* getPeerConfigMock() { return peer_config_mock_.get(); }
    MockChildConfig* getChildConfigMock() { return child_config_mock_.get(); }
    MockAuthConfig* getAuthConfigMock() { return auth_config_mock_.get(); }
    MockIkeSA* getIkeSAMock() { return ike_sa_mock_.get(); }
    MockChildSA* getChildSAMock() { return child_sa_mock_.get(); }
    
    // JSON Parser specific mock verification methods
    MOCK_METHOD(void, reset_state, (), ());
    MOCK_METHOD(bool, ike_cfg_create_called, (), ());
    MOCK_METHOD(bool, peer_cfg_create_called, (), ());
    MOCK_METHOD(bool, child_cfg_create_called, (), ());
    MOCK_METHOD(bool, auth_cfg_create_called, (), ());
    MOCK_METHOD(bool, identification_create_called, (), ());
    MOCK_METHOD(bool, shared_key_create_called, (), ());
    MOCK_METHOD(bool, proposal_create_called, (), ());
    MOCK_METHOD(bool, traffic_selector_create_called, (), ());
    MOCK_METHOD(void, simulate_memory_failure, (bool enable), ());
    MOCK_METHOD(void, simulate_api_failure, (bool enable), ());

private:
    std::unique_ptr<MockLinkedList> linked_list_mock_;
    std::unique_ptr<MockIkeConfig> ike_config_mock_;
    std::unique_ptr<MockPeerConfig> peer_config_mock_;
    std::unique_ptr<MockChildConfig> child_config_mock_;
    std::unique_ptr<MockAuthConfig> auth_config_mock_;
    std::unique_ptr<MockIkeSA> ike_sa_mock_;
    std::unique_ptr<MockChildSA> child_sa_mock_;
};

} // namespace mocks
} // namespace extsock_test

#endif // MOCK_STRONGSWAN_HPP_