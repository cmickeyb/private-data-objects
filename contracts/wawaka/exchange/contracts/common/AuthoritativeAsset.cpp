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

#include "Value.h"

#include "Asset.h"
#include "AuthoritativeAsset.h"
#include "Common.h"
#include "IssuerAuthorityChain.h"
#include "StateReference.h"

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// Class: ww::exchange::AuthoritativeAsset
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// -----------------------------------------------------------------
ww::exchange::AuthoritativeAsset::AuthoritativeAsset(void) :
    ww::value::Structure(AUTHORITATIVE_ASSET_SCHEMA)
{
    return;
}

// -----------------------------------------------------------------
SIMPLE_PROPERTY_GET(AuthoritativeAsset, asset, ww::exchange::Asset);
SIMPLE_PROPERTY_GET(AuthoritativeAsset, issuer_state_reference, ww::exchange::StateReference);
SIMPLE_PROPERTY_GET(AuthoritativeAsset, issuer_signature, ww::value::String);
SIMPLE_PROPERTY_GET(AuthoritativeAsset, issuer_authority_chain, ww::exchange::IssuerAuthorityChain)

SIMPLE_PROPERTY_SET(AuthoritativeAsset, asset, ww::exchange::Asset);
SIMPLE_PROPERTY_SET(AuthoritativeAsset, issuer_state_reference, ww::exchange::StateReference);
SIMPLE_PROPERTY_SET(AuthoritativeAsset, issuer_signature, ww::value::String);
SIMPLE_PROPERTY_SET(AuthoritativeAsset, issuer_authority_chain, ww::exchange::IssuerAuthorityChain)
