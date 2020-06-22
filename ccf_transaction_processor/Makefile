# Copyright 2020 Intel Corporation
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

# SCRIPTDIR defaults to the current directory
# SRCDIR defaults to the parent directory
# DSTDIR defaults to _dev under the current directory
# Both can be overridden with environment variables

ifndef PDO_INSTALL_ROOT
$(error Incomplete configuration, PDO_INSTALL_ROOT is not defined)
endif

ifndef SGX_MODE
$(error Incomplete configuration, SGX_MODE is not defined)
endif

SCRIPTDIR ?= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
CCFDIR := $(SCRIPTDIR)/CCF
DSTDIR ?= $(PDO_INSTALL_ROOT)
SRCDIR ?= $(abspath $(SCRIPTDIR)/..)
KEYDIR := $(DSTDIR)/opt/pdo/ccf/keys/
ETCDIR := $(DSTDIR)/opt/pdo/ccf/etc/
BLDDIR := $(SCRIPTDIR)/build

KEYGEN=$(abspath $(BLDDIR)/keygenerator.sh
CNFGEN=$(abspath $(PDO_SOURCE_ROOT)/build/__tools__/expand-config)

PYTHON_DIR=$(DSTDIR)/lib/python3.7

# this should be determined by SGX_MODE
CCF_MODE=virtual

all: environment keys config

build: build-ccf build-pdo-tp

build-ccf :
	mkdir -p $(CCFDIR)/build
	cd $(CCFDIR)/build; cmake -GNinja \
		-DCOMPILE_TARGETS=virtual \
		-DCMAKE_INSTALL_PREFIX=${DSTDIR}/opt/pdo/ccf \
		-DBUILD_END_TO_END_TESTS=OFF \
		-DBUILD_SMALLBANK=OFF \
		-DBUILD_TESTS=OFF \
		-DBUILD_UNIT_TESTS=OFF \
		..
	cd $(CCFDIR)/build; ninja; ninja install

build-pdo-tp : install-ccf $(BLDDIR)
	cd $(BLDDIR) ; cmake .. -GNinja \
		-DCCF_DIR=${DSTDIR}/opt/pdo/ccf \
		-DCOMPILE_TARGETS=virtual \
		-DCMAKE_INSTALL_PREFIX=${DSTDIR}/opt/pdo/ccf
	cd $(BLDDIR) ; ninja

clean : clean-build clean-install

clean-build:
	rm -rf $(BLDDIR) $(CCFDIR)/build

clean-install :
	rm -rf $(DSTDIR)

config : $(ETCDIR)/cchost.toml $(ETCDIR)/gov.lua

$(ETCDIR)/cchost.toml : etc/cchost.toml
	@ echo create configuration files for cchost
	@ . $(abspath $(DSTDIR)/bin/activate) ; \
		$(CNFGEN) --template $(notdir $<) --template-directory $(dir $<) \
			--output-directory $(dir $@) \
			single --file $(notdir $@)

$(ETCDIR)/gov.lua : $(CCFDIR)/src/runtime_config/gov.lua
	cp $(CCFDIR)/src/runtime_config/gov.lua $(ETCDIR)/gov.lua

environment : $(DSTDIR) $(PYTHON_DIR)

$(BLDDIR) :
	@echo CREATE BUILD DIRECTORY $(BLDDIR)
	mkdir -p $(BLDDIR)

$(DSTDIR) :
	@echo CREATE INSTALLATION DIRECTORY $(DSTDIR)
	mkdir -p $(DSTDIR)
	mkdir -p $(DSTDIR)/opt/pdo/ccf/keys
	mkdir -p $(DSTDIR)/opt/pdo/ccf/etc
	mkdir -p $(DSTDIR)/opt/pdo/ccf/logs

$(PYTHON_DIR) :
	echo ${PYTHON_DIR}
	virtualenv -p python3.7 --no-download $(DSTDIR)
	. $(abspath $(DSTDIR)/bin/activate) ; pip install --upgrade pip
	. $(abspath $(DSTDIR)/bin/activate) ; pip install --upgrade setuptools
	. $(abspath $(DSTDIR)/bin/activate) ; pip install --upgrade -r $(CCFDIR)/tests/requirements.txt

install : install-ccf install-pdo-tp

install-ccf : build-ccf
	cd $(CCFDIR)/build; ninja install

install-pdo-tp : build-pdo-tp
	cd $(BLDDIR); ninja install

keys :
	cd $(KEYDIR) ; $(CCFDIR)/tests/keygenerator.sh --name=member0 --gen-key-share

.PHONY : all build build-ccf build-pdo-tp
.PHONY : clean clean-build clean-install
.PHONY : config environment
.PHONY : install keys
