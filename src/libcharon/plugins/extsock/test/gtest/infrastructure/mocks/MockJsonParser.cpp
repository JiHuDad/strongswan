/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Mock based JSON Parser Mock Implementation
 * TASK-M002: Mock Infrastructure Construction
 */

#include "MockJsonParser.hpp"
#include <cstring>
#include <memory>

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnNull;
using ::testing::StrEq;
using ::testing::NotNull;

namespace extsock_test {
namespace mocks {

JsonParserMockManager::JsonParserMockManager() 
    : json_parser_mock_(std::make_unique<MockJsonParser>())
    , cjson_mock_(std::make_unique<MockCJson>())
{
    // Constructor - mocks are created but not configured
}

JsonParserMockManager::~JsonParserMockManager() {
    // Destructor - unique_ptrs and test_strings_ will clean up automatically
}

std::unique_ptr<MockJsonParser> JsonParserMockManager::createJsonParserMock() {
    auto mock = std::make_unique<MockJsonParser>();
    
    // Set up default behaviors for common operations
    ON_CALL(*mock, parse_ike_config(_, _))
        .WillByDefault(ReturnNull());
    
    ON_CALL(*mock, parse_auth_config(_, _, _))
        .WillByDefault(ReturnNull());
        
    ON_CALL(*mock, parse_proposals(_, _, _))
        .WillByDefault(ReturnNull());
        
    ON_CALL(*mock, parse_traffic_selectors(_, _))
        .WillByDefault(ReturnNull());
        
    ON_CALL(*mock, parse_child_configs(_, _, _))
        .WillByDefault(Return(false));
        
    ON_CALL(*mock, parse_config_entity(_, _))
        .WillByDefault(ReturnNull());
    
    return mock;
}

std::unique_ptr<MockCJson> JsonParserMockManager::createCJsonMock() {
    auto mock = std::make_unique<MockCJson>();
    
    // Set up default behaviors
    ON_CALL(*mock, Parse(_))
        .WillByDefault(ReturnNull());
    
    ON_CALL(*mock, CreateObject())
        .WillByDefault(Return(reinterpret_cast<cJSON*>(0x1000)));
        
    ON_CALL(*mock, CreateString(_))
        .WillByDefault(Return(reinterpret_cast<cJSON*>(0x1001)));
        
    ON_CALL(*mock, CreateNumber(_))
        .WillByDefault(Return(reinterpret_cast<cJSON*>(0x1002)));
        
    ON_CALL(*mock, CreateBool(_))
        .WillByDefault(Return(reinterpret_cast<cJSON*>(0x1003)));
        
    ON_CALL(*mock, CreateArray())
        .WillByDefault(Return(reinterpret_cast<cJSON*>(0x1004)));
    
    ON_CALL(*mock, GetObjectItem(_, _))
        .WillByDefault(ReturnNull());
        
    ON_CALL(*mock, HasObjectItem(_, _))
        .WillByDefault(Return(false));
        
    ON_CALL(*mock, GetArraySize(_))
        .WillByDefault(Return(0));
        
    ON_CALL(*mock, GetArrayItem(_, _))
        .WillByDefault(ReturnNull());
    
    ON_CALL(*mock, IsObject(_))
        .WillByDefault(Return(false));
        
    ON_CALL(*mock, IsArray(_))
        .WillByDefault(Return(false));
        
    ON_CALL(*mock, IsString(_))
        .WillByDefault(Return(false));
        
    ON_CALL(*mock, IsNumber(_))
        .WillByDefault(Return(false));
        
    ON_CALL(*mock, IsBool(_))
        .WillByDefault(Return(false));
    
    ON_CALL(*mock, GetStringValue(_))
        .WillByDefault(Return(""));
        
    ON_CALL(*mock, GetNumberValue(_))
        .WillByDefault(Return(0.0));
        
    ON_CALL(*mock, IsTrue(_))
        .WillByDefault(Return(false));
    
    ON_CALL(*mock, Print(_))
        .WillByDefault(ReturnNull());
        
    ON_CALL(*mock, PrintUnformatted(_))
        .WillByDefault(ReturnNull());
    
    return mock;
}

void JsonParserMockManager::setupValidIkeConfigScenario() {
    // Configure mocks for parsing a valid IKE configuration
    
    cJSON* mock_ike_json = reinterpret_cast<cJSON*>(0x2000);
    cJSON* mock_version_json = reinterpret_cast<cJSON*>(0x2001);
    cJSON* mock_local_port_json = reinterpret_cast<cJSON*>(0x2002);
    cJSON* mock_remote_port_json = reinterpret_cast<cJSON*>(0x2003);
    
    // cJSON parsing setup
    EXPECT_CALL(*cjson_mock_, Parse(_))
        .WillOnce(Return(mock_ike_json));
    
    EXPECT_CALL(*cjson_mock_, IsObject(mock_ike_json))
        .WillRepeatedly(Return(true));
    
    // IKE version field
    EXPECT_CALL(*cjson_mock_, GetObjectItem(mock_ike_json, StrEq("version")))
        .WillRepeatedly(Return(mock_version_json));
    EXPECT_CALL(*cjson_mock_, IsNumber(mock_version_json))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cjson_mock_, GetNumberValue(mock_version_json))
        .WillRepeatedly(Return(2.0)); // IKEv2
    
