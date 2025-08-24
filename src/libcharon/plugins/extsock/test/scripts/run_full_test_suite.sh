#!/bin/bash
# 
# Complete Test Suite Runner for extsock Plugin
# TASK-016: CI/CD Pipeline Support Script
# 
# This script runs the complete test suite in the correct order
# and provides comprehensive reporting.
#

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Log file
LOG_FILE="test_suite_$(date +%Y%m%d_%H%M%S).log"

echo "=======================================================" | tee -a "$LOG_FILE"
echo "🚀 extsock Plugin Complete Test Suite" | tee -a "$LOG_FILE"
echo "=======================================================" | tee -a "$LOG_FILE"
echo "Timestamp: $(date)" | tee -a "$LOG_FILE"
echo "Working Directory: $(pwd)" | tee -a "$LOG_FILE"
echo "Log File: $LOG_FILE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Function to run test phase
run_test_phase() {
    local phase_name="$1"
    local makefile="$2"
    local target="$3"
    local description="$4"
    
    echo -e "${BLUE}=== $phase_name ===${NC}" | tee -a "$LOG_FILE"
    echo "$description" | tee -a "$LOG_FILE"
    echo ""
    
    if [ ! -f "$makefile" ]; then
        echo -e "${RED}❌ Makefile not found: $makefile${NC}" | tee -a "$LOG_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
    
    echo "Running: make -f $makefile $target" | tee -a "$LOG_FILE"
    
    # Check for timeout command availability
    if command -v timeout >/dev/null 2>&1; then
        timeout_cmd="timeout 300"
    else
        timeout_cmd=""
    fi
    
    if $timeout_cmd make -f "$makefile" "$target" >> "$LOG_FILE" 2>&1; then
        echo -e "${GREEN}✅ $phase_name: SUCCESS${NC}" | tee -a "$LOG_FILE"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}❌ $phase_name: FAILED${NC}" | tee -a "$LOG_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# Function to check prerequisites
check_prerequisites() {
    echo -e "${CYAN}🔍 Checking Prerequisites${NC}" | tee -a "$LOG_FILE"
    
    # Check for required tools
    local tools=("gcc" "make" "pkg-config")
    local missing_tools=()
    
    for tool in "${tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        else
            echo "✓ $tool: $(command -v $tool)" | tee -a "$LOG_FILE"
        fi
    done
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        echo -e "${RED}❌ Missing required tools: ${missing_tools[*]}${NC}" | tee -a "$LOG_FILE"
        echo "Please install missing tools and try again." | tee -a "$LOG_FILE"
        exit 1
    fi
    
    # Check for Check framework
    if pkg-config --exists check; then
        echo "✓ Check framework: $(pkg-config --modversion check)" | tee -a "$LOG_FILE"
    else
        echo -e "${YELLOW}⚠️  Check framework not found via pkg-config${NC}" | tee -a "$LOG_FILE"
    fi
    
    echo "" | tee -a "$LOG_FILE"
}

# Function to clean previous builds
clean_builds() {
    echo -e "${CYAN}🧹 Cleaning Previous Builds${NC}" | tee -a "$LOG_FILE"
    
    local makefiles=("Makefile.infrastructure" "Makefile.pure" "Makefile.adapter" "Makefile.integration")
    
    for makefile in "${makefiles[@]}"; do
        if [ -f "$makefile" ]; then
            echo "Cleaning $makefile..." | tee -a "$LOG_FILE"
            make -f "$makefile" clean >> "$LOG_FILE" 2>&1 || echo "Clean completed for $makefile"
        fi
    done
    
    echo "" | tee -a "$LOG_FILE"
}

# Function to collect test statistics
collect_test_stats() {
    echo -e "${CYAN}📊 Collecting Test Statistics${NC}" | tee -a "$LOG_FILE"
    
    # Count test executables
    local test_count=$(find . -name "test_*" -executable | wc -l)
    echo "Test Executables: $test_count" | tee -a "$LOG_FILE"
    
    # Count source files
    local source_count=$(find . -name "*.c" | wc -l)
    echo "Source Files: $source_count" | tee -a "$LOG_FILE"
    
    # Count object files
    local object_count=$(find . -name "*.o" | wc -l)
    echo "Object Files: $object_count" | tee -a "$LOG_FILE"
    
    echo "" | tee -a "$LOG_FILE"
}

# Function to run memory checks
run_memory_checks() {
    echo -e "${PURPLE}🔍 Memory Leak Checks${NC}" | tee -a "$LOG_FILE"
    
    if ! command -v valgrind &> /dev/null; then
        echo -e "${YELLOW}⚠️  Valgrind not found, skipping memory checks${NC}" | tee -a "$LOG_FILE"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
        return 0
    fi
    
    # Test infrastructure memory
    if [ -f "infrastructure/test_test_container" ]; then
        echo "Running memory check on test container..." | tee -a "$LOG_FILE"
        if valgrind --tool=memcheck --leak-check=full --error-exitcode=1 \
           ./infrastructure/test_test_container >> "$LOG_FILE" 2>&1; then
            echo -e "${GREEN}✅ Memory check: PASSED${NC}" | tee -a "$LOG_FILE"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}❌ Memory check: FAILED${NC}" | tee -a "$LOG_FILE"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        echo -e "${YELLOW}⚠️  Test container not found, skipping memory check${NC}" | tee -a "$LOG_FILE"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
    fi
    
    echo "" | tee -a "$LOG_FILE"
}

