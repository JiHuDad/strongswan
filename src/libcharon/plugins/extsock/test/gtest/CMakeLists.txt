# extsock Plugin Google Test Suite
# CMake 빌드 시스템 for Google Test Migration

cmake_minimum_required(VERSION 3.14)
project(extsock_gtest LANGUAGES CXX C)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 컴파일 옵션
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -O0")

# Google Test 설정
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)

# Google Test를 시스템 라이브러리로 처리하여 경고 억제
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ========================================
# Real Plugin Integration Configuration  
# ========================================

# Phase 설정 (기본값: 1)
if(NOT DEFINED REAL_PLUGIN_PHASE)
    set(REAL_PLUGIN_PHASE 1)
endif()

message(STATUS "🚀 Real Plugin Test Phase: ${REAL_PLUGIN_PHASE}")

# extsock Plugin 경로 설정
set(EXTSOCK_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(EXTSOCK_PLUGIN_LA "${EXTSOCK_PLUGIN_DIR}/libstrongswan-extsock.la")

# strongSwan 라이브러리 경로 (자동 탐지)
execute_process(
    COMMAND pkg-config --variable=plugindir strongswan
    OUTPUT_VARIABLE STRONGSWAN_PLUGIN_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT STRONGSWAN_PLUGIN_DIR)
    set(STRONGSWAN_PLUGIN_DIR "/usr/local/lib/ipsec/plugins")
endif()

message(STATUS "📦 strongSwan Plugin Directory: ${STRONGSWAN_PLUGIN_DIR}")

# strongSwan 헤더 경로
find_path(STRONGSWAN_INCLUDE_DIR
    NAMES library.h
    PATHS /usr/local/include/strongswan /usr/include/strongswan
    PATH_SUFFIXES libstrongswan
)

if(STRONGSWAN_INCLUDE_DIR)
    message(STATUS "✅ strongSwan Headers: ${STRONGSWAN_INCLUDE_DIR}")
else()
    message(WARNING "⚠️  strongSwan headers not found - Real Plugin tests may fail")
endif()

# 기존 인클루드 디렉토리 설정
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # test 디렉토리
    ${CMAKE_CURRENT_SOURCE_DIR}/../..  # extsock 디렉토리
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..  # plugins 디렉토리
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../..  # libcharon 디렉토리
)

# strongSwan 헤더 추가 (Phase 2+에서 사용)
if(STRONGSWAN_INCLUDE_DIR AND REAL_PLUGIN_PHASE GREATER 1)
    include_directories(
        ${STRONGSWAN_INCLUDE_DIR}
        ${STRONGSWAN_INCLUDE_DIR}/../libcharon
        ${EXTSOCK_PLUGIN_DIR}
    )
endif()

# extsock 플러그인 소스 파일들 (Phase별 관리)
if(REAL_PLUGIN_PHASE GREATER 1)
    set(EXTSOCK_SOURCES
        # Phase 2+에서 실제 소스 연동
        # ../common/extsock_errors.c
        # ../adapters/json/extsock_json_parser.c
    )
else()
    set(EXTSOCK_SOURCES
        # Phase 1에서는 비어있음
    )
endif()

# 테스트 인프라 라이브러리
add_library(extsock_test_infrastructure
    # Mock 클래스들
    infrastructure/mocks/MockStrongSwan.cpp
    infrastructure/mocks/MockJsonParser.cpp
    infrastructure/mocks/MockSocketAdapter.cpp
    
    # Fixture 클래스들
    infrastructure/fixtures/ExtsockTestBase.cpp
    infrastructure/fixtures/IntegrationTestFixture.cpp
    
    # 유틸리티 클래스들 (향후 추가)
    # infrastructure/utils/TestDataFactory.cpp
)

target_link_libraries(extsock_test_infrastructure
    gtest
    gmock
    pthread
)

# Hello World 테스트 (초기 설정 검증용)
add_executable(hello_gtest_test
    infrastructure/hello_gtest_test.cpp
)

