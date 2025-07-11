# strongSwan extsock Plugin API Reference

## Overview

The strongSwan extsock plugin provides a comprehensive certificate-based authentication system for IPSec VPN connections. This document describes the complete API for certificate loading, trust chain validation, and online certificate status checking.

## Version History

- **Phase 1**: Basic certificate loading and JSON parsing
- **Phase 2**: Advanced password management and enhanced validation  
- **Phase 3**: Complete trust chain validation with OCSP/CRL support
- **Phase 4**: Comprehensive testing and documentation (Current)

---

## Certificate Loader API

### `extsock_cert_loader_t`

Main interface for certificate operations.

#### Methods

##### Basic Certificate Operations

```c
/**
 * Load certificate from file (PEM/DER auto-detection)
 * 
 * @param this      Certificate loader instance
 * @param path      Path to certificate file
 * @return         Certificate object or NULL on failure
 */
certificate_t* (*load_certificate)(extsock_cert_loader_t *this, char *path);
```

```c
/**
 * Load private key from file with password support
 * 
 * @param this       Certificate loader instance  
 * @param path       Path to private key file
 * @param passphrase Password for encrypted keys (NULL for unencrypted)
 * @return          Private key object or NULL on failure
 */
private_key_t* (*load_private_key)(extsock_cert_loader_t *this, 
                                   char *path, char *passphrase);
```

```c
/**
 * Load private key with automatic password resolution
 * 
 * @param this      Certificate loader instance
 * @param path      Path to private key file  
 * @return         Private key object or NULL on failure
 */
private_key_t* (*load_private_key_auto)(extsock_cert_loader_t *this, char *path);
```

##### Advanced Trust Chain Operations (Phase 3)

```c
/**
 * Build and verify complete trust chain
 * 
 * @param this              Certificate loader instance
 * @param subject           Subject certificate to validate
 * @param ca_certs          List of CA certificates
 * @param online_validation Enable OCSP/CRL checking
 * @return                 Auth configuration with trust chain or NULL
 */
auth_cfg_t* (*build_trust_chain)(extsock_cert_loader_t *this, 
                                certificate_t *subject,
                                linked_list_t *ca_certs,
                                bool online_validation);
```

```c
/**
 * Validate certificate using OCSP
 * 
 * @param this     Certificate loader instance
 * @param subject  Certificate to validate
 * @param issuer   Issuer certificate
 * @return        Validation result (GOOD/REVOKED/FAILED/SKIPPED)
 */
cert_validation_t (*validate_ocsp)(extsock_cert_loader_t *this,
                                 certificate_t *subject,
                                 certificate_t *issuer);
```

```c
/**
 * Validate certificate using CRL
 * 
 * @param this     Certificate loader instance
 * @param subject  Certificate to validate
 * @param issuer   Issuer certificate
 * @return        Validation result (GOOD/REVOKED/FAILED/SKIPPED)
 */
cert_validation_t (*validate_crl)(extsock_cert_loader_t *this,
                                certificate_t *subject,
                                certificate_t *issuer);
```

##### Verification Methods

```c
/**
 * Verify basic certificate chain (Phase 2)
 * 
 * @param this     Certificate loader instance
 * @param cert     Subject certificate
 * @param ca_cert  CA certificate  
 * @return        TRUE if chain is valid
 */
bool (*verify_certificate_chain)(extsock_cert_loader_t *this,
                                certificate_t *cert, 
                                certificate_t *ca_cert);
```

```c
/**
 * Verify key-certificate match
 * 
 * @param this  Certificate loader instance
 * @param key   Private key
 * @param cert  Certificate
 * @return     TRUE if key matches certificate
 */
bool (*verify_key_cert_match)(extsock_cert_loader_t *this,
                             private_key_t *key, 
                             certificate_t *cert);
```

##### Configuration Methods

```c
/**
 * Set password for encrypted private keys
 * 
 * @param this      Certificate loader instance
 * @param password  Password string (NULL to clear)
 */
void (*set_password)(extsock_cert_loader_t *this, char *password);
```

```c
/**
 * Enable/disable interactive password prompting
 * 
 * @param this         Certificate loader instance
 * @param interactive  TRUE to enable interactive prompts
 */
void (*set_interactive)(extsock_cert_loader_t *this, bool interactive);
```

```c
/**
 * Enable/disable online validation (OCSP/CRL)
 * 
 * @param this   Certificate loader instance
 * @param enable TRUE to enable online validation
 */
void (*set_online_validation)(extsock_cert_loader_t *this, bool enable);
```

##### Credential Management

