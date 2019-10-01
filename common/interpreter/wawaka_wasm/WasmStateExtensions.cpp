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

#include "WasmStateExtensions.h"
#include "WasmUtil.h"

namespace pe = pdo::error;
namespace pstate = pdo::state;

/* ----------------------------------------------------------------- *
 * NAME: _key_value_set_wrapper
 * ----------------------------------------------------------------- */
static bool _key_value_set_wrapper(
    wasm_module_inst_t module_inst,
    const int32 key_buffer_offset,
    const int32 key_buffer_length,
    const int32 val_buffer_offset,
    const int32 val_buffer_length)
{
    try {
        pstate::Basic_KV_Plus* state = (pstate::Basic_KV_Plus*)wasm_runtime_get_instance_data(module_inst);
        if (state == NULL)
        {
            SAFE_LOG(PDO_LOG_ERROR, "state was not initialized");
            return false;
        }

        uint8_t* key_buffer = (uint8_t*)get_buffer(module_inst, key_buffer_offset, key_buffer_length);
        if (key_buffer == NULL)
            return false;

        uint8_t* val_buffer = (uint8_t*)get_buffer(module_inst, val_buffer_offset, val_buffer_length);
        if (val_buffer == NULL)
            return false;

        ByteArray ba_key(key_buffer, key_buffer + key_buffer_length);
        ByteArray ba_val(val_buffer, val_buffer + val_buffer_length);

        state->UnprivilegedPut(ba_key, ba_val);
        return true;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "failed to save the key");
        return false;
    }
}

/* ----------------------------------------------------------------- *
 * NAME: _key_value_get_wrapper
 * ----------------------------------------------------------------- */
static bool _key_value_get_wrapper(
    wasm_module_inst_t module_inst,
    const int32 key_buffer_offset,
    const int32 key_buffer_length,
    int32 val_buffer_pointer_offset,
    int32 val_length_pointer_offset)
{
    try {
        pstate::Basic_KV_Plus* state = (pstate::Basic_KV_Plus*)wasm_runtime_get_instance_data(module_inst);
        if (state == NULL)
        {
            SAFE_LOG(PDO_LOG_ERROR, "state was not initialized");
            return false;
        }

        uint8_t* key_buffer = (uint8_t*)get_buffer(module_inst, key_buffer_offset, key_buffer_length);
        if (key_buffer == NULL)
            return false;

        ByteArray ba_key(key_buffer, key_buffer + key_buffer_length);
        ByteArray ba_val = state->UnprivilegedGet(ba_key);

        if (ba_val.size() == 0)
            return false;

        if (! save_buffer(module_inst,
                          (const char*)ba_val.data(), ba_val.size(),
                          val_buffer_pointer_offset, val_length_pointer_offset))
            return false;

        return true;
    }
    catch (...) {
        SAFE_LOG(PDO_LOG_ERROR, "failed to retrieve key value");
        return false;
    }
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#ifdef __cplusplus
extern "C" {
#endif

bool key_value_set_wrapper(
    wasm_module_inst_t module_inst,
    const int32 key_buffer_offset,
    const int32 key_buffer_length,
    const int32 val_buffer_offset,
    const int32 val_buffer_length)
{
    return _key_value_set_wrapper(
        module_inst,
        key_buffer_offset,
        key_buffer_length,
        val_buffer_offset,
        val_buffer_length);
}

bool key_value_get_wrapper(
    wasm_module_inst_t module_inst,
    const int32 key_buffer_offset,
    const int32 key_buffer_length,
    int32 val_buffer_pointer_offset,
    int32 val_length_pointer_offset)
{
    return _key_value_get_wrapper(
        module_inst,
        key_buffer_offset,
        key_buffer_length,
        val_buffer_pointer_offset,
        val_length_pointer_offset);
}

#ifdef __cplusplus
}
#endif
