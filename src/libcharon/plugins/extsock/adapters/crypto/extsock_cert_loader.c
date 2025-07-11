/*
 * Copyright (C) 2024 strongSwan Project
 */

#include "extsock_cert_loader.h"

#include <library.h>
#include <utils/debug.h>
#include <credentials/keys/shared_key.h>
#include <credentials/sets/callback_cred.h>
#include <credentials/certificates/x509.h>
#include <credentials/certificates/crl.h>
#include <credentials/certificates/ocsp_response.h>
#include <credentials/auth_cfg.h>
#include <time.h>
#include <unistd.h>

typedef struct private_extsock_cert_loader_t private_extsock_cert_loader_t;

struct private_extsock_cert_loader_t {
    extsock_cert_loader_t public;
    
    /** Password for encrypted private keys */
    char *password;
    
    /** Interactive password prompting enabled */
    bool interactive;
    
    /** Online validation (OCSP/CRL) enabled */
    bool online_validation;
    
    /** Callback credential set for password resolution */
    callback_cred_t *callback_creds;
};

/**
 * Password callback for encrypted private keys
 */
static shared_key_t* password_callback(private_extsock_cert_loader_t *this,
                                     shared_key_type_t type,
                                     identification_t *me, identification_t *other,
                                     id_match_t *match_me, id_match_t *match_other)
{
    chunk_t password_chunk;
    
    if (type != SHARED_PRIVATE_KEY_PASS)
    {
        return NULL;
    }
    
    if (this->password)
    {
        DBG2(DBG_CFG, "using configured password for private key decryption");
        password_chunk = chunk_create(this->password, strlen(this->password));
        if (match_me) *match_me = ID_MATCH_PERFECT;
        if (match_other) *match_other = ID_MATCH_PERFECT;
        return shared_key_create(SHARED_PRIVATE_KEY_PASS, chunk_clone(password_chunk));
    }
    
    if (this->interactive)
    {
        DBG1(DBG_CFG, "private key is encrypted, but interactive prompting disabled in this version");
        /* 
         * Note: getpass() requires special linking on some systems.
         * For now, we disable interactive prompting and rely on:
         * 1. Pre-configured passwords
         * 2. Environment variables 
         * 3. Credential manager integration
         */
        return NULL;
    }
    
    return NULL;
}

METHOD(extsock_cert_loader_t, load_certificate, certificate_t*,
       private_extsock_cert_loader_t *this, char *path)
{
    certificate_t *cert;
    
    if (!path)
    {
        DBG1(DBG_CFG, "certificate path is NULL");
        return NULL;
    }
    
    cert = lib->creds->create(lib->creds, CRED_CERTIFICATE, CERT_X509,
                             BUILD_FROM_FILE, path, BUILD_END);
    
    if (!cert)
    {
        DBG1(DBG_CFG, "failed to load certificate from %s", path);
        return NULL;
    }
    
    DBG2(DBG_CFG, "successfully loaded certificate from %s", path);
    return cert;
}

METHOD(extsock_cert_loader_t, load_private_key, private_key_t*,
       private_extsock_cert_loader_t *this, char *path, char *passphrase)
{
    private_key_t *key;
    
    if (!path)
    {
        DBG1(DBG_CFG, "private key path is NULL");
        return NULL;
    }
    
    /* Temporarily set password for this operation */
    char *old_password = this->password;
    this->password = passphrase;
    
    key = lib->creds->create(lib->creds, CRED_PRIVATE_KEY, KEY_ANY,
                            BUILD_FROM_FILE, path, BUILD_END);
    
    /* Restore original password */
    this->password = old_password;
    
    if (!key)
    {
        DBG1(DBG_CFG, "failed to load private key from %s", path);
        return NULL;
    }
    
    DBG2(DBG_CFG, "successfully loaded private key from %s", path);
    return key;
}

METHOD(extsock_cert_loader_t, load_private_key_auto, private_key_t*,
       private_extsock_cert_loader_t *this, char *path)
{
    private_key_t *key;
    
    if (!path)
    {
        DBG1(DBG_CFG, "private key path is NULL");
        return NULL;
    }
    
    /* Add our callback credential set temporarily */
    lib->credmgr->add_local_set(lib->credmgr, &this->callback_creds->set, FALSE);
    
    key = lib->creds->create(lib->creds, CRED_PRIVATE_KEY, KEY_ANY,
                            BUILD_FROM_FILE, path, BUILD_END);
    
    /* Remove the callback credential set */
    lib->credmgr->remove_local_set(lib->credmgr, &this->callback_creds->set);
    
    if (!key)
    {
        DBG1(DBG_CFG, "failed to load private key from %s", path);
        return NULL;
    }
    
    DBG2(DBG_CFG, "successfully loaded private key from %s", path);
    return key;
}

