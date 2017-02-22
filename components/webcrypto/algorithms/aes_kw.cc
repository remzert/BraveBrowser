// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/numerics/safe_math.h"
#include "components/webcrypto/algorithms/aes.h"
#include "components/webcrypto/blink_key_handle.h"
#include "components/webcrypto/crypto_data.h"
#include "components/webcrypto/status.h"
#include "crypto/openssl_util.h"
#include "third_party/boringssl/src/include/openssl/aes.h"
#include <openssl/mem.h>

/* kDefaultIV is the default IV value given in RFC 3394, 2.2.3.1. */
static const uint8_t kDefaultIV[] = {
    0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6,
};

static const unsigned kBound = 6;

int AES_wrap_key(const AES_KEY *key, const uint8_t *iv, uint8_t *out,
                 const uint8_t *in, size_t in_len) {
  /* See RFC 3394, section 2.2.1. */

  if (in_len > INT_MAX - 8 || in_len < 8 || in_len % 8 != 0) {
    return -1;
  }

  if (iv == NULL) {
    iv = kDefaultIV;
  }

  memmove(out + 8, in, in_len);
  uint8_t A[AES_BLOCK_SIZE];
  memcpy(A, iv, 8);

  size_t n = in_len / 8;

  for (unsigned j = 0; j < kBound; j++) {
    for (size_t i = 1; i <= n; i++) {
      memcpy(A + 8, out + 8 * i, 8);
      AES_encrypt(A, A, key);

      uint32_t t = (uint32_t)(n * j + i);
      A[7] ^= t & 0xff;
      A[6] ^= (t >> 8) & 0xff;
      A[5] ^= (t >> 16) & 0xff;
      A[4] ^= (t >> 24) & 0xff;
      memcpy(out + 8 * i, A + 8, 8);
    }
  }

  memcpy(out, A, 8);
  return (int)in_len + 8;
}

int AES_unwrap_key(const AES_KEY *key, const uint8_t *iv, uint8_t *out,
                   const uint8_t *in, size_t in_len) {
  /* See RFC 3394, section 2.2.2. */

  if (in_len > INT_MAX || in_len < 16 || in_len % 8 != 0) {
    return -1;
  }

  if (iv == NULL) {
    iv = kDefaultIV;
  }

  uint8_t A[AES_BLOCK_SIZE];
  memcpy(A, in, 8);
  memmove(out, in + 8, in_len - 8);

  size_t n = (in_len / 8) - 1;

  for (unsigned j = kBound - 1; j < kBound; j--) {
    for (size_t i = n; i > 0; i--) {
      uint32_t t = (uint32_t)(n * j + i);
      A[7] ^= t & 0xff;
      A[6] ^= (t >> 8) & 0xff;
      A[5] ^= (t >> 16) & 0xff;
      A[4] ^= (t >> 24) & 0xff;
      memcpy(A + 8, out + 8 * (i - 1), 8);
      AES_decrypt(A, A, key);
      memcpy(out + 8 * (i - 1), A + 8, 8);
    }
  }

  if (CRYPTO_memcmp(A, iv, 8) != 0) {
    return -1;
  }

  return (int)in_len - 8;
}

namespace webcrypto {

namespace {

class AesKwImplementation : public AesAlgorithm {
 public:
  AesKwImplementation()
      : AesAlgorithm(
            blink::WebCryptoKeyUsageWrapKey | blink::WebCryptoKeyUsageUnwrapKey,
            "KW") {}

  Status Encrypt(const blink::WebCryptoAlgorithm& algorithm,
                 const blink::WebCryptoKey& key,
                 const CryptoData& data,
                 std::vector<uint8_t>* buffer) const override {
    crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);

    // These length checks are done in order to give a more specific
    // error. These are not required for correctness.
    if (data.byte_length() < 16)
      return Status::ErrorDataTooSmall();
    if (data.byte_length() % 8)
      return Status::ErrorInvalidAesKwDataLength();

    // Key import validates key sizes, so the bits computation will not
    // overflow.
    const std::vector<uint8_t>& raw_key = GetSymmetricKeyData(key);
    AES_KEY aes_key;
    if (AES_set_encrypt_key(raw_key.data(),
                            static_cast<unsigned>(raw_key.size() * 8),
                            &aes_key) < 0) {
      return Status::OperationError();
    }

    // Key wrap's overhead is 8 bytes.
    base::CheckedNumeric<size_t> length(data.byte_length());
    length += 8;
    if (!length.IsValid())
      return Status::ErrorDataTooLarge();

    buffer->resize(length.ValueOrDie());
    if (AES_wrap_key(&aes_key, nullptr /* default IV */, buffer->data(),
                     data.bytes(), data.byte_length()) < 0) {
      return Status::OperationError();
    }

    return Status::Success();
  }

  Status Decrypt(const blink::WebCryptoAlgorithm& algorithm,
                 const blink::WebCryptoKey& key,
                 const CryptoData& data,
                 std::vector<uint8_t>* buffer) const override {
    crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);

    // These length checks are done in order to give a more specific
    // error. These are not required for correctness.
    if (data.byte_length() < 24)
      return Status::ErrorDataTooSmall();
    if (data.byte_length() % 8)
      return Status::ErrorInvalidAesKwDataLength();

    // Key import validates key sizes, so the bits computation will not
    // overflow.
    const std::vector<uint8_t>& raw_key = GetSymmetricKeyData(key);
    AES_KEY aes_key;
    if (AES_set_decrypt_key(raw_key.data(),
                            static_cast<unsigned>(raw_key.size() * 8),
                            &aes_key) < 0) {
      return Status::OperationError();
    }

    // Key wrap's overhead is 8 bytes.
    buffer->resize(data.byte_length() - 8);

    if (AES_unwrap_key(&aes_key, nullptr /* default IV */, buffer->data(),
                       data.bytes(), data.byte_length()) < 0) {
      return Status::OperationError();
    }

    return Status::Success();
  }
};

}  // namespace

std::unique_ptr<AlgorithmImplementation> CreateAesKwImplementation() {
  return base::WrapUnique(new AesKwImplementation);
}

}  // namespace webcrypto
