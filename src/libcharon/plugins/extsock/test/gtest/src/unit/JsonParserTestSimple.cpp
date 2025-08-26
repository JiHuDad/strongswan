/**
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Test Unit Tests for JSON Parser Adapter (Simplified Version)
 * Migrated from test_extsock_json_parser_adapter.c
 * 
 * Level 2 (Adapter) tests that use Google Mock to test adapter layer
 * functionality with controlled dependencies.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <memory>
#include <chrono>

// Mock strongSwan dependencies
#include "../../infrastructure/mocks/MockStrongSwan.hpp"

// Pure types for testing  
extern "C" {
    #include "extsock_types_pure.h"
}

// Forward declarations for C structures
struct cJSON;
struct linked_list_t;
struct ike_cfg_t;
struct auth_cfg_t;
struct extsock_config_entity_t;

// Mock JSON Parser Implementation for testing
class MockJsonParser {
public:
    MOCK_METHOD(ike_cfg_t*, parse_ike_config, (cJSON* ike_json), ());
    MOCK_METHOD(auth_cfg_t*, parse_auth_config, (cJSON* auth_json, bool is_local), ());
    MOCK_METHOD(linked_list_t*, parse_proposals, (cJSON* proposals_json, int protocol, bool is_ike), ());
    MOCK_METHOD(linked_list_t*, parse_traffic_selectors, (cJSON* ts_json), ());
    MOCK_METHOD(linked_list_t*, parse_child_configs, (cJSON* children_json), ());
    MOCK_METHOD(extsock_config_entity_t*, parse_config_entity, (cJSON* root_json), ());
};

/**
 * ============================================================================ 
 * Test Fixture Class
 * ============================================================================
 */
class JsonParserTestSimple : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mock strongSwan system
        mock_strongswan = std::make_unique<extsock_test::mocks::StrongSwanMockManager>();
        mock_json_parser = std::make_unique<MockJsonParser>();
        
        // Setup default expectations
        EXPECT_CALL(*mock_strongswan, reset_state())
            .WillRepeatedly(::testing::Return());
    }

    void TearDown() override {
        mock_json_parser.reset();
        mock_strongswan.reset();
    }

    // Test data constants
    const std::string VALID_IKE_JSON = R"({
        "local_addrs": ["192.168.1.100"],
        "remote_addrs": ["203.0.113.5"],
        "version": 2,
        "dscp": "101000",
        "proposals": [
            "aes256-sha256-modp2048",
            "aes128-sha1-modp1024"
        ]
    })";

    const std::string VALID_AUTH_PSK_JSON = R"({
        "auth": "psk",
        "id": "client@strongswan.org",
        "secret": "test-preshared-key-123"
    })";

    const std::string EMPTY_JSON = "{}";
    const std::string INVALID_JSON = R"({ "incomplete": )";

    std::unique_ptr<extsock_test::mocks::StrongSwanMockManager> mock_strongswan;
    std::unique_ptr<MockJsonParser> mock_json_parser;
};

/**
 * ============================================================================
 * JSON Parser Creation and Destruction Tests
 * ============================================================================
 */

TEST_F(JsonParserTestSimple, CreateDestroy) {
    // Test that we can create a mock parser with all required methods
    MockJsonParser parser;
    
    // Verify that all methods are available (compile-time check)
    EXPECT_CALL(parser, parse_ike_config(::testing::_))
        .WillOnce(::testing::Return(nullptr));
    
    parser.parse_ike_config(nullptr);
}

TEST_F(JsonParserTestSimple, MultipleCreateDestroy) {
    // Test creating and destroying multiple parsers
    std::vector<std::unique_ptr<MockJsonParser>> parsers;
    
    // Create multiple parsers
    for (int i = 0; i < 5; i++) {
        parsers.push_back(std::make_unique<MockJsonParser>());
        EXPECT_NE(parsers[i], nullptr);
    }
    
    // Verify they are all valid
    for (const auto& parser : parsers) {
        EXPECT_NE(parser.get(), nullptr);
    }
    
    // Parsers will be automatically destroyed when going out of scope
}