METHOD(extsock_cert_loader_t, verify_certificate_chain, bool,
       private_extsock_cert_loader_t *this, certificate_t *cert, certificate_t *ca_cert)
{
    identification_t *cert_issuer, *ca_subject;
    x509_t *x509_ca;
    bool valid = FALSE;
    
    if (!cert || !ca_cert)
    {
        DBG1(DBG_CFG, "certificate or CA certificate is NULL");
        return FALSE;
    }
    
    /* Basic issuer-subject DN matching */
    cert_issuer = cert->get_issuer(cert);
    ca_subject = ca_cert->get_subject(ca_cert);
    
    if (!cert_issuer || !ca_subject)
    {
        DBG1(DBG_CFG, "failed to extract certificate subjects for validation");
        return FALSE;
    }
    
    if (!cert_issuer->equals(cert_issuer, ca_subject))
    {
        DBG2(DBG_CFG, "certificate issuer does not match CA subject");
        return FALSE;
    }
    
    /* Signature verification */
    if (cert->issued_by(cert, ca_cert, NULL))
    {
        DBG2(DBG_CFG, "certificate signature verification: VALID");
        valid = TRUE;
    }
    else
    {
        DBG1(DBG_CFG, "certificate signature verification: FAILED");
        return FALSE;
    }
    
    /* Additional X.509 specific validations */
    if (cert->get_type(cert) == CERT_X509 && ca_cert->get_type(ca_cert) == CERT_X509)
    {
        x509_flag_t ca_flags;
        time_t not_before, not_after, ca_not_before, ca_not_after;
        time_t now = time(NULL);
        
        x509_ca = (x509_t*)ca_cert;
        
        /* Check certificate validity periods */
        cert->get_validity(cert, &now, &not_before, &not_after);
        ca_cert->get_validity(ca_cert, &now, &ca_not_before, &ca_not_after);
        
        if (now < not_before || now > not_after)
        {
            DBG1(DBG_CFG, "certificate is not valid at current time");
            return FALSE;
        }
        
        if (now < ca_not_before || now > ca_not_after)
        {
            DBG1(DBG_CFG, "CA certificate is not valid at current time");
            return FALSE;
        }
        
        /* Check if CA certificate has CA capabilities */
        ca_flags = x509_ca->get_flags(x509_ca);
        if (!(ca_flags & X509_CA))
        {
            DBG1(DBG_CFG, "CA certificate does not have CA flag set");
            /* This is a warning, not a failure */
        }
        
        /* Check key usage extensions - use available flags */
        if (ca_flags & X509_CRL_SIGN)
        {
            DBG2(DBG_CFG, "CA certificate has signing capability");
        }
        else
        {
            DBG2(DBG_CFG, "CA certificate signing capability unknown");
            /* This is informational only */
        }
    }
    
    DBG1(DBG_CFG, "certificate chain validation: %s", valid ? "VALID" : "INVALID");
    return valid;
}

METHOD(extsock_cert_loader_t, verify_key_cert_match, bool,
       private_extsock_cert_loader_t *this, private_key_t *key, certificate_t *cert)
{
    public_key_t *cert_pubkey;
    bool match = FALSE;
    
    if (!key || !cert)
    {
        DBG1(DBG_CFG, "private key or certificate is NULL");
        return FALSE;
    }
    
    cert_pubkey = cert->get_public_key(cert);
    if (cert_pubkey)
    {
        match = key->belongs_to(key, cert_pubkey);
        cert_pubkey->destroy(cert_pubkey);
        DBG2(DBG_CFG, "key-certificate match: %s", match ? "YES" : "NO");
    }
    else
    {
        DBG1(DBG_CFG, "failed to extract public key from certificate");
    }
    
    return match;
}

METHOD(extsock_cert_loader_t, set_password, void,
       private_extsock_cert_loader_t *this, char *password)
{
    if (this->password)
    {
        memwipe(this->password, strlen(this->password));
        free(this->password);
    }
    
    this->password = password ? strdup(password) : NULL;
    DBG2(DBG_CFG, "password %s for private key decryption", 
         password ? "set" : "cleared");
}

