/**
 * Real Plugin Loader Header
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 4 Implementation - Direct .so library loading and function calls
 */

#ifndef REAL_PLUGIN_LOADER_HPP
#define REAL_PLUGIN_LOADER_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

extern "C" {
    // Forward declarations for extsock types
    struct plugin_t;
    struct extsock_json_parser_t;
    struct extsock_error_info_t;
    typedef int extsock_error_t;
}

namespace strongswan_test {

/**
 * Phase 4: Real Plugin Function Loader
 * 실제 .so 라이브러리에서 함수들을 동적으로 로드하고 호출
 */
class RealPluginLoader {
public:
    RealPluginLoader();
    ~RealPluginLoader();
    
    // 라이브러리 로딩
    bool LoadExtsockLibrary(const std::string& library_path);
    void UnloadLibrary();
    bool IsLibraryLoaded() const { return library_handle_ != nullptr; }
    
    // 함수 포인터 획득
    template<typename FuncType>
    bool GetFunction(const std::string& function_name, FuncType*& func_ptr);
    
    // 실제 extsock 함수 호출 래퍼들
    struct plugin_t* CallPluginCreate();
    struct extsock_json_parser_t* CallJsonParserCreate();
    struct extsock_error_info_t* CallErrorCreate(extsock_error_t code, const char* message);
    void CallErrorDestroy(struct extsock_error_info_t* error_info);
    
    // 상태 및 디버깅
    std::string GetLoadedLibraryPath() const { return library_path_; }
    std::vector<std::string> GetLoadedFunctions() const;
    
private:
    void* library_handle_;
    void* mock_library_handle_;  // strongSwan Mock Library handle
    std::string library_path_;
    std::unordered_map<std::string, void*> loaded_functions_;
    
    // 함수 포인터 타입 정의
    using plugin_create_func_t = struct plugin_t* (*)();
    using json_parser_create_func_t = struct extsock_json_parser_t* (*)();
    using error_create_func_t = struct extsock_error_info_t* (*)(extsock_error_t, const char*);
    using error_destroy_func_t = void (*)(struct extsock_error_info_t*);
    
    // 실제 함수 포인터들
    plugin_create_func_t plugin_create_func_;
    json_parser_create_func_t json_parser_create_func_;
    error_create_func_t error_create_func_;
    error_destroy_func_t error_destroy_func_;
    
    // 내부 헬퍼 함수들
    bool LoadCoreFunctions();
    void ClearFunctionPointers();
};

/**
 * Phase 4 전용 매크로들
 */
#define REAL_PLUGIN_PHASE_4_ONLY(test_case_name, test_name) \
    TEST(test_case_name, test_name) { \
        if (REAL_PLUGIN_PHASE < 4) { \
            GTEST_SKIP() << "Phase 4+ only test"; \
            return; \
        }

#define REAL_PLUGIN_ASSERT_LOADED(loader) \
    ASSERT_TRUE(loader.IsLibraryLoaded()) << "Plugin library must be loaded for Phase 4 tests"

#define REAL_PLUGIN_CALL_FUNCTION(loader, func_name, ...) \
    do { \
        REAL_PLUGIN_INFO("Calling real function: " + std::string(func_name)); \
        auto result = loader.func_name(__VA_ARGS__); \
        REAL_PLUGIN_SUCCESS("Function call completed: " + std::string(func_name)); \
    } while(0)

} // namespace strongswan_test

#endif // REAL_PLUGIN_LOADER_HPP