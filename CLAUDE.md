# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

strongSwan is an OpenSource IPsec-based VPN solution for Linux, FreeBSD, macOS, Android and other platforms. This repository contains the complete strongSwan implementation including:

- Core IPsec daemon (charon)
- Various plugins for different platforms and features
- Configuration tools (swanctl, ipsec)
- Certificate management tools (pki)
- Comprehensive testing framework

## Build System

strongSwan uses GNU Autotools (autoconf/automake) as its primary build system:

### Initial Setup
```bash
# Generate build system files
./autogen.sh

# Configure build (basic)
./configure

# Configure with common development options
./configure --enable-all --disable-defaults --enable-vici --enable-swanctl --enable-systemd --enable-openssl
```

### Building
```bash
# Build everything
make

# Build with parallel jobs
make -j$(nproc)

# Install (usually requires root)
make install
```

### Cleaning
```bash
# Clean build artifacts
make clean

# Full clean including generated files
make distclean
```

## Testing

strongSwan has multiple testing frameworks:

### Unit Tests
```bash
# Run all unit tests
make check

# Run specific component tests
cd src/libstrongswan && make check
cd src/libcharon && make check
cd src/libtls && make check
```

### Integration Testing
```bash
# Full test suite (requires VM setup)
cd testing && ./do-tests

# Individual test scenarios
cd testing && ./do-tests <test-name>
```

### Plugin-Specific Testing
The extsock plugin has its own test suite:
```bash
cd src/libcharon/plugins/extsock/test
make -f Makefile.tests
./run_phase_tests.sh
```

## Code Architecture

### Core Libraries
- **libstrongswan**: Base library with crypto, utils, collections
- **libcharon**: IKE daemon core with SA management, networking
- **libtls**: TLS implementation for EAP-TLS and other TLS users
- **libipsec**: User-space IPsec implementation
- **libimcv**: Integrity Measurement Collection/Verification

### Key Directories
- `src/libcharon/plugins/`: Protocol plugins (EAP methods, kernel interfaces, etc.)
- `src/libstrongswan/plugins/`: Crypto and utility plugins
- `src/charon/`: Main IKE daemon
- `src/swanctl/`: Modern configuration tool
- `src/pki/`: Certificate management utilities
- `src/starter/`: Legacy configuration system
- `testing/`: Comprehensive test framework with VM-based scenarios

### Plugin Architecture
strongSwan uses a modular plugin system. Plugins implement specific features:
- **Kernel plugins**: Interface with OS IPsec stack (kernel-netlink, kernel-pfkey)
- **Crypto plugins**: Cryptographic implementations (openssl, gcrypt, aes)
- **EAP plugins**: Extensible Authentication Protocol methods
- **Attribute plugins**: Configuration attribute handling
- **Socket plugins**: Network communication backends

### Configuration
- Modern: `swanctl.conf` with swanctl tool (recommended)
- Legacy: `ipsec.conf` with ipsec tool (deprecated)
- Daemon config: `strongswan.conf`

## Development Commands

### Code Style
strongSwan follows specific coding conventions documented at https://docs.strongswan.org/docs/latest/devs/devs.html

### Debugging
```bash
# Build with debug symbols
./configure --enable-debug-symbols

# Enable various debug options
./configure --enable-leak-detective --enable-lock-profiler

# Run with increased verbosity
charon --debug-all 2
```

### Static Analysis
```bash
# Use with clang static analyzer
scan-build make

# Memory checking with valgrind
valgrind --tool=memcheck --leak-check=full ./charon
```

## Plugin Development

When working on plugins (especially the extsock plugin):

1. **Location**: `src/libcharon/plugins/<plugin-name>/`
2. **Structure**: Each plugin has `<name>_plugin.c/h` as main entry point
3. **Makefile**: `Makefile.am` defines build rules
4. **Registration**: Plugins register via `plugin_create()` function

### extsock Plugin Specifics
The extsock plugin implements external socket functionality:
- **Main files**: `extsock_plugin.c`, `extsock_plugin.h`
- **Architecture**: Clean architecture with adapters, domain, usecases
- **Testing**: Extensive test suite in `test/` directory
- **Documentation**: `README.md` and `docs/` contain architecture details