target_link_libraries(hello_gtest_test
    gtest_main
    extsock_test_infrastructure
)

# Mock Integration 테스트 (Mock 인프라 검증용)
add_executable(mock_integration_test
    infrastructure/mock_integration_test.cpp
)

target_link_libraries(mock_integration_test
    gtest_main
    gmock_main
    extsock_test_infrastructure
)

# Unit Tests - Level 1 Pure + Level 2 Adapter Tests
set(UNIT_TEST_SOURCES
    # Level 1 Pure Tests
    src/unit/ExtsockErrorsTest.cpp
    src/unit/ExtsockTypesTest.cpp
    
    # Level 2 Adapter Tests
    src/unit/JsonParserTestSimple.cpp
    src/unit/SocketAdapterTest.cpp
    src/unit/StrongswanAdapterTest.cpp
    
    # unit/domain/ConfigEntityTest.cpp
    # unit/domain/ConfigUsecaseTest.cpp
)

# C Wrapper 소스 파일들 (Pure 구현)
set(C_WRAPPER_SOURCES
    src/c_wrappers/extsock_errors_pure.c
)

add_executable(unit_tests ${UNIT_TEST_SOURCES} ${C_WRAPPER_SOURCES})

target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/c_wrappers
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(unit_tests
    gtest_main
    gmock_main
    extsock_test_infrastructure
)

# Integration Tests
# 현재는 비어있음 - 마이그레이션 진행하면서 추가

# set(INTEGRATION_TEST_SOURCES  
#     integration/WorkflowIntegrationTest.cpp
#     integration/PluginLifecycleTest.cpp
#     integration/FailoverIntegrationTest.cpp
# )

# add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})
# target_link_libraries(integration_tests
#     gtest_main  
#     gmock_main
#     extsock_test_infrastructure
# )

# Performance Tests (선택사항)
# Google Benchmark 사용 시 활성화

# find_package(benchmark QUIET)
# if(benchmark_FOUND)
#     add_executable(performance_tests
#         performance/BenchmarkTests.cpp
#         performance/StressTests.cpp
#     )
#     target_link_libraries(performance_tests
#         benchmark::benchmark
#         extsock_test_infrastructure
#     )
# endif()

# CTest 활성화
enable_testing()

# 테스트 등록
add_test(NAME hello_gtest_test COMMAND hello_gtest_test)
add_test(NAME mock_integration_test COMMAND mock_integration_test)
add_test(NAME unit_tests COMMAND unit_tests)
# add_test(NAME integration_tests COMMAND integration_tests)

# Google Test Discovery (CMake 3.10+)
include(GoogleTest)
gtest_discover_tests(hello_gtest_test)

# 커스텀 타겟들

# 모든 테스트 빌드
add_custom_target(build_all_tests
    DEPENDS hello_gtest_test mock_integration_test unit_tests
    # integration_tests
    COMMENT "Building all Google Test executables"
)

# 테스트 실행 (상세 출력)
add_custom_target(run_tests_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS build_all_tests
    COMMENT "Running all tests with verbose output"
)

# XML 출력으로 테스트 실행 (CI/CD용)
add_custom_target(run_tests_xml
    COMMAND hello_gtest_test --gtest_output=xml:hello_gtest_results.xml
    COMMAND mock_integration_test --gtest_output=xml:mock_integration_results.xml
    COMMAND unit_tests --gtest_output=xml:unit_test_results.xml
    # COMMAND integration_tests --gtest_output=xml:integration_test_results.xml
    DEPENDS build_all_tests  
    COMMENT "Running all tests with XML output for CI/CD"
)

# ========================================
# Real Plugin Tests Configuration
# ========================================

# Real Plugin 소스 파일들 (Phase별)
if(REAL_PLUGIN_PHASE EQUAL 1)
    set(REAL_PLUGIN_SOURCES
        src/real_integration/StrongSwanTestEnvironment.cpp
        src/real_integration/RealPluginTestBase.cpp  
        src/real_integration/RealExtsockErrorsTest.cpp
        src/real_integration/strongswan_test_helpers.cpp
        src/real_integration/TestMain.cpp
    )