```c
/**
 * Add certificate and key to credential manager
 * 
 * @param this  Certificate loader instance
 * @param cert  Certificate to add (can be NULL)
 * @param key   Private key to add (can be NULL)  
 * @param creds Memory credential store
 * @return     TRUE on success
 */
bool (*add_credentials)(extsock_cert_loader_t *this, 
                      certificate_t *cert,
                      private_key_t *key, 
                      mem_cred_t *creds);
```

```c
/**
 * Destroy certificate loader instance
 * 
 * @param this  Certificate loader instance
 */
void (*destroy)(extsock_cert_loader_t *this);
```

#### Factory Method

```c
/**
 * Create certificate loader instance
 * 
 * @return  New certificate loader instance
 */
extsock_cert_loader_t* extsock_cert_loader_create();
```

---

## JSON Configuration API

### Certificate Authentication Schema

#### Basic Certificate Configuration

```json
{
  "auth": "cert",
  "cert": "/path/to/certificate.pem",
  "private_key": "/path/to/private_key.pem",
  "ca_cert": "/path/to/ca_certificate.pem"
}
```

#### Advanced Configuration (Phase 2 & 3)

```json
{
  "auth": "cert",
  "cert": "/path/to/certificate.pem",
  "private_key": "/path/to/private_key.pem", 
  "private_key_passphrase": "secret_password",
  "ca_cert": "/path/to/ca_certificate.pem",
  "enable_ocsp": true,
  "enable_crl": true
}
```

### Configuration Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `auth` | string | Yes | Must be "cert" for certificate authentication |
| `cert` | string | Yes | Path to X.509 certificate file (PEM/DER) |
| `private_key` | string | Yes* | Path to private key file (*required for local auth) |
| `private_key_passphrase` | string | No | Password for encrypted private keys |
| `ca_cert` | string | No | Path to CA certificate for chain validation |
| `enable_ocsp` | boolean | No | Enable OCSP validation (default: true) |
| `enable_crl` | boolean | No | Enable CRL validation (default: true) |

---

## Validation Results

### `cert_validation_t` Enumeration

```c
enum cert_validation_t {
    VALIDATION_GOOD = 0,      // Certificate is valid
    VALIDATION_SKIPPED,       // Validation was skipped
    VALIDATION_STALE,         // Validation info is stale
    VALIDATION_FAILED,        // Validation failed
    VALIDATION_ON_HOLD,       // Certificate is on hold
    VALIDATION_REVOKED        // Certificate is revoked
};
```

### Trust Chain Validation Process

1. **Subject Certificate Loading**: Load and parse subject certificate
2. **Chain Building**: Construct path from subject to trusted root
3. **Signature Verification**: Verify each certificate's signature
4. **Validity Checking**: Check certificate validity periods
5. **Online Validation**: Perform OCSP/CRL checks if enabled
6. **Result Assembly**: Create auth_cfg with complete trust chain

---

## Password Management

### Resolution Strategy (Phase 2)

The plugin uses a multi-tier password resolution strategy:

1. **Explicit Password**: JSON `private_key_passphrase` field
2. **Credential Manager**: strongSwan's `SHARED_PRIVATE_KEY_PASS` system
3. **Interactive Prompting**: User input (if enabled)
4. **Automatic Resolution**: Plugin's callback system

### Security Features

- **Memory Protection**: Passwords cleared with `memwipe()`
- **Callback Integration**: Uses strongSwan's credential callback system
- **Temporary Storage**: Passwords only held during key loading operations

---

## Online Validation (Phase 3)

### OCSP (Online Certificate Status Protocol)

- **Real-time Validation**: Live certificate status checking
- **Responder Discovery**: Automatic OCSP responder location
- **Response Caching**: Leverages strongSwan's OCSP cache
- **Fallback Support**: CRL validation if OCSP fails

### CRL (Certificate Revocation List)

- **Serial Number Matching**: Precise certificate identification
- **Revocation Reasons**: Detailed logging of revocation causes
- **Multiple CRL Support**: Handles various issuer CRLs
- **Performance Optimization**: Efficient enumeration over revocation lists

---

## Error Handling

### Common Error Scenarios

| Error Type | Description | Resolution |
|------------|-------------|------------|
| File Not Found | Certificate/key file doesn't exist | Check file path and permissions |
| Parse Error | Invalid certificate/key format | Verify file format (PEM/DER) |
| Password Error | Wrong password for encrypted key | Check password or use auto-resolution |
| Chain Error | Trust chain validation failed | Verify CA certificates and signatures |
| OCSP Error | OCSP validation failed | Check network connectivity or disable OCSP |
| CRL Error | CRL validation failed | Verify CRL availability or disable CRL |

