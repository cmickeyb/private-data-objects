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

#include "DependencyList.h"
#include "StringArray.h"

// {
//    "authorized-issuer-key" : <ecdsa key>,
//    "dependency" : [ <contract id>, <state hash> ],
//    "parent-authority" : {...},
//    "parent-signature" : <signature>
// }

// the parent-authority grants the issuer the right
// to issue assets of the given asset type; specifically
// the issuer-key can sign statements of issuance on
// the authority of the parent assuming the depenendent
// transactions are committed

class Authority
{
protected:
    bool deserialize(const ww::value::Object& serialized)

public:
    ww::value::String authorized_key_;
    ww::value::String authorizing_signature_;
    ww::value::Array dependency_;

    Authority *authorizing_authority_ = NULL;

    Authority(void);
    ~Authority(void);

    bool deserialize(const StringArray& serialized);
    bool serialize(StringArray& serialized);

    bool serialize_for_signing(
        const StringArray& asset_type_id,
        StringArray& serialized);

    bool sign(
        const StringArray& asset_type_id,
        const StringArray& parent_signing_key,
        StringArray& signature);

    bool verify(
