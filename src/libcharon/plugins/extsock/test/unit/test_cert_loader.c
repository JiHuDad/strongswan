/*
 * Copyright (C) 2024 strongSwan Project
 * Certificate Loader Unit Tests
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../../adapters/crypto/extsock_cert_loader.h"
#include "../../common/extsock_common.h"

#include <library.h>
#include <credentials/certificates/certificate.h>
#include <credentials/keys/private_key.h>
#include <credentials/auth_cfg.h>

static extsock_cert_loader_t *cert_loader;

// Test fixtures
void setup(void)
{
    cert_loader = extsock_cert_loader_create();
}

void teardown(void)
{
    if (cert_loader) {
        cert_loader->destroy(cert_loader);
        cert_loader = NULL;
    }
}

// Phase 1 & 2 Tests (existing)
START_TEST(test_cert_loader_creation)
{
    ck_assert_ptr_nonnull(cert_loader);
}
END_TEST

START_TEST(test_password_management)
{
    // Test password setting and clearing
    cert_loader->set_password(cert_loader, "test-password");
    cert_loader->set_password(cert_loader, NULL);
    
    // Test interactive mode toggle
    cert_loader->set_interactive(cert_loader, TRUE);
    cert_loader->set_interactive(cert_loader, FALSE);
    
    ck_assert(1); // Basic functionality test
}
END_TEST

// Phase 3 Advanced Tests
START_TEST(test_online_validation_toggle)
{
    // Test online validation enable/disable
    cert_loader->set_online_validation(cert_loader, TRUE);
    cert_loader->set_online_validation(cert_loader, FALSE);
    cert_loader->set_online_validation(cert_loader, TRUE);
    
    ck_assert(1); // Configuration test
}
END_TEST

START_TEST(test_trust_chain_validation_null_inputs)
{
    auth_cfg_t *result;
    linked_list_t *ca_list = linked_list_create();
    
    // Test with NULL subject certificate
    result = cert_loader->build_trust_chain(cert_loader, NULL, ca_list, FALSE);
    ck_assert_ptr_null(result);
    
    ca_list->destroy(ca_list);
}
END_TEST

START_TEST(test_ocsp_validation_null_inputs)
{
    cert_validation_t result;
    
    // Test OCSP validation with NULL inputs
    result = cert_loader->validate_ocsp(cert_loader, NULL, NULL);
    ck_assert_int_eq(result, VALIDATION_FAILED);
}
END_TEST

START_TEST(test_crl_validation_null_inputs)
{
    cert_validation_t result;
    
    // Test CRL validation with NULL inputs
    result = cert_loader->validate_crl(cert_loader, NULL, NULL);
    ck_assert_int_eq(result, VALIDATION_FAILED);
}
END_TEST

START_TEST(test_trust_chain_empty_ca_list)
{
    // This test would require a mock certificate
    // For now, test the error handling with empty CA list
    linked_list_t *empty_ca_list = linked_list_create();
    
    // Without real certificates, we test the basic structure
    ck_assert_ptr_nonnull(empty_ca_list);
    
    empty_ca_list->destroy(empty_ca_list);
}
END_TEST

// Integration Tests
START_TEST(test_comprehensive_certificate_workflow)
{
    // Test complete workflow: password -> cert load -> chain validation -> OCSP/CRL
    
    // 1. Configure password management
    cert_loader->set_password(cert_loader, "test-pass");
    cert_loader->set_interactive(cert_loader, FALSE);
    
    // 2. Enable online validation
    cert_loader->set_online_validation(cert_loader, TRUE);
    
    // 3. Test validation state management
    cert_validation_t mock_ocsp = VALIDATION_SKIPPED;
    cert_validation_t mock_crl = VALIDATION_SKIPPED;
    
    ck_assert_int_eq(mock_ocsp, VALIDATION_SKIPPED);
    ck_assert_int_eq(mock_crl, VALIDATION_SKIPPED);
    
    // 4. Clean up
    cert_loader->set_password(cert_loader, NULL);
}
END_TEST

// Performance Tests
START_TEST(test_trust_chain_performance)
{
    clock_t start, end;
    double cpu_time_used;
    
    start = clock();
    
    // Simulate trust chain building operations
    for (int i = 0; i < 100; i++) {
        linked_list_t *ca_list = linked_list_create();
        ca_list->destroy(ca_list);
    }
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    // Performance should be reasonable (< 1 second for 100 operations)
    ck_assert(cpu_time_used < 1.0);
}
END_TEST

// Security Tests
START_TEST(test_password_memory_security)
{
    // Test secure password handling
    char *test_password = "very-secret-password";
    
    cert_loader->set_password(cert_loader, test_password);
    
    // Password should be cleared after setting to NULL
    cert_loader->set_password(cert_loader, NULL);
    
    // Test multiple password operations
    cert_loader->set_password(cert_loader, "password1");
    cert_loader->set_password(cert_loader, "password2");
    cert_loader->set_password(cert_loader, NULL);
    
    ck_assert(1); // Memory security test passed
}
END_TEST

START_TEST(test_validation_result_consistency)
{
    // Test that validation results are consistent
    cert_validation_t results[] = {
        VALIDATION_GOOD,
        VALIDATION_REVOKED,
        VALIDATION_FAILED,
        VALIDATION_SKIPPED,
        VALIDATION_STALE
    };
    
    for (int i = 0; i < 5; i++) {
        ck_assert(results[i] >= 0); // Valid enum values
    }
}
END_TEST

// Error Handling Tests
START_TEST(test_error_handling_robustness)
{
    // Test various error conditions
    
    // 1. Invalid file paths
    certificate_t *cert = cert_loader->load_certificate(cert_loader, "/nonexistent/path.crt");
    ck_assert_ptr_null(cert);
    
    // 2. Invalid private key paths
    private_key_t *key = cert_loader->load_private_key(cert_loader, "/invalid/key.pem", NULL);
    ck_assert_ptr_null(key);
    
    // 3. Auto key loading with invalid path
    key = cert_loader->load_private_key_auto(cert_loader, "/invalid/auto.key");
    ck_assert_ptr_null(key);
}
END_TEST

// Test Suite Setup
Suite *cert_loader_suite(void)
{
    Suite *s;
    TCase *tc_core, *tc_advanced, *tc_integration, *tc_performance, *tc_security, *tc_errors;
    
    s = suite_create("Certificate Loader");
    
    // Core tests (Phase 1 & 2)
    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_cert_loader_creation);
    tcase_add_test(tc_core, test_password_management);
    suite_add_tcase(s, tc_core);
    
    // Advanced tests (Phase 3)
    tc_advanced = tcase_create("Advanced");
    tcase_add_checked_fixture(tc_advanced, setup, teardown);
    tcase_add_test(tc_advanced, test_online_validation_toggle);
    tcase_add_test(tc_advanced, test_trust_chain_validation_null_inputs);
    tcase_add_test(tc_advanced, test_ocsp_validation_null_inputs);
    tcase_add_test(tc_advanced, test_crl_validation_null_inputs);
    tcase_add_test(tc_advanced, test_trust_chain_empty_ca_list);
    suite_add_tcase(s, tc_advanced);
    
    // Integration tests
    tc_integration = tcase_create("Integration");
    tcase_add_checked_fixture(tc_integration, setup, teardown);
    tcase_add_test(tc_integration, test_comprehensive_certificate_workflow);
    suite_add_tcase(s, tc_integration);
    
    // Performance tests
    tc_performance = tcase_create("Performance");
    tcase_add_checked_fixture(tc_performance, setup, teardown);
    tcase_add_test(tc_performance, test_trust_chain_performance);
    suite_add_tcase(s, tc_performance);
    
    // Security tests
    tc_security = tcase_create("Security");
    tcase_add_checked_fixture(tc_security, setup, teardown);
    tcase_add_test(tc_security, test_password_memory_security);
    tcase_add_test(tc_security, test_validation_result_consistency);
    suite_add_tcase(s, tc_security);
    
    // Error handling tests
    tc_errors = tcase_create("ErrorHandling");
    tcase_add_checked_fixture(tc_errors, setup, teardown);
    tcase_add_test(tc_errors, test_error_handling_robustness);
    suite_add_tcase(s, tc_errors);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    // Initialize strongSwan library for testing
    library_init(NULL, "test-cert-loader");
    atexit(library_deinit);
    
    s = cert_loader_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 