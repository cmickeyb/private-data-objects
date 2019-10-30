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

#include <malloc.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "Dispatch.h"

#include "KeyValue.h"
#include "Environment.h"
#include "Message.h"
#include "Response.h"
#include "StringArray.h"
#include "Value.h"
#include "WasmExtensions.h"

static KeyValueStore meta_store("values");
static KeyValueStore approved_keys("keys");

const StringArray md_owner_key("owner");
const StringArray md_signing_key("ecdsa-private-key");
const StringArray md_verifying_key("ecdsa-public-key");
const StringArray md_asset_type_id_key("asset-type-id");
const StringArray md_initialized_key("initialized");

// -----------------------------------------------------------------
// METHOD: initialize
//   contract initialization method
//
// JSON PARAMETERS:
//   none
//
// RETURNS:
//   true if successfully initialized
// -----------------------------------------------------------------
bool initialize(const Message& msg, const Environment& env, Response& rsp)
{
    // ---------- Mark as uninitialized ----------
    if (! meta_store.set(md_initialized_key, 0))
        return rsp.error("failed to save initialization state");

    // ---------- Save owner information ----------
    const StringArray owner_val(env.creator_id_);

    if (! meta_store.set(md_owner_key, owner_val))
        return rsp.error("failed to save creator metadata");

    // ---------- Create and save the ECDSA key pair ----------
    StringArray public_key;
    StringArray private_key;

    if (! ecdsa_create_signing_keys((char**)&private_key.value_, &private_key.size_,
                                    (char**)&public_key.value_, &public_key.size_))
        return rsp.error("failed to create contract ecdsa keys");

    if (! meta_store.set(md_verifying_key, public_key))
        return rsp.error("failed to save ecdsa public key");

    if (! meta_store.set(md_signing_key, private_key))
        return rsp.error("failed to save ecdsa private key");

    // ---------- RETURN ----------
    return rsp.success(true);
}

// -----------------------------------------------------------------
// METHOD: set_asset_type_id
//
// JSON PARAMETERS:
//   asset-type-id -- ecdsa public key for the asset type
//
// RETURNS:
//   true if asset type id successfully saved
// -----------------------------------------------------------------
bool set_asset_type_id(const Message& msg, const Environment& env, Response& rsp)
{
    // Only the owner can set the asset type id
    if (strcmp(env.creator_id_, env.message_id_) != 0)
        return rsp.error("only the owner may set the asset type identifier");

    // Cannot change the asset type id once it has been initialized
    uint32_t initialized = 0;
    if (! meta_store.get(md_initialized_key, initialized))
        return rsp.error("contract state corrupted, no initialized key");

    if (initialized != 0)
        return rsp.error("asset type is already set");

    // Set the asset type
    const StringArray asset_type_id(msg.get_string("asset-type-id"));

    // would be good to make sure that this is a valid ECDSA key
    if (! meta_store.set(md_asset_type_id_key, asset_type_id))
        return rsp.error("failed to store the asset type id");

    // Mark as initialized
    initialized = 1;
    if (! meta_store.set(md_initialized_key, initialized))
        return rsp.error("failed to store the initialized key");

    // ---------- RETURN ----------
    return rsp.success(true);
}

// -----------------------------------------------------------------
// METHOD: get_asset_type_id
//
// JSON PARAMETERS:
//   none
//
// RETURNS:
//   asset type id as a string
// -----------------------------------------------------------------
bool get_asset_type_id(const Message& msg, const Environment& env, Response& rsp)
{
    // Cannot get the asset type id unless it has been initialized
    uint32_t initialized = 0;
    if (! meta_store.get(md_initialized_key, initialized))
        return rsp.error("contract state corrupted, no initialized key");

    if (initialized == 0)
        return rsp.error("asset type is not set");

    // Set the asset type
    StringArray asset_type_id;

    // would be good to make sure that this is a valid ECDSA key
    if (! meta_store.get(md_asset_type_id_key, asset_type_id))
        return rsp.error("failed to retrieve the asset type id");

    // ---------- RETURN ----------
    Value v((char*)asset_type_id.value_);
    return rsp.value(v, false);
}

// -----------------------------------------------------------------
// METHOD: add_approved_issuer
//
// JSON PARAMETERS:
//   verifying-key -- verifying key of the asset issuer
//
// RETURNS:
//   true if key is successfully stored
// -----------------------------------------------------------------
bool add_approved_issuer(const Message& msg, const Environment& env, Response& rsp)
{
    // Only the owner can perform this operation
    if (strcmp(env.creator_id_,env.message_id_) != 0)
        return rsp.error("only the owner may set the asset type identifier");

    // Make sure the contract is initialized
    uint32_t initialized = 0;
    if (! meta_store.get(md_initialized_key, initialized))
        return rsp.error("contract state corrupted, no initialized key");

    if (initialized == 0)
        return rsp.error("asset type is not set");

    // Save the issuer's key
    StringArray verifying_key(msg.get_string("verifying-key"));
    // would be good to make sure that this is a valid ECDSA key

    if (! approved_keys.set(verifying_key, 1))
        return rsp.error("failed to save the verifying key");

    // ---------- RETURN ----------
    return rsp.success(true);
}

// -----------------------------------------------------------------
// METHOD: get_issuer_authority
//
// JSON PARAMETERS:
//   verifying-key -- verifying key of the asset issuer
//
// RETURNS:
//   serialized authority object
// -----------------------------------------------------------------
bool get_issuer_authority(const Message& msg, const Environment& env, Response& rsp)
{
    return rsp.error("not implemented");
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
contract_method_reference_t contract_method_dispatch_table[] = {
    CONTRACT_METHOD(initialize),
    CONTRACT_METHOD(set_asset_type_id),
    CONTRACT_METHOD(get_asset_type_id),
    CONTRACT_METHOD(add_approved_issuer),
    CONTRACT_METHOD(get_issuer_authority),
    { NULL, NULL }
};
