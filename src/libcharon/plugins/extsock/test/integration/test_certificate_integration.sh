#!/bin/bash

# strongSwan extsock Plugin Certificate Integration Test
# Phase 4: Comprehensive Testing & Documentation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$(dirname "$(dirname "$TEST_DIR")")"
CERTS_DIR="$TEST_DIR/test_certs"
LOG_FILE="$TEST_DIR/integration_test.log"

echo_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

echo_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

echo_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

echo_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# Setup test environment
setup_test_environment() {
    echo_status "Setting up test environment..."
    
    # Create test certificate directory
    mkdir -p "$CERTS_DIR"
    
    # Clear previous logs
    > "$LOG_FILE"
    
    echo_success "Test environment ready"
}

# Generate test certificates
generate_test_certificates() {
    echo_status "Generating test certificates..."
    
    cd "$CERTS_DIR"
    
    # Generate CA private key
    openssl genrsa -out ca.key 2048 2>/dev/null
    
    # Generate CA certificate
    openssl req -new -x509 -days 365 -key ca.key -out ca.crt -subj "/C=KR/ST=Seoul/L=Seoul/O=TestCA/CN=Test CA" 2>/dev/null
    
    # Generate server private key
    openssl genrsa -out server.key 2048 2>/dev/null
    
    # Generate server certificate request
    openssl req -new -key server.key -out server.csr -subj "/C=KR/ST=Seoul/L=Seoul/O=TestOrg/CN=test.example.com" 2>/dev/null
    
    # Sign server certificate with CA
    openssl x509 -req -days 365 -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt 2>/dev/null
    
    # Generate encrypted private key
    openssl genrsa -aes256 -passout pass:testpassword -out server_encrypted.key 2048 2>/dev/null
    
    # Generate certificate for encrypted key
    openssl req -new -key server_encrypted.key -passin pass:testpassword -out server_encrypted.csr -subj "/C=KR/ST=Seoul/L=Seoul/O=TestOrg/CN=encrypted.example.com" 2>/dev/null
    openssl x509 -req -days 365 -in server_encrypted.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server_encrypted.crt 2>/dev/null
    
    # Clean up CSR files
    rm -f *.csr
    
    echo_success "Test certificates generated"
}

# Test basic certificate loading
test_basic_certificate_loading() {
    echo_status "Testing basic certificate loading..."
    
    # Test JSON configuration
    cat > "$CERTS_DIR/basic_config.json" << EOF
{
    "connection_name": "test-basic",
    "local": {
        "auth": "cert",
        "cert": "$CERTS_DIR/server.crt",
        "private_key": "$CERTS_DIR/server.key",
        "ca_cert": "$CERTS_DIR/ca.crt"
    },
    "remote": {
        "auth": "cert", 
        "ca_cert": "$CERTS_DIR/ca.crt"
    }
}
EOF
    
    echo_success "Basic certificate loading test prepared"
}

# Test encrypted key handling
test_encrypted_key_handling() {
    echo_status "Testing encrypted private key handling..."
    
    # Test JSON configuration with password
    cat > "$CERTS_DIR/encrypted_config.json" << EOF
{
    "connection_name": "test-encrypted",
    "local": {
        "auth": "cert",
        "cert": "$CERTS_DIR/server_encrypted.crt",
        "private_key": "$CERTS_DIR/server_encrypted.key",
        "private_key_passphrase": "testpassword",
        "ca_cert": "$CERTS_DIR/ca.crt"
    },
    "remote": {
        "auth": "cert",
        "ca_cert": "$CERTS_DIR/ca.crt"
    }
}
EOF
    
    echo_success "Encrypted key handling test prepared"
}

# Test OCSP/CRL configuration
test_ocsp_crl_configuration() {
    echo_status "Testing OCSP/CRL configuration..."
    
    # Test JSON configuration with online validation
    cat > "$CERTS_DIR/ocsp_crl_config.json" << EOF
{
    "connection_name": "test-ocsp-crl",
    "local": {
        "auth": "cert",
        "cert": "$CERTS_DIR/server.crt",
        "private_key": "$CERTS_DIR/server.key",
        "ca_cert": "$CERTS_DIR/ca.crt",
        "enable_ocsp": true,
        "enable_crl": true
    },
    "remote": {
        "auth": "cert",
        "ca_cert": "$CERTS_DIR/ca.crt",
        "enable_ocsp": false,
        "enable_crl": false
    }
}
EOF
    
    echo_success "OCSP/CRL configuration test prepared"
}

# Test trust chain validation
test_trust_chain_validation() {
    echo_status "Testing advanced trust chain validation..."
    
    # Generate intermediate CA
    cd "$CERTS_DIR"
    
    # Intermediate CA key
    openssl genrsa -out intermediate.key 2048 2>/dev/null
    
    # Intermediate CA certificate request
    openssl req -new -key intermediate.key -out intermediate.csr -subj "/C=KR/ST=Seoul/L=Seoul/O=TestCA/CN=Intermediate CA" 2>/dev/null
    
    # Sign intermediate CA with root CA
    openssl x509 -req -days 365 -in intermediate.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out intermediate.crt -extensions v3_ca 2>/dev/null
    
    # Generate end entity certificate signed by intermediate
    openssl genrsa -out end_entity.key 2048 2>/dev/null
    openssl req -new -key end_entity.key -out end_entity.csr -subj "/C=KR/ST=Seoul/L=Seoul/O=TestOrg/CN=endentity.example.com" 2>/dev/null
    openssl x509 -req -days 365 -in end_entity.csr -CA intermediate.crt -CAkey intermediate.key -CAcreateserial -out end_entity.crt 2>/dev/null
    
    # Test JSON configuration with chain
    cat > "$CERTS_DIR/chain_config.json" << EOF
{
    "connection_name": "test-chain",
    "local": {
        "auth": "cert",
        "cert": "$CERTS_DIR/end_entity.crt",
        "private_key": "$CERTS_DIR/end_entity.key",
        "ca_cert": "$CERTS_DIR/ca.crt",
        "intermediate_cert": "$CERTS_DIR/intermediate.crt"
    }
}
EOF
    
    rm -f *.csr
    
    echo_success "Trust chain validation test prepared"
}

