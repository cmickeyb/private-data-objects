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
#include "common/LedgerEntry.h"
#include "common/LedgerStore.h"

static ww::exchange::LedgerStore ledger_store("ledger");

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
#define ISSUE_PARAMETER_SCHEMA "{\"owner_identity\":\"\", \"count\":0}"

bool issue(const Message& msg, const Environment& env, Response& rsp)
{
    ASSERT_SENDER_IS_OWNER(env, rsp);
    ASSERT_INITIALIZED(rsp);

    if (! msg.validate_schema(ISSUE_PARAMETER_SCHEMA))
        return rsp.error("invalid request, missing required parameters");

    // in theory, owner is an escda key, in practice it could be anything
    // but only an ecdsa key can be used meaningfully
    const StringArray owner(msg.get_string("owner_identity"));
    if (owner.size() == 0)
        return rsp.error("invalid request, invalid owner identity parameter");

    if (ledger_store.exists(owner))
        return rsp.error("invalid request, duplicate issuance");

    const int count = (int) msg.get_number("count");
    if (count <= 0)
        return rsp.error("invalid request, invalid asset count");

    if (! ledger_store.add_entry(owner, (uint32_t)count))
        return rsp.error("ledger operation failed, unable to save issuance");

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
    ASSERT_INITIALIZED(rsp);

    const StringArray owner(env.originator_id_);

    uint32_t balance = 0;

    ww::exchange::LedgerEntry entry;
    if (ledger_store.get_entry(owner, entry))
        balance = entry.get_count();

    ww::value::Number balance_value(balance);
    return rsp.value(balance_value, false);
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
#define TRANSFER_PARAMETER_SCHEMA "{\"new_owner_identity\":\"\", \"count\":0}"

bool transfer(const Message& msg, const Environment& env, Response& rsp)
{
    ASSERT_INITIALIZED(rsp);

    if (! msg.validate_schema(TRANSFER_PARAMETER_SCHEMA))
        return rsp.error("invalid request, missing required parameters");

    const int count = (int) msg.get_number("count");
    if (count <= 0)
        return rsp.error("invalid transfer request, invalid asset count");

    const StringArray new_owner(msg.get_string("new_owner_identity"));
    if (new_owner.size() == 0)
        return rsp.error("invalid transfer request, invalid owner identity parameter");

    const StringArray old_owner(env.originator_id_);

    // if there is no issuance for this identity, we treat it as a 0 balance
    ww::exchange::LedgerEntry old_entry;

    if (! ledger_store.get_entry(old_owner, old_entry))
        return rsp.error("transfer failed, insufficient balance for transfer");

    if (old_entry.get_count() < count)
        return rsp.error("transfer failed, insufficient balance for transfer");

    if (! old_entry.is_active())
        return rsp.error("transfer failed, old assets are escrowed");

    // in theory, owner is an escda key, in practice it could be anything
    // but only an ecdsa key can be used meaningfully
    if (! ledger_store.exists(new_owner))
        if (! ledger_store.add_entry(new_owner, 0))
            return rsp.error("transfer failed, failed to add new owner");

    ww::exchange::LedgerEntry new_entry;
    if (! ledger_store.get_entry(new_owner, new_entry))
        return rsp.error("transfer failed, failed to find new owner");

    if (! new_entry.is_active())
        return rsp.error("invalid transfer request, new assets are escrowed");

    // after all the set up, finally transfer the assets
    old_entry.set_count(old_entry.get_count() - (uint32_t)count);
    if (! ledger_store.set_entry(old_owner, old_entry))
        return rsp.error("transfer failed, unable to update old entry");

    new_entry.set_count(new_entry.get_count() + (uint32_t)count);
    if (! ledger_store.set_entry(new_owner, new_entry))
        return rsp.error("transfer failed, unable to update new entry");

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
#define ESCROW_PARAMETER_SCHEMA "{\"escrow_agent_identity\":\"\"}"

bool escrow(const Message& msg, const Environment& env, Response& rsp)
{
    ASSERT_INITIALIZED(rsp);

    if (! msg.validate_schema(ESCROW_PARAMETER_SCHEMA))
        return rsp.error("invalid escrow request, missing required parameters");

    ww::value::String escrow_agent(msg.get_string("escrow_agent_identity"));

    const StringArray owner(env.originator_id_);

    // if there is no issuance for this identity, we treat it as a 0 balance
    ww::exchange::LedgerEntry entry;

    if (! ledger_store.get_entry(owner, entry))
        return rsp.error("escrow failed, no entry for requestor");

    if (! entry.is_active())
        return rsp.error("escrow failed, assets are already escrowed");

    entry.set_inactive(escrow_agent);

    if (! ledger_store.set_entry(owner, entry))
        return rsp.error("escrow failed, unable to update entry");

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
    ASSERT_INITIALIZED(rsp);

    const StringArray owner(env.originator_id_);

    // if there is no issuance for this identity, we treat it as a 0 balance
    ww::exchange::LedgerEntry entry;
    if (! ledger_store.get_entry(owner, entry))
        return rsp.error("invalid escrow attestation request; no entry for requestor");

    if (entry.is_active())
        return rsp.error("invalid escrow attestation request, asset is not in escrow");

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
#define DISBURSE_PARAMETER_SCHEMA "{"                                   \
    "\"escrow_agent_state_reference\":" STATE_REFERENCE_SCHEMA ","      \
    SCHEMA_KW(escrow_agent_signature,"")                                \
    "}"

bool disburse(const Message& msg, const Environment& env, Response& rsp)
{
    ASSERT_INITIALIZED(rsp);

    if (! msg.validate_schema(DISBURSE_PARAMETER_SCHEMA))
        return rsp.error("invalid request, missing required parameters");

    const StringArray owner(env.originator_id_);

    ww::exchange::LedgerEntry entry;
    if (! ledger_store.get_entry(owner, entry))
        return rsp.error("invalid disburse request, no entry for requestor");

    if (entry.is_active())
        return rsp.error("invalid disburse request, assets are not escrowed");

    // verify that escrow agent signature
    const StringArray signature(msg.get_string("escrow_agent_signature"));
    const StringArray escrow_agent(entry.get_escrow_agent_identity());

    // create the authoritative asset

    // verify the signature

    // now modify the entry to mark it as active
    entry.set_active();
    if (! ledger_store.set_entry(owner, entry))
        return rsp.error("escrow failed, unable to update entry");

    // add the dependency to the response
    ww::exchange::StateReference escrow_state_reference;
    if (! msg.get_value("escrow_agent_state_reference", escrow_state_reference))
        return rsp.error("invalid disburse request, missing state reference");

    if (! escrow_state_reference.add_to_response(rsp))
        return rsp.error("disburse request failed, unable to save state reference");

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
