/**
 * Real Plugin Test Base Implementation
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 1 Implementation - Basic fixture implementations
 */

#include "plugin_test_fixtures.hpp"
#include "StrongSwanTestEnvironment.hpp"
#include "real_plugin_macros.hpp"
#include <filesystem>

namespace strongswan_test {

// ============================================================================
// RealPluginTestFixture Implementation
// ============================================================================

void RealPluginTestFixture::SetUp() {
    REAL_PLUGIN_DEBUG("RealPluginTestFixture::SetUp() - Phase " + std::to_string(REAL_PLUGIN_PHASE));
    
    // 1. 전역 환경이 초기화되었는지 확인
    if (!g_strongswan_env || !g_strongswan_env->IsInitialized()) {
        FAIL() << "StrongSwan test environment not initialized. "
               << "Make sure StrongSwanTestEnvironment is registered as global test environment.";
    }
    
    // 2. 환경 정보 가져오기
    env_info_ = g_strongswan_env->GetEnvironmentInfo();
    
    // 3. 현재 Phase와 환경 Phase 일치 확인
    ASSERT_EQ(env_info_.phase, REAL_PLUGIN_PHASE) 
        << "Environment phase mismatch. Expected: " << REAL_PLUGIN_PHASE 
        << ", Got: " << env_info_.phase;
    
    // 4. Phase별 추가 검증
    if (REAL_PLUGIN_PHASE == 1) {
        EXPECT_EQ(env_info_.status, StrongSwanStatus::MOCK_MODE)
            << "Phase 1 should use MOCK_MODE";
    }
    
    environment_ready_ = true;
    REAL_PLUGIN_DEBUG("RealPluginTestFixture setup complete");
}

void RealPluginTestFixture::TearDown() {
    REAL_PLUGIN_DEBUG("RealPluginTestFixture::TearDown()");
    
    environment_ready_ = false;
    
    // Phase별 정리 작업 (현재는 no-op)
    if (REAL_PLUGIN_PHASE >= 2) {
        // Phase 2+에서 추가 정리 작업 구현 예정
    }
}

// ============================================================================
// StrongSwanIntegrationFixture Implementation
// ============================================================================

void StrongSwanIntegrationFixture::SetUp() {
    // 부모 클래스 setup 호출
    RealPluginTestFixture::SetUp();
    
    REAL_PLUGIN_DEBUG("StrongSwanIntegrationFixture::SetUp()");
    
    // strongSwan 환경 상태 확인
    const auto& env_info = GetEnvironmentInfo();
    
    if (REAL_PLUGIN_PHASE == 1) {
        // Phase 1: Mock 모드에서는 항상 ready
        strongswan_ready_ = true;
        REAL_PLUGIN_INFO("strongSwan integration ready (Mock mode)");
    } else {
        // Phase 2+: 실제 strongSwan 초기화 상태 확인
        strongswan_ready_ = (env_info.status == StrongSwanStatus::REAL_MODE);
        
        if (strongswan_ready_) {
            REAL_PLUGIN_SUCCESS("strongSwan integration ready (Real mode)");
        } else {
            REAL_PLUGIN_WARNING("strongSwan integration not ready - using fallback");
        }
    }
}

void StrongSwanIntegrationFixture::TearDown() {
    REAL_PLUGIN_DEBUG("StrongSwanIntegrationFixture::TearDown()");
    
    strongswan_ready_ = false;
    
    // strongSwan 상태 리셋 (Phase 2+에서 필요시)
    if (REAL_PLUGIN_PHASE >= 2 && g_strongswan_env) {
        g_strongswan_env->ResetLibraryState();
    }
    
    // 부모 클래스 teardown 호출
    RealPluginTestFixture::TearDown();
}

// ============================================================================
// ExtsockPluginFixture Implementation  
// ============================================================================

void ExtsockPluginFixture::SetUp() {
    // 부모 클래스 setup 호출
    StrongSwanIntegrationFixture::SetUp();
    
    REAL_PLUGIN_DEBUG("ExtsockPluginFixture::SetUp()");
    
    const auto& env_info = GetEnvironmentInfo();
    
    // extsock plugin 로딩 상태 확인
    if (REAL_PLUGIN_PHASE == 1) {
        // Phase 1: Mock 모드에서는 항상 loaded
        plugin_loaded_ = true;
        REAL_PLUGIN_INFO("extsock plugin loaded (Mock mode)");
    } else {
        // Phase 2+: 실제 plugin 라이브러리 확인
        plugin_loaded_ = env_info.plugin_library_available;
        
        if (plugin_loaded_) {
            REAL_PLUGIN_SUCCESS("extsock plugin library available: " + env_info.plugin_path);
        } else {
            REAL_PLUGIN_ERROR("extsock plugin library not available");
        }
    }
    
    // Plugin이 로드되지 않은 경우 테스트를 건너뛸지 결정
    if (!plugin_loaded_ && REAL_PLUGIN_PHASE >= 2) {
        GTEST_SKIP() << "extsock plugin not available - skipping plugin-specific test";
    }
}

void ExtsockPluginFixture::TearDown() {
    REAL_PLUGIN_DEBUG("ExtsockPluginFixture::TearDown()");
    
    plugin_loaded_ = false;
    
    // Plugin 정리 작업 (Phase 2+에서 필요시)
    if (REAL_PLUGIN_PHASE >= 2) {
        // 실제 plugin unload 로직 구현 예정
    }
    
    // 부모 클래스 teardown 호출
    StrongSwanIntegrationFixture::TearDown();
}

} // namespace strongswan_test