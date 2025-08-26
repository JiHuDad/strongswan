/**
 * Plugin Test Fixtures for Real Plugin Testing
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 1 Implementation - Basic fixture classes
 */

#ifndef PLUGIN_TEST_FIXTURES_HPP
#define PLUGIN_TEST_FIXTURES_HPP

#include <gtest/gtest.h>
#include "strongswan_test_helpers.hpp"

namespace strongswan_test {

/**
 * Real Plugin 테스트를 위한 기본 Fixture
 * Phase 1: strongSwan 초기화 없이 기본 환경만 제공
 */
class RealPluginTestFixture : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    
    // 환경 정보 접근
    const TestEnvironmentInfo& GetEnvironmentInfo() const { return env_info_; }
    
    // Phase별 초기화 체크
    bool IsEnvironmentReady() const { return environment_ready_; }
    
private:
    TestEnvironmentInfo env_info_;
    bool environment_ready_ = false;
};

/**
 * strongSwan 통합 테스트를 위한 Fixture (Phase 2+)
 * Phase 1에서는 Mock 모드로 동작
 */
class StrongSwanIntegrationFixture : public RealPluginTestFixture {
protected:
    void SetUp() override;
    void TearDown() override;
    
    // strongSwan 상태 확인 (Phase 1에서는 Mock)
    bool IsStrongSwanReady() const { return strongswan_ready_; }
    
private:
    bool strongswan_ready_ = false;
};

/**
 * extsock Plugin 특화 Fixture
 */
class ExtsockPluginFixture : public StrongSwanIntegrationFixture {
protected:
    void SetUp() override;
    void TearDown() override;
    
    // Plugin 상태 확인
    bool IsPluginLoaded() const { return plugin_loaded_; }
    
private:
    bool plugin_loaded_ = false;
};

} // namespace strongswan_test

#endif // PLUGIN_TEST_FIXTURES_HPP