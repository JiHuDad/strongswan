/*
 * Mock implementation for extsock_cert_loader
 * For testing purposes only
 */

#include "../../adapters/crypto/extsock_cert_loader.h"
#include "../../common/extsock_common.h"

typedef struct private_mock_cert_loader_t {
    extsock_cert_loader_t public;
} private_mock_cert_loader_t;

static certificate_t* mock_load_certificate(private_mock_cert_loader_t *this, char *path)
{
    EXTSOCK_DBG(1, "Mock cert loader: load_certificate called with path: %s", path ? path : "NULL");
    return NULL; // Mock implementation
}

static private_key_t* mock_load_private_key(private_mock_cert_loader_t *this, char *path, char *passphrase)
{
    EXTSOCK_DBG(1, "Mock cert loader: load_private_key called");
    return NULL; // Mock implementation
}

static private_key_t* mock_load_private_key_auto(private_mock_cert_loader_t *this, char *path)
{
    EXTSOCK_DBG(1, "Mock cert loader: load_private_key_auto called");
    return NULL; // Mock implementation
}

static bool mock_verify_certificate_chain(private_mock_cert_loader_t *this, certificate_t *cert, certificate_t *ca_cert)
{
    EXTSOCK_DBG(1, "Mock cert loader: verify_certificate_chain called");
    return TRUE; // Mock implementation
}

static auth_cfg_t* mock_build_trust_chain(private_mock_cert_loader_t *this, certificate_t *subject, linked_list_t *ca_certs, bool online_validation)
{
    EXTSOCK_DBG(1, "Mock cert loader: build_trust_chain called");
    return NULL; // Mock implementation
}

static cert_validation_t mock_validate_ocsp(private_mock_cert_loader_t *this, certificate_t *subject, certificate_t *issuer)
{
    EXTSOCK_DBG(1, "Mock cert loader: validate_ocsp called");
    return VALIDATION_GOOD; // Mock implementation
}

static cert_validation_t mock_validate_crl(private_mock_cert_loader_t *this, certificate_t *subject, certificate_t *issuer)
{
    EXTSOCK_DBG(1, "Mock cert loader: validate_crl called");
    return VALIDATION_GOOD; // Mock implementation
}

static bool mock_verify_key_cert_match(private_mock_cert_loader_t *this, private_key_t *key, certificate_t *cert)
{
    EXTSOCK_DBG(1, "Mock cert loader: verify_key_cert_match called");
    return TRUE; // Mock implementation
}

static void mock_set_password(private_mock_cert_loader_t *this, char *password)
{
    EXTSOCK_DBG(1, "Mock cert loader: set_password called");
}

static void mock_set_interactive(private_mock_cert_loader_t *this, bool interactive)
{
    EXTSOCK_DBG(1, "Mock cert loader: set_interactive called with %s", interactive ? "TRUE" : "FALSE");
}

static void mock_set_online_validation(private_mock_cert_loader_t *this, bool enable)
{
    EXTSOCK_DBG(1, "Mock cert loader: set_online_validation called with %s", enable ? "TRUE" : "FALSE");
}

static bool mock_add_credentials(private_mock_cert_loader_t *this, certificate_t *cert, private_key_t *key, mem_cred_t *creds)
{
    EXTSOCK_DBG(1, "Mock cert loader: add_credentials called");
    return TRUE; // Mock implementation
}

static void mock_destroy(private_mock_cert_loader_t *this)
{
    EXTSOCK_DBG(1, "Mock cert loader: destroy called");
    free(this);
}

extsock_cert_loader_t *extsock_cert_loader_create()
{
    private_mock_cert_loader_t *this;

    INIT(this,
        .public = {
            .load_certificate = (certificate_t*(*)(extsock_cert_loader_t*, char*))mock_load_certificate,
            .load_private_key = (private_key_t*(*)(extsock_cert_loader_t*, char*, char*))mock_load_private_key,
            .load_private_key_auto = (private_key_t*(*)(extsock_cert_loader_t*, char*))mock_load_private_key_auto,
            .verify_certificate_chain = (bool(*)(extsock_cert_loader_t*, certificate_t*, certificate_t*))mock_verify_certificate_chain,
            .build_trust_chain = (auth_cfg_t*(*)(extsock_cert_loader_t*, certificate_t*, linked_list_t*, bool))mock_build_trust_chain,
            .validate_ocsp = (cert_validation_t(*)(extsock_cert_loader_t*, certificate_t*, certificate_t*))mock_validate_ocsp,
            .validate_crl = (cert_validation_t(*)(extsock_cert_loader_t*, certificate_t*, certificate_t*))mock_validate_crl,
            .verify_key_cert_match = (bool(*)(extsock_cert_loader_t*, private_key_t*, certificate_t*))mock_verify_key_cert_match,
            .set_password = (void(*)(extsock_cert_loader_t*, char*))mock_set_password,
            .set_interactive = (void(*)(extsock_cert_loader_t*, bool))mock_set_interactive,
            .set_online_validation = (void(*)(extsock_cert_loader_t*, bool))mock_set_online_validation,
            .add_credentials = (bool(*)(extsock_cert_loader_t*, certificate_t*, private_key_t*, mem_cred_t*))mock_add_credentials,
            .destroy = (void(*)(extsock_cert_loader_t*))mock_destroy,
        },
    );

    EXTSOCK_DBG(1, "Mock cert loader created successfully");
    return &this->public;
}