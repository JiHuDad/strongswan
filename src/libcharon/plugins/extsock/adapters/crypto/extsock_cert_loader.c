/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_cert_loader.h"
#include "../../common/extsock_common.h"

#include <credentials/credential_factory.h>
#include <credentials/keys/shared_key.h>
#include <library.h>
#include <utils/debug.h>

typedef struct private_extsock_cert_loader_t private_extsock_cert_loader_t;

/**
 * 인증서 로더 내부 구조체
 */
struct private_extsock_cert_loader_t {
    
    /**
     * 공개 인터페이스
     */
    extsock_cert_loader_t public;
};

METHOD(extsock_cert_loader_t, load_certificate, certificate_t*,
    private_extsock_cert_loader_t *this, const char *file_path)
{
    certificate_t *cert = NULL;
    
    if (!file_path) {
        EXTSOCK_DBG(1, "Certificate file path is NULL");
        return NULL;
    }
    
    // strongSwan의 credential factory 활용 (PEM/DER 자동 감지)
    cert = lib->creds->create(lib->creds, CRED_CERTIFICATE, CERT_ANY,
                             BUILD_FROM_FILE, file_path, BUILD_END);
    
    if (!cert) {
        EXTSOCK_DBG(1, "Failed to load certificate from: %s", file_path);
        return NULL;
    }
    
    EXTSOCK_DBG(2, "Successfully loaded certificate: %Y", 
                cert->get_subject(cert));
    return cert;
}

METHOD(extsock_cert_loader_t, load_private_key, private_key_t*,
    private_extsock_cert_loader_t *this, const char *file_path, 
    const char *passphrase)
{
    private_key_t *key = NULL;
    
    if (!file_path) {
        EXTSOCK_DBG(1, "Private key file path is NULL");
        return NULL;
    }
    
    // strongSwan의 credential factory 활용
    // 참고: 패스워드 지원은 pluto/stroke에서 주로 처리됨
    key = lib->creds->create(lib->creds, CRED_PRIVATE_KEY, KEY_ANY,
                            BUILD_FROM_FILE, file_path, BUILD_END);
    
    if (!key) {
        EXTSOCK_DBG(1, "Failed to load private key from: %s", file_path);
        if (passphrase && strlen(passphrase) > 0) {
            EXTSOCK_DBG(1, "Note: Encrypted private key support requires manual setup in ipsec.secrets");
        }
        return NULL;
    }
    
    EXTSOCK_DBG(2, "Successfully loaded private key from: %s", file_path);
    if (passphrase && strlen(passphrase) > 0) {
        EXTSOCK_DBG(1, "Warning: Passphrase ignored - use unencrypted keys or configure ipsec.secrets");
    }
    return key;
}

METHOD(extsock_cert_loader_t, verify_certificate_chain, bool,
    private_extsock_cert_loader_t *this, certificate_t *cert, 
    certificate_t *ca_cert)
{
    if (!cert || !ca_cert) {
        EXTSOCK_DBG(1, "Certificate or CA certificate is NULL");
        return FALSE;
    }
    
    // 간단한 issuer 체크
    identification_t *cert_issuer = cert->get_issuer(cert);
    identification_t *ca_subject = ca_cert->get_subject(ca_cert);
    
    if (!cert_issuer || !ca_subject) {
        EXTSOCK_DBG(1, "Failed to get certificate identities");
        return FALSE;
    }
    
    bool chain_valid = cert_issuer->equals(cert_issuer, ca_subject);
    
    if (chain_valid) {
        EXTSOCK_DBG(2, "Certificate chain validation successful");
        EXTSOCK_DBG(3, "  Certificate: %Y", cert->get_subject(cert));
        EXTSOCK_DBG(3, "  CA Certificate: %Y", ca_subject);
    } else {
        EXTSOCK_DBG(1, "Certificate chain validation failed");
        EXTSOCK_DBG(2, "  Certificate issuer: %Y", cert_issuer);
        EXTSOCK_DBG(2, "  CA subject: %Y", ca_subject);
    }
    
    return chain_valid;
}

METHOD(extsock_cert_loader_t, verify_key_cert_match, bool,
    private_extsock_cert_loader_t *this, private_key_t *key, 
    certificate_t *cert)
{
    if (!key || !cert) {
        EXTSOCK_DBG(1, "Private key or certificate is NULL");
        return FALSE;
    }
    
    // 인증서에서 공개키 추출
    public_key_t *public_key = cert->get_public_key(cert);
    if (!public_key) {
        EXTSOCK_DBG(1, "Failed to extract public key from certificate");
        return FALSE;
    }
    
    // 개인키와 공개키 매칭 확인
    bool match = key->belongs_to(key, public_key);
    public_key->destroy(public_key);
    
    if (match) {
        EXTSOCK_DBG(2, "Private key and certificate match successfully");
    } else {
        EXTSOCK_DBG(1, "Private key and certificate do not match");
    }
    
    return match;
}

METHOD(extsock_cert_loader_t, destroy, void,
    private_extsock_cert_loader_t *this)
{
    free(this);
}

/**
 * 인증서 로더 인스턴스 생성
 */
extsock_cert_loader_t *extsock_cert_loader_create()
{
    private_extsock_cert_loader_t *this;

    INIT(this,
        .public = {
            .load_certificate = _load_certificate,
            .load_private_key = _load_private_key,
            .verify_certificate_chain = _verify_certificate_chain,
            .verify_key_cert_match = _verify_key_cert_match,
            .destroy = _destroy,
        },
    );

    EXTSOCK_DBG(2, "Certificate loader created");
    return &this->public;
} 