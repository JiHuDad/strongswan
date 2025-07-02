#include "../include/test_utils.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstdarg>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

// MemoryTracker implementation
MemoryTracker& MemoryTracker::getInstance() {
    static MemoryTracker instance;
    return instance;
}

void MemoryTracker::recordAllocation(void* ptr, size_t size, const std::string& location) {
    allocations_[ptr] = {size, location};
    totalAllocated_ += size;
    allocationCount_++;
}

void MemoryTracker::recordDeallocation(void* ptr) {
    auto it = allocations_.find(ptr);
    if (it != allocations_.end()) {
        totalAllocated_ -= it->second.size;
        allocations_.erase(it);
    }
}

size_t MemoryTracker::getAllocatedBytes() const {
    return totalAllocated_;
}

size_t MemoryTracker::getAllocationCount() const {
    return allocationCount_;
}

void MemoryTracker::reset() {
    allocations_.clear();
    totalAllocated_ = 0;
    allocationCount_ = 0;
}

bool MemoryTracker::hasLeaks() const {
    return !allocations_.empty();
}

std::vector<std::string> MemoryTracker::getLeakReport() const {
    std::vector<std::string> report;
    for (const auto& [ptr, info] : allocations_) {
        std::ostringstream oss;
        oss << "Leaked " << info.size << " bytes at " << ptr << " from " << info.location;
        report.push_back(oss.str());
    }
    return report;
}

// StringUtils implementation
std::string StringUtils::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> StringUtils::split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

std::string StringUtils::join(const std::vector<std::string>& parts, const std::string& delimiter) {
    if (parts.empty()) return "";
    std::ostringstream oss;
    oss << parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        oss << delimiter << parts[i];
    }
    return oss.str();
}

bool StringUtils::startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

bool StringUtils::endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

std::string StringUtils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string StringUtils::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// JsonTestHelper implementation
std::string JsonTestHelper::createTestConfig(const std::string& id, const std::string& type) {
    return R"({"id": ")" + id + R"(", "type": ")" + type + R"(", "enabled": true})";
}

std::string JsonTestHelper::createInvalidJson() {
    return "{ invalid json structure without closing brace";
}

std::string JsonTestHelper::createMinimalConfig() {
    return R"({"name": "minimal", "version": "1.0"})";
}

std::string JsonTestHelper::createComplexConfig() {
    return R"({
        "name": "complex_config",
        "version": "2.0",
        "settings": {
            "timeout": 30,
            "retries": 3,
            "endpoints": ["192.168.1.1", "192.168.1.2"]
        },
        "features": ["auth", "encryption"]
    })";
}

bool JsonTestHelper::isValidJson(const std::string& json) {
    // Simple JSON validation - check for balanced braces
    int brace_count = 0;
    bool in_string = false;
    bool escaped = false;
    
    for (char c : json) {
        if (escaped) {
            escaped = false;
            continue;
        }
        
        if (c == '\\') {
            escaped = true;
            continue;
        }
        
        if (c == '"') {
            in_string = !in_string;
            continue;
        }
        
        if (!in_string) {
            if (c == '{') brace_count++;
            else if (c == '}') brace_count--;
        }
    }
    
    return brace_count == 0 && !in_string;
}

// FileSystemHelper implementation
bool FileSystemHelper::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool FileSystemHelper::createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

bool FileSystemHelper::removeFile(const std::string& path) {
    return unlink(path.c_str()) == 0;
}

std::string FileSystemHelper::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool FileSystemHelper::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    file << content;
    return file.good();
}

std::string FileSystemHelper::getTempDirectory() {
    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = getenv("TMP");
    if (!tmpdir) tmpdir = getenv("TEMP");
    if (!tmpdir) tmpdir = "/tmp";
    return tmpdir;
}

std::string FileSystemHelper::createTempFile(const std::string& content) {
    std::string temp_path = getTempDirectory() + "/test_XXXXXX";
    std::vector<char> path_buf(temp_path.begin(), temp_path.end());
    path_buf.push_back('\0');
    
    int fd = mkstemp(path_buf.data());
    if (fd == -1) return "";
    
    std::string path(path_buf.data());
    close(fd);
    
    if (!content.empty()) {
        writeFile(path, content);
    }
    
    return path;
}

// TimeHelper implementation
void TimeHelper::start() {
    startTime_ = std::chrono::high_resolution_clock::now();
}

double TimeHelper::elapsed() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime_);
    return duration.count() / 1000.0; // Return milliseconds
}

void TimeHelper::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// TestDataFactory implementation
std::vector<std::string> TestDataFactory::createTestConfigs(size_t count) {
    std::vector<std::string> configs;
    for (size_t i = 0; i < count; ++i) {
        configs.push_back(JsonTestHelper::createTestConfig(
            "config_" + std::to_string(i), 
            "test_type_" + std::to_string(i % 3)
        ));
    }
    return configs;
}

std::vector<extsock_error_t> TestDataFactory::createErrorCodes() {
    return {
        EXTSOCK_SUCCESS,
        EXTSOCK_ERROR_JSON_PARSE,
        EXTSOCK_ERROR_CONFIG_INVALID,
        EXTSOCK_ERROR_SOCKET_FAILED,
        EXTSOCK_ERROR_MEMORY_ALLOCATION,
        EXTSOCK_ERROR_STRONGSWAN_API
    };
}

std::vector<extsock_event_type_t> TestDataFactory::createEventTypes() {
    return {
        EXTSOCK_EVENT_TUNNEL_UP,
        EXTSOCK_EVENT_TUNNEL_DOWN,
        EXTSOCK_EVENT_CONFIG_APPLIED,
        EXTSOCK_EVENT_ERROR
    };
}

std::vector<extsock_command_type_t> TestDataFactory::createCommandTypes() {
    return {
        EXTSOCK_CMD_APPLY_CONFIG,
        EXTSOCK_CMD_START_DPD,
        EXTSOCK_CMD_REMOVE_CONFIG
    };
} 