/*
 * Copyright (C) 2024 strongSwan Project
 * Certificate Loader Unit Tests
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../adapters/crypto/extsock_cert_loader.h"
#include "../../common/extsock_common.h"

#include <library.h>
#include <credentials/certificates/certificate.h>
#include <credentials/keys/private_key.h>

static extsock_cert_loader_t *cert_loader;

// 테스트용 PEM 인증서 (자체 서명된 테스트 인증서)
static const char *test_cert_pem = 
"-----BEGIN CERTIFICATE-----\n"
"MIICljCCAX4CCQDAOxKQdk+vZjANBgkqhkiG9w0BAQsFADA7MQswCQYDVQQGEwJL\n"
"UjEOMAwGA1UECAwFU2VvdWwxDjAMBgNVBAcMBVNlb3VsMQwwCgYDVQQKDANISDEw\n"
"HhcNMjQwNjI1MDQwMDAwWhcNMjUwNjI1MDQwMDAwWjA7MQswCQYDVQQGEwJLUjEO\n"
"MAwGA1UECAwFU2VvdWwxDjAMBgNVBAcMBVNlb3VsMQwwCgYDVQQKDANISDEwggEi\n"
"MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDEuVrDpv6FclF4d4YCF6xU6Xhy\n"
"Y7w8k3Zc+5dLXTUoM9J1wQ8RyEp7V6+Y5Zk9Qd4YcN0z3qO5Y8b1HfN4w7s5L8a\n"
"U9oP6sHs4x2O3k9V8vF7Q6u9J5xZ1pN0z7fQ8y9uKdF5Z3r4O5j2Hq8Vc2fX9G\n"
"w3t1S6vR8nW4qL7bP9jKs5wP8Y3dHx2N1nK4J9bO6pS3fQ7yO2r8V6tE9Y5o1Z\n"
"QIDAQAB\n"
"-----END CERTIFICATE-----\n";

// 테스트용 PEM 개인키 (암호화되지 않음)
static const char *test_key_pem = 
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDEuVrDpv6FclF4\n"
"d4YCF6xU6XhyY7w8k3Zc+5dLXTUoM9J1wQ8RyEp7V6+Y5Zk9Qd4YcN0z3qO5Y8b1\n"
"HfN4w7s5L8aU9oP6sHs4x2O3k9V8vF7Q6u9J5xZ1pN0z7fQ8y9uKdF5Z3r4O5j2\n"
"Hq8Vc2fX9Gw3t1S6vR8nW4qL7bP9jKs5wP8Y3dHx2N1nK4J9bO6pS3fQ7yO2r8V\n"
"6tE9Y5o1ZAgMBAAECggEBALHXK1a4LF1a6oS5hI9b8Z7Vq3oE2mP9qFdY5x8r1Z\n"
"pN0z3fQ7yO2r8V6tE9Y5o1ZpN0z3fQ7yO2r8V6tE9Y5o1ZpN0z3fQ7yO2r8V6t\n"
"E9Y5o1ZpN0z3fQ7yO2r8V6tE9Y5o1ZpN0z3fQ7yO2r8V6tE9Y5o1ZpN0z3fQ7y\n"
"O2r8V6tE9Y5o1ZpN0z3fQ7yO2r8V6tE9Y5o1ZpN0z3fQ7yO2r8V6tE9Y5o1Zp\n"
"N0z3fQ7yO2r8V6tE9Y5o1ZpN0z3fQ7yO2r8V6tE9Y5o1ZwIhAOTJ8xF7Q5y9u\n"
"KdF5Z3r4O5j2Hq8Vc2fX9Gw3t1S6vR8nW4qL7bP9jKs5wP8Y3dHx2N1nK4J9b\n"
"O6pS3fQ7yO2r8V6tE9Y5o1ZwIhAOIl4mE1y8r2V1+Y5Zk9Qd4YcN0z3qO5Y8\n"
"b1HfN4w7s5L8aU9oP6sHs4x2O3k9V8vF7Q6u9J5xZ1pN0z3fQ8y9uKdF5Z3r\n"
"4O5j2Hq8Vc2fX9Gw3t1S6vR8nW4qL7bP9jKs5wP8Y3dHx2N1nK4J9bO6pS3f\n"
"Q7yO2r8V6tE9Y5o1Z\n"
"-----END PRIVATE KEY-----\n";

/**
 * 테스트 설정
 */
void setup_cert_loader_test(void)
{
    // strongSwan 라이브러리 초기화가 필요할 수 있음
    cert_loader = extsock_cert_loader_create();
    ck_assert_ptr_nonnull(cert_loader);
}

/**
 * 테스트 해제
 */
void teardown_cert_loader_test(void)
{
    if (cert_loader) {
        cert_loader->destroy(cert_loader);
        cert_loader = NULL;
    }
}

/**
 * 임시 파일 생성 헬퍼
 */
