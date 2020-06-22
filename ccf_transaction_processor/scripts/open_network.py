#!/usr/bin/env python

# Copyright 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Return the number of registered endpoints in the exit status
"""

import argparse
import os
import sys
import toml

## -----------------------------------------------------------------
ContractHome = os.environ["PDO_HOME"]

# -----------------------------------------------------------------
parser = argparse.ArgumentParser(description='Script to enable the CCF network')

parser.add_argument('--ccf-config', help='Name of the CCF configuration file', default=os.path.join(ContractHome, 'ccf/etc/cchost.toml'))
options = parser.parse_args()

# -----------------------------------------------------------------
try :
    config = toml.load(options.ccf_config)
except :
