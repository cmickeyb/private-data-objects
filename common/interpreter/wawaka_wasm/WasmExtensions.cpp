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
static bool _contract_log_wrapper(
    wasm_module_inst_t module_inst,
    const int32 loglevel,
    const int32 buffer_offset)
{
    try {
        if (! wasm_runtime_validate_app_addr(module_inst, buffer_offset, 0))
        {
            SAFE_LOG(PDO_LOG_INFO, "invalid address passed as key");
            return false;
        }

        const char* buffer = (char*)wasm_runtime_addr_app_to_native(module_inst, buffer_offset);
        SAFE_LOG(loglevel, "CONTRACT: %s", buffer);

        return true;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "failed in contract_log");
        return false;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: simple_hash
 * ----------------------------------------------------------------- */
static int _simple_hash_wrapper(
    wasm_module_inst_t module_inst,
    int32 buffer_offset,
    const int32 buffer_length)
{
    try {
        uint8_t *buffer = (uint8_t*)get_buffer(module_inst, buffer_offset, buffer_length);
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
        SAFE_LOG(PDO_LOG_ERROR, "failed to compute simple hash");
        return -1;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: memchr
 * ----------------------------------------------------------------- */
static int32 _memchr_wrapper(
    wasm_module_inst_t module_inst,
    int32 src_offset,
    int32 ch,
    uint32 src_size)
{
    try {
        if (src_size == 0)
            return 0;

        void *src = get_buffer(module_inst, src_offset, src_size);
        if (src == NULL)
            return 0;

        void *ptr = memchr(src, ch, src_size);
        if (ptr == NULL)
            return 0;

        return wasm_runtime_addr_native_to_app(module_inst, ptr);
    }

    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "memchr failed");
        return 0;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: strtod
 * ----------------------------------------------------------------- */
static double _strtod_wrapper(
    wasm_module_inst_t module_inst,
    int32 src_offset,
    int32 endptr_offset)
{
    try {
        /* could figure out the minimum length */
        if (! wasm_runtime_validate_app_addr(module_inst, src_offset, 1))
            return 0;

        if (! wasm_runtime_validate_app_addr(module_inst, endptr_offset, 1))
            return 0;

        char *src = (char*)wasm_runtime_addr_app_to_native(module_inst, src_offset);
        char *end;

        double value = strtod(src, &end);
        if (endptr_offset != 0)
        {
            int32* endptr = (int32*)wasm_runtime_addr_app_to_native(module_inst, endptr_offset);
            if (! wasm_runtime_validate_app_addr(module_inst, (*endptr), 1))
                return 0;

            (*endptr) = wasm_runtime_addr_native_to_app(module_inst, end);
        }

        return value;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "strtod failed");
        return 0;
    }
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#ifdef __cplusplus
extern "C" {
#endif

static bool contract_log_wrapper(
    wasm_module_inst_t module_inst,
    const int32 loglevel,
    const int32 buffer_offset)
{
    return _contract_log_wrapper(module_inst, loglevel, buffer_offset);
}

static int simple_hash_wrapper(
    wasm_module_inst_t module_inst,
    int32 buffer_offset,
    const int32 buflen)
{
    return _simple_hash_wrapper(module_inst, buffer_offset, buflen);
}

static int32 memchr_wrapper(
    wasm_module_inst_t module_inst,
    int32 src_offset,
    int32 ch,
    uint32 size)
{
    return _memchr_wrapper(module_inst, src_offset, ch, size);
}

static double strtod_wrapper(
    wasm_module_inst_t module_inst,
    int32 src_offset,
    int32 endptr_offset)
{
    return _strtod_wrapper(module_inst, src_offset, endptr_offset);
}

#define CPP_EXPORT_WASM_API(symbol)  {#symbol, (void*)symbol}
#define CPP_EXPORT_WASM_API2(symbol) {#symbol, (void*)symbol##_wrapper}

#define WASM_PASSTHRU_FUNCTION(function) \
    static int function##_wrapper(wasm_module_inst_t m, int c) { return function(c); }

WASM_PASSTHRU_FUNCTION(isalnum)
WASM_PASSTHRU_FUNCTION(isalpha)
WASM_PASSTHRU_FUNCTION(iscntrl)
WASM_PASSTHRU_FUNCTION(isdigit)
WASM_PASSTHRU_FUNCTION(isgraph)
WASM_PASSTHRU_FUNCTION(islower)
WASM_PASSTHRU_FUNCTION(isprint)
WASM_PASSTHRU_FUNCTION(ispunct)
WASM_PASSTHRU_FUNCTION(isspace)
WASM_PASSTHRU_FUNCTION(isupper)
WASM_PASSTHRU_FUNCTION(isxdigit)
WASM_PASSTHRU_FUNCTION(isblank)

#if 0
WASM_PASSTHRU_FUNCTION(isascii)
#endif

static NativeSymbol extended_native_symbol_defs[] = {
    CPP_EXPORT_WASM_API2(isalnum),
    CPP_EXPORT_WASM_API2(isalpha),
    CPP_EXPORT_WASM_API2(iscntrl),
    CPP_EXPORT_WASM_API2(isdigit),
    CPP_EXPORT_WASM_API2(isgraph),
    CPP_EXPORT_WASM_API2(islower),
    CPP_EXPORT_WASM_API2(isprint),
    CPP_EXPORT_WASM_API2(ispunct),
    CPP_EXPORT_WASM_API2(isspace),
    CPP_EXPORT_WASM_API2(isupper),
    CPP_EXPORT_WASM_API2(isxdigit),
#if 0
    CPP_EXPORT_WASM_API2(isascii),
#endif
    /* from WasmCryptoExtensions.h */
    CPP_EXPORT_WASM_API2(b64_encode),
    CPP_EXPORT_WASM_API2(b64_decode),
    CPP_EXPORT_WASM_API2(ecdsa_create_signing_keys),
    CPP_EXPORT_WASM_API2(ecdsa_sign_message),
    CPP_EXPORT_WASM_API2(ecdsa_verify_signature),
    CPP_EXPORT_WASM_API2(crypto_hash),

    /* from WasmStateExtensions.h */
    CPP_EXPORT_WASM_API2(key_value_set),
    CPP_EXPORT_WASM_API2(key_value_get),

    CPP_EXPORT_WASM_API2(isblank),

    CPP_EXPORT_WASM_API2(contract_log),
    CPP_EXPORT_WASM_API2(simple_hash),
    CPP_EXPORT_WASM_API2(memchr),
    CPP_EXPORT_WASM_API2(strtod)
};

#ifdef __cplusplus
}
#endif

#include "ext_lib_export.h"
