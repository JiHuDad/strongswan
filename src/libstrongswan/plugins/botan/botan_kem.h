/*
 * Copyright (C) 2024 Tobias Brunner, codelabs GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * Implements key encapsulation methods (KEM) using Botan.
 *
 * @defgroup botan_kem botan_kem
 * @{ @ingroup botan_p
 */

#ifndef BOTAN_KEM_H_
#define BOTAN_KEM_H_

#include <crypto/key_exchange.h>

/**
 * Creates a new key_exchange_t object.
 *
 * @param method	KEM to instantiate
 * @return			key_exchange_t object, NULL if not supported
 */
key_exchange_t *botan_kem_create(key_exchange_method_t method);

#endif /** BOTAN_KEM_H_ @}*/