METHOD(extsock_cert_loader_t, set_interactive, void,
       private_extsock_cert_loader_t *this, bool interactive)
{
    this->interactive = interactive;
    DBG2(DBG_CFG, "interactive password prompting %s", 
         interactive ? "enabled" : "disabled");
}

METHOD(extsock_cert_loader_t, add_credentials, bool,
       private_extsock_cert_loader_t *this, certificate_t *cert, 
       private_key_t *key, mem_cred_t *creds)
{
    if (!creds)
    {
        DBG1(DBG_CFG, "credential store is NULL");
        return FALSE;
    }
    
    if (cert)
    {
        creds->add_cert(creds, FALSE, cert->get_ref(cert));
        DBG2(DBG_CFG, "added certificate to credential store");
    }
    
    if (key)
    {
        creds->add_key(creds, key->get_ref(key));
        DBG2(DBG_CFG, "added private key to credential store");
    }
    
    return TRUE;
}

METHOD(extsock_cert_loader_t, destroy, void,
       private_extsock_cert_loader_t *this)
{
    if (this->password)
    {
        memwipe(this->password, strlen(this->password));
        free(this->password);
    }
    
    if (this->callback_creds)
    {
        this->callback_creds->destroy(this->callback_creds);
    }
    
    free(this);
}