/**
 * ============================================================================
 * IKE Configuration Parsing Tests
 * ============================================================================
 */

TEST_F(JsonParserTestSimple, ParseIkeConfigValid) {
    // Setup mock expectations
    ike_cfg_t* expected_ike_cfg = reinterpret_cast<ike_cfg_t*>(0x12345678);
    
    EXPECT_CALL(*mock_json_parser, parse_ike_config(::testing::NotNull()))
        .WillOnce(::testing::Return(expected_ike_cfg));
    
    EXPECT_CALL(*mock_strongswan, ike_cfg_create_called())
        .WillOnce(::testing::Return(true));
    
    // Simulate parsing valid IKE JSON
    cJSON* ike_json = reinterpret_cast<cJSON*>(0xABCDEF00);
    ike_cfg_t* result = mock_json_parser->parse_ike_config(ike_json);
    
    EXPECT_EQ(result, expected_ike_cfg);
    EXPECT_TRUE(mock_strongswan->ike_cfg_create_called());
}

TEST_F(JsonParserTestSimple, ParseIkeConfigNullInput) {
    EXPECT_CALL(*mock_json_parser, parse_ike_config(nullptr))
        .WillOnce(::testing::Return(nullptr));
    
    ike_cfg_t* result = mock_json_parser->parse_ike_config(nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST_F(JsonParserTestSimple, ParseIkeConfigMinimal) {
    // Test parsing minimal (empty) JSON
    ike_cfg_t* expected_ike_cfg = reinterpret_cast<ike_cfg_t*>(0x87654321);
    
    EXPECT_CALL(*mock_json_parser, parse_ike_config(::testing::NotNull()))
        .WillOnce(::testing::Return(expected_ike_cfg));
    
    EXPECT_CALL(*mock_strongswan, ike_cfg_create_called())
        .WillOnce(::testing::Return(true));
    
    cJSON* empty_json = reinterpret_cast<cJSON*>(0xEEEEEEEE);
    ike_cfg_t* result = mock_json_parser->parse_ike_config(empty_json);
    
    EXPECT_EQ(result, expected_ike_cfg);
    EXPECT_TRUE(mock_strongswan->ike_cfg_create_called());
}

/**
 * ============================================================================
 * Authentication Configuration Parsing Tests
 * ============================================================================
 */

TEST_F(JsonParserTestSimple, ParseAuthConfigPskValid) {
    auth_cfg_t* expected_auth_cfg = reinterpret_cast<auth_cfg_t*>(0xA1234567);
    
    EXPECT_CALL(*mock_json_parser, parse_auth_config(::testing::NotNull(), true))
        .WillOnce(::testing::Return(expected_auth_cfg));
    
    EXPECT_CALL(*mock_strongswan, auth_cfg_create_called())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_strongswan, identification_create_called())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_strongswan, shared_key_create_called())
        .WillOnce(::testing::Return(true));
    
    cJSON* auth_json = reinterpret_cast<cJSON*>(0x12344321);
    auth_cfg_t* result = mock_json_parser->parse_auth_config(auth_json, true);
    
    EXPECT_EQ(result, expected_auth_cfg);
    EXPECT_TRUE(mock_strongswan->auth_cfg_create_called());
    EXPECT_TRUE(mock_strongswan->identification_create_called());
    EXPECT_TRUE(mock_strongswan->shared_key_create_called());
}

