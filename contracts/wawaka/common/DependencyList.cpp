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

#include "DependencyList.h"
#include "StringArray.h"
#include "Value.h"

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// class: Dependency
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Dependency::Dependency(
    const StringArray& contract_identifier,
    const StringArray& state_hash) :
    contract_identifier_(contract_identifier),
    state_hash_(state_hash)
{}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// class: DependencyNode
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// -----------------------------------------------------------------
DependencyNode::DependencyNode(
    const StringArray& contract_identifier,
    const StringArray& state_hash,
    DependencyNode* next) :
    dependency_(contract_identifier, state_hash)
{
    next_ = next;
}

// -----------------------------------------------------------------
DependencyNode::~DependencyNode(void)
{
    if (next_ != NULL)
        delete next_;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// class: DependencyList
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// -----------------------------------------------------------------
DependencyList::DependencyList(void)
{
    head_ = NULL;
}

// -----------------------------------------------------------------
DependencyList::~DependencyList(void)
{
    if (head_ != NULL)
        delete head_;
}

// -----------------------------------------------------------------
bool DependencyList::add_dependency(
    const StringArray& contract_identifier,
    const StringArray& state_hash)
{
    DependencyNode* dptr = new DependencyNode(contract_identifier, state_hash, head_);
    if (dptr == NULL)
        return false;

    head_ = dptr;
    return true;
}

// -----------------------------------------------------------------
bool DependencyList::attach(ww::value::Object& value) const
{
    ww::value::Array dependency_array;

    for (DependencyNode* dptr = head_; dptr != NULL; dptr = dptr->next_) {
        ww::value::Array dependency;
        ww::value::String contract_id((const char*)dptr->dependency_.contract_identifier_.c_data());
        ww::value::String state_hash((const char*)dptr->dependency_.state_hash_.c_data());
        dependency.append_value(contract_id);
        dependency.append_value(state_hash);
        dependency_array.append_value(dependency);
    }

    value.set_value("dependencies", dependency_array);
    return true;
}