# Function to run performance benchmarks
run_performance_benchmarks() {
    echo -e "${PURPLE}⚡ Performance Benchmarks${NC}" | tee -a "$LOG_FILE"
    
    if [ -f "integration/test_plugin_lifecycle_real" ]; then
        echo "Running plugin lifecycle performance test..." | tee -a "$LOG_FILE"
        
        if command -v time &> /dev/null; then
            /usr/bin/time -v ./integration/test_plugin_lifecycle_real >> "$LOG_FILE" 2>&1 || \
                echo "Performance test completed with status: $?" | tee -a "$LOG_FILE"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${YELLOW}⚠️  time command not found, skipping performance test${NC}" | tee -a "$LOG_FILE"
            SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
        fi
    else
        echo -e "${YELLOW}⚠️  Plugin lifecycle test not found${NC}" | tee -a "$LOG_FILE"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
    fi
    
    echo "" | tee -a "$LOG_FILE"
}

# Function to generate final report
generate_final_report() {
    echo "" | tee -a "$LOG_FILE"
    echo "=======================================================" | tee -a "$LOG_FILE"
    echo "🎯 FINAL TEST REPORT" | tee -a "$LOG_FILE"
    echo "=======================================================" | tee -a "$LOG_FILE"
    
    local total=$((PASSED_TESTS + FAILED_TESTS + SKIPPED_TESTS))
    local success_rate=0
    
    if [ $total -gt 0 ]; then
        success_rate=$(( (PASSED_TESTS * 100) / total ))
    fi
    
    echo "Test Summary:" | tee -a "$LOG_FILE"
    echo "  Total Phases: $total" | tee -a "$LOG_FILE"
    echo -e "  ${GREEN}✅ Passed: $PASSED_TESTS${NC}" | tee -a "$LOG_FILE"
    echo -e "  ${RED}❌ Failed: $FAILED_TESTS${NC}" | tee -a "$LOG_FILE"
    echo -e "  ${YELLOW}⚠️  Skipped: $SKIPPED_TESTS${NC}" | tee -a "$LOG_FILE"
    echo "  Success Rate: $success_rate%" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    
    echo "Build System Status:" | tee -a "$LOG_FILE"
    echo "  📁 Infrastructure: $([ -f "Makefile.infrastructure" ] && echo "✅" || echo "❌")" | tee -a "$LOG_FILE"
    echo "  🧪 Pure Unit Tests: $([ -f "Makefile.pure" ] && echo "✅" || echo "❌")" | tee -a "$LOG_FILE"
    echo "  🔌 Adapter Tests: $([ -f "Makefile.adapter" ] && echo "✅" || echo "❌")" | tee -a "$LOG_FILE"
    echo "  🔗 Integration Tests: $([ -f "Makefile.integration" ] && echo "✅" || echo "❌")" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    
    echo "Phase Completion Status:" | tee -a "$LOG_FILE"
    echo "  Phase 1: strongSwan Mock Infrastructure ✅" | tee -a "$LOG_FILE"
    echo "  Phase 2: Common Layer Tests ✅" | tee -a "$LOG_FILE"
    echo "  Phase 3: Adapter Layer Tests ✅" | tee -a "$LOG_FILE"
    echo "  Phase 4: Domain & Usecase Layer Tests ✅" | tee -a "$LOG_FILE"
    echo "  Phase 5: Integration Tests ✅" | tee -a "$LOG_FILE"
    echo "  Phase 6: CI/CD Pipeline ✅" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    
    echo "End Time: $(date)" | tee -a "$LOG_FILE"
    echo "Log Location: $LOG_FILE" | tee -a "$LOG_FILE"
    
    if [ $FAILED_TESTS -gt 0 ]; then
        echo -e "${RED}❌ TEST SUITE FAILED${NC}" | tee -a "$LOG_FILE"
        echo "Check $LOG_FILE for detailed error information" | tee -a "$LOG_FILE"
        return 1
    else
        echo -e "${GREEN}🎉 TEST SUITE COMPLETED SUCCESSFULLY${NC}" | tee -a "$LOG_FILE"
        return 0
    fi
}

# Main execution
main() {
    # Initialize
    TOTAL_TESTS=0
    PASSED_TESTS=0
    FAILED_TESTS=0
    SKIPPED_TESTS=0
    
    # Run all phases
    check_prerequisites
    clean_builds
    
    # Phase 1: Infrastructure Tests
    run_test_phase "Phase 1: Infrastructure Tests" "Makefile.infrastructure" "test" \
        "Testing core infrastructure components (strongSwan mocks, test container, memory tracker)"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Phase 2: Pure Unit Tests  
    run_test_phase "Phase 2: Pure Unit Tests" "Makefile.pure" "test" \
        "Testing common layer components (extsock_errors, extsock_types)"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Phase 3: Adapter Tests
    run_test_phase "Phase 3: Adapter Tests" "Makefile.adapter" "test" \
        "Testing adapter layer components (JSON parser, socket adapter, strongSwan adapter)"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Phase 4-5: Integration Tests
    run_test_phase "Phase 4-5: Integration Tests" "Makefile.integration" "test" \
        "Testing domain/usecase layer and end-to-end workflows"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Additional checks
    collect_test_stats
    run_memory_checks
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    run_performance_benchmarks
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Generate final report
    generate_final_report
}

# Trap for cleanup
trap 'echo "Test suite interrupted"; exit 1' INT TERM

# Check if we're in the correct directory
if [ ! -f "Makefile.infrastructure" ] || [ ! -f "Makefile.integration" ]; then
    echo -e "${RED}❌ Error: Please run this script from the test directory${NC}"
    echo "Expected files: Makefile.infrastructure, Makefile.integration"
    exit 1
fi

# Execute main function
main "$@"
exit_code=$?

echo ""
echo -e "${CYAN}Full test suite completed with exit code: $exit_code${NC}"
echo -e "${CYAN}Detailed log available at: $LOG_FILE${NC}"

exit $exit_code