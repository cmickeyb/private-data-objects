/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>

#include "bh_platform.h"
#include "wasm_export.h"
#include "lib_export.h"

#include "crypto.h"
#include "error.h"
#include "log.h"
#include "pdo_error.h"
#include "types.h"

#include <string.h>
#include <ctype.h>
#include <math.h>

#include "WasmCryptoExtensions.h"
#include "WasmUtil.h"

namespace pe = pdo::error;
namespace pcrypto = pdo::crypto;

/* ----------------------------------------------------------------- *
 * NAME: _b64_encode_wrapper
 * ----------------------------------------------------------------- */
static bool _b64_encode_wrapper(
    wasm_module_inst_t module_inst,
    const int32 dec_buffer_offset,
    const int32 dec_buffer_length,
    int32 enc_buffer_pointer_offset,
    int32 enc_length_pointer_offset)
{
    try {
        uint8_t* dec_buffer = (uint8_t*)get_buffer(module_inst, dec_buffer_offset, dec_buffer_length);
        if (dec_buffer == NULL)
            return false;

        ByteArray src(dec_buffer, dec_buffer + dec_buffer_length);
        Base64EncodedString encoded = ByteArrayToBase64EncodedString(src);
        encoded += '\0';

        if (! save_buffer(module_inst,
                          encoded.c_str(), encoded.length(),
                          enc_buffer_pointer_offset, enc_length_pointer_offset))
            return false;

        return true;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "failed to compute b64 encoding");
        return false;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: _b64_decode_wrapper
 * ----------------------------------------------------------------- */
static bool _b64_decode_wrapper(
    wasm_module_inst_t module_inst,
    const int32 enc_buffer_offset,
    const int32 enc_buffer_length,
    int32 dec_buffer_pointer_offset,
    int32 dec_length_pointer_offset)
{
    try {
        uint8_t* enc_buffer = (uint8_t*)get_buffer(module_inst, enc_buffer_offset, enc_buffer_length);
        if (enc_buffer == NULL)
            return false;

        Base64EncodedString encoded(enc_buffer, enc_buffer + enc_buffer_length);
        ByteArray decoded = Base64EncodedStringToByteArray(encoded);
        if (decoded.size() == 0)
            return false;

        if (! save_buffer(module_inst,
                          (const char*)decoded.data(), decoded.size(),
                          dec_buffer_pointer_offset, dec_length_pointer_offset))
            return false;

        return true;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "failed to compute b64 encoding");
        return false;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: _ecdsa_create_signing_keys
 * ----------------------------------------------------------------- */
static bool _ecdsa_create_signing_keys_wrapper(
    wasm_module_inst_t module_inst,
    const int32 private_buffer_pointer_offset,
    const int32 private_length_pointer_offset,
    const int32 public_buffer_pointer_offset,
    const int32 public_length_pointer_offset)
{
    try {
        pcrypto::sig::PrivateKey privkey;
        privkey.Generate();
        pcrypto::sig::PublicKey pubkey(privkey);

        std::string encpriv = privkey.Serialize();
        encpriv += '\0';        // add a null terminator since this is a char*

        std::string encpub = pubkey.Serialize();
        encpub += '\0';         // add a null terminator since this is a char*

        if (! save_buffer(module_inst,
                          encpriv.c_str(), encpriv.length(),
                          private_buffer_pointer_offset, private_length_pointer_offset))
            return false;

        if (! save_buffer(module_inst,
                          encpub.c_str(), encpub.length(),
                          public_buffer_pointer_offset, public_length_pointer_offset))
            return false;

        return true;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "failed to generate ecdsa key");
        return false;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: _ecdsa_sign_message_wrapper
 * ----------------------------------------------------------------- */
static bool _ecdsa_sign_message_wrapper(
    wasm_module_inst_t module_inst,
    const int32 msg_buffer_offset,
    const int32 msg_length,
    const int32 key_buffer_offset,
    const int32 key_length,
    int32 sig_buffer_pointer_offset,
    int32 sig_length_pointer_offset)
{
    try {
        const uint8_t* msg_buffer = (uint8_t*)get_buffer(module_inst, msg_buffer_offset, msg_length);
        if (msg_buffer == NULL)
            return false;

        const char* key_buffer = (const char*)get_buffer(module_inst, key_buffer_offset, key_length);
        if (key_buffer == NULL)
            return false;

        ByteArray msg(msg_buffer, msg_buffer + msg_length);
        std::string key(key_buffer, key_length);

        pcrypto::sig::PrivateKey privkey(key);
        ByteArray signature = privkey.SignMessage(msg);

        if (! save_buffer(module_inst,
                          (const char*)signature.data(), signature.size(),
                          sig_buffer_pointer_offset, sig_length_pointer_offset))
            return false;

        return true;
    }
    catch (pdo::error::Error& e) {
        SAFE_LOG(PDO_LOG_ERROR, "_ecdsa_sign_message_wrapper failed; %s", e.what());
        return false;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "_ecdsa_sign_message_wrapper failed");
        return false;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: _ecdsa_verify_signature_wrapper
 * ----------------------------------------------------------------- */
bool _ecdsa_verify_signature_wrapper(
    wasm_module_inst_t module_inst,
    const int32 msg_buffer_offset,
    const int32 msg_length,
    const int32 key_buffer_offset,
    const int32 key_length,
    const int32 sig_buffer_offset,
    const int32 sig_length)
{
    try {
        const uint8_t* msg_buffer = (uint8_t*)get_buffer(module_inst, msg_buffer_offset, msg_length);
        if (msg_buffer == NULL)
            return false;

        const char* key_buffer = (const char*)get_buffer(module_inst, key_buffer_offset, key_length);
        if (key_buffer == NULL)
            return false;

        const uint8_t* sig_buffer = (uint8_t*)get_buffer(module_inst, sig_buffer_offset, sig_length);
        if (key_buffer == NULL)
            return false;

        ByteArray msg(msg_buffer, msg_buffer + msg_length);
        std::string key(key_buffer, key_length);
        ByteArray signature(sig_buffer, sig_buffer + sig_length);

        pcrypto::sig::PublicKey pubkey(key);
        return pubkey.VerifySignature(msg, signature);
    }
    catch (pdo::error::Error& e) {
        SAFE_LOG(PDO_LOG_ERROR, "_ecdsa_verify_signature_wrapper failed; %s", e.what());
        return false;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "_ecdsa_verify_signature_wrapper failed");
        return false;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: _crypto_hash
 * ----------------------------------------------------------------- */
static bool _crypto_hash_wrapper(
    wasm_module_inst_t module_inst,
    const int32 msg_buffer_offset,
    const int32 msg_buffer_length,
    int32 hash_buffer_pointer_offset,
    int32 hash_length_pointer_offset)
{
    try {
        uint8_t* msg_buffer = (uint8_t*)get_buffer(module_inst, msg_buffer_offset, msg_buffer_length);
        if (msg_buffer == NULL)
            return false;

        ByteArray msg(msg_buffer, msg_buffer + msg_buffer_length);
        ByteArray hash = pcrypto::ComputeMessageHash(msg);
        if (hash.size() == 0)
            return false;

        if (! save_buffer(module_inst,
                          (const char*)hash.data(), hash.size(),
                          hash_buffer_pointer_offset, hash_length_pointer_offset))
            return false;

        return true;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "failed to compute cryptographic hash");
        return false;
    }
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#ifdef __cplusplus
extern "C" {
#endif

bool b64_encode_wrapper(
    wasm_module_inst_t module_inst,
    const int32 dec_buffer_offset,
    const int32 dec_buffer_length,
    int32 enc_buffer_pointer_offset,
    int32 enc_length_pointer_offset)
{
    return _b64_encode_wrapper(
        module_inst,
        dec_buffer_offset,
        dec_buffer_length,
        enc_buffer_pointer_offset,
        enc_length_pointer_offset);
}

bool b64_decode_wrapper(
    wasm_module_inst_t module_inst,
    const int32 enc_buffer_offset,
    const int32 enc_buffer_length,
    int32 dec_buffer_pointer_offset,
    int32 dec_length_pointer_offset)
{
    return _b64_decode_wrapper(
        module_inst,
        enc_buffer_offset,
        enc_buffer_length,
        dec_buffer_pointer_offset,
        dec_length_pointer_offset);
}

bool ecdsa_create_signing_keys_wrapper(
    wasm_module_inst_t module_inst,
    const int32 private_buffer_pointer_offset,
    const int32 private_length_pointer_offset,
    const int32 public_buffer_pointer_offset,
    const int32 public_length_pointer_offset)
{
    return _ecdsa_create_signing_keys_wrapper(
        module_inst,
        private_buffer_pointer_offset,
        private_length_pointer_offset,
        public_buffer_pointer_offset,
        public_length_pointer_offset);
}

bool ecdsa_sign_message_wrapper(
    wasm_module_inst_t module_inst,
    const int32 msg_buffer_offset,
    const int32 msg_length,
    const int32 key_buffer_offset,
    const int32 key_length,
    int32 sig_buffer_pointer_offset,
    int32 sig_length_pointer_offset)
{
    return  _ecdsa_sign_message_wrapper(
        module_inst,
        msg_buffer_offset,
        msg_length,
        key_buffer_offset,
        key_length,
        sig_buffer_pointer_offset,
        sig_length_pointer_offset);
}

bool ecdsa_verify_signature_wrapper(
    wasm_module_inst_t module_inst,
    const int32 msg_buffer_offset,
    const int32 msg_length,
    const int32 key_buffer_offset,
    const int32 key_length,
    const int32 sig_buffer_offset,
    const int32 sig_length)
{
    return _ecdsa_verify_signature_wrapper(
        module_inst,
        msg_buffer_offset,
        msg_length,
        key_buffer_offset,
        key_length,
        sig_buffer_offset,
        sig_length);
}

bool crypto_hash_wrapper(
    wasm_module_inst_t module_inst,
    const int32 msg_buffer_offset,
    const int32 msg_buffer_length,
    int32 hash_buffer_pointer_offset,
    int32 hash_length_pointer_offset)
{
    return _crypto_hash_wrapper(
        module_inst,
        msg_buffer_offset,
        msg_buffer_length,
        hash_buffer_pointer_offset,
        hash_length_pointer_offset);
}

#ifdef __cplusplus
}
#endif
