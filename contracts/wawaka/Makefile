# Copyright 2019 Intel Corporation
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

PDO_HOME ?= /opt/pdo

SRCDIR?=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ROOTDIR := $(abspath $(SRCDIR)/../..)
INSTALLDIR = $(PDO_HOME)/contracts

CONTRACTS = mock-contract test-contract
CONTRACT_TARGETS = $(addsuffix .b64,$(CONTRACTS))

CONTRACT_EXPORTS=['_dispatch']

LIBRARY_SOURCES = ${PDO_SOURCE_ROOT}/common/packages/parson/parson.cpp
COMMON_PACKAGE_SOURCES = $(wildcard common/*.cpp)
MOCK_CONTRACT_SOURCES = $(wildcard mock-contract/*.cpp)
TEST_CONTRACT_SOURCES = $(wildcard test-contract/*.cpp)

PACKAGE_INCLUDES = -I${PDO_SOURCE_ROOT}/contracts/wawaka/common
PACKAGE_INCLUDES += -I${PDO_SOURCE_ROOT}/common/interpreter/wawaka_wasm
PACKAGE_INCLUDES += -I${PDO_SOURCE_ROOT}/common/packages/parson
PACKAGE_INCLUDES += -I${PDO_SOURCE_ROOT}/common

all : $(CONTRACT_TARGETS)

debug :

test :
	@echo No automated tests defined

install : $(CONTRACT_TARGETS)
	@echo copy $(CONTRACTS) to $(INSTALLDIR)
	@mkdir -p $(INSTALLDIR)
	@cp $(CONTRACT_TARGETS) $(INSTALLDIR)

%.b64 : %.wasm
	rm -f $@
	base64 -w 0 $< > $@

mock-contract.wasm : ${MOCK_CONTRACT_SOURCES} ${COMMON_PACKAGE_SOURCES}
	em++ -g -O3 ${LIBRARY_SOURCES} ${COMMON_PACKAGE_SOURCES} ${MOCK_CONTRACT_SOURCES} \
		-o mock-contract.wasm \
		${PACKAGE_INCLUDES} \
		-I${WASM_SRC}/core/iwasm/lib/native-interface \
		-I${WASM_SRC}/core/iwasm/lib/app-libs/base \
		-s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
		-s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
		-s "EXPORTED_FUNCTIONS=${CONTRACT_EXPORTS}"

test-contract.wasm : ${TEST_CONTRACT_SOURCES} ${COMMON_PACKAGE_SOURCES}
	em++ -g -O3 ${LIBRARY_SOURCES} ${COMMON_PACKAGE_SOURCES} ${TEST_CONTRACT_SOURCES} \
		-o test-contract.wasm \
		${PACKAGE_INCLUDES} \
		-I${WASM_SRC}/core/iwasm/lib/native-interface \
		-I${WASM_SRC}/core/iwasm/lib/app-libs/base \
		-s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
		-s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
		-s "EXPORTED_FUNCTIONS=${CONTRACT_EXPORTS}"

clean:
	rm -f $(addsuffix .b64,$(CONTRACTS)) $(addsuffix .wasm,$(CONTRACTS))
	rm -f $(addsuffix .map,$(CONTRACTS)) $(addsuffix .wast,$(CONTRACTS))

.PHONY : all clean debug install test