    // Local port field
    EXPECT_CALL(*cjson_mock_, GetObjectItem(mock_ike_json, StrEq("local_port")))
        .WillRepeatedly(Return(mock_local_port_json));
    EXPECT_CALL(*cjson_mock_, IsNumber(mock_local_port_json))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cjson_mock_, GetNumberValue(mock_local_port_json))
        .WillRepeatedly(Return(500.0));
    
    // Remote port field
    EXPECT_CALL(*cjson_mock_, GetObjectItem(mock_ike_json, StrEq("remote_port")))
        .WillRepeatedly(Return(mock_remote_port_json));
    EXPECT_CALL(*cjson_mock_, IsNumber(mock_remote_port_json))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cjson_mock_, GetNumberValue(mock_remote_port_json))
        .WillRepeatedly(Return(500.0));
    
    // JSON Parser setup
    EXPECT_CALL(*json_parser_mock_, parse_ike_config(_, mock_ike_json))
        .WillOnce(Return(reinterpret_cast<ike_cfg_t*>(0x3000)));
}

void JsonParserMockManager::setupValidChildConfigScenario() {
    // Configure mocks for parsing valid Child SA configuration
    
    cJSON* mock_child_json = reinterpret_cast<cJSON*>(0x2100);
    cJSON* mock_name_json = reinterpret_cast<cJSON*>(0x2101);
    cJSON* mock_proposals_json = reinterpret_cast<cJSON*>(0x2102);
    
    EXPECT_CALL(*cjson_mock_, GetObjectItem(mock_child_json, StrEq("name")))
        .WillRepeatedly(Return(mock_name_json));
    EXPECT_CALL(*cjson_mock_, IsString(mock_name_json))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cjson_mock_, GetStringValue(mock_name_json))
        .WillRepeatedly(Return("test_child"));
    
    EXPECT_CALL(*cjson_mock_, GetObjectItem(mock_child_json, StrEq("esp_proposals")))
        .WillRepeatedly(Return(mock_proposals_json));
    EXPECT_CALL(*cjson_mock_, IsArray(mock_proposals_json))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cjson_mock_, GetArraySize(mock_proposals_json))
        .WillRepeatedly(Return(1));
    
    // Child config parsing should succeed
    EXPECT_CALL(*json_parser_mock_, parse_child_configs(_, mock_child_json, NotNull()))
        .WillOnce(Return(true));
}