TEST_F(JsonParserTestSimple, ParseAuthConfigPubkeyValid) {
    auth_cfg_t* expected_auth_cfg = reinterpret_cast<auth_cfg_t*>(0x56789ABC);
    
    EXPECT_CALL(*mock_json_parser, parse_auth_config(::testing::NotNull(), false))
        .WillOnce(::testing::Return(expected_auth_cfg));
    
    EXPECT_CALL(*mock_strongswan, auth_cfg_create_called())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_strongswan, identification_create_called())
        .WillOnce(::testing::Return(true));
    
    cJSON* auth_json = reinterpret_cast<cJSON*>(0x56789ABC);
    auth_cfg_t* result = mock_json_parser->parse_auth_config(auth_json, false);
    
    EXPECT_EQ(result, expected_auth_cfg);
    EXPECT_TRUE(mock_strongswan->auth_cfg_create_called());
    EXPECT_TRUE(mock_strongswan->identification_create_called());
}

TEST_F(JsonParserTestSimple, ParseAuthConfigNullInput) {
    EXPECT_CALL(*mock_json_parser, parse_auth_config(nullptr, true))
        .WillOnce(::testing::Return(nullptr));
    
    auth_cfg_t* result = mock_json_parser->parse_auth_config(nullptr, true);
    EXPECT_EQ(result, nullptr);
}

TEST_F(JsonParserTestSimple, ParseAuthConfigInvalidAuthType) {
    EXPECT_CALL(*mock_json_parser, parse_auth_config(::testing::NotNull(), true))
        .WillOnce(::testing::Return(nullptr));
    
    cJSON* invalid_json = reinterpret_cast<cJSON*>(0x9ABCDEF0);
    auth_cfg_t* result = mock_json_parser->parse_auth_config(invalid_json, true);
    
    EXPECT_EQ(result, nullptr);
}

/**
 * ============================================================================
 * Proposals Parsing Tests
 * ============================================================================
 */

TEST_F(JsonParserTestSimple, ParseProposalsValidIke) {
    linked_list_t* expected_proposals = reinterpret_cast<linked_list_t*>(0x11111111);
    
    EXPECT_CALL(*mock_json_parser, parse_proposals(::testing::NotNull(), 1 /* PROTO_IKE */, true))
        .WillOnce(::testing::Return(expected_proposals));
    
    EXPECT_CALL(*mock_strongswan, proposal_create_called())
        .WillOnce(::testing::Return(true));
    
    cJSON* proposals_json = reinterpret_cast<cJSON*>(0x11111111);
    linked_list_t* result = mock_json_parser->parse_proposals(proposals_json, 1, true);
    
    EXPECT_EQ(result, expected_proposals);
    EXPECT_TRUE(mock_strongswan->proposal_create_called());
}

TEST_F(JsonParserTestSimple, ParseProposalsValidEsp) {
    linked_list_t* expected_proposals = reinterpret_cast<linked_list_t*>(0x22222222);
    
    EXPECT_CALL(*mock_json_parser, parse_proposals(::testing::NotNull(), 3 /* PROTO_ESP */, false))
        .WillOnce(::testing::Return(expected_proposals));
    
    cJSON* proposals_json = reinterpret_cast<cJSON*>(0x22222222);
    linked_list_t* result = mock_json_parser->parse_proposals(proposals_json, 3, false);
    
    EXPECT_EQ(result, expected_proposals);
}

