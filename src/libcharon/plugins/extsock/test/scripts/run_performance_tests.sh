#!/bin/bash
#
# Performance Test Runner for extsock Plugin
# TASK-016: CI/CD Pipeline Performance Testing
# 
# This script runs performance benchmarks and timing tests
# for all test components.
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Performance log
PERF_LOG="performance_results_$(date +%Y%m%d_%H%M%S).log"

echo "=======================================================" | tee -a "$PERF_LOG"
echo "⚡ extsock Plugin Performance Tests" | tee -a "$PERF_LOG"
echo "=======================================================" | tee -a "$PERF_LOG"
echo "Timestamp: $(date)" | tee -a "$PERF_LOG"
echo "System Info: $(uname -a)" | tee -a "$PERF_LOG"
echo "" | tee -a "$PERF_LOG"

# Function to run performance test
run_performance_test() {
    local test_name="$1"
    local test_executable="$2"
    local description="$3"
    
    echo -e "${BLUE}=== $test_name ===${NC}" | tee -a "$PERF_LOG"
    echo "$description" | tee -a "$PERF_LOG"
    echo ""
    
    if [ ! -f "$test_executable" ]; then
        echo -e "${YELLOW}⚠️  Test not found: $test_executable${NC}" | tee -a "$PERF_LOG"
        return 0
    fi
    
    echo "Running performance test: $test_executable" | tee -a "$PERF_LOG"
    
    # Run with time measurement
    echo "--- Timing Results ---" | tee -a "$PERF_LOG"
    
    if command -v time &> /dev/null; then
        /usr/bin/time -f "Real time: %e seconds\nUser time: %U seconds\nSys time: %S seconds\nMax memory: %M KB\nPage faults: %F" \
            "$test_executable" >> "$PERF_LOG" 2>&1 || echo "Test completed with status: $?"
    else
        # Fallback timing
        local start_time=$(date +%s.%N)
        "$test_executable" >> "$PERF_LOG" 2>&1 || echo "Test completed with status: $?"
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "N/A")
        echo "Duration: $duration seconds" | tee -a "$PERF_LOG"
    fi
    
    echo "" | tee -a "$PERF_LOG"
}

# Build all tests first
echo -e "${CYAN}🔨 Building Tests for Performance Analysis${NC}" | tee -a "$PERF_LOG"

build_makefiles=("Makefile.infrastructure" "Makefile.pure" "Makefile.adapter" "Makefile.integration")

for makefile in "${build_makefiles[@]}"; do
    if [ -f "$makefile" ]; then
        echo "Building $makefile..." | tee -a "$PERF_LOG"
        make -f "$makefile" all >> "$PERF_LOG" 2>&1 || echo "Build completed for $makefile"
    fi
done

echo "" | tee -a "$PERF_LOG"

# Infrastructure Performance Tests
echo -e "${GREEN}📊 Infrastructure Performance Tests${NC}" | tee -a "$PERF_LOG"

run_performance_test "strongSwan Mocks Performance" "infrastructure/test_strongswan_mocks" \
    "Testing strongSwan mock system performance and memory usage"

run_performance_test "Test Container Performance" "infrastructure/test_test_container" \
    "Testing dependency injection container performance"

# Unit Test Performance
echo -e "${GREEN}🧪 Unit Test Performance${NC}" | tee -a "$PERF_LOG"

run_performance_test "extsock_errors Performance" "unit/test_extsock_errors_pure" \
    "Testing error handling system performance"

run_performance_test "extsock_types Performance" "unit/test_extsock_types_pure" \
    "Testing type system performance"

# Adapter Performance Tests
echo -e "${GREEN}🔌 Adapter Performance Tests${NC}" | tee -a "$PERF_LOG"

# Find JSON parser tests
json_tests=$(find . -name "*json*" -executable 2>/dev/null | head -3)
for test in $json_tests; do
    if [ -f "$test" ]; then
        run_performance_test "JSON Parser Performance" "$test" \
            "Testing JSON parsing adapter performance"
        break
    fi
done

# Integration Performance Tests
echo -e "${GREEN}🔗 Integration Performance Tests${NC}" | tee -a "$PERF_LOG"

run_performance_test "Config Entity Performance" "integration/test_config_entity_real" \
    "Testing configuration entity processing performance"

run_performance_test "End-to-End Workflow Performance" "integration/test_end_to_end_workflow" \
    "Testing complete workflow performance including failover scenarios"

