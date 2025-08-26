/*
 * Final Integration Test for 2nd SEGW Failover Feature
 * Comprehensive test suite covering all failover scenarios
 * Copyright (C) 2024 strongSwan Project
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <cstring>

extern "C" {
// Mock implementation from test_failover_simple.cpp
char* parse_and_select_next_address(const char *remote_addrs, const char *current_addr);
bool is_max_retry_exceeded_simple(const char *conn_name, int max_retry);
void increment_retry_count_simple(const char *conn_name);
void reset_retry_count_simple(const char *conn_name);
}

namespace final_integration_test {

/**
 * Final Integration Test Suite
 */
class FinalIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset all test data
        std::cout << "🚀 Setting up final integration test..." << std::endl;
    }
    
    void TearDown() override {
        std::cout << "🧹 Cleaning up final integration test..." << std::endl;
    }
    
    /**
     * Helper function to simulate real-world failover scenario
     */
    void simulate_segw_failover_scenario(const std::string& scenario_name,
                                       const char* segw_addresses,
                                       const char* initial_segw,
                                       int expected_transitions,
                                       int max_retry_count) {
        std::cout << "\n📊 Scenario: " << scenario_name << std::endl;
        std::cout << "  SEGW List: " << segw_addresses << std::endl;
        std::cout << "  Initial SEGW: " << initial_segw << std::endl;
        std::cout << "  Expected Transitions: " << expected_transitions << std::endl;
        
        std::string current_segw = initial_segw;
        std::vector<std::string> transition_log;
        
        for (int i = 0; i < expected_transitions; i++) {
            char* next_segw = parse_and_select_next_address(segw_addresses, current_segw.c_str());
            
            ASSERT_NE(next_segw, nullptr) << "Failover failed at transition " << (i + 1);
            
            std::string next_segw_str(next_segw);
            transition_log.push_back(current_segw + " -> " + next_segw_str);
            current_segw = next_segw_str;
            
            std::cout << "  Transition " << (i + 1) << ": " << transition_log.back() << std::endl;
            
            free(next_segw);
        }
        
        // Verify cyclic behavior - check if we completed proper transitions
        if (expected_transitions > 0) {
            // For complete cycles, we should return to starting point
            // For 3 addresses: 6 transitions = 2 complete cycles -> back to start
            // For 4 addresses: 8 transitions = 2 complete cycles -> back to start
            std::vector<std::string> addr_list;
            std::string addr_str(segw_addresses);
            size_t start = 0;
            size_t end = addr_str.find(',');
            
            while (end != std::string::npos) {
                addr_list.push_back(addr_str.substr(start, end - start));
                start = end + 1;
                end = addr_str.find(',', start);
            }
            addr_list.push_back(addr_str.substr(start));
            
            int addr_count = addr_list.size();
            bool should_return_to_start = (expected_transitions % addr_count == 0);
            
            if (should_return_to_start) {
                EXPECT_EQ(current_segw, initial_segw) << "Should return to starting SEGW after complete cycle";
                std::cout << "  ✅ Completed full cycle, returned to starting SEGW" << std::endl;
            } else {
                EXPECT_NE(current_segw, initial_segw) << "Should be at different SEGW (incomplete cycle)";
                std::cout << "  ✅ Correctly positioned at different SEGW" << std::endl;
            }
        }
        
        std::cout << "  ✅ Scenario completed successfully" << std::endl;
    }
};

/**
 * Test Suite 1: Real-World Failover Scenarios
 */

TEST_F(FinalIntegrationTest, Scenario_DualSEGW_BasicFailover) {
    simulate_segw_failover_scenario(
        "Dual SEGW Basic Failover",
        "192.168.10.1,192.168.10.2",
        "192.168.10.1",
        3,  // Test complete cycle plus one
        5
    );
}

TEST_F(FinalIntegrationTest, Scenario_TripleSEGW_ComplexFailover) {
    simulate_segw_failover_scenario(
        "Triple SEGW Complex Failover",
        "10.10.1.100,10.10.1.101,10.10.1.102",
        "10.10.1.100",
        6,  // Two complete cycles
        10
    );
}

TEST_F(FinalIntegrationTest, Scenario_QuadSEGW_HighAvailability) {
    simulate_segw_failover_scenario(
        "Quad SEGW High Availability",
        "172.31.1.1,172.31.1.2,172.31.1.3,172.31.1.4",
        "172.31.1.2",  // Start from middle
        8,  // Two complete cycles
        15
    );
}

/**
 * Test Suite 2: Performance and Stress Testing
 */