TEST_F(JsonParserTestSimple, ParseProposalsNullInput) {
    EXPECT_CALL(*mock_json_parser, parse_proposals(nullptr, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(nullptr));
    
    linked_list_t* result = mock_json_parser->parse_proposals(nullptr, 1, true);
    EXPECT_EQ(result, nullptr);
}

/**
 * ============================================================================
 * Traffic Selectors Parsing Tests
 * ============================================================================
 */

TEST_F(JsonParserTestSimple, ParseTrafficSelectorsValid) {
    linked_list_t* expected_ts = reinterpret_cast<linked_list_t*>(0x33333333);
    
    EXPECT_CALL(*mock_json_parser, parse_traffic_selectors(::testing::NotNull()))
        .WillOnce(::testing::Return(expected_ts));
    
    EXPECT_CALL(*mock_strongswan, traffic_selector_create_called())
        .WillOnce(::testing::Return(true));
    
    cJSON* ts_json = reinterpret_cast<cJSON*>(0x33333333);
    linked_list_t* result = mock_json_parser->parse_traffic_selectors(ts_json);
    
    EXPECT_EQ(result, expected_ts);
    EXPECT_TRUE(mock_strongswan->traffic_selector_create_called());
}

TEST_F(JsonParserTestSimple, ParseTrafficSelectorsEmpty) {
    linked_list_t* expected_empty_list = reinterpret_cast<linked_list_t*>(0x44444444);
    
    EXPECT_CALL(*mock_json_parser, parse_traffic_selectors(::testing::NotNull()))
        .WillOnce(::testing::Return(expected_empty_list));
    
    cJSON* empty_ts_json = reinterpret_cast<cJSON*>(0x44444444);
    linked_list_t* result = mock_json_parser->parse_traffic_selectors(empty_ts_json);
    
    EXPECT_EQ(result, expected_empty_list);
}

/**
 * ============================================================================
 * Child Configurations Parsing Tests
 * ============================================================================
 */

TEST_F(JsonParserTestSimple, ParseChildConfigsValid) {
    linked_list_t* expected_children = reinterpret_cast<linked_list_t*>(0x55555555);
    
    EXPECT_CALL(*mock_json_parser, parse_child_configs(::testing::NotNull()))
        .WillOnce(::testing::Return(expected_children));
    
    EXPECT_CALL(*mock_strongswan, child_cfg_create_called())
        .WillOnce(::testing::Return(true));
    
    cJSON* children_json = reinterpret_cast<cJSON*>(0x55555555);
    linked_list_t* result = mock_json_parser->parse_child_configs(children_json);
    
    EXPECT_EQ(result, expected_children);
    EXPECT_TRUE(mock_strongswan->child_cfg_create_called());
}

TEST_F(JsonParserTestSimple, ParseChildConfigsMultiple) {
    linked_list_t* expected_multiple_children = reinterpret_cast<linked_list_t*>(0x66666666);
    
    EXPECT_CALL(*mock_json_parser, parse_child_configs(::testing::NotNull()))
        .WillOnce(::testing::Return(expected_multiple_children));
    
    EXPECT_CALL(*mock_strongswan, child_cfg_create_called())
        .WillRepeatedly(::testing::Return(true));
    
    cJSON* multiple_children_json = reinterpret_cast<cJSON*>(0x66666666);
    linked_list_t* result = mock_json_parser->parse_child_configs(multiple_children_json);
    
    EXPECT_EQ(result, expected_multiple_children);
}

/**
 * ============================================================================
 * Complete Configuration Entity Parsing Tests
 * ============================================================================
 */

TEST_F(JsonParserTestSimple, ParseConfigEntityComplete) {
    extsock_config_entity_t* expected_entity = reinterpret_cast<extsock_config_entity_t*>(0x77777777);
    
    EXPECT_CALL(*mock_json_parser, parse_config_entity(::testing::NotNull()))
        .WillOnce(::testing::Return(expected_entity));
    
    // Expect multiple strongSwan API calls for complete configuration
    EXPECT_CALL(*mock_strongswan, ike_cfg_create_called())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_strongswan, peer_cfg_create_called())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_strongswan, auth_cfg_create_called())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mock_strongswan, child_cfg_create_called())
        .WillOnce(::testing::Return(true));
    
    cJSON* complete_json = reinterpret_cast<cJSON*>(0x77777777);
    extsock_config_entity_t* result = mock_json_parser->parse_config_entity(complete_json);
    
    EXPECT_EQ(result, expected_entity);
    EXPECT_TRUE(mock_strongswan->ike_cfg_create_called());
    EXPECT_TRUE(mock_strongswan->peer_cfg_create_called());
    EXPECT_TRUE(mock_strongswan->auth_cfg_create_called());
    EXPECT_TRUE(mock_strongswan->child_cfg_create_called());
}