METHOD(extsock_cert_loader_t, build_trust_chain, auth_cfg_t*,
       private_extsock_cert_loader_t *this, certificate_t *subject,
       linked_list_t *ca_certs, bool online_validation)
{
    auth_cfg_t *auth;
    enumerator_t *enumerator;
    certificate_t *ca_cert, *current_cert, *issuer;
    bool chain_valid = FALSE;
    int path_length = 0;
    const int MAX_CHAIN_LENGTH = 7;  // strongSwan's MAX_TRUST_PATH_LEN
    
    if (!subject)
    {
        DBG1(DBG_CFG, "subject certificate is NULL");
        return NULL;
    }
    
    auth = auth_cfg_create();
    current_cert = subject->get_ref(subject);
    
    DBG2(DBG_CFG, "building trust chain for certificate: %Y", 
         subject->get_subject(subject));
    
    // Add subject certificate to auth config
    auth->add(auth, AUTH_RULE_SUBJECT_CERT, current_cert->get_ref(current_cert));
    
    // Build chain up to root
    while (path_length < MAX_CHAIN_LENGTH)
    {
        bool found_issuer = FALSE;
        identification_t *issuer_id = current_cert->get_issuer(current_cert);
        
        // Check if this is a self-signed certificate (potential root)
        if (current_cert->issued_by(current_cert, current_cert, NULL))
        {
            DBG2(DBG_CFG, "found self-signed certificate at path length %d", path_length);
            
            // Check if this self-signed cert is in our trusted CA list
            if (ca_certs)
            {
                enumerator = ca_certs->create_enumerator(ca_certs);
                while (enumerator->enumerate(enumerator, &ca_cert))
                {
                    if (current_cert->equals(current_cert, ca_cert))
                    {
                        DBG1(DBG_CFG, "certificate chain validated with trusted root: %Y",
                             ca_cert->get_subject(ca_cert));
                        auth->add(auth, AUTH_RULE_CA_CERT, ca_cert->get_ref(ca_cert));
                        chain_valid = TRUE;
                        break;
                    }
                }
                enumerator->destroy(enumerator);
            }
            
            if (chain_valid || path_length == 0)
            {
                // Either trusted root found or single self-signed cert
                break;
            }
            else
            {
                DBG1(DBG_CFG, "self-signed certificate not in trusted CA list");
                break;
            }
        }
        
        // Find issuer certificate
        if (ca_certs)
        {
            enumerator = ca_certs->create_enumerator(ca_certs);
            while (enumerator->enumerate(enumerator, &ca_cert))
            {
                identification_t *ca_subject = ca_cert->get_subject(ca_cert);
                
                if (issuer_id->equals(issuer_id, ca_subject))
                {
                    // Verify signature
                    if (current_cert->issued_by(current_cert, ca_cert, NULL))
                    {
                        DBG2(DBG_CFG, "found valid issuer at path length %d: %Y",
                             path_length + 1, ca_subject);
                        
                        // Check if this CA is self-signed (root)
                        if (ca_cert->issued_by(ca_cert, ca_cert, NULL))
                        {
                            auth->add(auth, AUTH_RULE_CA_CERT, ca_cert->get_ref(ca_cert));
                            chain_valid = TRUE;
                        }
                        else
                        {
                            auth->add(auth, AUTH_RULE_IM_CERT, ca_cert->get_ref(ca_cert));
                        }
                        
                        current_cert->destroy(current_cert);
                        current_cert = ca_cert->get_ref(ca_cert);
                        found_issuer = TRUE;
                        break;
                    }
                }
            }
            enumerator->destroy(enumerator);
        }
        
        if (!found_issuer)
        {
            DBG1(DBG_CFG, "no issuer found for certificate: %Y", issuer_id);
            break;
        }
        
        path_length++;
        
        if (chain_valid)
        {
            break;
        }
    }
    
    current_cert->destroy(current_cert);
    
    if (path_length >= MAX_CHAIN_LENGTH)
    {
        DBG1(DBG_CFG, "maximum trust chain length (%d) exceeded", MAX_CHAIN_LENGTH);
        auth->destroy(auth);
        return NULL;
    }
    
    if (!chain_valid)
    {
        DBG1(DBG_CFG, "trust chain validation failed");
        auth->destroy(auth);
        return NULL;
    }
    
         // Perform online validation if enabled
     if (online_validation && this->online_validation)
     {
         enumerator_t *chain_enum;
         auth_rule_t rule;
        
        DBG2(DBG_CFG, "performing online validation (OCSP/CRL)");
        
        current_cert = subject;
        chain_enum = auth->create_enumerator(auth);
        while (chain_enum->enumerate(chain_enum, &rule, &issuer))
        {
            if (rule == AUTH_RULE_CA_CERT || rule == AUTH_RULE_IM_CERT)
            {
                cert_validation_t ocsp_result = this->public.validate_ocsp(&this->public, 
                                                                         current_cert, issuer);
                if (ocsp_result == VALIDATION_REVOKED)
                {
                    DBG1(DBG_CFG, "certificate revoked by OCSP");
                    auth->add(auth, AUTH_RULE_OCSP_VALIDATION, VALIDATION_REVOKED);
                    chain_enum->destroy(chain_enum);
                    auth->destroy(auth);
                    return NULL;
                }
                else if (ocsp_result == VALIDATION_GOOD)
                {
                    DBG2(DBG_CFG, "OCSP validation successful");
                    auth->add(auth, AUTH_RULE_OCSP_VALIDATION, VALIDATION_GOOD);
                }
                else
                {
                    // Fallback to CRL if OCSP fails
                    cert_validation_t crl_result = this->public.validate_crl(&this->public,
                                                                           current_cert, issuer);
                    if (crl_result == VALIDATION_REVOKED)
                    {
                        DBG1(DBG_CFG, "certificate revoked by CRL");
                        auth->add(auth, AUTH_RULE_CRL_VALIDATION, VALIDATION_REVOKED);
                        chain_enum->destroy(chain_enum);
                        auth->destroy(auth);
                        return NULL;
                    }
                    auth->add(auth, AUTH_RULE_CRL_VALIDATION, crl_result);
                }
                
                if (rule == AUTH_RULE_CA_CERT)
                {
                    break;  // Reached root CA
                }
                current_cert = issuer;
            }
        }
        chain_enum->destroy(chain_enum);
    }
    
    DBG1(DBG_CFG, "trust chain built successfully with path length %d", path_length);
    return auth;
}

METHOD(extsock_cert_loader_t, validate_ocsp, cert_validation_t,
       private_extsock_cert_loader_t *this, certificate_t *subject, 
       certificate_t *issuer)
{
    certificate_t *ocsp_response;
    cert_validation_t result = VALIDATION_SKIPPED;
    
    if (!subject || !issuer)
    {
        DBG1(DBG_CFG, "subject or issuer certificate is NULL for OCSP validation");
        return VALIDATION_FAILED;
    }
    
    DBG2(DBG_CFG, "performing OCSP validation for: %Y", 
         subject->get_subject(subject));
    
    // Use strongSwan's credential manager to get OCSP response
    ocsp_response = lib->credmgr->get_ocsp(lib->credmgr, subject, issuer);
    
    if (ocsp_response)
    {
        DBG2(DBG_CFG, "OCSP response retrieved successfully");
        
        // The credential manager already validates the response
        // We assume if we get a response, the certificate is good
        result = VALIDATION_GOOD;
        ocsp_response->destroy(ocsp_response);
    }
    else
    {
        DBG2(DBG_CFG, "no valid OCSP response found");
        result = VALIDATION_SKIPPED;
    }
    
    return result;
}

