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

#include <stddef.h>
#include <stdint.h>

#include "Dispatch.h"

#include "KeyValue.h"
#include "Environment.h"
#include "Message.h"
#include "Response.h"
#include "StringArray.h"
#include "Util.h"
#include "Value.h"
#include "WasmExtensions.h"

#include "exchange_base.h"
#include "issuer_authority_base.h"

#include "common/AuthoritativeAsset.h"

static KeyValueStore ledger_store("ledger");

// -----------------------------------------------------------------
// METHOD: initialize_contract
//   contract initialization method
//
// JSON PARAMETERS:
//   none
//
// RETURNS:
//   true if successfully initialized
// -----------------------------------------------------------------
bool initialize_contract(const Environment& env, Response& rsp)
{
    return ww::exchange::exchange_base::initialize_contract(env, rsp);
}

// -----------------------------------------------------------------
// METHOD: initialize
//
// JSON PARAMETERS:
//  asset_authority_chain -- the object that grants issuance
//    authority to this contract
//
// RETURNS:
//   true if asset type id successfully saved
// -----------------------------------------------------------------
bool initialize(const Message& msg, const Environment& env, Response& rsp)
{
    return ww::exchange::issuer_authority_base::initialize_derived_authority(msg, env, rsp);
}

// -----------------------------------------------------------------
// METHOD: get_verifying_key
//
// JSON PARAMETERS:
//   none
//
// RETURNS:
//   ecdsa verifying key
// -----------------------------------------------------------------
bool get_verifying_key(const Message& msg, const Environment& env, Response& rsp)
{
    return ww::exchange::exchange_base::get_verifying_key(msg, env, rsp);
}

// -----------------------------------------------------------------
// METHOD: get_asset_type_identifier
//
// JSON PARAMETERS:
//   none
//
// RETURNS:
//   asset type id as a string
// -----------------------------------------------------------------
bool get_asset_type_identifier(const Message& msg, const Environment& env, Response& rsp)
{
    return ww::exchange::issuer_authority_base::get_asset_type_identifier(msg, env, rsp);
}

// -----------------------------------------------------------------
// METHOD: add_approved_issuer
//
// JSON PARAMETERS:
//   issuer-verifying-key -- verifying key of the asset issuer
//
// RETURNS:
//   true if key is successfully stored
// -----------------------------------------------------------------
bool add_approved_issuer(const Message& msg, const Environment& env, Response& rsp)
{
    return ww::exchange::issuer_authority_base::add_approved_issuer(msg, env, rsp);
}

// -----------------------------------------------------------------
// METHOD: get_issuer_authority
//
// JSON PARAMETERS:
//   issuer-verifying-key -- verifying key of the asset issuer
//
// RETURNS:
//   serialized authority object
// -----------------------------------------------------------------
bool get_issuer_authority(const Message& msg, const Environment& env, Response& rsp)
{
    return ww::exchange::issuer_authority_base::get_issuer_authority(msg, env, rsp);
}

// -----------------------------------------------------------------
// METHOD: get_authority
//
// JSON PARAMETERS:
//   none
//
// RETURNS:
//   serialized authority object for this issuer
// -----------------------------------------------------------------
bool get_authority(const Message& msg, const Environment& env, Response& rsp)
{
    return ww::exchange::issuer_authority_base::get_authority(msg, env, rsp);
}

// -----------------------------------------------------------------
// METHOD: issue
//
// JSON PARAMETERS:
//   owner_identity
//   count
//
// RETURNS:
//   boolean
// -----------------------------------------------------------------
bool issue(const Message& msg, const Environment& env, Response& rsp)
{
    return rsp.success(true);
}

// -----------------------------------------------------------------
// METHOD: get_balance
//
// JSON PARAMETERS:
//   none
//
// RETURNS:
//   current number of assets assigned to the requestor
// -----------------------------------------------------------------
bool get_balance(const Message& msg, const Environment& env, Response& rsp)
{
    ww::value::Number balance(0);

    return rsp.value(balance, false);
}

// -----------------------------------------------------------------
// METHOD: transfer
//
// JSON PARAMETERS:
//   new_owner_identity
//   count
//
// RETURNS:
//   boolean
// -----------------------------------------------------------------
bool transfer(const Message& msg, const Environment& env, Response& rsp)
{
    return rsp.success(true);
}

// -----------------------------------------------------------------
// METHOD: escrow
//
// JSON PARAMETERS:
//   escrow_agent_identity
//
// RETURNS:
//   boolean
// -----------------------------------------------------------------
bool escrow(const Message& msg, const Environment& env, Response& rsp)
{
    return rsp.success(true);
}

// -----------------------------------------------------------------
// METHOD: escrow_attestation
//
// JSON PARAMETERS:
//   none
//
// RETURNS:
//   authoritative_asset_type
// -----------------------------------------------------------------
bool escrow_attestation(const Message& msg, const Environment& env, Response& rsp)
{
    ww::exchange::AuthoritativeAsset asset;

    return rsp.value(asset, false);
}

// -----------------------------------------------------------------
// METHOD: disburse
//
// JSON PARAMETERS:
//   escrow_agent_state_reference
//   escrow_agent_signature
//
// RETURNS:
//   boolean
// -----------------------------------------------------------------
bool disburse(const Message& msg, const Environment& env, Response& rsp)
{
    return rsp.success(true);
}

// -----------------------------------------------------------------
// METHOD: claim
//
// JSON PARAMETERS:
//   escrow_claim
//
// RETURNS:
//   boolean
// -----------------------------------------------------------------
bool claim(const Message& msg, const Environment& env, Response& rsp)
{
    return rsp.success(true);
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
contract_method_reference_t contract_method_dispatch_table[] = {
    CONTRACT_METHOD(initialize),
    CONTRACT_METHOD(get_verifying_key),
    CONTRACT_METHOD(get_asset_type_identifier),
    CONTRACT_METHOD(add_approved_issuer),
    CONTRACT_METHOD(get_issuer_authority),
    CONTRACT_METHOD(get_authority),

    CONTRACT_METHOD(issue),
    CONTRACT_METHOD(get_balance),
    CONTRACT_METHOD(transfer),
    CONTRACT_METHOD(escrow),
    CONTRACT_METHOD(escrow_attestation),
    CONTRACT_METHOD(disburse),
    CONTRACT_METHOD(claim),
    { NULL, NULL }
};