void JsonParserMockManager::setupInvalidJsonScenario() {
    // Configure mocks for invalid JSON scenarios
    
    // Parse fails
    EXPECT_CALL(*cjson_mock_, Parse(_))
        .WillRepeatedly(ReturnNull());
    
    // All parser methods should fail gracefully
    EXPECT_CALL(*json_parser_mock_, parse_config_entity(_, _))
        .WillRepeatedly(ReturnNull());
}

void JsonParserMockManager::setupParseErrorScenario() {
    // Configure mocks for parsing errors (valid JSON but invalid structure)
    
    cJSON* mock_json = reinterpret_cast<cJSON*>(0x2200);
    
    EXPECT_CALL(*cjson_mock_, Parse(_))
        .WillOnce(Return(mock_json));
    
    // JSON is not an object
    EXPECT_CALL(*cjson_mock_, IsObject(mock_json))
        .WillRepeatedly(Return(false));
    
    // Parser should handle gracefully
    EXPECT_CALL(*json_parser_mock_, parse_config_entity(_, _))
        .WillOnce(ReturnNull());
}

void JsonParserMockManager::setupComplexConfigScenario() {
    // Configure mocks for a complex configuration with multiple children
    
    cJSON* mock_root = reinterpret_cast<cJSON*>(0x2300);
    cJSON* mock_children_array = reinterpret_cast<cJSON*>(0x2301);
    cJSON* mock_child1 = reinterpret_cast<cJSON*>(0x2302);
    cJSON* mock_child2 = reinterpret_cast<cJSON*>(0x2303);
    
    // Root object parsing
    EXPECT_CALL(*cjson_mock_, Parse(_))
        .WillOnce(Return(mock_root));
    EXPECT_CALL(*cjson_mock_, IsObject(mock_root))
        .WillRepeatedly(Return(true));
    
    // Children array
    EXPECT_CALL(*cjson_mock_, GetObjectItem(mock_root, StrEq("children")))
        .WillRepeatedly(Return(mock_children_array));
    EXPECT_CALL(*cjson_mock_, IsArray(mock_children_array))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cjson_mock_, GetArraySize(mock_children_array))
        .WillRepeatedly(Return(2));
    
    // Individual children
    EXPECT_CALL(*cjson_mock_, GetArrayItem(mock_children_array, 0))
        .WillRepeatedly(Return(mock_child1));
    EXPECT_CALL(*cjson_mock_, GetArrayItem(mock_children_array, 1))
        .WillRepeatedly(Return(mock_child2));
    
    // Both children should be parsed successfully
    EXPECT_CALL(*json_parser_mock_, parse_child_configs(_, mock_children_array, NotNull()))
        .WillOnce(Return(true));
}

cJSON* JsonParserMockManager::createMockIkeConfigJson() {
    return reinterpret_cast<cJSON*>(0x4000);
}

cJSON* JsonParserMockManager::createMockChildConfigJson() {
    return reinterpret_cast<cJSON*>(0x4001);
}

cJSON* JsonParserMockManager::createMockAuthConfigJson() {
    return reinterpret_cast<cJSON*>(0x4002);
}

const char* JsonParserMockManager::getValidIkeConfigJsonString() {
    const char* json_str = R"({
        "version": 2,
        "local_port": 500,
        "remote_port": 500,
        "proposals": ["aes256-sha256-modp2048"]
    })";
    
    storeTestString(json_str);
    return test_strings_.back().get();
}

const char* JsonParserMockManager::getInvalidJsonString() {
    const char* json_str = "{ invalid json syntax missing closing brace";
    
    storeTestString(json_str);
    return test_strings_.back().get();
}

void JsonParserMockManager::storeTestString(const char* str) {
    size_t len = strlen(str) + 1;
    auto stored_str = std::make_unique<char[]>(len);
    strncpy(stored_str.get(), str, len);
    test_strings_.push_back(std::move(stored_str));
}

void JsonParserMockManager::resetAllMocks() {
    ::testing::Mock::VerifyAndClearExpectations(json_parser_mock_.get());
    ::testing::Mock::VerifyAndClearExpectations(cjson_mock_.get());
}

} // namespace mocks
} // namespace extsock_test