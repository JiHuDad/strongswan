/**
 * Real Plugin Tests Main Entry Point
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 1 Implementation - Basic test execution environment
 */

#include <gtest/gtest.h>
#include "StrongSwanTestEnvironment.hpp"
#include "strongswan_test_helpers.hpp"
#include "real_plugin_macros.hpp"
#include <iostream>
#include <string>

using namespace strongswan_test;

/**
 * Phase별 시작 메시지 출력
 */
void PrintPhaseWelcomeMessage() {
    std::cout << "\n";
    std::cout << "🚀====================================================================🚀" << std::endl;
    std::cout << "        Real Plugin Tests - strongSwan extsock Plugin" << std::endl; 
    std::cout << "🚀====================================================================🚀" << std::endl;
    std::cout << "Phase: " << REAL_PLUGIN_PHASE << std::endl;
    
    if (REAL_PLUGIN_PHASE == 1) {
        std::cout << "📋 Scope: Infrastructure Setup and Environment Verification" << std::endl;
        std::cout << "🎯 Goal: Establish testing foundation without strongSwan dependencies" << std::endl;
        std::cout << "🔧 Mode: Mock Environment (no actual strongSwan initialization)" << std::endl;
    } else if (REAL_PLUGIN_PHASE == 2) {
        std::cout << "📋 Scope: Real strongSwan Integration and Core Component Testing" << std::endl;
        std::cout << "🎯 Goal: Test actual extsock plugin functions with strongSwan APIs" << std::endl;
        std::cout << "🔧 Mode: Real Environment (strongSwan library integration)" << std::endl;
    } else {
        std::cout << "📋 Scope: Full End-to-End Integration and Performance Testing" << std::endl;
        std::cout << "🎯 Goal: Complete workflow validation and production readiness" << std::endl;
        std::cout << "🔧 Mode: Production Environment (full strongSwan ecosystem)" << std::endl;
    }
    
    std::cout << "⏰ Started: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << "======================================================================" << std::endl;
    std::cout << "\n";
}

/**
 * 테스트 결과 요약 출력
 */
void PrintTestSummary(int result) {
    std::cout << "\n";
    std::cout << "======================================================================" << std::endl;
    std::cout << "                    Real Plugin Tests Summary" << std::endl;
    std::cout << "======================================================================" << std::endl;
    std::cout << "Phase " << REAL_PLUGIN_PHASE << " Execution Result: ";
    
    if (result == 0) {
        std::cout << "✅ SUCCESS" << std::endl;
        std::cout << "🎉 All tests passed!" << std::endl;
        
        if (REAL_PLUGIN_PHASE == 1) {
            std::cout << "✨ Infrastructure setup verification complete" << std::endl;
            std::cout << "📋 Next Steps:" << std::endl;
            std::cout << "   1. Review test results and environment status" << std::endl;
            std::cout << "   2. Proceed to Phase 2 implementation" << std::endl;
            std::cout << "   3. Add real strongSwan integration" << std::endl;
        }
    } else {
        std::cout << "❌ FAILED" << std::endl;
        std::cout << "🚨 Some tests failed. Please review the output above." << std::endl;
        std::cout << "🔧 Troubleshooting:" << std::endl;
        std::cout << "   1. Check environment setup" << std::endl;
        std::cout << "   2. Verify required dependencies" << std::endl;
        std::cout << "   3. Review failed test details" << std::endl;
    }
    
    std::cout << "======================================================================" << std::endl;
    std::cout << "\n";
}

/**
 * 환경 정보 출력
 */
void PrintEnvironmentInfo() {
    std::cout << "🔍 Environment Information:" << std::endl;
    std::cout << "   - Build Type: " << 
    #ifdef NDEBUG
        "Release" 
    #else
        "Debug"
    #endif
        << std::endl;
    
    std::cout << "   - strongSwan Test Mode: " << 
    #ifdef STRONGSWAN_TEST_MODE
        "Enabled"
    #else
        "Disabled"
    #endif
        << std::endl;
    
    std::cout << "   - Real Plugin Mode: " << 
    #ifdef USE_REAL_PLUGIN
        "Enabled"
    #else
        "Disabled"
    #endif
        << std::endl;
    
    std::cout << "   - Current Phase: " << REAL_PLUGIN_PHASE << std::endl;
    
    // Phase별 추가 정보
    if (REAL_PLUGIN_PHASE == 1) {
        std::cout << "   - strongSwan Dependencies: Not Required (Mock Mode)" << std::endl;
        std::cout << "   - Plugin Library: Optional (Structure Verification Only)" << std::endl;
    } else {
        std::cout << "   - strongSwan Dependencies: Required" << std::endl;
        std::cout << "   - Plugin Library: Required" << std::endl;
    }
    
    std::cout << std::endl;
}

/**
 * Command line argument 파싱
 */
void ProcessCommandLineArgs(int argc, char** argv) {
    std::cout << "📋 Command Line Arguments:" << std::endl;
    
    for (int i = 0; i < argc; ++i) {
        std::cout << "   [" << i << "]: " << argv[i] << std::endl;
    }
    
    // gtest 필터 확인
    bool has_filter = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--gtest_filter=") == 0) {
            has_filter = true;
            std::cout << "🔍 Test Filter: " << arg.substr(15) << std::endl;
            break;
        }
    }
    
    if (!has_filter) {
        std::cout << "🔍 Test Filter: All tests (no filter specified)" << std::endl;
        
        // Phase별 권장 필터 출력
        if (REAL_PLUGIN_PHASE == 1) {
            std::cout << "💡 Suggested filters for Phase 1:" << std::endl;
            std::cout << "   --gtest_filter=\"*Infrastructure*\"  (infrastructure tests only)" << std::endl;
            std::cout << "   --gtest_filter=\"*Environment*\"     (environment tests only)" << std::endl;
        }
    }
    
    std::cout << std::endl;
}

/**
 * Main 함수
 */
int main(int argc, char **argv) {
    // Phase별 환영 메시지
    PrintPhaseWelcomeMessage();
    
    // 환경 정보 출력
    PrintEnvironmentInfo();
    
    // Command line arguments 처리
    ProcessCommandLineArgs(argc, argv);
    
    // Google Test 초기화
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "🧪 Initializing Google Test framework..." << std::endl;
    
    // strongSwan 테스트 환경 등록
    std::cout << "🔧 Setting up strongSwan test environment..." << std::endl;
    g_strongswan_env = new StrongSwanTestEnvironment();
    ::testing::AddGlobalTestEnvironment(g_strongswan_env);
    
    std::cout << "✅ Test environment registered successfully" << std::endl;
    std::cout << "\n";
    
    // 테스트 실행
    std::cout << "🏃 Running Real Plugin Tests..." << std::endl;
    std::cout << "======================================================================" << std::endl;
    
    int result = RUN_ALL_TESTS();
    
    // 결과 요약 출력
    PrintTestSummary(result);
    
    return result;
}