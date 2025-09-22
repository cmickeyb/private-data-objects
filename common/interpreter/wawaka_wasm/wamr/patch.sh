#! /bin/bash
# Copyright 2025 Intel Corporation
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

source ${PDO_SOURCE_ROOT}/bin/lib/common.sh

OUTPUT_FILE=${PDO_OUTPUT_FILE:-${PWD}/.applied}
PATCH_DIR=${PWD}/patches
WAMR_ROOT_DIR=${WASM_SRC}

while getopts "o:p:w:" opt; do
    case $opt in
        o)
           OUTPUT_FILE=$OPTARG ;;
        p)
            PATCH_DIR=$OPTARG ;;
        w)
            WAMR_ROOT_DIR=$OPTARG ;;
        \?)
            die "Invalid option: -$OPTARG" >&2 ;;
    esac
done

# -----------------------------------------------------------------
# Patch wamr
# -----------------------------------------------------------------
# The following patches are applied to the source code, we use a dry
# run in order to check if the patch is already applied. If it is not
# then apply it

pushd ${WAMR_ROOT_DIR} || exit 1

for patch in ${PATCH_DIR}/*.patch; do
    patch -p1 -N --dry-run --silent <"$patch" >/dev/null 2>/dev/null || continue
    patch -p1 <"$patch"
done

popd

# -----------------------------------------------------------------
# record that we applied the patches
# -----------------------------------------------------------------
touch ${OUTPUT_FILE}
