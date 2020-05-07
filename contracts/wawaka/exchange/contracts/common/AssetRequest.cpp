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

#include "AssetRequest.h"

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// Class: ww::exchange::AssetRequest
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// -----------------------------------------------------------------
ww::exchange::AssetRequest::AssetRequest(void) :
    Structure(ASSET_REQUEST_SCHEMA)
{
    return;
}

SIMPLE_PROPERTY_GET(AssetRequest, asset_type_identifier, ww::value::String);
SIMPLE_PROPERTY_GET(AssetRequest, count, ww::value::Number);
SIMPLE_PROPERTY_GET(AssetRequest, owner_identity, ww::value::String);

SIMPLE_PROPERTY_SET(AssetRequest, asset_type_identifier, ww::value::String);
SIMPLE_PROPERTY_SET(AssetRequest, count, ww::value::Number);
SIMPLE_PROPERTY_SET(AssetRequest, owner_identity, ww::value::String);
