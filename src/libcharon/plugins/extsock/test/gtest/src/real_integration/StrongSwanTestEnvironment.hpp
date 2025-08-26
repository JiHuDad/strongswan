/**
 * StrongSwan Test Environment Header
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 1 Implementation - Basic test environment management
 */

#ifndef STRONGSWAN_TEST_ENVIRONMENT_HPP
#define STRONGSWAN_TEST_ENVIRONMENT_HPP

#include <gtest/gtest.h>
#include <string>

namespace strongswan_test {

// Forward declaration
struct TestEnvironmentInfo;
enum class StrongSwanStatus;

/**
 * strongSwan 테스트 환경을 관리하는 전역 Environment 클래스
 * Phase별로 다른 초기화 방식을 제공
 */
class StrongSwanTestEnvironment : public ::testing::Environment {
public:
    StrongSwanTestEnvironment();
    virtual ~StrongSwanTestEnvironment() = default;
    
    // Google Test Environment 인터페이스
    void SetUp() override;
    void TearDown() override;
    
    // 상태 확인
    bool IsInitialized() const { return initialized_; }
    bool ArePluginsLoaded() const { return plugins_loaded_; }
    
    // strongSwan 관련 유틸리티 (Phase별 구현)
    void ResetLibraryState();
    bool LoadMinimalPlugins();
    
    // 환경 정보 접근
    TestEnvironmentInfo GetEnvironmentInfo() const;
    
private:
    bool initialized_;
    bool plugins_loaded_;
    
    // Phase별 환경 설정
    std::string plugin_library_path_;
    StrongSwanStatus strongswan_status_;
    
    // Phase별 초기화 메서드
    void SetupPhase1MockEnvironment();
    void SetupPhase2RealEnvironment();
    void SetupPhase3FullEnvironment();
    
    // 환경 검증 메서드
    void VerifyRequiredDirectories();
    void CheckPluginLibraryExists();
    void SetupEnvironmentInfo();
    void CleanupStrongSwanEnvironment();
    
    // Phase 2+ 전용 메서드
    bool InitializeStrongSwanMockAPI();
    
    // Phase 3 전용 메서드
    bool InitializeAdvancedStrongSwanAPI();
    bool SetupAdvancedFeatures();
    bool EnableAdvancedCertificateSupport();
    bool EnableAdvancedNetworking();
    bool EnableEndToEndTestSupport();
    bool LoadExtendedPlugins();
    bool PrepareEndToEndEnvironment();
    bool PrepareTestScenarios();
    bool SetupPerformanceMonitoring();
    bool SetupAdvancedLogging();
};

// 전역 환경 인스턴스
extern StrongSwanTestEnvironment* g_strongswan_env;

} // namespace strongswan_test

#endif // STRONGSWAN_TEST_ENVIRONMENT_HPP