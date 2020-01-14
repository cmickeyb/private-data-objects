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

PDO_HOME ?= /opt/pdo

SRCDIR?=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ROOTDIR := $(abspath $(SRCDIR)/../..)
GIPSY_SCHEME := $(ROOTDIR)/contracts/bin/gipsyscheme
INSTALLDIR = $(PDO_HOME)/contracts

CONTRACT_SRC :=
CONTRACTS :=

PACKAGE_SRC := \
	../packages/contract-base.scm ../packages/key-store.scm ../packages/persistent-key-store.scm \
	../packages/safe-key-store.scm ../packages/hashtab.scm ../packages/alist.scm ../packages/hash.scm

TEST_PACKAGE_SRC := \
	../exchange/tests/test_common.scm \
	../packages/debug.scm

TEST_CONTRACTS := _integer_key_test.scm _mock_contract_test.scm

all: $(CONTRACTS)

debug :

test : $(TEST_CONTRACTS)
	@echo Run exchange contract tests
	@$(GIPSY_SCHEME) -1 _mock_contract_test.scm loglevel 1 version 1
	@$(GIPSY_SCHEME) -1 _mock_contract_test.scm loglevel 1 version 2
	@$(GIPSY_SCHEME) -1 _integer_key_test.scm loglevel 1 version 1


_mock_contract_test.scm : tests/mock_contract_test.bld tests/mock_contract_test.scm $(CONTRACT_SRC) $(PACKAGE_SRC) $(TEST_PACKAGE_SRC)
	$(GIPSY_SCHEME) -1 tests/mock_contract_test.bld debug

_integer_key_test.scm : tests/integer_key_test.bld tests/integer_key_test.scm $(CONTRACT_SRC) $(PACKAGE_SRC) $(TEST_PACKAGE_SRC)
	$(GIPSY_SCHEME) -1 tests/integer_key_test.bld debug

install : $(CONTRACTS) $(PLUGINS)
	@echo copy $(CONTRACTS) to $(INSTALLDIR)
	@mkdir -p $(INSTALLDIR)
	@cp $(CONTRACTS) $(INSTALLDIR)
	@cp $(PLUGINS) $(INSTALLDIR)

clean :
	@rm -f $(CONTRACTS) $(TEST_CONTRACTS)
	@rm -f dispatch-test.mdb dispatch-test.mdb-lock

.PHONY : all clean debug install test
