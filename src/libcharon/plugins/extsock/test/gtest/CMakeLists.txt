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

# 인클루드 디렉토리 설정
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # test 디렉토리
    ${CMAKE_CURRENT_SOURCE_DIR}/../..  # extsock 디렉토리
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..  # plugins 디렉토리
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../..  # libcharon 디렉토리
)

# extsock 플러그인 소스 파일들 (필요시)
set(EXTSOCK_SOURCES
    # 현재는 비어있음 - 실제 소스 연동 시 추가
    # ../common/extsock_errors.c
    # ../adapters/json/extsock_json_parser.c
)

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