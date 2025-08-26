/**
 * strongSwan Mock API Implementation
 * 
 * Copyright (C) 2025 strongSwan Project
 * Phase 2 Implementation - Mock strongSwan API for testing
 */

#include "strongswan_mock_api.hpp"
#include "real_plugin_macros.hpp"
#include <cstring>
#include <cstdlib>
#include <cstdint>

// ============================================================================
// Global Mock State
// ============================================================================

static strongswan_library_t g_library_state = {false, "", "", 0};
static strongswan_hydra_t g_hydra_state = {false, "", 0};
static strongswan_plugin_t g_plugins[32]; // Max 32 plugins
static int g_plugin_count = 0;

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

int strongswan_library_init_mock(const char* config_file, const char* daemon_name) {
    REAL_PLUGIN_INFO("strongSwan Library Init Mock");
    
    if (g_library_state.initialized) {
        REAL_PLUGIN_WARNING("Library already initialized, incrementing count");
        g_library_state.init_count++;
        return 1; // TRUE
    }
    
    g_library_state.initialized = true;
    g_library_state.config_file = config_file ? config_file : "";
    g_library_state.daemon_name = daemon_name ? daemon_name : "mock-daemon";
    g_library_state.init_count = 1;
    
    REAL_PLUGIN_SUCCESS("Library initialized: " + g_library_state.daemon_name);
    return 1; // TRUE
}

int strongswan_hydra_init_mock(const char* daemon_name) {
    REAL_PLUGIN_INFO("strongSwan Hydra Init Mock");
    
    if (!g_library_state.initialized) {
        REAL_PLUGIN_ERROR("Library not initialized, cannot initialize Hydra");
        return 0; // FALSE
    }
    
    if (g_hydra_state.initialized) {
        REAL_PLUGIN_WARNING("Hydra already initialized");
        return 1; // TRUE
    }
    
    g_hydra_state.initialized = true;
    g_hydra_state.daemon_name = daemon_name ? daemon_name : "mock-hydra";
    g_hydra_state.plugin_count = 0;
    
    REAL_PLUGIN_SUCCESS("Hydra initialized: " + g_hydra_state.daemon_name);
    return 1; // TRUE
}

int strongswan_load_plugin_mock(const char* plugin_name) {
    REAL_PLUGIN_INFO("Loading plugin mock: " + std::string(plugin_name ? plugin_name : "NULL"));
    
    if (!g_hydra_state.initialized) {
        REAL_PLUGIN_ERROR("Hydra not initialized, cannot load plugins");
        return 0; // FALSE
    }
    
    if (!plugin_name) {
        REAL_PLUGIN_ERROR("Plugin name is NULL");
        return 0; // FALSE
    }
    
    if (g_plugin_count >= 32) {
        REAL_PLUGIN_ERROR("Maximum plugin count reached (32)");
        return 0; // FALSE
    }
    
    // Check if plugin already loaded
    for (int i = 0; i < g_plugin_count; i++) {
        if (strcmp(g_plugins[i].name, plugin_name) == 0) {
            REAL_PLUGIN_WARNING("Plugin already loaded: " + std::string(plugin_name));
            return 1; // TRUE
        }
    }
    
    // Add new plugin
    strncpy(g_plugins[g_plugin_count].name, plugin_name, 63);
    g_plugins[g_plugin_count].name[63] = '\0';
    g_plugins[g_plugin_count].loaded = true;
    g_plugins[g_plugin_count].handle = (void*)(intptr_t)(0x1000 + g_plugin_count); // Mock handle
    
    g_plugin_count++;
    g_hydra_state.plugin_count = g_plugin_count;
    
    REAL_PLUGIN_SUCCESS("Plugin loaded successfully: " + std::string(plugin_name));
    return 1; // TRUE
}

void strongswan_library_deinit_mock(void) {
    REAL_PLUGIN_INFO("strongSwan Library Deinit Mock");
    
    if (!g_library_state.initialized) {
        REAL_PLUGIN_WARNING("Library not initialized, nothing to deinitialize");
        return;
    }
    
    g_library_state.initialized = false;
    g_library_state.config_file = "";
    g_library_state.daemon_name = "";
    g_library_state.init_count = 0;
    
    REAL_PLUGIN_SUCCESS("Library deinitialized");
}

void strongswan_hydra_deinit_mock(void) {
    REAL_PLUGIN_INFO("strongSwan Hydra Deinit Mock");
    
    if (!g_hydra_state.initialized) {
        REAL_PLUGIN_WARNING("Hydra not initialized, nothing to deinitialize");
        return;
    }
    
    // Unload all plugins
    for (int i = 0; i < g_plugin_count; i++) {
        g_plugins[i].loaded = false;
        g_plugins[i].handle = nullptr;
        memset(g_plugins[i].name, 0, 64);
    }
    g_plugin_count = 0;
    
    g_hydra_state.initialized = false;
    g_hydra_state.daemon_name = "";
    g_hydra_state.plugin_count = 0;
    
    REAL_PLUGIN_SUCCESS("Hydra deinitialized");
}

strongswan_library_t* strongswan_get_library_status_mock(void) {
    return &g_library_state;
}

strongswan_hydra_t* strongswan_get_hydra_status_mock(void) {
    return &g_hydra_state;
}

int strongswan_get_loaded_plugins_mock(strongswan_plugin_t* plugins, int max_plugins) {
    if (!plugins || max_plugins <= 0) {
        return 0;
    }
    
    int count = (g_plugin_count < max_plugins) ? g_plugin_count : max_plugins;
    for (int i = 0; i < count; i++) {
        plugins[i] = g_plugins[i];
    }
    
    return count;
}

} // extern "C"

