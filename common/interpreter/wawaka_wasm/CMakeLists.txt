# Copyright (C) 2019 Intel Corporation.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)

################################################################################
# Configure Environment
################################################################################

# Error checking on environment variables
IF (NOT DEFINED ENV{WASM_SRC})
  MESSAGE(FATAL_ERROR "WASM_SRC environment variable not defined!")
ENDIF()
SET (WAMR_ROOT_DIR "$ENV{WASM_SRC}")

# Make sure wasm-micro-runtime submodule has been cloned
FILE(GLOB WAMR_SUBMOD ${WAMR_ROOT_DIR})
LIST(LENGTH WAMR_SUBMOD SUBMOD_CONTENTS)
IF (SUBMOD_CONTENTS EQUAL 0)
  MESSAGE(FATAL_ERROR "WAMR git submodule has not been cloned. Please run `git submodule update --init` first.")
ENDIF()

# Memory size should be set in Memory.cmake which must be included
# before this file. There are three values for memory size: SMALL, MEDIUM
# and LARGE. Each provides defaults for memory allocation. See the note
# about memory size in Memory.cmake regarding dependencies between
# memory settings here and in other parts of PDO.
IF (NOT DEFINED PDO_MEMORY_CONFIG)
  MESSAGE(FATAL_ERROR "PDO_MEMORY_CONFIG not defined")
ENDIF()

#################################################################
# WAMR Library
#
# The WAMR library is built using a slightly modified version
# of the build script found in product-mini/platforms/linux-sgx/
#################################################################

# Set up the WAMR build configuration for use with Wawaka
SET (WAMR_BUILD_AOT 0)
SET (WAMR_BUILD_JIT 0)
SET (WAMR_BUILD_INTERP 1)

# Set the interpreter type
IF (PDO_INTERPRETER STREQUAL "wawaka-opt")
  SET (WAMR_BUILD_FAST_INTERP 1)
  ADD_DEFINITIONS (-DUSE_WAWAKA_OPT=1)
  MESSAGE(STATUS "Building wawaka in optimized INTERP mode")
ELSE()
  SET (WAMR_BUILD_FAST_INTERP 0)
  ADD_DEFINITIONS (-DUSE_WAWAKA_OPT=0)
  MESSAGE(STATUS "Building wawaka in regular INTERP mode")
ENDIF()

# Build LIBC support but we do not want any of the
# WASI system calls or pthread support
SET (WAMR_BUILD_LIBC_BUILTIN 1)
SET (WAMR_BUILD_LIBC_WASI 0)
SET (WAMR_BUILD_LIB_PTHREAD 0)

# No need for multi-module support
SET (WAMR_BUILD_MULTI_MODULE 0)

# WASI/clang compiles references so we need
# to ensure WAMR supports them
SET (WAMR_BUILD_REF_TYPES 1)

# We do not want heap sharing support between modules
SET (WASM_ENABLE_SHARED_HEAP 0)

# --------------- Other flags that are not required ---------------

# In theory this should remove the mmap calls, in practice it doesn't
# appear to work that way.
# SET(WAMR_DISABLE_STACK_HW_BOUND_CHECK 1)
# SET(WAMR_DISABLE_HW_BOUND_CHECK 1)

# This is used to distinguish between pre-allocated buffers and WAMR
# managed dynamic allocators in the application. It is not needed.
# SET (WASM_ENABLE_GLOBAL_HEAP_POOL 0)

SET(IWASM_STATIC_NAME iwasm)
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/wamr/wamr.cmake)

#################################################################
# Wawaka Interpreter
#################################################################
PROJECT(wawaka_wasm C CXX)

SET(WAWAKA_STATIC_NAME wwasm)

SET (SHARED_DIR ${WAMR_ROOT_DIR}/core/shared)
SET (IWASM_DIR ${WAMR_ROOT_DIR}/core/iwasm)

FILE(GLOB WWASM_PROJECT_HEADERS *.h)
FILE(GLOB WWASM_PROJECT_SOURCES *.cpp wamr/*.c)

ADD_LIBRARY(${WAWAKA_STATIC_NAME}
             ${WWASM_PROJECT_HEADERS}
             ${WWASM_PROJECT_SOURCES})
SGX_PREPARE_TRUSTED(${WAWAKA_STATIC_NAME})

# Set the memory configuration for wawaka
# RUNTIME_MEM_POOL_SIZE: The WASM runtime's global memory pool size
# HEAP_SIZE: Size of the runtime's heap for dynamic allocations by a WASM module.
# STACK_SIZE: Size of the runtime's stack for executing a WASM module
SET (WW_STACK_SIZE ${ENCLAVE_WORKER_STACK})
SET (WW_HEAP_SIZE ${ENCLAVE_WORKER_HEAP})

# MEM_POOL_SIZE determined from the stack and heap size plus padding, padding
# factor is just a multiplier that can be adjusted to increase the padding
MATH(EXPR WW_RUNTIME_MEM_POOL_SIZE "${WW_STACK_SIZE} + ${WW_HEAP_SIZE}")

# Check that the runtime memory pool size is not larger than the enclave heap size
IF (WW_RUNTIME_MEM_POOL_SIZE GREATER ENCLAVE_HEAP_SIZE)
  MESSAGE(FATAL_ERROR "Invalid memory size; RUNTIME_MEM_POOL_SIZE too big for the enclave heap size")
ENDIF()

TARGET_COMPILE_DEFINITIONS(${WAWAKA_STATIC_NAME} PRIVATE MAXIMUM_SIZE=${CONTRACT_MAXIMUM_SIZE})
TARGET_COMPILE_DEFINITIONS(${WAWAKA_STATIC_NAME} PRIVATE GLOBAL_HEAP_SIZE=${CONTRACT_GLOBAL_HEAP_SIZE})
TARGET_COMPILE_DEFINITIONS(${WAWAKA_STATIC_NAME} PRIVATE HEAP_SIZE=${CONTRACT_HEAP_SIZE})
TARGET_COMPILE_DEFINITIONS(${WAWAKA_STATIC_NAME} PRIVATE STACK_SIZE=${CONTRACT_STACK_SIZE})

TARGET_INCLUDE_DIRECTORIES(${WAWAKA_STATIC_NAME} PRIVATE ${INTERPRETER_INCLUDE_DIRS})
TARGET_INCLUDE_DIRECTORIES(${WAWAKA_STATIC_NAME} PRIVATE ${IWASM_DIR}/include)
TARGET_INCLUDE_DIRECTORIES(${WAWAKA_STATIC_NAME} PRIVATE ${SHARED_DIR}/include)

SET_TARGET_PROPERTIES(${WAWAKA_STATIC_NAME} PROPERTIES LINKER_LANGUAGE CXX)
