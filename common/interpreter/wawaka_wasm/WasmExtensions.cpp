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

#include "basic_kv.h"
#include "error.h"
#include "log.h"
#include "pdo_error.h"
#include "types.h"

//#include <stddef.h>   /* size_t */
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "WasmCryptoExtensions.h"
#include "WasmStateExtensions.h"
#include "WasmUtil.h"

namespace pe = pdo::error;

/* ----------------------------------------------------------------- *
 * NAME: _contract_log_wrapper
 * ----------------------------------------------------------------- */
extern "C" bool contract_log_wrapper(
    wasm_exec_env_t exec_env,
    const int32 loglevel,
    const char* buffer)
{
    // wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    try {
        SAFE_LOG(loglevel, "CONTRACT: %s", buffer);
        return true;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "unexpected failure in %s", __FUNCTION__);
        return false;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: _contract_abort_wrapper
 * ----------------------------------------------------------------- */
extern "C" void contract_abort_wrapper(
    wasm_exec_env_t exec_env,
    const char* buffer)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_runtime_set_exception(module_inst, buffer);
}

/* ----------------------------------------------------------------- *
 * NAME: simple_hash
 * ----------------------------------------------------------------- */
extern "C" int simple_hash_wrapper(
    wasm_exec_env_t exec_env,
    uint8_t* buffer,
    const int buffer_length)
{
    // wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    try {
        if (buffer == NULL)
            return -1;

        unsigned int result = 0;
        for (int i = 0; i < buffer_length; i++, buffer++)
        {
            int temp;
            temp = (result << 6) + (result << 16) - result;
            result = (*buffer) + temp;
        }

        return result;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "unexpected failure in %s", __FUNCTION__);
        return -1;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: strtod
 * ----------------------------------------------------------------- */
extern "C" double strtod_wrapper(
    wasm_exec_env_t exec_env,
    const char *nptr,
    char **endptr)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    double num = 0;

    if (! wasm_runtime_validate_native_addr(module_inst, endptr, sizeof(uint32)))
        return 0;

    num = strtod(nptr, endptr);
    *(int32*)endptr = wasm_runtime_addr_native_to_app(module_inst, *endptr);

    return num;
}

extern "C" void abort_wrapper(wasm_exec_env_t exec_env)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_runtime_set_exception(module_inst, "env.abort()");
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#ifdef __cplusplus
extern "C" {
#endif

#define WASM_PASSTHRU_FUNCTION(function) \
    static int function##_wrapper(wasm_module_inst_t m, int c) { return function(c); }

WASM_PASSTHRU_FUNCTION(iscntrl)
WASM_PASSTHRU_FUNCTION(islower)
WASM_PASSTHRU_FUNCTION(ispunct)
WASM_PASSTHRU_FUNCTION(isblank)

static NativeSymbol native_symbols[] =
{
    /* Missing libc functions */
    EXPORT_WASM_API_WITH_SIG2(iscntrl,"(i)i"),
    EXPORT_WASM_API_WITH_SIG2(islower,"(i)i"),
    EXPORT_WASM_API_WITH_SIG2(ispunct,"(i)i"),
    EXPORT_WASM_API_WITH_SIG2(isblank,"(i)i"),
    EXPORT_WASM_API_WITH_SIG2(abort,"()"),

    /* Crypto operations from WasmCryptoExtensions.h */
    EXPORT_WASM_API_WITH_SIG2(b64_encode,"(iiii)i"),
    EXPORT_WASM_API_WITH_SIG2(b64_decode,"(iiii)i"),
    EXPORT_WASM_API_WITH_SIG2(ecdsa_create_signing_keys,"(iiii)i"),
    EXPORT_WASM_API_WITH_SIG2(ecdsa_create_signing_keys_from_extended_key,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(ecdsa_sign_message,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(ecdsa_verify_signature,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(aes_generate_key,"(ii)i"),
    EXPORT_WASM_API_WITH_SIG2(aes_generate_iv,"(iiii)i"),
    EXPORT_WASM_API_WITH_SIG2(aes_encrypt_message,"(iiiiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(aes_decrypt_message,"(iiiiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(rsa_generate_keys,"(iiii)i"),
    EXPORT_WASM_API_WITH_SIG2(rsa_encrypt_message,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(rsa_decrypt_message,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(sha256_hash,"(iiii)i"),
    EXPORT_WASM_API_WITH_SIG2(sha384_hash,"(iiii)i"),
    EXPORT_WASM_API_WITH_SIG2(sha512_hash,"(iiii)i"),
    EXPORT_WASM_API_WITH_SIG2(sha256_hmac,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(sha384_hmac,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(sha512_hmac,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(sha512_pbkd,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(random_identifier,"(ii)i"),
    EXPORT_WASM_API_WITH_SIG2(verify_sgx_report,"(iiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(parse_sgx_report,"(iiii)i"),

    /* Persistent store operations from WasmStateExtensions.h */
    EXPORT_WASM_API_WITH_SIG2(key_value_set,"(i*~*~)i"),
    EXPORT_WASM_API_WITH_SIG2(key_value_get,"(i*~ii)i"),
    EXPORT_WASM_API_WITH_SIG2(privileged_key_value_get,"(*~ii)i"),

    EXPORT_WASM_API_WITH_SIG2(key_value_create,"(*~)i"),
    EXPORT_WASM_API_WITH_SIG2(key_value_open,"(*~*~)i"),
    EXPORT_WASM_API_WITH_SIG2(key_value_finalize,"(iii)i"),

    /* Utility functions */
    EXPORT_WASM_API_WITH_SIG2(contract_log, "(i$)i"),
    EXPORT_WASM_API_WITH_SIG2(contract_abort, "($)"),
    EXPORT_WASM_API_WITH_SIG2(simple_hash, "(*~)i"),
    EXPORT_WASM_API_WITH_SIG2(strtod, "($*)F"),
};

#ifdef __cplusplus
}
#endif

bool InitializeNativeSymbols(RuntimeInitArgs& init_args)
{
    size_t native_symbols_count = sizeof(native_symbols)/sizeof(NativeSymbol);

    init_args.native_module_name = "env";
    init_args.n_native_symbols = native_symbols_count;
    init_args.native_symbols = native_symbols;

    return true;
}

//#include "ext_lib_export.h"