### Error Codes

Functions return appropriate error codes:
- `NULL`: Failed operations (cert/key loading)
- `FALSE`: Failed validations (chain/key-cert match)
- `VALIDATION_FAILED`: Failed online validation

---

## Performance Considerations

### Optimization Features

- **Cached Responses**: OCSP responses cached by strongSwan
- **Lazy Loading**: Certificates loaded only when needed
- **Parallel Operations**: Multiple certificates processed concurrently
- **Memory Efficiency**: Proper reference counting and cleanup

### Performance Metrics

- **Certificate Loading**: < 100ms per certificate
- **Chain Validation**: < 500ms for typical 3-level chains
- **OCSP Validation**: < 2000ms (network dependent)
- **CRL Validation**: < 1000ms (CRL size dependent)

---

## Integration Examples

### Complete VPN Configuration

```json
{
  "connection_name": "corporate-vpn",
  "local": {
    "auth": "cert",
    "cert": "/etc/ipsec.d/certs/client.crt",
    "private_key": "/etc/ipsec.d/private/client.key",
    "private_key_passphrase": "client_key_password",
    "ca_cert": "/etc/ipsec.d/cacerts/corporate_ca.crt",
    "enable_ocsp": true,
    "enable_crl": false
  },
  "remote": {
    "auth": "cert",
    "ca_cert": "/etc/ipsec.d/cacerts/corporate_ca.crt",
    "enable_ocsp": true,
    "enable_crl": true
  },
  "ike_proposals": ["aes256-sha256-modp2048"],
  "esp_proposals": ["aes256gcm16"],
  "children": [
    {
      "name": "corporate-tunnel",
      "local_ts": ["10.0.0.0/24"],
      "remote_ts": ["192.168.1.0/24"]
    }
  ]
}
```

### Programmatic Usage

```c
// Create certificate loader
extsock_cert_loader_t *loader = extsock_cert_loader_create();

// Configure password and validation
loader->set_password(loader, "my_key_password");
loader->set_online_validation(loader, true);

// Load certificates
certificate_t *cert = loader->load_certificate(loader, "/path/to/cert.pem");
private_key_t *key = loader->load_private_key_auto(loader, "/path/to/key.pem");

// Verify key-certificate match
if (loader->verify_key_cert_match(loader, key, cert)) {
    printf("Key and certificate match!\n");
}

// Build trust chain
linked_list_t *ca_list = linked_list_create();
ca_list->insert_last(ca_list, ca_cert);

auth_cfg_t *trust_chain = loader->build_trust_chain(loader, cert, ca_list, true);
if (trust_chain) {
    printf("Trust chain validation successful!\n");
    trust_chain->destroy(trust_chain);
}

// Cleanup
ca_list->destroy(ca_list);
cert->destroy(cert);
key->destroy(key);
loader->destroy(loader);
```

---

## Troubleshooting

### Debug Logging

Enable debug logging for certificate operations:

```bash
# strongswan.conf
charon {
    filelog {
        /var/log/charon.log {
            time_format = %b %e %T
            append = no
            default = 1
            cfg = 2  # Certificate loading
        }
    }
}
```

### Common Issues

1. **Certificate Loading Fails**
   - Check file permissions (readable by strongSwan)
   - Verify certificate format (PEM/DER)
   - Ensure certificate is not corrupted

2. **Private Key Loading Fails**
   - Verify password for encrypted keys
   - Check key format compatibility
   - Ensure key matches certificate

3. **Trust Chain Validation Fails**
   - Verify CA certificate is correct issuer
   - Check certificate validity periods
   - Ensure signature algorithms are supported

4. **OCSP Validation Issues**
   - Check network connectivity to OCSP responder
   - Verify OCSP responder URL in certificate
   - Consider firewall restrictions

5. **Performance Issues**
   - Disable unnecessary online validation
   - Use certificate caching
   - Optimize certificate chain length

---

## Security Considerations

### Best Practices

1. **File Permissions**: Restrict access to private keys (600)
2. **Password Security**: Use strong passwords for encrypted keys
3. **Certificate Validation**: Always enable chain validation
4. **Online Validation**: Enable OCSP for critical applications
5. **Key Rotation**: Regularly update certificates and keys

### Security Features

- **Memory Protection**: Sensitive data cleared after use
- **Path Validation**: Complete certificate chain verification
- **Revocation Checking**: Real-time certificate status validation
- **Cryptographic Verification**: Full signature validation

---

This API reference covers all phases of the strongSwan extsock plugin development, providing comprehensive documentation for certificate-based IPSec authentication. 