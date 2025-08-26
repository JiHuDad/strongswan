/**
 * Real Plugin Test Macros
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 1 Implementation - Test utility macros
 */

#ifndef REAL_PLUGIN_MACROS_HPP
#define REAL_PLUGIN_MACROS_HPP

#include <gtest/gtest.h>

// Phase 상태 매크로
#define REAL_PLUGIN_PHASE_1 1
#define REAL_PLUGIN_PHASE_2 2 
#define REAL_PLUGIN_PHASE_3 3

// 현재 Phase 확인
#ifndef REAL_PLUGIN_PHASE
    #define REAL_PLUGIN_PHASE REAL_PLUGIN_PHASE_1
#endif

// Phase별 테스트 실행 제어
#define TEST_PHASE_1_ONLY(test_suite, test_name) \
    TEST_F(test_suite, test_name) { \
        if (REAL_PLUGIN_PHASE != REAL_PLUGIN_PHASE_1) { \
            GTEST_SKIP() << "Phase 1 only test"; \
        }

#define TEST_PHASE_2_PLUS(test_suite, test_name) \
    TEST_F(test_suite, test_name) { \
        if (REAL_PLUGIN_PHASE < REAL_PLUGIN_PHASE_2) { \
            GTEST_SKIP() << "Requires Phase 2+"; \
        }

#define TEST_PHASE_3_ONLY(test_suite, test_name) \
    TEST_F(test_suite, test_name) { \
        if (REAL_PLUGIN_PHASE != REAL_PLUGIN_PHASE_3) { \
            GTEST_SKIP() << "Phase 3 only test"; \
        }

#define TEST_PHASE_3_PLUS(test_suite, test_name) \
    TEST_F(test_suite, test_name) { \
        if (REAL_PLUGIN_PHASE < REAL_PLUGIN_PHASE_3) { \
            GTEST_SKIP() << "Requires Phase 3+"; \
        }

// Phase별 조건부 실행
#define SKIP_IF_NOT_PHASE_1() \
    if (REAL_PLUGIN_PHASE != REAL_PLUGIN_PHASE_1) { \
        GTEST_SKIP() << "Phase 1 only"; \
    }

#define SKIP_IF_PHASE_LESS_THAN_2() \
    if (REAL_PLUGIN_PHASE < REAL_PLUGIN_PHASE_2) { \
        GTEST_SKIP() << "Requires Phase 2+"; \
    }

#define SKIP_IF_PHASE_LESS_THAN_3() \
    if (REAL_PLUGIN_PHASE < REAL_PLUGIN_PHASE_3) { \
        GTEST_SKIP() << "Requires Phase 3+"; \
    }

// 환경 검증 매크로
#define ASSERT_TEST_ENVIRONMENT_READY(env_info) \
    ASSERT_EQ(env_info.phase, REAL_PLUGIN_PHASE) \
    << "Environment phase mismatch"; \
    ASSERT_TRUE(env_info.status != strongswan_test::StrongSwanStatus::NOT_INITIALIZED) \
    << "Test environment not initialized"

#define EXPECT_STRONGSWAN_STATUS(expected_status, actual_status) \
    EXPECT_EQ(actual_status, expected_status) \
    << "Expected strongSwan status: " << static_cast<int>(expected_status) \
    << ", but got: " << static_cast<int>(actual_status)

// 로깅 매크로
#define REAL_PLUGIN_LOG(message) \
    std::cout << "[Phase " << REAL_PLUGIN_PHASE << "] " << message << std::endl

#define REAL_PLUGIN_DEBUG(message) \
    std::cout << "[Phase " << REAL_PLUGIN_PHASE << " DEBUG] " << message << std::endl

#define REAL_PLUGIN_INFO(message) \
    std::cout << "ℹ️  [Phase " << REAL_PLUGIN_PHASE << "] " << message << std::endl

#define REAL_PLUGIN_WARNING(message) \
    std::cout << "⚠️  [Phase " << REAL_PLUGIN_PHASE << " WARNING] " << message << std::endl

#define REAL_PLUGIN_ERROR(message) \
    std::cout << "❌ [Phase " << REAL_PLUGIN_PHASE << " ERROR] " << message << std::endl

#define REAL_PLUGIN_SUCCESS(message) \
    std::cout << "✅ [Phase " << REAL_PLUGIN_PHASE << "] " << message << std::endl

// Test 카테고리별 필터 매크로 (gtest_filter에서 사용)
#define REAL_PLUGIN_TEST_INFRASTRUCTURE "RealPlugin.Infrastructure.*"
#define REAL_PLUGIN_TEST_UNIT "RealPlugin.Unit.*"
#define REAL_PLUGIN_TEST_INTEGRATION "RealPlugin.Integration.*"
#define REAL_PLUGIN_TEST_ENDTOEND "RealPlugin.EndToEnd.*"

#endif // REAL_PLUGIN_MACROS_HPP