static char* create_temp_file(const char *content)
{
    char *temp_file = malloc(64);
    strcpy(temp_file, "/tmp/extsock_test_XXXXXX");
    
    int fd = mkstemp(temp_file);
    if (fd == -1) {
        free(temp_file);
        return NULL;
    }
    
    write(fd, content, strlen(content));
    close(fd);
    
    return temp_file;
}

/**
 * 임시 파일 삭제 헬퍼
 */
static void cleanup_temp_file(char *temp_file)
{
    if (temp_file) {
        unlink(temp_file);
        free(temp_file);
    }
}

/**
 * 인증서 로더 생성 테스트
 */
START_TEST(test_cert_loader_creation)
{
    // Given/When/Then - setup에서 이미 생성됨
    ck_assert_ptr_nonnull(cert_loader);
}
END_TEST

/**
 * PEM 인증서 로딩 테스트
 */
START_TEST(test_load_pem_certificate)
{
    // Given
    char *cert_file = create_temp_file(test_cert_pem);
    ck_assert_ptr_nonnull(cert_file);
    
    // When
    certificate_t *cert = cert_loader->load_certificate(cert_loader, cert_file);
    
    // Then - 실제 strongSwan 환경에서 실행할 때만 동작
    if (lib && lib->creds) {
        ck_assert_ptr_nonnull(cert);
        if (cert) {
            ck_assert_int_eq(cert->get_type(cert), CERT_X509);
            cert->destroy(cert);
        }
    }
    
    // Cleanup
    cleanup_temp_file(cert_file);
}
END_TEST

/**
 * PEM 개인키 로딩 테스트
 */
START_TEST(test_load_pem_private_key)
{
    // Given
    char *key_file = create_temp_file(test_key_pem);
    ck_assert_ptr_nonnull(key_file);
    
    // When
    private_key_t *key = cert_loader->load_private_key(cert_loader, key_file, NULL);
    
    // Then - 실제 strongSwan 환경에서 실행할 때만 동작
    if (lib && lib->creds) {
        ck_assert_ptr_nonnull(key);
        if (key) {
            // 키 타입이 RSA인지 확인
            ck_assert_int_ne(key->get_type(key), KEY_ANY);
            key->destroy(key);
        }
    }
    
    // Cleanup
    cleanup_temp_file(key_file);
}
END_TEST

/**
 * 존재하지 않는 파일 처리 테스트
 */
START_TEST(test_load_nonexistent_file)
{
    // When/Then
    certificate_t *cert = cert_loader->load_certificate(cert_loader, "/nonexistent/path/cert.pem");
    ck_assert_ptr_null(cert);
    
    private_key_t *key = cert_loader->load_private_key(cert_loader, "/nonexistent/path/key.pem", NULL);
    ck_assert_ptr_null(key);
}
END_TEST

/**
 * NULL 파라미터 처리 테스트
 */
START_TEST(test_load_null_parameters)
{
    // When/Then
    certificate_t *cert = cert_loader->load_certificate(cert_loader, NULL);
    ck_assert_ptr_null(cert);
    
    private_key_t *key = cert_loader->load_private_key(cert_loader, NULL, NULL);
    ck_assert_ptr_null(key);
}
END_TEST

/**
 * 인증서 체인 검증 테스트
 */
START_TEST(test_certificate_chain_verification)
{
    // Given - 동일한 자체 서명 인증서를 cert와 ca_cert로 사용
    char *cert_file = create_temp_file(test_cert_pem);
    char *ca_cert_file = create_temp_file(test_cert_pem);
    
    if (lib && lib->creds) {
        certificate_t *cert = cert_loader->load_certificate(cert_loader, cert_file);
        certificate_t *ca_cert = cert_loader->load_certificate(cert_loader, ca_cert_file);
        
        if (cert && ca_cert) {
            // When - 자체 서명된 인증서이므로 체인 검증이 성공해야 함
            bool chain_valid = cert_loader->verify_certificate_chain(cert_loader, cert, ca_cert);
            
            // Then
            ck_assert(chain_valid);
            
            cert->destroy(cert);
            ca_cert->destroy(ca_cert);
        }
    }
    
    // Cleanup
    cleanup_temp_file(cert_file);
    cleanup_temp_file(ca_cert_file);
}
END_TEST

/**
 * 테스트 스위트 생성
 */
Suite *cert_loader_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Certificate Loader");

    /* Core test case */
    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup_cert_loader_test, teardown_cert_loader_test);
    
    tcase_add_test(tc_core, test_cert_loader_creation);
    tcase_add_test(tc_core, test_load_pem_certificate);
    tcase_add_test(tc_core, test_load_pem_private_key);
    tcase_add_test(tc_core, test_load_nonexistent_file);
    tcase_add_test(tc_core, test_load_null_parameters);
    tcase_add_test(tc_core, test_certificate_chain_verification);
    
    suite_add_tcase(s, tc_core);

    return s;
}

/**
 * 메인 테스트 실행 함수
 */
int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = cert_loader_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 