## Common Development Workflows

### Adding New Features
1. Implement in appropriate library/plugin
2. Add unit tests in corresponding `tests/` directory
3. Update configuration if new options needed
4. Add integration tests if protocol changes involved

### Testing Changes
1. Run unit tests: `make check`
2. Test specific scenarios in `testing/` framework
3. Verify no regressions with existing tests

### Certificate Testing
```bash
# Generate test certificates
cd testing && ./scripts/build-certs

# Test with generated certs
./scripts/load-testconfig <scenario>
```

## Build Options Reference

Key configure options for development:
- `--enable-all`: Enable most plugins
- `--enable-debug-symbols`: Debug information
- `--enable-leak-detective`: Memory leak detection
- `--enable-vici`: Modern configuration interface
- `--enable-swanctl`: Modern configuration tool
- `--disable-defaults`: Start with minimal feature set
- `--with-systemdsystemunitdir`: SystemD integration

For a full list: `./configure --help`

## extsock Plugin Real Integration Testing (2025-08-26)

The extsock plugin has advanced Google Test integration that includes **Real Plugin Testing** with actual strongSwan library integration.

### Testing Architecture Overview
- **Pure Tests**: Independent unit tests (no strongSwan deps) - ✅ Complete (116 tests)
- **Mock Tests**: Google Mock-based integration tests - ✅ Complete  
- **Real Plugin Tests**: Actual strongSwan + extsock library integration - 🚧 **IN PROGRESS**

### Current Real Plugin Testing Status

**Phase 1: Infrastructure Setup** - ✅ **COMPLETED (2025-08-26)**
- Status: 5/5 테스트 통과, Mock 환경에서 완전 검증
- Result: libstrongswan-extsock.la 라이브러리 탐지 성공
- Performance: 1ms 실행 시간, 100% 성공률

**Phase 2: Real strongSwan Integration** - 🚧 **READY TO START**
- Target: 실제 strongSwan library_init() 및 API 호출 테스트
- Preparation: CMake 빌드 시스템 및 테스트 인프라 완료

**Documents**: 
  - [Design Specification](src/libcharon/plugins/extsock/test/gtest/docs/REAL_PLUGIN_TEST_DESIGN.md)
  - [Implementation Plan](src/libcharon/plugins/extsock/test/gtest/docs/REAL_PLUGIN_IMPLEMENTATION_PLAN.md)
  - [Test Rules & Commands](src/libcharon/plugins/extsock/test/gtest/GTEST_RULES.md) ⭐ **NEW**

**Key Implementation Tasks**:
1. ✅ CMakeLists.txt extension for real plugin library linking
2. ✅ StrongSwanTestEnvironment setup (Mock mode completed)
3. 🚧 Real plugin function testing (Phase 2: strongSwan API integration)
4. 📋 End-to-end integration with strongSwan APIs (Phase 3: planned)

### Quick Commands for Real Plugin Testing

**🔗 완전한 명령어 참조**: [GTEST_RULES.md](src/libcharon/plugins/extsock/test/gtest/GTEST_RULES.md)

```bash
# Navigate to Google Test directory  
cd src/libcharon/plugins/extsock/test/gtest/

# Phase 1: Build infrastructure tests (Mock 환경)
mkdir -p build && cd build
cmake .. -DREAL_PLUGIN_PHASE=1 && make
./real_plugin_tests

# Phase 2: Real strongSwan integration (준비 중)
cmake .. -DREAL_PLUGIN_PHASE=2 && make
./real_plugin_tests

# 모든 테스트 타입 실행
make clean && make && make test
```

### Important Notes for Development
- **Preserve existing Pure/Mock tests**: Never break the 116 working tests
- **Incremental approach**: Real plugin tests are additive, not replacement
- **strongSwan dependencies**: Real tests require proper strongSwan development environment
- **Documentation**: All progress tracked in the referenced design documents above

**Context**: This represents a significant advancement in strongSwan plugin testing methodology, creating a 3-tier testing system that validates both isolated functionality and real-world strongSwan integration.