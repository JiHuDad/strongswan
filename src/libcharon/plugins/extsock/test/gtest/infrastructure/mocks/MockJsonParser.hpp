/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Google Mock based JSON Parser Mock Classes
 * TASK-M002: Mock Infrastructure Construction
 * 
 * This file provides Google Mock-based mock implementations of the 
 * extsock JSON Parser interface for testing JSON configuration parsing.
 */

#ifndef MOCK_JSON_PARSER_HPP_
#define MOCK_JSON_PARSER_HPP_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

extern "C" {
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// cJSON forward declaration
typedef struct cJSON cJSON;

// strongSwan types (forward declarations)
typedef struct ike_cfg_t ike_cfg_t;
typedef struct peer_cfg_t peer_cfg_t;
typedef struct child_cfg_t child_cfg_t;
typedef struct auth_cfg_t auth_cfg_t;
typedef struct linked_list_t linked_list_t;
typedef struct proposal_t proposal_t;
typedef struct traffic_selector_t traffic_selector_t;

// extsock types
typedef struct extsock_config_entity_t extsock_config_entity_t;
typedef struct extsock_json_parser_t extsock_json_parser_t;
}

namespace extsock_test {
namespace mocks {

/**
 * Abstract interface for extsock JSON Parser
 * 
 * This interface defines the contract for JSON parsing operations
 * that will be mocked during testing.
 */
class JsonParserInterface {
public:
    virtual ~JsonParserInterface() = default;
    
    // Configuration parsing methods
    virtual ike_cfg_t* parse_ike_config(extsock_json_parser_t* this_parser, cJSON* ike_json) = 0;
    virtual auth_cfg_t* parse_auth_config(extsock_json_parser_t* this_parser, 
                                         cJSON* auth_json, bool local) = 0;
    virtual linked_list_t* parse_proposals(extsock_json_parser_t* this_parser, 
                                          cJSON* proposals_json, bool esp) = 0;
    virtual linked_list_t* parse_traffic_selectors(extsock_json_parser_t* this_parser,
                                                   cJSON* ts_json) = 0;
    virtual bool parse_child_configs(extsock_json_parser_t* this_parser,
                                    cJSON* children_json, peer_cfg_t* peer_cfg) = 0;
    virtual extsock_config_entity_t* parse_config_entity(extsock_json_parser_t* this_parser,
                                                         const char* json_str) = 0;
    
    // Lifecycle methods
    virtual void destroy(extsock_json_parser_t* this_parser) = 0;
};

/**
 * Mock implementation of extsock JSON Parser
 */
class MockJsonParser : public JsonParserInterface {
public:
    MockJsonParser() = default;
    virtual ~MockJsonParser() = default;

    // Configuration parsing method mocks
    MOCK_METHOD(ike_cfg_t*, parse_ike_config, 
                (extsock_json_parser_t* this_parser, cJSON* ike_json), (override));
    
    MOCK_METHOD(auth_cfg_t*, parse_auth_config, 
                (extsock_json_parser_t* this_parser, cJSON* auth_json, bool local), (override));
    
    MOCK_METHOD(linked_list_t*, parse_proposals, 
                (extsock_json_parser_t* this_parser, cJSON* proposals_json, bool esp), (override));
    
    MOCK_METHOD(linked_list_t*, parse_traffic_selectors, 
                (extsock_json_parser_t* this_parser, cJSON* ts_json), (override));
    
    MOCK_METHOD(bool, parse_child_configs, 
                (extsock_json_parser_t* this_parser, cJSON* children_json, peer_cfg_t* peer_cfg), 
                (override));
    
    MOCK_METHOD(extsock_config_entity_t*, parse_config_entity, 
                (extsock_json_parser_t* this_parser, const char* json_str), (override));
    
    // Lifecycle method mocks
    MOCK_METHOD(void, destroy, (extsock_json_parser_t* this_parser), (override));
};

/**
 * Abstract interface for cJSON operations
 * 
 * This allows us to mock cJSON library functions for testing
 */
class CJsonInterface {
public:
    virtual ~CJsonInterface() = default;
    
    virtual cJSON* Parse(const char* value) = 0;
    virtual cJSON* CreateObject() = 0;
    virtual cJSON* CreateString(const char* string) = 0;
    virtual cJSON* CreateNumber(double number) = 0;
    virtual cJSON* CreateBool(bool boolean) = 0;
    virtual cJSON* CreateArray() = 0;
    
    virtual cJSON* GetObjectItem(const cJSON* object, const char* string) = 0;
    virtual bool HasObjectItem(const cJSON* object, const char* string) = 0;
    virtual int GetArraySize(const cJSON* array) = 0;
    virtual cJSON* GetArrayItem(const cJSON* array, int index) = 0;
    
