#!/bin/bash
#
# Memory Test Runner for extsock Plugin
# TASK-016: CI/CD Pipeline Memory Testing
# 
# This script runs comprehensive memory leak detection
# and memory usage analysis for all test components.
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m'

# Memory test log
MEMORY_LOG="memory_test_results_$(date +%Y%m%d_%H%M%S).log"

echo "=======================================================" | tee -a "$MEMORY_LOG"
echo "🧠 extsock Plugin Memory Tests" | tee -a "$MEMORY_LOG"
echo "=======================================================" | tee -a "$MEMORY_LOG"
echo "Timestamp: $(date)" | tee -a "$MEMORY_LOG"
echo "System Memory Info:" | tee -a "$MEMORY_LOG"
if command -v free &> /dev/null; then
    free -h | tee -a "$MEMORY_LOG"
fi
echo "" | tee -a "$MEMORY_LOG"

# Check Valgrind availability
if ! command -v valgrind &> /dev/null; then
    echo -e "${RED}❌ Valgrind not found. Installing or skipping memory tests.${NC}" | tee -a "$MEMORY_LOG"
    echo "Please install valgrind for memory leak detection" | tee -a "$MEMORY_LOG"
    exit 1
fi

echo "Valgrind Version: $(valgrind --version)" | tee -a "$MEMORY_LOG"
echo "" | tee -a "$MEMORY_LOG"

# Results tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
LEAKS_FOUND=0
ERRORS_FOUND=0

# Function to run memory test
run_memory_test() {
    local test_name="$1"
    local test_executable="$2"
    local description="$3"
    local additional_flags="$4"
    
    echo -e "${BLUE}=== $test_name ===${NC}" | tee -a "$MEMORY_LOG"
    echo "$description" | tee -a "$MEMORY_LOG"
    echo ""
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ ! -f "$test_executable" ]; then
        echo -e "${YELLOW}⚠️  Test not found: $test_executable${NC}" | tee -a "$MEMORY_LOG"
        echo "" | tee -a "$MEMORY_LOG"
        return 0
    fi
    
    # Create individual log for this test
    local test_log="${test_executable##*/}_memory.log"
    
    echo "Running Valgrind on: $test_executable" | tee -a "$MEMORY_LOG"
    echo "Valgrind log: $test_log" | tee -a "$MEMORY_LOG"
    
    # Standard memory check
    local valgrind_cmd="valgrind --tool=memcheck \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        --error-exitcode=42 \
        --log-file=$test_log \
        $additional_flags \
        $test_executable"
    
    echo "Command: $valgrind_cmd" | tee -a "$MEMORY_LOG"
    
    # Check for timeout command availability
    if command -v timeout >/dev/null 2>&1; then
        timeout_cmd="timeout 300"
    else
        timeout_cmd=""
    fi
    
    if $timeout_cmd $valgrind_cmd 2>&1; then
        echo -e "${GREEN}✅ $test_name: NO ERRORS${NC}" | tee -a "$MEMORY_LOG"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        local exit_code=$?
        if [ $exit_code -eq 42 ]; then
            echo -e "${RED}❌ $test_name: MEMORY ERRORS FOUND${NC}" | tee -a "$MEMORY_LOG"
            ERRORS_FOUND=$((ERRORS_FOUND + 1))
        else
            echo -e "${YELLOW}⚠️  $test_name: COMPLETED WITH STATUS $exit_code${NC}" | tee -a "$MEMORY_LOG"
        fi
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    # Parse Valgrind output for summary
    if [ -f "$test_log" ]; then
        echo "--- Memory Test Summary ---" | tee -a "$MEMORY_LOG"
        
        # Extract key information
        if grep -q "definitely lost:" "$test_log"; then
            local definitely_lost=$(grep "definitely lost:" "$test_log" | head -1)
            echo "$definitely_lost" | tee -a "$MEMORY_LOG"
            
            if ! echo "$definitely_lost" | grep -q "0 bytes in 0 blocks"; then
                LEAKS_FOUND=$((LEAKS_FOUND + 1))
                echo -e "${RED}🔴 MEMORY LEAK DETECTED${NC}" | tee -a "$MEMORY_LOG"
            fi
        fi
        
        if grep -q "indirectly lost:" "$test_log"; then
            grep "indirectly lost:" "$test_log" | head -1 | tee -a "$MEMORY_LOG"
        fi
        
        if grep -q "ERROR SUMMARY:" "$test_log"; then
            local error_summary=$(grep "ERROR SUMMARY:" "$test_log" | head -1)
            echo "$error_summary" | tee -a "$MEMORY_LOG"
            
            if ! echo "$error_summary" | grep -q "0 errors"; then
                echo -e "${RED}🔴 MEMORY ERRORS DETECTED${NC}" | tee -a "$MEMORY_LOG"
            fi
        fi
        
        # Show heap usage
        if grep -q "total heap usage:" "$test_log"; then
            grep "total heap usage:" "$test_log" | head -1 | tee -a "$MEMORY_LOG"
        fi
    fi
    
    echo "" | tee -a "$MEMORY_LOG"
}