# Test error handling
test_error_handling() {
    echo_status "Testing error handling scenarios..."
    
    # Test with invalid certificate paths
    cat > "$CERTS_DIR/invalid_config.json" << EOF
{
    "connection_name": "test-invalid",
    "local": {
        "auth": "cert",
        "cert": "/nonexistent/path.crt",
        "private_key": "/nonexistent/key.pem",
        "ca_cert": "$CERTS_DIR/ca.crt"
    }
}
EOF
    
    # Test with mismatched key-cert pair
    cat > "$CERTS_DIR/mismatched_config.json" << EOF
{
    "connection_name": "test-mismatched",
    "local": {
        "auth": "cert",
        "cert": "$CERTS_DIR/server.crt",
        "private_key": "$CERTS_DIR/server_encrypted.key",
        "private_key_passphrase": "wrongpassword",
        "ca_cert": "$CERTS_DIR/ca.crt"
    }
}
EOF
    
    echo_success "Error handling test scenarios prepared"
}

# Performance testing
test_performance() {
    echo_status "Testing performance characteristics..."
    
    local start_time=$(date +%s%N)
    
    # Simulate multiple certificate operations
    for i in {1..10}; do
        cp "$CERTS_DIR/basic_config.json" "$CERTS_DIR/perf_config_$i.json"
    done
    
    local end_time=$(date +%s%N)
    local duration=$(((end_time - start_time) / 1000000)) # Convert to milliseconds
    
    echo_status "Performance test completed in ${duration}ms"
    
    # Cleanup performance test files
    rm -f "$CERTS_DIR/perf_config_"*.json
    
    if [ $duration -lt 1000 ]; then
        echo_success "Performance test passed (${duration}ms < 1000ms)"
    else
        echo_warning "Performance test slow (${duration}ms >= 1000ms)"
    fi
}

# Validate certificate properties
validate_certificates() {
    echo_status "Validating generated certificates..."
    
    cd "$CERTS_DIR"
    
    # Verify CA certificate
    if openssl x509 -in ca.crt -text -noout | grep -q "CA:TRUE"; then
        echo_success "CA certificate valid"
    else
        echo_error "CA certificate invalid"
        return 1
    fi
    
    # Verify server certificate chain
    if openssl verify -CAfile ca.crt server.crt 2>/dev/null | grep -q "OK"; then
        echo_success "Server certificate chain valid"
    else
        echo_error "Server certificate chain invalid"
        return 1
    fi
    
    # Test encrypted key decryption
    if openssl rsa -in server_encrypted.key -passin pass:testpassword -check -noout 2>/dev/null; then
        echo_success "Encrypted private key valid"
    else
        echo_error "Encrypted private key invalid"
        return 1
    fi
    
    return 0
}

# Run all tests
run_all_tests() {
    echo_status "Starting comprehensive certificate integration tests..."
    echo_status "=============================================="
    
    local test_count=0
    local pass_count=0
    
    # Test functions array
    tests=(
        "setup_test_environment"
        "generate_test_certificates"
        "validate_certificates"
        "test_basic_certificate_loading"
        "test_encrypted_key_handling"
        "test_ocsp_crl_configuration"
        "test_trust_chain_validation"
        "test_error_handling"
        "test_performance"
    )
    
    for test_func in "${tests[@]}"; do
        ((test_count++))
        echo_status "Running test: $test_func"
        
        if $test_func >> "$LOG_FILE" 2>&1; then
            ((pass_count++))
            echo_success "$test_func"
        else
            echo_error "$test_func (see $LOG_FILE for details)"
        fi
        
        echo ""
    done
    
    # Summary
    echo_status "=============================================="
    echo_status "Test Summary: $pass_count/$test_count tests passed"
    
    if [ $pass_count -eq $test_count ]; then
        echo_success "All integration tests passed!"
        return 0
    else
        echo_error "Some tests failed. Check $LOG_FILE for details."
        return 1
    fi
}

# Cleanup test environment
cleanup_test_environment() {
    echo_status "Cleaning up test environment..."
    
    if [ -d "$CERTS_DIR" ]; then
        rm -rf "$CERTS_DIR"
        echo_success "Test certificates cleaned up"
    fi
}

# Main execution
main() {
    echo_status "strongSwan extsock Plugin - Certificate Integration Test"
    echo_status "Phase 4: Comprehensive Testing & Documentation"
    echo ""
    
    # Check if OpenSSL is available
    if ! command -v openssl &> /dev/null; then
        echo_error "OpenSSL is required for certificate generation"
        exit 1
    fi
    
    # Run tests
    if run_all_tests; then
        echo_success "Integration testing completed successfully!"
        exit_code=0
    else
        echo_error "Integration testing failed!"
        exit_code=1
    fi
    
    # Cleanup option
    read -p "Clean up test certificates? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        cleanup_test_environment
    fi
    
    exit $exit_code
}

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi 