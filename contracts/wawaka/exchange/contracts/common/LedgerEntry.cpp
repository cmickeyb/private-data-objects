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

#include "LedgerEntry.h"

#include "StringArray.h"
#include "WasmExtensions.h"

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// Class: ww::exchange::LedgerEntry
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// -----------------------------------------------------------------
ww::exchange::LedgerEntry::LedgerEntry(void) :
    ww::value::Structure(LEDGER_ENTRY_SCHEMA)
{}

// -----------------------------------------------------------------
ww::exchange::LedgerEntry::LedgerEntry(const StringArray& serialized) :
    ww::value::Structure(LEDGER_ENTRY_SCHEMA)
{
    if (! serialized.null_terminated())
    {
        CONTRACT_SAFE_LOG(1, "JSON string is not null terminated");
        return;
    }

    deserialize((const char*)serialized.c_data());
}

// -----------------------------------------------------------------
bool ww::exchange::LedgerEntry::is_active(void) const
{
    ww::value::Boolean flag(true);
    if (! get_value("active", flag))
        return false;

    return flag.get();
}

// -----------------------------------------------------------------
bool ww::exchange::LedgerEntry::set_active(void)
{
    const ww::value::String null_string("");
    if (! set_escrow_identifier(null_string))
        return false;

    if (! set_escrow_agent_identity(null_string))
        return false;

    ww::value::Boolean flag(true);
    return set_value("active", flag);
}

// -----------------------------------------------------------------
bool ww::exchange::LedgerEntry::set_inactive(const ww::value::String& escrow_agent_identity)
{
    if (! set_escrow_agent_identity(escrow_agent_identity))
        return false;

    if (! initialize_escrow_identifier())
        return false;

    ww::value::Boolean flag(false);
    return set_value("active", flag);
}

// -----------------------------------------------------------------
uint32_t ww::exchange::LedgerEntry::get_count(void) const
{
    ww::value::Number value;
    if (! get_value("count", value))
        return 0;

    return (uint32_t)value.get();
}

// -----------------------------------------------------------------
bool ww::exchange::LedgerEntry::set_count(uint32_t count)
{
    ww::value::Number value(count);
    return set_value("count", value);
}

// -----------------------------------------------------------------
const char* ww::exchange::LedgerEntry::get_owner_identity(void) const
{
    return get_string("owner_identity");
}

SIMPLE_PROPERTY_GET(LedgerEntry, owner_identity, ww::value::String);
SIMPLE_PROPERTY_SET(LedgerEntry, owner_identity, ww::value::String);

// -----------------------------------------------------------------
const char* ww::exchange::LedgerEntry::get_escrow_agent_identity(void) const
{
    return get_string("escrow_agent_identity");
}

SIMPLE_PROPERTY_GET(LedgerEntry, escrow_agent_identity, ww::value::String);
SIMPLE_PROPERTY_SET(LedgerEntry, escrow_agent_identity, ww::value::String);

// -----------------------------------------------------------------
const char* ww::exchange::LedgerEntry::get_escrow_identifier(void) const
{
    return get_string("escrow_identifier");
}

SIMPLE_PROPERTY_GET(LedgerEntry, escrow_identifier, ww::value::String);
SIMPLE_PROPERTY_SET(LedgerEntry, escrow_identifier, ww::value::String);

// -----------------------------------------------------------------
bool ww::exchange::LedgerEntry::initialize_escrow_identifier(void)
{
    // create a random identifier
    StringArray id_array(32);
    if (! random_identifier(id_array.size(), id_array.data()))
        return false;

    // base64 encode the random identifier
    StringArray encoded;
    if (! b64_encode(id_array.value_, id_array.size_, (char**)&encoded.value_, &encoded.size_))
        return false;

    // and save it in the ledger entry
    ww::value::String id_string((const char*)encoded.c_data());
    return set_escrow_identifier(id_string);
}
