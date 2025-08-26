/*
 * Test utilities for Google Test
 */

#include <string>
#include <iostream>

namespace test_utils {

void print_test_header(const std::string& test_name) {
    std::cout << "=== " << test_name << " ===" << std::endl;
}

void print_test_result(const std::string& test_name, bool passed) {
    if (passed) {
        std::cout << "✅ " << test_name << " PASSED" << std::endl;
    } else {
        std::cout << "❌ " << test_name << " FAILED" << std::endl;
    }
}

} // namespace test_utils