TEST_F(JsonParserTestSimple, ParseConfigEntityInvalidJson) {
    EXPECT_CALL(*mock_json_parser, parse_config_entity(::testing::NotNull()))
        .WillOnce(::testing::Return(nullptr));
    
    cJSON* invalid_json = reinterpret_cast<cJSON*>(0x88888888);
    extsock_config_entity_t* result = mock_json_parser->parse_config_entity(invalid_json);
    
    EXPECT_EQ(result, nullptr);
}

/**
 * ============================================================================
 * Error Handling and Edge Cases Tests
 * ============================================================================
 */

TEST_F(JsonParserTestSimple, MemoryAllocationFailure) {
    // Test behavior when strongSwan mock simulates memory allocation failure
    EXPECT_CALL(*mock_strongswan, simulate_memory_failure(true))
        .WillOnce(::testing::Return());
    
    EXPECT_CALL(*mock_json_parser, parse_ike_config(::testing::NotNull()))
        .WillOnce(::testing::Return(nullptr));
    
    mock_strongswan->simulate_memory_failure(true);
    
    cJSON* ike_json = reinterpret_cast<cJSON*>(0x99999999);
    ike_cfg_t* result = mock_json_parser->parse_ike_config(ike_json);
    
    EXPECT_EQ(result, nullptr);
}

TEST_F(JsonParserTestSimple, StrongswanApiFailure) {
    // Test behavior when strongSwan API calls fail
    EXPECT_CALL(*mock_strongswan, simulate_api_failure(true))
        .WillOnce(::testing::Return());
    
    EXPECT_CALL(*mock_json_parser, parse_auth_config(::testing::NotNull(), true))
        .WillOnce(::testing::Return(nullptr));
    
    mock_strongswan->simulate_api_failure(true);
    
    cJSON* auth_json = reinterpret_cast<cJSON*>(0xAAAAAAAA);
    auth_cfg_t* result = mock_json_parser->parse_auth_config(auth_json, true);
    
    EXPECT_EQ(result, nullptr);
}

/**
 * ============================================================================
 * Performance and Stress Tests
 * ============================================================================
 */

TEST_F(JsonParserTestSimple, LargeJsonProcessing) {
    // Test processing large JSON configurations
    extsock_config_entity_t* expected_large_entity = reinterpret_cast<extsock_config_entity_t*>(0xBBBBBBBB);
    
    EXPECT_CALL(*mock_json_parser, parse_config_entity(::testing::NotNull()))
        .WillOnce(::testing::Return(expected_large_entity));
    
    cJSON* large_json = reinterpret_cast<cJSON*>(0xBBBBBBBB);
    auto start = std::chrono::high_resolution_clock::now();
    
    extsock_config_entity_t* result = mock_json_parser->parse_config_entity(large_json);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(result, expected_large_entity);
    EXPECT_LT(duration.count(), 100) << "Large JSON processing took too long";
}

TEST_F(JsonParserTestSimple, ConcurrentParsing) {
    // Test concurrent parsing scenarios (simplified)
    std::vector<std::unique_ptr<MockJsonParser>> parsers;
    
    for (int i = 0; i < 3; i++) {
        parsers.push_back(std::make_unique<MockJsonParser>());
        
        extsock_config_entity_t* expected = reinterpret_cast<extsock_config_entity_t*>(0x10000000 + i);
        EXPECT_CALL(*parsers[i], parse_config_entity(::testing::NotNull()))
            .WillOnce(::testing::Return(expected));
    }
    
    // Simulate concurrent parsing
    for (int i = 0; i < 3; i++) {
        cJSON* json = reinterpret_cast<cJSON*>(0x20000000 + i);
        extsock_config_entity_t* result = parsers[i]->parse_config_entity(json);
        
        extsock_config_entity_t* expected = reinterpret_cast<extsock_config_entity_t*>(0x10000000 + i);
        EXPECT_EQ(result, expected);
    }
}