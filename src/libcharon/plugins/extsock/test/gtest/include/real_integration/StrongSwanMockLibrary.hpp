/**
 * StrongSwan Mock Library Header
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 4+ Implementation - Mock all strongSwan dependencies for direct library loading
 */

#ifndef STRONGSWAN_MOCK_LIBRARY_HPP
#define STRONGSWAN_MOCK_LIBRARY_HPP

#include <string>
#include <vector>

extern "C" {
    // Forward declarations for strongSwan types
    struct chunk_t;
    struct auth_cfg_t;
    struct child_cfg_t;
    struct peer_cfg_t;
    struct ike_cfg_t;
    struct identification_t;
    struct linked_list_t;
    struct proposal_t;
    struct traffic_selector_t;
    struct shared_key_t;
    struct mem_cred_t;
    struct callback_cred_t;
    struct thread_t;
    struct mutex_t;
    struct daemon_t;
    struct library_t;
    
    // Global variables (extern declarations)
    extern struct daemon_t* charon;
    extern struct library_t* lib;
    extern struct chunk_t chunk_empty;
}

namespace strongswan_test {

/**
 * strongSwan Mock Library Management
 * 26개 strongSwan 함수에 대한 Mock 구현 관리
 */
class StrongSwanMockLibrary {
public:
    /**
     * Mock 라이브러리 초기화
     * @return true if successful
     */
    static bool InitializeStrongSwanMockLibrary();
    
    /**
     * Mock 라이브러리 정리
     */
    static void CleanupStrongSwanMockLibrary();
    
    /**
     * Mock된 함수 목록 반환
     * @return vector of mocked function names
     */
    static std::vector<std::string> GetMockedFunctions();
    
    /**
     * Mock 함수가 정상 작동하는지 테스트
     * @return true if all mock functions work correctly
     */
    static bool TestMockFunctions();
    
private:
    StrongSwanMockLibrary() = delete;  // Static class only
};

/**
 * Phase 4 Mock Library 매크로들
 */
#define STRONGSWAN_MOCK_INIT() \
    do { \
        ASSERT_TRUE(strongswan_test::StrongSwanMockLibrary::InitializeStrongSwanMockLibrary()) \
            << "Failed to initialize strongSwan Mock Library"; \
    } while(0)

#define STRONGSWAN_MOCK_CLEANUP() \
    do { \
        strongswan_test::StrongSwanMockLibrary::CleanupStrongSwanMockLibrary(); \
    } while(0)

#define STRONGSWAN_MOCK_TEST() \
    do { \
        ASSERT_TRUE(strongswan_test::StrongSwanMockLibrary::TestMockFunctions()) \
            << "strongSwan Mock functions validation failed"; \
    } while(0)

} // namespace strongswan_test

#endif // STRONGSWAN_MOCK_LIBRARY_HPP