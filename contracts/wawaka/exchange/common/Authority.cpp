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

#include "Authority.h"
#include "DependencyList.h"
#include "StringArray.h"

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// CLASS: Authority
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// -----------------------------------------------------------------
Authority::Authority(void)
{
    authorizing_authority_ = NULL;
}

Authority::~Authority(void)
{
    if (authorizing_authority_ != NULL)
        delete authorizing_authority_;
}

// -----------------------------------------------------------------
bool Authority::deserialize(const StringArray& serialized)
{
    ww::value::Object value;
    if (! value.deserialize(serialized.c_data()))
    {
        // log the parsing error
        return false;
    }

    return deserialize(value);
}

// -----------------------------------------------------------------
bool Authority::deserialize(
    const ww::value::Object& serialized)
{
    if (! serialized.get_value("authorized_key", authorized_key_))
    {
        // log error
        return false;
    }

    if (! serialized.get_value("dependency", dependency_))
    {
        // log error
        return false;
    }

    ww::value::Object parent_authority;
    if (value.get_value("parent_authority", parent_authority))
    {
        if (! serialized.get_value("authorizing_signature", authorizing_signature_))
        {
            // log error
            return false;
        }

        authorizing_authority_ = new Authority();
        if (! authorizing_authority_->deserialize(parent_authority))
        {
            // log error
            delete authorizing_authority_;
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------
bool Authority::serialize(ww::value::Object& serialized)
{
    serialized.set_value("authorized_key", authorized_key_);
    serialized.set_value("dependency", dependency_);

    if (authorizing_authority_ != NULL)
    {
        serialized.set_value("authorizing_signature", authorizing_signature_);

        ww::value::Object parent_authority;
        if (! authorizing_authority_->serialize(parent_authority))
        {
            // log error
            return false;
        }
        serialize.set_value("parent_authority", parent_authority);
    }

    return true;
}

bool Authority::serialize(StringArray& serialized)
{
    ww::value::Object result;
    if (! serialize(result))
    {
        // log error
        return false;
    }

    return false;
}

// -----------------------------------------------------------------
bool Authority::sign(
    const StringArray& asset_type_id,
    const StringArray& parent_signing_key,
    StringArray& signature)
{
    return false;
}

// -----------------------------------------------------------------
bool Authority::verify(const StringArray& asset_type_id)
{
    return false;
}