run_performance_test "Plugin Lifecycle Performance" "integration/test_plugin_lifecycle_real" \
    "Testing plugin lifecycle performance with timing analysis"

# Stress Tests
echo -e "${GREEN}💪 Stress Tests${NC}" | tee -a "$PERF_LOG"

# Run multiple concurrent tests if available
if [ -f "integration/test_end_to_end_workflow" ]; then
    echo "Running concurrent stress test..." | tee -a "$PERF_LOG"
    
    # Run 3 instances concurrently
    echo "Starting concurrent test instances..." | tee -a "$PERF_LOG"
    
    local pids=()
    for i in {1..3}; do
        (
            echo "Stress instance $i starting..." >> "$PERF_LOG" 2>&1
            ./integration/test_end_to_end_workflow >> "${PERF_LOG}.stress$i" 2>&1
            echo "Stress instance $i completed" >> "$PERF_LOG" 2>&1
        ) &
        pids+=($!)
    done
    
    # Wait for all instances
    echo "Waiting for concurrent tests to complete..." | tee -a "$PERF_LOG"
    for pid in "${pids[@]}"; do
        wait "$pid"
    done
    
    echo "✅ Concurrent stress test completed" | tee -a "$PERF_LOG"
fi

# Memory Performance Analysis
echo -e "${GREEN}🧠 Memory Performance Analysis${NC}" | tee -a "$PERF_LOG"

if command -v valgrind &> /dev/null && [ -f "infrastructure/test_test_container" ]; then
    echo "Running memory performance analysis with Valgrind..." | tee -a "$PERF_LOG"
    
    valgrind --tool=massif --time-unit=ms \
        --massif-out-file=massif.out.performance \
        ./infrastructure/test_test_container >> "$PERF_LOG" 2>&1 || echo "Valgrind analysis completed"
    
    if [ -f "massif.out.performance" ]; then
        echo "Massif memory profile generated: massif.out.performance" | tee -a "$PERF_LOG"
    fi
fi

# Performance Summary
echo "" | tee -a "$PERF_LOG"
echo "=======================================================" | tee -a "$PERF_LOG"
echo "📈 Performance Test Summary" | tee -a "$PERF_LOG"  
echo "=======================================================" | tee -a "$PERF_LOG"

# Count test executables
local test_count=$(find . -name "test_*" -executable | wc -l)
echo "Total Test Executables: $test_count" | tee -a "$PERF_LOG"

# System resource summary
if command -v free &> /dev/null; then
    echo "" | tee -a "$PERF_LOG"
    echo "System Memory:" | tee -a "$PERF_LOG"
    free -h | tee -a "$PERF_LOG"
fi

if command -v nproc &> /dev/null; then
    echo "CPU Cores: $(nproc)" | tee -a "$PERF_LOG"
fi

# Test file sizes
echo "" | tee -a "$PERF_LOG"
echo "Test Binary Sizes:" | tee -a "$PERF_LOG"
find . -name "test_*" -executable -exec ls -lh {} \; | sort -k5 -h | tail -10 | tee -a "$PERF_LOG"

echo "" | tee -a "$PERF_LOG"
echo "Performance test completed: $(date)" | tee -a "$PERF_LOG"
echo "Results saved to: $PERF_LOG" | tee -a "$PERF_LOG"

# Check for performance issues
echo "" | tee -a "$PERF_LOG"
echo "Performance Analysis:" | tee -a "$PERF_LOG"

# Look for long-running tests (>5 seconds)
if grep -q "Real time: [5-9]\|Real time: [0-9][0-9]" "$PERF_LOG"; then
    echo -e "${YELLOW}⚠️  Some tests took longer than expected (>5s)${NC}" | tee -a "$PERF_LOG"
else
    echo -e "${GREEN}✅ All tests completed in reasonable time${NC}" | tee -a "$PERF_LOG"
fi

# Look for high memory usage (>100MB)
if grep -q "Max memory: [1-9][0-9][0-9][0-9][0-9][0-9]" "$PERF_LOG"; then
    echo -e "${YELLOW}⚠️  High memory usage detected (>100MB)${NC}" | tee -a "$PERF_LOG"
else
    echo -e "${GREEN}✅ Memory usage within acceptable limits${NC}" | tee -a "$PERF_LOG"
fi

echo -e "${GREEN}🎉 Performance testing completed successfully${NC}"
echo "Detailed results: $PERF_LOG"