// ============================================================================
// C++ Wrapper Implementation
// ============================================================================

namespace strongswan_mock {

// LibraryManager Implementation
LibraryManager::LibraryManager() : initialized_(false), init_count_(0) {}

LibraryManager::~LibraryManager() {
    if (initialized_) {
        Cleanup();
    }
}

bool LibraryManager::Initialize(const std::string& config_file, const std::string& daemon_name) {
    REAL_PLUGIN_DEBUG("LibraryManager::Initialize(" + daemon_name + ")");
    
    config_file_ = config_file;
    daemon_name_ = daemon_name;
    
    int result = strongswan_library_init_mock(
        config_file.empty() ? nullptr : config_file.c_str(),
        daemon_name.c_str()
    );
    
    initialized_ = (result == 1);
    if (initialized_) {
        init_count_++;
    }
    
    return initialized_;
}

void LibraryManager::Cleanup() {
    REAL_PLUGIN_DEBUG("LibraryManager::Cleanup()");
    
    if (initialized_) {
        strongswan_library_deinit_mock();
        initialized_ = false;
        init_count_ = 0;
    }
}

void LibraryManager::Reset() {
    REAL_PLUGIN_DEBUG("LibraryManager::Reset()");
    Cleanup();
    config_file_.clear();
    daemon_name_.clear();
}

// HydraManager Implementation
HydraManager::HydraManager() : initialized_(false) {}

HydraManager::~HydraManager() {
    if (initialized_) {
        Cleanup();
    }
}

bool HydraManager::Initialize(const std::string& daemon_name) {
    REAL_PLUGIN_DEBUG("HydraManager::Initialize(" + daemon_name + ")");
    
    daemon_name_ = daemon_name;
    
    int result = strongswan_hydra_init_mock(daemon_name.c_str());
    initialized_ = (result == 1);
    
    if (initialized_) {
        loaded_plugins_.clear();
    }
    
    return initialized_;
}

void HydraManager::Cleanup() {
    REAL_PLUGIN_DEBUG("HydraManager::Cleanup()");
    
    if (initialized_) {
        strongswan_hydra_deinit_mock();
        initialized_ = false;
        loaded_plugins_.clear();
    }
}

bool HydraManager::LoadPlugin(const std::string& plugin_name) {
    REAL_PLUGIN_DEBUG("HydraManager::LoadPlugin(" + plugin_name + ")");
    
    if (!initialized_) {
        REAL_PLUGIN_ERROR("HydraManager not initialized");
        return false;
    }
    
    int result = strongswan_load_plugin_mock(plugin_name.c_str());
    if (result == 1) {
        // Add to our tracking list if not already present
        auto it = std::find(loaded_plugins_.begin(), loaded_plugins_.end(), plugin_name);
        if (it == loaded_plugins_.end()) {
            loaded_plugins_.push_back(plugin_name);
        }
        return true;
    }
    
    return false;
}

void HydraManager::Reset() {
    REAL_PLUGIN_DEBUG("HydraManager::Reset()");
    Cleanup();
    daemon_name_.clear();
}

// MockEnvironment Implementation
MockEnvironment::MockEnvironment() : fully_initialized_(false) {
    library_manager_ = std::make_unique<LibraryManager>();
    hydra_manager_ = std::make_unique<HydraManager>();
}

MockEnvironment::~MockEnvironment() {
    if (fully_initialized_) {
        CleanupFull();
    }
}

bool MockEnvironment::InitializeFull(const std::string& daemon_name) {
    REAL_PLUGIN_INFO("MockEnvironment::InitializeFull(" + daemon_name + ")");
    
    if (fully_initialized_) {
        REAL_PLUGIN_WARNING("Mock environment already initialized");
        return true;
    }
    
    // Step 1: Initialize library
    if (!library_manager_->Initialize("", daemon_name)) {
        REAL_PLUGIN_ERROR("Failed to initialize library manager");
        return false;
    }
    
    // Step 2: Initialize hydra
    if (!hydra_manager_->Initialize(daemon_name)) {
        REAL_PLUGIN_ERROR("Failed to initialize hydra manager");
        library_manager_->Cleanup();
        return false;
    }
    
    // Step 3: Load basic plugins
    std::vector<std::string> basic_plugins = {
        "random", "nonce", "x509", "pubkey", "pkcs1", 
        "pem", "openssl", "extsock"
    };
    
    int loaded_count = 0;
    for (const auto& plugin : basic_plugins) {
        if (hydra_manager_->LoadPlugin(plugin)) {
            loaded_count++;
        }
    }
    
    REAL_PLUGIN_INFO("Loaded " + std::to_string(loaded_count) + "/" + 
                     std::to_string(basic_plugins.size()) + " plugins");
    
    fully_initialized_ = true;
    REAL_PLUGIN_SUCCESS("Mock environment fully initialized");
    
    return true;
}

void MockEnvironment::CleanupFull() {
    REAL_PLUGIN_INFO("MockEnvironment::CleanupFull()");
    
    if (fully_initialized_) {
        hydra_manager_->Cleanup();
        library_manager_->Cleanup();
        fully_initialized_ = false;
        
        REAL_PLUGIN_SUCCESS("Mock environment cleaned up");
    }
}

void MockEnvironment::ResetAll() {
    REAL_PLUGIN_INFO("MockEnvironment::ResetAll()");
    
    CleanupFull();
    hydra_manager_->Reset();
    library_manager_->Reset();
}

MockEnvironment& MockEnvironment::Instance() {
    static MockEnvironment instance;
    return instance;
}

} // namespace strongswan_mock