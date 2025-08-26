/*
 * Copyright (C) 2024 strongSwan Project
 * 
 * Pure unit test error definitions (without strongSwan dependencies)
 * This file provides error handling functions for Level 1 Pure tests
 * that don't require strongSwan library dependencies.
 */

#ifndef EXTSOCK_ERRORS_PURE_H_
#define EXTSOCK_ERRORS_PURE_H_

#include "extsock_types_pure.h"

/**
 * 에러 정보 객체 생성 (Pure)
 */
extsock_error_info_t *extsock_error_create(extsock_error_t code, const char *message);

/**
 * 에러 정보 객체 소멸 (Pure)
 */
void extsock_error_destroy(extsock_error_info_t *error_info);

/**
 * 에러 코드를 문자열로 변환 (Pure)
 */
const char *extsock_error_to_string(extsock_error_t error);

#endif /* EXTSOCK_ERRORS_PURE_H_ */