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

#include "StringArray.h"
#include "Value.h"

class Dependency
{
public:
    StringArray contract_identifier_;
    StringArray state_hash_;

    Dependency(
        const StringArray& contract_identifier,
        const StringArray& state_hash);
};

class DependencyNode
{
public :
    Dependency dependency_;
    DependencyNode* next_;

    DependencyNode(
        const StringArray& contract_identifier,
        const StringArray& state_hash,
        DependencyNode* next = NULL);

    ~DependencyNode(void);
};


class DependencyList
{
    DependencyNode* head_;

public :
    DependencyList(void);
    ~DependencyList(void);

    bool add_dependency(const StringArray& contract_identifier, const StringArray& state_hash);
    bool attach(ww::value::Object& value) const;
};