elseif(REAL_PLUGIN_PHASE EQUAL 2)
    set(REAL_PLUGIN_SOURCES
        src/real_integration/StrongSwanTestEnvironment.cpp
        src/real_integration/RealPluginTestBase.cpp
        src/real_integration/RealExtsockErrorsTest.cpp
        src/real_integration/strongswan_mock_api.cpp
        src/real_integration/strongswan_test_helpers.cpp
        src/real_integration/TestMain.cpp
        # Future Phase 2 tests (placeholder)
        # src/real_integration/RealJsonParserTest.cpp
        # src/real_integration/RealSocketAdapterTest.cpp
    )
elseif(REAL_PLUGIN_PHASE EQUAL 4)
    # Phase 4: 실제 .so 라이브러리 직접 호출
    set(REAL_PLUGIN_SOURCES
        src/real_integration/StrongSwanTestEnvironment.cpp
        src/real_integration/RealPluginTestBase.cpp
        src/real_integration/RealExtsockErrorsTest.cpp
        src/real_integration/RealExtsockFunctionTest.cpp
        src/real_integration/RealEndToEndTest.cpp
        src/real_integration/RealPluginLoader.cpp
        src/real_integration/strongswan_mock_api.cpp
        src/real_integration/strongswan_test_helpers.cpp
        src/real_integration/TestMain.cpp
        # Phase 4 특화 테스트들
        src/real_integration/RealDirectLibraryTest.cpp
        # Phase 4: strongSwan Mock Library 포함하지 않음 (별도 타겟으로 분리)
    )
else()
    # Phase 3+: 모든 Real Plugin 테스트 포함
    set(REAL_PLUGIN_SOURCES
        src/real_integration/StrongSwanTestEnvironment.cpp
        src/real_integration/RealPluginTestBase.cpp
        src/real_integration/RealExtsockErrorsTest.cpp
        src/real_integration/RealExtsockFunctionTest.cpp
        src/real_integration/RealEndToEndTest.cpp
        src/real_integration/strongswan_mock_api.cpp
        src/real_integration/strongswan_test_helpers.cpp
        src/real_integration/TestMain.cpp
        # Future Phase 3+ tests
        # src/real_integration/RealJsonParserTest.cpp
        # src/real_integration/RealSocketAdapterTest.cpp  
        # src/real_integration/RealPluginLifecycleTest.cpp
    )
endif()