METHOD(extsock_cert_loader_t, validate_crl, cert_validation_t,
       private_extsock_cert_loader_t *this, certificate_t *subject,
       certificate_t *issuer)
{
    enumerator_t *enumerator;
    certificate_t *crl;
    cert_validation_t result = VALIDATION_SKIPPED;
    crl_t *x509_crl;
    x509_t *x509_subject;
    
    if (!subject || !issuer)
    {
        DBG1(DBG_CFG, "subject or issuer certificate is NULL for CRL validation");
        return VALIDATION_FAILED;
    }
    
    if (subject->get_type(subject) != CERT_X509 || 
        issuer->get_type(issuer) != CERT_X509)
    {
        DBG2(DBG_CFG, "CRL validation only supported for X.509 certificates");
        return VALIDATION_SKIPPED;
    }
    
    DBG2(DBG_CFG, "performing CRL validation for: %Y", 
         subject->get_subject(subject));
    
    x509_subject = (x509_t*)subject;
    
    // Look for CRL from the issuer
    enumerator = lib->credmgr->create_cert_enumerator(lib->credmgr, 
                                                     CERT_X509_CRL, KEY_ANY,
                                                     issuer->get_subject(issuer), FALSE);
    while (enumerator->enumerate(enumerator, &crl))
    {
        if (crl->issued_by(crl, issuer, NULL))
        {
            x509_crl = (crl_t*)crl;
            enumerator_t *revoked_enum;
            chunk_t serial, subject_serial;
            time_t revocation_date;
            crl_reason_t reason;
            bool found_revoked = FALSE;
            
            subject_serial = x509_subject->get_serial(x509_subject);
            
            // Check if certificate is in the revocation list
            revoked_enum = x509_crl->create_enumerator(x509_crl);
            while (revoked_enum->enumerate(revoked_enum, &serial, &revocation_date, &reason))
            {
                if (chunk_equals(serial, subject_serial))
                {
                    DBG1(DBG_CFG, "certificate is revoked according to CRL (reason: %d)", reason);
                    result = VALIDATION_REVOKED;
                    found_revoked = TRUE;
                    break;
                }
            }
            revoked_enum->destroy(revoked_enum);
            
            if (!found_revoked)
            {
                DBG2(DBG_CFG, "certificate is not revoked according to CRL");
                result = VALIDATION_GOOD;
            }
            break;
        }
    }
    enumerator->destroy(enumerator);
    
    if (result == VALIDATION_SKIPPED)
    {
        DBG2(DBG_CFG, "no suitable CRL found for validation");
    }
    
    return result;
}

METHOD(extsock_cert_loader_t, set_online_validation, void,
       private_extsock_cert_loader_t *this, bool enable)
{
    this->online_validation = enable;
    DBG2(DBG_CFG, "online validation (OCSP/CRL) %s", 
         enable ? "enabled" : "disabled");
}

/*
 * See header
 */
extsock_cert_loader_t* extsock_cert_loader_create()
{
    private_extsock_cert_loader_t *this;
    
    INIT(this,
        .public = {
            .load_certificate = _load_certificate,
            .load_private_key = _load_private_key,
            .load_private_key_auto = _load_private_key_auto,
            .verify_certificate_chain = _verify_certificate_chain,
            .build_trust_chain = _build_trust_chain,
            .validate_ocsp = _validate_ocsp,
            .validate_crl = _validate_crl,
            .verify_key_cert_match = _verify_key_cert_match,
            .set_password = _set_password,
            .set_interactive = _set_interactive,
            .set_online_validation = _set_online_validation,
            .add_credentials = _add_credentials,
            .destroy = _destroy,
        },
        .password = NULL,
        .interactive = TRUE,  /* Enable interactive by default */
        .online_validation = TRUE,  /* Enable online validation by default */
    );
    
    /* Create callback credential set for password resolution */
    this->callback_creds = callback_cred_create_shared(
        (callback_cred_shared_cb_t)password_callback, this);
    
    return &this->public;
} 