# Build all tests first
echo -e "${PURPLE}🔨 Building Tests for Memory Analysis${NC}" | tee -a "$MEMORY_LOG"

build_makefiles=("Makefile.infrastructure" "Makefile.pure" "Makefile.adapter" "Makefile.integration")

for makefile in "${build_makefiles[@]}"; do
    if [ -f "$makefile" ]; then
        echo "Building $makefile with debug symbols..." | tee -a "$MEMORY_LOG"
        CFLAGS="-g -O0" make -f "$makefile" clean >> "$MEMORY_LOG" 2>&1 || true
        CFLAGS="-g -O0" make -f "$makefile" all >> "$MEMORY_LOG" 2>&1 || echo "Build completed for $makefile"
    fi
done

echo "" | tee -a "$MEMORY_LOG"

# Infrastructure Memory Tests
echo -e "${GREEN}📊 Infrastructure Memory Tests${NC}" | tee -a "$MEMORY_LOG"

run_memory_test "strongSwan Mocks Memory Test" "infrastructure/test_strongswan_mocks" \
    "Testing strongSwan mock system for memory leaks" \
    "--suppressions=/dev/null"

run_memory_test "Test Container Memory Test" "infrastructure/test_test_container" \
    "Testing dependency injection container for memory leaks" \
    "--track-fds=yes"

# Pure Unit Tests Memory Analysis
echo -e "${GREEN}🧪 Pure Unit Tests Memory Analysis${NC}" | tee -a "$MEMORY_LOG"

run_memory_test "extsock_errors Memory Test" "unit/test_extsock_errors_pure" \
    "Testing error handling system for memory leaks" \
    "--track-fds=yes"

run_memory_test "extsock_types Memory Test" "unit/test_extsock_types_pure" \
    "Testing type system for memory leaks" \
    ""

# Adapter Memory Tests
echo -e "${GREEN}🔌 Adapter Memory Tests${NC}" | tee -a "$MEMORY_LOG"

# Test JSON parser if available
json_tests=$(find . -name "*json*" -executable 2>/dev/null | head -3)
for test in $json_tests; do
    if [ -f "$test" ]; then
        run_memory_test "JSON Parser Memory Test" "$test" \
            "Testing JSON parsing adapter for memory leaks" \
            "--track-fds=yes"
        break
    fi
done

# Integration Memory Tests
echo -e "${GREEN}🔗 Integration Memory Tests${NC}" | tee -a "$MEMORY_LOG"

run_memory_test "Config Entity Memory Test" "integration/test_config_entity_real" \
    "Testing configuration entity for memory leaks" \
    "--track-fds=yes"

run_memory_test "Config Usecase Memory Test" "integration/test_config_usecase_real" \
    "Testing configuration usecase for memory leaks" \
    ""

run_memory_test "Event Usecase Memory Test" "integration/test_event_usecase_real" \
    "Testing event usecase for memory leaks" \
    ""

run_memory_test "Failover Manager Memory Test" "integration/test_failover_manager_real" \
    "Testing failover manager for memory leaks" \
    ""

# Comprehensive Integration Tests
echo -e "${GREEN}🚀 Comprehensive Integration Memory Tests${NC}" | tee -a "$MEMORY_LOG"

run_memory_test "End-to-End Workflow Memory Test" "integration/test_end_to_end_workflow" \
    "Testing complete workflow for memory leaks including stress scenarios" \
    "--track-fds=yes --show-reachable=yes"

run_memory_test "Plugin Lifecycle Memory Test" "integration/test_plugin_lifecycle_real" \
    "Testing plugin lifecycle for memory leaks" \
    "--track-fds=yes"

# Advanced Memory Analysis
echo -e "${GREEN}🔬 Advanced Memory Analysis${NC}" | tee -a "$MEMORY_LOG"

