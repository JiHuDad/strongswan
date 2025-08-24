#!/bin/bash
# extsock Plugin Test Runner Script
# TASK-003: Build System Separation
#
# This script provides a convenient interface for running the 3-level test system

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Help function
show_help() {
    echo "extsock Plugin Test Runner"
    echo "========================="
    echo ""
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo ""
    echo "Commands:"
    echo "  build         Build all test levels"
    echo "  test          Run all tests"
    echo "  clean         Clean all build artifacts"
    echo "  status        Show detailed status"
    echo "  help          Show this help message"
    echo ""
    echo "Level-specific commands:"
    echo "  build-pure    Build Level 1 (Pure unit tests)"
    echo "  build-adapter Build Level 2 (Adapter tests with mocks)"  
    echo "  build-integration Build Level 3 (Integration tests)"
    echo "  test-pure     Run Level 1 tests"
    echo "  test-adapter  Run Level 2 tests"
    echo "  test-integration Run Level 3 tests"
    echo ""
    echo "Analysis commands:"
    echo "  memcheck      Run memory checking on all levels"
    echo "  coverage      Run coverage analysis on all levels"
    echo ""
    echo "Examples:"
    echo "  $0 build      # Build all test levels"
    echo "  $0 test       # Run all tests"
    echo "  $0 test-adapter # Run only adapter tests"
    echo "  $0 status     # Show current status"
}

# Main command processing
case "$1" in
    "build")
        echo -e "${BLUE}🔨 Building all test levels...${NC}"
        make -f Makefile.master all
        ;;
    "test")
        echo -e "${BLUE}🧪 Running all tests...${NC}"
        make -f Makefile.master test
        ;;
    "clean")
        echo -e "${YELLOW}🧹 Cleaning all build artifacts...${NC}"
        make -f Makefile.master clean
        ;;
    "status")
        echo -e "${BLUE}📊 Showing test system status...${NC}"
        make -f Makefile.master status
        ;;
    "build-pure")
        echo -e "${BLUE}🔨 Building Pure Unit Tests (Level 1)...${NC}"
        make -f Makefile.master pure
        ;;
    "build-adapter")  
        echo -e "${BLUE}🔨 Building Adapter Unit Tests (Level 2)...${NC}"
        make -f Makefile.master adapter
        ;;
    "build-integration")
        echo -e "${BLUE}🔨 Building Integration Tests (Level 3)...${NC}"
        make -f Makefile.master integration
        ;;
    "test-pure")
        echo -e "${BLUE}🧪 Running Pure Unit Tests (Level 1)...${NC}"
        make -f Makefile.master test-pure
        ;;
    "test-adapter")
        echo -e "${BLUE}🧪 Running Adapter Unit Tests (Level 2)...${NC}"
        make -f Makefile.master test-adapter
        ;;  
    "test-integration")
        echo -e "${BLUE}🧪 Running Integration Tests (Level 3)...${NC}"
        make -f Makefile.master test-integration
        ;;
    "memcheck")
        echo -e "${BLUE}🔍 Running memory checking on all levels...${NC}"
        make -f Makefile.master memcheck
        ;;
    "coverage")
        echo -e "${BLUE}📊 Running coverage analysis on all levels...${NC}"
        make -f Makefile.master coverage
        ;;
    "help"|"-h"|"--help"|"")
        show_help
        ;;
    *)
        echo -e "${RED}❌ Unknown command: $1${NC}"
        echo ""
        show_help
        exit 1
        ;;
esac