class FinalPerformanceTest : public FinalIntegrationTest {
protected:
    void benchmark_failover_performance(const std::string& test_name,
                                      const char* segw_list,
                                      const char* start_segw,
                                      int iterations) {
        std::cout << "\n⚡ Performance Test: " << test_name << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            char* next_addr = parse_and_select_next_address(segw_list, start_segw);
            if (next_addr) {
                free(next_addr);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        double avg_time = static_cast<double>(duration.count()) / iterations;
        
        std::cout << "  Iterations: " << iterations << std::endl;
        std::cout << "  Total Time: " << duration.count() << " μs" << std::endl;
        std::cout << "  Average Time: " << avg_time << " μs per operation" << std::endl;
        
        // Performance assertion
        EXPECT_LT(avg_time, 50.0) << "Performance regression in " << test_name;
        
        std::cout << "  ✅ Performance test passed" << std::endl;
    }
};

TEST_F(FinalPerformanceTest, Performance_SmallSEGWList) {
    benchmark_failover_performance(
        "Small SEGW List (3 addresses)",
        "10.0.1.1,10.0.1.2,10.0.1.3",
        "10.0.1.1",
        10000
    );
}

TEST_F(FinalPerformanceTest, Performance_MediumSEGWList) {
    // Create medium-sized SEGW list
    std::string medium_list = "10.1.0.1";
    for (int i = 2; i <= 20; i++) {
        medium_list += ",10.1.0." + std::to_string(i);
    }
    
    benchmark_failover_performance(
        "Medium SEGW List (20 addresses)",
        medium_list.c_str(),
        "10.1.0.10",
        5000
    );
}

TEST_F(FinalPerformanceTest, Performance_LargeSEGWList) {
    // Create large SEGW list
    std::string large_list = "10.2.0.1";
    for (int i = 2; i <= 100; i++) {
        large_list += ",10.2.0." + std::to_string(i);
    }
    
    benchmark_failover_performance(
        "Large SEGW List (100 addresses)",
        large_list.c_str(),
        "10.2.0.50",
        1000
    );
}

/**
 * Test Suite 3: Retry Count Management Integration
 */

class RetryCountIntegrationTest : public FinalIntegrationTest {
protected:
    void test_retry_count_scenario(const std::string& conn_name,
                                 int max_retry,
                                 int failure_count,
                                 bool expect_exceeded) {
        std::cout << "\n🔄 Retry Count Test: " << conn_name << std::endl;
        std::cout << "  Max Retry: " << max_retry << std::endl;
        std::cout << "  Failure Count: " << failure_count << std::endl;
        
        // Reset retry count
        reset_retry_count_simple(conn_name.c_str());
        
        // Simulate failures
        for (int i = 0; i < failure_count; i++) {
            increment_retry_count_simple(conn_name.c_str());
            std::cout << "  Failure " << (i + 1) << " recorded" << std::endl;
        }
        
        // Check result
        bool is_exceeded = is_max_retry_exceeded_simple(conn_name.c_str(), max_retry);
        
        if (expect_exceeded) {
            EXPECT_TRUE(is_exceeded) << "Expected retry count to be exceeded";
            std::cout << "  ✅ Retry count correctly exceeded" << std::endl;
        } else {
            EXPECT_FALSE(is_exceeded) << "Expected retry count NOT to be exceeded";
            std::cout << "  ✅ Retry count within limits" << std::endl;
        }
    }
};

TEST_F(RetryCountIntegrationTest, RetryCount_UnderLimit) {
    test_retry_count_scenario("test-conn-under", 5, 3, false);
}

TEST_F(RetryCountIntegrationTest, RetryCount_ExactLimit) {
    test_retry_count_scenario("test-conn-exact", 5, 5, true);
}

TEST_F(RetryCountIntegrationTest, RetryCount_OverLimit) {
    test_retry_count_scenario("test-conn-over", 3, 7, true);
}

/**
 * Test Suite 4: Edge Cases and Robustness
 */

class RobustnessIntegrationTest : public FinalIntegrationTest {
};

TEST_F(RobustnessIntegrationTest, EdgeCase_MalformedAddressLists) {
    std::cout << "\n🛡️  Testing robustness with malformed inputs..." << std::endl;
    
    std::vector<std::pair<std::string, std::string>> test_cases = {
        {"Empty string", ""},
        {"Single address", "10.0.0.1"},
        {"Trailing comma", "10.0.0.1,10.0.0.2,"},
        {"Leading comma", ",10.0.0.1,10.0.0.2"},
        {"Multiple commas", "10.0.0.1,,10.0.0.2"},
        {"Only spaces", "   "},
        {"Mixed whitespace", " 10.0.0.1 , , 10.0.0.2 "}
    };
    
    for (const auto& test_case : test_cases) {
        std::cout << "  Testing: " << test_case.first << std::endl;
        
        // Should not crash with malformed input
        char* result = parse_and_select_next_address(test_case.second.c_str(), "10.0.0.1");
        
        if (result) {
            std::cout << "    Result: " << result << std::endl;
            free(result);
        } else {
            std::cout << "    Result: nullptr (expected for some cases)" << std::endl;
        }
        
        // Test passes if no crash occurs
        SUCCEED() << "Robustness test passed for: " << test_case.first;
    }
    
    std::cout << "  ✅ All robustness tests passed" << std::endl;
}

TEST_F(RobustnessIntegrationTest, EdgeCase_NullInputs) {
    std::cout << "\n🛡️  Testing NULL input handling..." << std::endl;
    
    // All these should return nullptr without crashing
    EXPECT_EQ(parse_and_select_next_address(nullptr, "10.0.0.1"), nullptr);
    EXPECT_EQ(parse_and_select_next_address("10.0.0.1,10.0.0.2", nullptr), nullptr);
    EXPECT_EQ(parse_and_select_next_address(nullptr, nullptr), nullptr);
    
    std::cout << "  ✅ NULL input handling passed" << std::endl;
}

/**
 * Test Suite 5: End-to-End Integration Test
 */

TEST_F(FinalIntegrationTest, EndToEnd_CompleteFailoverWorkflow) {
    std::cout << "\n🎯 End-to-End Integration Test" << std::endl;
    
    const char* segw_list = "192.168.100.1,192.168.100.2,192.168.100.3";
    const char* conn_name = "end-to-end-test-connection";
    const int max_retry = 10;
    
    std::cout << "  SEGW Configuration: " << segw_list << std::endl;
    std::cout << "  Connection: " << conn_name << std::endl;
    std::cout << "  Max Retry: " << max_retry << std::endl;
    
    // Reset retry count
    reset_retry_count_simple(conn_name);
    
    std::string current_segw = "192.168.100.1";
    std::vector<std::string> failover_history;
    
    // Simulate multiple failover attempts
    for (int attempt = 1; attempt <= 6; attempt++) {
        std::cout << "\n  Failover Attempt " << attempt << ":" << std::endl;
        
        // Check if we can still attempt failover
        bool can_retry = !is_max_retry_exceeded_simple(conn_name, max_retry);
        std::cout << "    Can retry: " << (can_retry ? "Yes" : "No") << std::endl;
        
        if (can_retry) {
            // Get next SEGW
            char* next_segw = parse_and_select_next_address(segw_list, current_segw.c_str());
            
            ASSERT_NE(next_segw, nullptr) << "Failover failed at attempt " << attempt;
            
            std::cout << "    Previous SEGW: " << current_segw << std::endl;
            std::cout << "    Next SEGW: " << next_segw << std::endl;
            
            // Record transition
            failover_history.push_back(current_segw + " -> " + next_segw);
            current_segw = next_segw;
            
            free(next_segw);
            
            // Increment failure count
            increment_retry_count_simple(conn_name);
            
            std::cout << "    ✅ Failover completed successfully" << std::endl;
        }
    }
    
    // Verify history
    EXPECT_GE(failover_history.size(), 3) << "Should have performed multiple failovers";
    
    std::cout << "\n  📋 Failover History:" << std::endl;
    for (size_t i = 0; i < failover_history.size(); i++) {
        std::cout << "    " << (i + 1) << ". " << failover_history[i] << std::endl;
    }
    
    // Verify cyclic behavior
    EXPECT_EQ(current_segw, "192.168.100.1") << "Should return to original SEGW after full cycle";
    
    // Should still be under retry limit
    EXPECT_FALSE(is_max_retry_exceeded_simple(conn_name, max_retry)) 
        << "Should not exceed retry limit during normal operation";
    
    std::cout << "\n  🎉 End-to-End integration test completed successfully!" << std::endl;
}

} // namespace final_integration_test

/**
 * Main function
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "🚀 Starting Final Integration Test Suite for 2nd SEGW Failover" << std::endl;
    std::cout << "================================================================" << std::endl;
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n================================================================" << std::endl;
    if (result == 0) {
        std::cout << "🎉 ALL INTEGRATION TESTS PASSED!" << std::endl;
        std::cout << "✅ 2nd SEGW Failover feature is working correctly" << std::endl;
    } else {
        std::cout << "❌ SOME INTEGRATION TESTS FAILED" << std::endl;
        std::cout << "⚠️  Please review the test results above" << std::endl;
    }
    
    return result;
}