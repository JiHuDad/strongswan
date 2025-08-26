/*
 * Test functions only - extracted from test_failover_simple.cpp
 */

#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>

extern "C" {

// Mock implementation of address parsing logic
char* parse_and_select_next_address(const char *remote_addrs, const char *current_addr) {
    if (!remote_addrs || !current_addr) {
        return nullptr;
    }
    
    // Simple comma-separated parsing
    std::string addr_str(remote_addrs);
    std::vector<std::string> addresses;
    
    size_t start = 0;
    size_t end = addr_str.find(',');
    
    while (end != std::string::npos) {
        std::string addr = addr_str.substr(start, end - start);
        // Trim whitespace
        size_t first = addr.find_first_not_of(' ');
        size_t last = addr.find_last_not_of(' ');
        if (first != std::string::npos) {
            addr = addr.substr(first, (last - first + 1));
            if (!addr.empty()) {
                addresses.push_back(addr);
            }
        }
        start = end + 1;
        end = addr_str.find(',', start);
    }
    
    // Last address
    std::string last_addr = addr_str.substr(start);
    size_t first = last_addr.find_first_not_of(' ');
    size_t last = last_addr.find_last_not_of(' ');
    if (first != std::string::npos) {
        last_addr = last_addr.substr(first, (last - first + 1));
        if (!last_addr.empty()) {
            addresses.push_back(last_addr);
        }
    }
    
    if (addresses.size() < 2) {
        return nullptr; // No failover possible
    }
    
    // Find current address and return next
    for (size_t i = 0; i < addresses.size(); i++) {
        if (addresses[i] == current_addr) {
            size_t next_index = (i + 1) % addresses.size();
            return strdup(addresses[next_index].c_str());
        }
    }
    
    // Current address not found, return first address
    return strdup(addresses[0].c_str());
}

// Mock retry count management
static int retry_counts[100] = {0}; // Simple array for testing

bool is_max_retry_exceeded_simple(const char *conn_name, int max_retry) {
    // Simple hash: use connection name length as index
    int index = strlen(conn_name) % 100;
    return retry_counts[index] >= max_retry;
}

void increment_retry_count_simple(const char *conn_name) {
    int index = strlen(conn_name) % 100;
    retry_counts[index]++;
}

void reset_retry_count_simple(const char *conn_name) {
    int index = strlen(conn_name) % 100;
    retry_counts[index] = 0;
}

} // extern "C"