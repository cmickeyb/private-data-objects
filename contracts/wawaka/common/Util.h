/* Copyright 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#define ASSERT_SENDER_IS_OWNER(_env, _rsp)                              \
    if (strcmp(_env.creator_id_, _env.message_id_) != 0)                \
        return _rsp.error("only the owner may set the asset type identifier");

#define ASSERT_INITIALIZED(_store, _key, _rsp)                          \
    {                                                                   \
    uint32_t _initialized = 0;                                          \
    if (! _store.get(_key, _initialized))                               \
        return _rsp.error("contract state corrupted, no initialized key"); \
                                                                        \
    if (_initialized == 0)                                              \
        return _rsp.error("contract is not initialized");               \
    }

#define ASSERT_UNINITIALIZED(_store, _key, _rsp)                        \
    {                                                                   \
    uint32_t _initialized = 0;                                          \
    if (! _store.get(_key, _initialized))                               \
        return _rsp.error("contract state corrupted, no initialized key"); \
                                                                        \
    if (_initialized != 0)                                              \
        return _rsp.error("contract is already initialized");           \
    }
