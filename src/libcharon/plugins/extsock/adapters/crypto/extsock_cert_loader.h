/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_cert_loader extsock_cert_loader
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_CERT_LOADER_H_
#define EXTSOCK_CERT_LOADER_H_

#include <credentials/certificates/certificate.h>
#include <credentials/keys/private_key.h>
#include <credentials/keys/shared_key.h>
#include <credentials/sets/mem_cred.h>
#include <credentials/sets/callback_cred.h>
#include <credentials/auth_cfg.h>
#include <credentials/cert_validator.h>

typedef struct extsock_cert_loader_t extsock_cert_loader_t;

/**
 * Certificate loader utility for extsock plugin
 */
struct extsock_cert_loader_t {

    /**
     * Load certificate from file (PEM/DER auto-detection)
     */
    certificate_t* (*load_certificate)(extsock_cert_loader_t *this, char *path);

    /**
     * Load private key from file with password support
     */
    private_key_t* (*load_private_key)(extsock_cert_loader_t *this, char *path, char *passphrase);

    /**
     * Load private key with automatic password resolution
     */
    private_key_t* (*load_private_key_auto)(extsock_cert_loader_t *this, char *path);

    /**
     * Verify certificate chain (basic)
     */
    bool (*verify_certificate_chain)(extsock_cert_loader_t *this, 
                                    certificate_t *cert, certificate_t *ca_cert);

    /**
     * Build and verify complete trust chain (Phase 3 Advanced)
     */
    auth_cfg_t* (*build_trust_chain)(extsock_cert_loader_t *this, 
                                    certificate_t *subject, 
                                    linked_list_t *ca_certs,
                                    bool online_validation);

    /**
     * Validate certificate using OCSP (Phase 3)
     */
    cert_validation_t (*validate_ocsp)(extsock_cert_loader_t *this,
                                     certificate_t *subject,
                                     certificate_t *issuer);

    /**
     * Validate certificate using CRL (Phase 3)
     */
    cert_validation_t (*validate_crl)(extsock_cert_loader_t *this,
                                    certificate_t *subject,
                                    certificate_t *issuer);

    /**
     * Verify key-certificate match
     */
    bool (*verify_key_cert_match)(extsock_cert_loader_t *this,
                                 private_key_t *key, certificate_t *cert);

    /**
     * Set password for encrypted private keys
     */
    void (*set_password)(extsock_cert_loader_t *this, char *password);

    /**
     * Enable/disable interactive password prompting
     */
    void (*set_interactive)(extsock_cert_loader_t *this, bool interactive);

    /**
     * Enable/disable online validation (OCSP/CRL)
     */
    void (*set_online_validation)(extsock_cert_loader_t *this, bool enable);

    /**
     * Add certificate and key to credential manager
     */
    bool (*add_credentials)(extsock_cert_loader_t *this, certificate_t *cert, 
                          private_key_t *key, mem_cred_t *creds);

    /**
     * Destroy instance
     */
    void (*destroy)(extsock_cert_loader_t *this);
};

/**
 * Create extsock certificate loader instance
 */
extsock_cert_loader_t* extsock_cert_loader_create();

#endif /** EXTSOCK_CERT_LOADER_H_ @}*/ 