# Real Plugin 테스트 실행파일 (Phase 1+)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/real_integration/TestMain.cpp")
    add_executable(real_plugin_tests ${REAL_PLUGIN_SOURCES})

    # 헤더 경로 설정
    target_include_directories(real_plugin_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/real_integration
    )

    # 라이브러리 링크 (Phase별)
    if(REAL_PLUGIN_PHASE EQUAL 1)
        # Phase 1: 기본 라이브러리만 (strongSwan 없이)
        target_link_libraries(real_plugin_tests
            gtest_main
            gmock_main
            extsock_test_infrastructure
            cjson
            pthread
            dl
        )
    elseif(REAL_PLUGIN_PHASE EQUAL 4)
        # Phase 4: strongSwan Mock Library (static + 컴파일 타임 링킹)
        set(STRONGSWAN_MOCK_SOURCES
            src/real_integration/StrongSwanMockLibrary.cpp
        )
        
        add_library(strongswan_mock_library STATIC ${STRONGSWAN_MOCK_SOURCES})
        target_include_directories(strongswan_mock_library PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include/real_integration
        )
        
        # Phase 4: 실제 .so 라이브러리 동적 로딩
        target_link_libraries(real_plugin_tests
            gtest_main
            gmock_main
            extsock_test_infrastructure
            strongswan_mock_library  # strongSwan Mock Library 링크
            cjson
            pthread
            dl  # dlopen/dlsym를 위해 필수
        )
        
        # Phase 4: Mock 심볼들을 dynamic loader에서 사용 가능하게 export
        target_link_options(real_plugin_tests PRIVATE "-Wl,--export-dynamic")
        
        # Phase 4 전용 컴파일 옵션
        target_compile_definitions(real_plugin_tests PRIVATE
            -DUSE_REAL_LIBRARY_LOADING=1
        )
        
        # Phase 4: 링커 옵션 - Mock 심볼들을 전역적으로 export 가능하게 만들기
        
        # .so 파일 경로를 컴파일 타임에 정의
        get_filename_component(EXTSOCK_SO_PATH 
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../extsock/.libs/libstrongswan-extsock.so" 
            ABSOLUTE)
        target_compile_definitions(real_plugin_tests PRIVATE
            -DEXTSOCK_LIBRARY_PATH="${EXTSOCK_SO_PATH}"
        )
        
        message(STATUS "📚 Phase 4: Will load library from ${EXTSOCK_SO_PATH}")
    else()
        # Phase 2-3: strongSwan 라이브러리 추가
        target_link_libraries(real_plugin_tests
            gtest_main
            gmock_main
            extsock_test_infrastructure
            cjson
            pthread
            dl
        )
    endif()

    # 컴파일러 플래그
    target_compile_definitions(real_plugin_tests PRIVATE
        -DUSE_REAL_PLUGIN=1
        -DSTRONGSWAN_TEST_MODE=1
        -DREAL_PLUGIN_PHASE=${REAL_PLUGIN_PHASE}
    )

    # 테스트 등록
    add_test(NAME real_plugin_tests COMMAND real_plugin_tests)

    message(STATUS "✅ Real Plugin Tests configured for Phase ${REAL_PLUGIN_PHASE}")
else()
    message(STATUS "⏳ Real Plugin Tests source not found - will be created during implementation")
endif()

# CI 전용 Real Plugin Tests (빠른 검증)
if(TARGET real_plugin_tests)
    add_custom_target(ci_real_tests
        COMMAND real_plugin_tests --gtest_filter="-*EndToEnd*:*Performance*"
        DEPENDS real_plugin_tests
        COMMENT "Running Real Plugin Tests for CI (excluding long-running tests)"
    )

    # Full Real Plugin Tests (nightly build용)
    add_custom_target(full_real_tests  
        COMMAND real_plugin_tests
        DEPENDS real_plugin_tests
        COMMENT "Running Full Real Plugin Tests (including End-to-End tests)"
    )
endif()

# 코드 커버리지 (gcov/lcov 필요)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_target(coverage
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
        COMMAND lcov --remove coverage.info '*/_deps/*' --output-file coverage.info  
        COMMAND genhtml coverage.info --output-directory coverage_report
        DEPENDS run_tests_verbose
        COMMENT "Generating code coverage report"
    )
    
    # Real Plugin 커버리지 (별도)
    if(TARGET real_plugin_tests)
        add_custom_target(coverage_real
            COMMAND lcov --capture --directory . --output-file coverage_real.info
            COMMAND lcov --remove coverage_real.info '/usr/*' --output-file coverage_real.info
            COMMAND lcov --remove coverage_real.info '*/_deps/*' --output-file coverage_real.info  
            COMMAND genhtml coverage_real.info --output-directory coverage_real_report
            DEPENDS full_real_tests
            COMMENT "Generating Real Plugin code coverage report"
        )
    endif()
endif()

# 정리 타겟
add_custom_target(clean_gtest
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove *.xml
    COMMAND ${CMAKE_COMMAND} -E remove_directory coverage_report
    COMMAND ${CMAKE_COMMAND} -E remove coverage.info
    COMMENT "Cleaning Google Test build artifacts"
)

# 설치 타겟 (선택사항)
# install(TARGETS hello_gtest_test DESTINATION bin)

# 패키징 설정 (선택사항) 
# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)