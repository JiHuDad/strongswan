/**
 * strongSwan Test Helpers for Real Plugin Testing
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 1 Implementation - Basic test utilities
 */

#ifndef STRONGSWAN_TEST_HELPERS_HPP
#define STRONGSWAN_TEST_HELPERS_HPP

#include <gtest/gtest.h>
#include <string>
#include <memory>

// Phase 1: 기본 테스트 헬퍼 (strongSwan API 없이)
namespace strongswan_test {

/**
 * Phase별 기능 확인
 */
inline bool IsPhaseSupported(int phase) {
    #ifdef REAL_PLUGIN_PHASE
        return REAL_PLUGIN_PHASE >= phase;
    #else
        return phase == 1;  // 기본값은 Phase 1
    #endif
}

/**
 * 현재 Phase 반환
 */
inline int GetCurrentPhase() {
    #ifdef REAL_PLUGIN_PHASE
        return REAL_PLUGIN_PHASE;
    #else
        return 1;
    #endif
}

/**
 * strongSwan 환경 상태 (Phase 1에서는 Mock)
 */
enum class StrongSwanStatus {
    NOT_INITIALIZED,
    MOCK_MODE,      // Phase 1
    REAL_MODE       // Phase 2+
};

/**
 * 테스트 환경 정보
 */
struct TestEnvironmentInfo {
    StrongSwanStatus status;
    int phase;
    bool strongswan_available;
    bool plugin_library_available;
    std::string plugin_path;
};

/**
 * 현재 테스트 환경 정보 반환
 */
TestEnvironmentInfo GetTestEnvironmentInfo();

/**
 * Phase 1 전용: 기본 환경 검증
 */
class Phase1TestHelper {
public:
    static bool VerifyBasicEnvironment();
    static bool CheckRequiredDirectories();
    static std::string GetPhaseDescription();
};

} // namespace strongswan_test

// 테스트 매크로 정의
#define ASSERT_PHASE_SUPPORTED(phase) \
    ASSERT_TRUE(strongswan_test::IsPhaseSupported(phase)) \
    << "Test requires Phase " << phase << " but current phase is " << strongswan_test::GetCurrentPhase()

#define SKIP_IF_PHASE_NOT_SUPPORTED(phase) \
    if (!strongswan_test::IsPhaseSupported(phase)) { \
        GTEST_SKIP() << "Skipping test - requires Phase " << phase; \
    }

#endif // STRONGSWAN_TEST_HELPERS_HPP