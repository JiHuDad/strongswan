/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_cert_loader extsock_cert_loader
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_CERT_LOADER_H_
#define EXTSOCK_CERT_LOADER_H_

#include "../../common/extsock_common.h"
#include <credentials/certificates/certificate.h>
#include <credentials/keys/private_key.h>

typedef struct extsock_cert_loader_t extsock_cert_loader_t;

/**
 * 인증서 및 개인키 로딩을 담당하는 인터페이스
 */
struct extsock_cert_loader_t {

    /**
     * 인증서 파일 로딩 (PEM/DER 자동 감지)
     * 
     * @param this          인스턴스
     * @param file_path     인증서 파일 경로
     * @return              certificate_t*, NULL if failed
     */
    certificate_t* (*load_certificate)(extsock_cert_loader_t *this, 
                                       const char *file_path);
    
    /**
     * 개인키 파일 로딩 (암호화된 키 지원)
     * 
     * @param this          인스턴스
     * @param file_path     개인키 파일 경로
     * @param passphrase    암호화 패스워드 (NULL이면 암호화 안됨)
     * @return              private_key_t*, NULL if failed
     */
    private_key_t* (*load_private_key)(extsock_cert_loader_t *this,
                                       const char *file_path, 
                                       const char *passphrase);
    
    /**
     * 인증서 체인 검증
     * 
     * @param this          인스턴스
     * @param cert          검증할 인증서
     * @param ca_cert       CA 인증서
     * @return              TRUE if valid chain
     */
    bool (*verify_certificate_chain)(extsock_cert_loader_t *this,
                                     certificate_t *cert, 
                                     certificate_t *ca_cert);
    
    /**
     * 인증서와 개인키 매칭 확인
     * 
     * @param this          인스턴스
     * @param key           개인키
     * @param cert          인증서
     * @return              TRUE if matching
     */
    bool (*verify_key_cert_match)(extsock_cert_loader_t *this,
                                  private_key_t *key, 
                                  certificate_t *cert);
    
    /**
     * 인스턴스 해제
     */
    void (*destroy)(extsock_cert_loader_t *this);
};

/**
 * 인증서 로더 인스턴스 생성
 * 
 * @return          extsock_cert_loader_t instance
 */
extsock_cert_loader_t *extsock_cert_loader_create();

#endif /** EXTSOCK_CERT_LOADER_H_ @}*/ 