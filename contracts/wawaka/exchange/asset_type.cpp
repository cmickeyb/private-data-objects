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

const StringArray md_owner_key("owner");
const StringArray md_initialized_key("initialized");

const StringArray md_description_key("description");
const StringArray md_link_key("link");
const StringArray md_name_key("owner");

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

    // ---------- RETURN ----------
    return rsp.success(true);
}

// -----------------------------------------------------------------
// METHOD: set_meta_information
//   set the basic information for the asset type
//
// JSON PARAMETERS:
//   description -- string description of the type
//   link -- URL for more information
//   name -- short handle for the asset type
//
// RETURNS:
//   true if successfully initialized
// -----------------------------------------------------------------
bool set_meta_information(const Message& msg, const Environment& env, Response& rsp)
{
    // Only the owner can set the asset type id
    if (strcmp(env.creator_id_, env.message_id_) != 0)
        return rsp.error("only the owner may set asset type meta information");

    // Cannot change the asset type id once it has been initialized
    uint32_t initialized = 0;
    if (! meta_store.get(md_initialized_key, initialized))
        return rsp.error("contract state corrupted, no initialized key");

    if (initialized != 0)
        return rsp.error("asset type meta information already set");


    // save the description
    const StringArray description(msg.get_string("description"));
    if (! meta_store.set(md_description_key, description))
        return rsp.error("failed to store the description");

    // save the link
    const StringArray link(msg.get_string("link"));
    if (! meta_store.set(md_link_key, link))
        return rsp.error("failed to store the link");

    // save the name
    const StringArray name(msg.get_string("name"));
    if (! meta_store.set(md_name_key, name))
        return rsp.error("failed to store the name");

    // Mark as initialized
    initialized = 1;
    if (! meta_store.set(md_initialized_key, initialized))
        return rsp.error("failed to store the initialized key");

    // ---------- RETURN ----------
    return rsp.success(true);
}

// -----------------------------------------------------------------
// METHOD: get_identifier
//   set asset type identifier
//
// JSON PARAMETERS:
//   None
//
// RETURNS:
//   asset type identifier (the contract id)
// -----------------------------------------------------------------
bool get_identifier(const Message& msg, const Environment& env, Response& rsp)
{
    // Cannot retrieve information until the asset type is initialized
    uint32_t initialized = 0;
    if (! meta_store.get(md_initialized_key, initialized))
        return rsp.error("contract state corrupted, no initialized key");

    if (initialized == 0)
        return rsp.error("asset type meta information is not set");

    // save the description
    Value v(env.contract_id_);
    return rsp.value(v, false);
}

// -----------------------------------------------------------------
// METHOD: get_description
//   set asset type description
//
// JSON PARAMETERS:
//   None
//
// RETURNS:
//   asset type description
// -----------------------------------------------------------------
bool get_description(const Message& msg, const Environment& env, Response& rsp)
{
    // Cannot retrieve information until the asset type is initialized
    uint32_t initialized = 0;
    if (! meta_store.get(md_initialized_key, initialized))
        return rsp.error("contract state corrupted, no initialized key");

    if (initialized == 0)
        return rsp.error("asset type meta information is not set");

    StringArray description;
    if (! meta_store.get(md_description_key, description))
        return rsp.error("contract state corrupted, no description");

    Value v((char*)description.value_);
    return rsp.value(v, false);
}

// -----------------------------------------------------------------
// METHOD: get_link
//   set asset type link
//
// JSON PARAMETERS:
//   None
//
// RETURNS:
//   asset type link
// -----------------------------------------------------------------
bool get_link(const Message& msg, const Environment& env, Response& rsp)
{
    // Cannot retrieve information until the asset type is initialized
    uint32_t initialized = 0;
    if (! meta_store.get(md_initialized_key, initialized))
        return rsp.error("contract state corrupted, no initialized key");

    if (initialized == 0)
        return rsp.error("asset type meta information is not set");

    StringArray link;
    if (! meta_store.get(md_link_key, link))
        return rsp.error("contract state corrupted, no link");

    Value v((char*)link.value_);
    return rsp.value(v, false);
}

// -----------------------------------------------------------------
// METHOD: get_name
//   set asset type name
//
// JSON PARAMETERS:
//   None
//
// RETURNS:
//   asset type name
// -----------------------------------------------------------------
bool get_name(const Message& msg, const Environment& env, Response& rsp)
{
    // Cannot retrieve information until the asset type is initialized
    uint32_t initialized = 0;
    if (! meta_store.get(md_initialized_key, initialized))
        return rsp.error("contract state corrupted, no initialized key");

    if (initialized == 0)
        return rsp.error("asset type meta information is not set");

    StringArray name;
    if (! meta_store.get(md_name_key, name))
        return rsp.error("contract state corrupted, no name");

    Value v((char*)name.value_);
    return rsp.value(v, false);
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
contract_method_reference_t contract_method_dispatch_table[] = {
    CONTRACT_METHOD(initialize),
    CONTRACT_METHOD(set_meta_information),
    CONTRACT_METHOD(get_identifier),
    CONTRACT_METHOD(get_description),
    CONTRACT_METHOD(get_link),
    CONTRACT_METHOD(get_name),
    { NULL, NULL }
};
