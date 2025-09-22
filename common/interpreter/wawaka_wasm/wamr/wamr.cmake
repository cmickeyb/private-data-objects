# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.14)

## -----------------------------------------------------------------
## Modified from WAMR product-mini/platforms/linux-sgx/CMakeLists_minimal.txt
## to support custom library name and custom WAMR_ROOT_DIR and automated
## patching of WAMR source code.
## -----------------------------------------------------------------
project (iwasm)

set (WAMR_BUILD_PLATFORM "linux-sgx")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Set WAMR_BUILD_TARGET
if (NOT DEFINED WAMR_BUILD_TARGET)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (WAMR_BUILD_TARGET "X86_64")
  elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    # Build as X86_32 by default in 32-bit platform
    set (WAMR_BUILD_TARGET "X86_32")
  else ()
    message(SEND_ERROR "Unsupported build target platform!")
  endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

if (NOT DEFINED WAMR_BUILD_INTERP)
  # Enable Interpreter by default
  set (WAMR_BUILD_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_AOT)
  # Enable AOT by default
  # Please install Intel SGX SDKv2.8 or later.
  set (WAMR_BUILD_AOT 1)
endif ()

if (NOT DEFINED WAMR_BUILD_JIT)
  # Disable JIT by default.
  set (WAMR_BUILD_JIT 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_BUILTIN)
  # Enable libc builtin support by default
  set (WAMR_BUILD_LIBC_BUILTIN 1)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_WASI)
  # Enable libc wasi support by default
  set (WAMR_BUILD_LIBC_WASI 0)
endif ()

if (NOT DEFINED WAMR_BUILD_FAST_INTERP)
  # Enable fast interpreter
  set (WAMR_BUILD_FAST_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_MULTI_MODULE)
  # Enable multiple modules
  set (WAMR_BUILD_MULTI_MODULE 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIB_PTHREAD)
  # Enable pthread library by default
  set (WAMR_BUILD_LIB_PTHREAD 0)
endif ()

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -ffunction-sections -fdata-sections \
                                     -Wall -Wno-unused-parameter -Wno-pedantic \
                                     -nostdinc -fvisibility=hidden -fpie" )

## -----------------------------------------------------------------
## local changes for Wawaka customization
## -----------------------------------------------------------------
INCLUDE(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
ADD_LIBRARY(${IWASM_STATIC_NAME} ${WAMR_RUNTIME_LIB_SOURCE})
SET_VERSION_INFO (${IWASM_STATIC_NAME})

# apply patches to the WAMR source code before compiling, the patch command
# will apply patches found in the patches directory. A sentinel file is
# created in the build directory to indicate that patches have been applied.
# Note that the WAMR submodule will be in a dirty state after patches.
ADD_CUSTOM_COMMAND(
  PRE_BUILD
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/.patches_applied
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wamr/patches/*.patch ${CMAKE_CURRENT_SOURCE_DIR}/wamr/patch.sh
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/wamr/patch.sh
      -p ${CMAKE_CURRENT_SOURCE_DIR}/wamr/patches
      -w ${WAMR_ROOT_DIR}
      -o ${CMAKE_CURRENT_BINARY_DIR}/.patches_applied
  COMMENT "Applying patch to WAMR source code"
)

ADD_CUSTOM_TARGET(
  apply_wamr_patch
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/.patches_applied
  COMMENT "Applying patch to WAMR source code"
)

ADD_DEPENDENCIES(${IWASM_STATIC_NAME} apply_wamr_patch)