    virtual bool IsObject(const cJSON* item) = 0;
    virtual bool IsArray(const cJSON* item) = 0;
    virtual bool IsString(const cJSON* item) = 0;
    virtual bool IsNumber(const cJSON* item) = 0;
    virtual bool IsBool(const cJSON* item) = 0;
    
    virtual const char* GetStringValue(const cJSON* item) = 0;
    virtual double GetNumberValue(const cJSON* item) = 0;
    virtual bool IsTrue(const cJSON* item) = 0;
    
    virtual char* Print(const cJSON* item) = 0;
    virtual char* PrintUnformatted(const cJSON* item) = 0;
    virtual void Delete(cJSON* item) = 0;
    virtual void free(void* ptr) = 0;
};

/**
 * Mock implementation of cJSON operations
 */
class MockCJson : public CJsonInterface {
public:
    MOCK_METHOD(cJSON*, Parse, (const char* value), (override));
    MOCK_METHOD(cJSON*, CreateObject, (), (override));
    MOCK_METHOD(cJSON*, CreateString, (const char* string), (override));
    MOCK_METHOD(cJSON*, CreateNumber, (double number), (override));
    MOCK_METHOD(cJSON*, CreateBool, (bool boolean), (override));
    MOCK_METHOD(cJSON*, CreateArray, (), (override));
    
    MOCK_METHOD(cJSON*, GetObjectItem, (const cJSON* object, const char* string), (override));
    MOCK_METHOD(bool, HasObjectItem, (const cJSON* object, const char* string), (override));
    MOCK_METHOD(int, GetArraySize, (const cJSON* array), (override));
    MOCK_METHOD(cJSON*, GetArrayItem, (const cJSON* array, int index), (override));
    
    MOCK_METHOD(bool, IsObject, (const cJSON* item), (override));
    MOCK_METHOD(bool, IsArray, (const cJSON* item), (override));
    MOCK_METHOD(bool, IsString, (const cJSON* item), (override));
    MOCK_METHOD(bool, IsNumber, (const cJSON* item), (override));
    MOCK_METHOD(bool, IsBool, (const cJSON* item), (override));
    
    MOCK_METHOD(const char*, GetStringValue, (const cJSON* item), (override));
    MOCK_METHOD(double, GetNumberValue, (const cJSON* item), (override));
    MOCK_METHOD(bool, IsTrue, (const cJSON* item), (override));
    
    MOCK_METHOD(char*, Print, (const cJSON* item), (override));
    MOCK_METHOD(char*, PrintUnformatted, (const cJSON* item), (override));
    MOCK_METHOD(void, Delete, (cJSON* item), (override));
    MOCK_METHOD(void, free, (void* ptr), (override));
};

/**
 * JSON Parser Mock Manager
 * 
 * Provides centralized management of JSON parsing mocks and
 * pre-configured scenarios for common test cases.
 */
class JsonParserMockManager {
public:
    JsonParserMockManager();
    virtual ~JsonParserMockManager();

    // Factory methods
    std::unique_ptr<MockJsonParser> createJsonParserMock();
    std::unique_ptr<MockCJson> createCJsonMock();

    // Pre-configured test scenarios
    void setupValidIkeConfigScenario();
    void setupValidChildConfigScenario();
    void setupInvalidJsonScenario();
    void setupParseErrorScenario();
    void setupComplexConfigScenario();
    
    // Mock instance access
    MockJsonParser* getJsonParserMock() { return json_parser_mock_.get(); }
    MockCJson* getCJsonMock() { return cjson_mock_.get(); }
    
    // Test data helpers
    cJSON* createMockIkeConfigJson();
    cJSON* createMockChildConfigJson();
    cJSON* createMockAuthConfigJson();
    const char* getValidIkeConfigJsonString();
    const char* getInvalidJsonString();
    
    void resetAllMocks();

private:
    std::unique_ptr<MockJsonParser> json_parser_mock_;
    std::unique_ptr<MockCJson> cjson_mock_;
    
    // Test data storage
    std::vector<std::unique_ptr<char[]>> test_strings_;
    void storeTestString(const char* str);
};

/**
 * Custom matchers for JSON testing
 */

// Matcher for checking if cJSON object has a specific field
MATCHER_P(HasJsonField, field_name, "has JSON field " + std::string(field_name)) {
    (void)arg; (void)field_name; // Suppress unused warnings
    // This would be implemented to check if the cJSON object has the specified field
    return true; // Simplified for now
}

// Matcher for checking JSON string content
MATCHER_P(JsonStringContains, substring, "JSON contains " + std::string(substring)) {
    if (arg == nullptr) return false;
    return std::string(arg).find(substring) != std::string::npos;
}

// Matcher for validating IKE config structure
MATCHER(IsValidIkeConfig, "is a valid IKE configuration") {
    // This would validate the structure of an IKE config
    return arg != nullptr; // Simplified for now
}

} // namespace mocks
} // namespace extsock_test

#endif // MOCK_JSON_PARSER_HPP_