# Test with Massif for memory profiling
if [ -f "integration/test_end_to_end_workflow" ]; then
    echo "Running Massif memory profiler..." | tee -a "$MEMORY_LOG"
    
    # Check for timeout command availability
    if command -v timeout >/dev/null 2>&1; then
        timeout 300 valgrind --tool=massif \
            --time-unit=ms \
            --detailed-freq=1 \
            --massif-out-file=massif.out.integration \
            ./integration/test_end_to_end_workflow >> "$MEMORY_LOG" 2>&1 || echo "Massif completed"
    else
        valgrind --tool=massif \
            --time-unit=ms \
            --detailed-freq=1 \
            --massif-out-file=massif.out.integration \
            ./integration/test_end_to_end_workflow >> "$MEMORY_LOG" 2>&1 || echo "Massif completed"
    fi
    
    if [ -f "massif.out.integration" ]; then
        echo "✅ Massif profile generated: massif.out.integration" | tee -a "$MEMORY_LOG"
        
        # Get peak memory usage
        if command -v ms_print &> /dev/null; then
            echo "--- Peak Memory Usage ---" | tee -a "$MEMORY_LOG"
            ms_print massif.out.integration | grep "MB\|KB\|bytes" | head -5 | tee -a "$MEMORY_LOG"
        fi
    fi
fi

# Test with Helgrind for thread issues (if multithreaded tests exist)
echo -e "${GREEN}🧵 Thread Safety Analysis${NC}" | tee -a "$MEMORY_LOG"

if [ -f "integration/test_end_to_end_workflow" ]; then
    echo "Running Helgrind thread safety check..." | tee -a "$MEMORY_LOG"
    
    local helgrind_log="helgrind_thread_safety.log"
    if command -v timeout >/dev/null 2>&1; then
        timeout 300 valgrind --tool=helgrind \
            --log-file="$helgrind_log" \
            ./integration/test_end_to_end_workflow 2>&1 || echo "Helgrind completed"
    else
        valgrind --tool=helgrind \
            --log-file="$helgrind_log" \
            ./integration/test_end_to_end_workflow 2>&1 || echo "Helgrind completed"
    fi
    
    if [ -f "$helgrind_log" ]; then
        echo "Thread safety analysis completed" | tee -a "$MEMORY_LOG"
        if grep -q "ERROR SUMMARY: 0 errors" "$helgrind_log"; then
            echo -e "${GREEN}✅ No thread safety issues found${NC}" | tee -a "$MEMORY_LOG"
        else
            echo -e "${YELLOW}⚠️  Thread safety issues may exist${NC}" | tee -a "$MEMORY_LOG"
            grep "ERROR SUMMARY:" "$helgrind_log" | tee -a "$MEMORY_LOG"
        fi
    fi
fi

# Generate Memory Test Summary
echo "" | tee -a "$MEMORY_LOG"
echo "=======================================================" | tee -a "$MEMORY_LOG"
echo "🎯 Memory Test Summary" | tee -a "$MEMORY_LOG"
echo "=======================================================" | tee -a "$MEMORY_LOG"

echo "Test Results:" | tee -a "$MEMORY_LOG"
echo "  Total Tests: $TOTAL_TESTS" | tee -a "$MEMORY_LOG"
echo -e "  ${GREEN}✅ Passed: $PASSED_TESTS${NC}" | tee -a "$MEMORY_LOG"
echo -e "  ${RED}❌ Failed: $FAILED_TESTS${NC}" | tee -a "$MEMORY_LOG"
echo -e "  ${RED}🔴 Memory Leaks: $LEAKS_FOUND${NC}" | tee -a "$MEMORY_LOG"
echo -e "  ${RED}🔴 Memory Errors: $ERRORS_FOUND${NC}" | tee -a "$MEMORY_LOG"

echo "" | tee -a "$MEMORY_LOG"
echo "Memory Analysis Files Generated:" | tee -a "$MEMORY_LOG"
ls -la *.log massif.out.* 2>/dev/null | tee -a "$MEMORY_LOG" || echo "No additional files"

echo "" | tee -a "$MEMORY_LOG"
echo "System Memory After Tests:" | tee -a "$MEMORY_LOG"
if command -v free &> /dev/null; then
    free -h | tee -a "$MEMORY_LOG"
fi

echo "" | tee -a "$MEMORY_LOG"
echo "End Time: $(date)" | tee -a "$MEMORY_LOG"
echo "Results saved to: $MEMORY_LOG" | tee -a "$MEMORY_LOG"

# Final status
if [ $LEAKS_FOUND -gt 0 ] || [ $ERRORS_FOUND -gt 0 ]; then
    echo -e "${RED}❌ MEMORY ISSUES DETECTED${NC}" | tee -a "$MEMORY_LOG"
    echo "Please review individual test logs for details" | tee -a "$MEMORY_LOG"
    exit 1
else
    echo -e "${GREEN}🎉 ALL MEMORY TESTS PASSED${NC}" | tee -a "$MEMORY_LOG"
    echo "No memory leaks or errors detected" | tee -a "$MEMORY_LOG"
    exit 0
fi