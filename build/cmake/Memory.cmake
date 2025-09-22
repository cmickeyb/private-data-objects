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

################################################################################
# Memory size configuration
################################################################################

# The memory size option configures enclave and interpreter memory
# size values. The variable may have the value of "SMALL", "MEDIUM" or
# "LARGE". This is a project variable because the configurations
# depend on one another (the interpreter heap size must fit into the
# enclave heap, for example).
SET(PDO_MEMORY_CONFIG "MEDIUM" CACHE STRING "Set memory size parameters for enclave and interpreter")
IF (DEFINED ENV{PDO_MEMORY_CONFIG})
  SET(PDO_MEMORY_CONFIG $ENV{PDO_MEMORY_CONFIG})
ENDIF()
SET(MEMORY_SIZE_OPTIONS "SMALL" "MEDIUM" "LARGE")
IF (NOT ${PDO_MEMORY_CONFIG} IN_LIST MEMORY_SIZE_OPTIONS)
  MESSAGE(FATAL_ERROR "Invalid memory size; ${PDO_MEMORY_CONFIG}")
ENDIF()

# Module heap and stack, these are related to the contract maximum
# size, we could specify these and derive the maximum contract size
# from them
IF (${PDO_MEMORY_CONFIG} STREQUAL "SMALL")
  MATH(EXPR CONTRACT_HEAP_SIZE "2 * 1024 * 1024")
  MATH(EXPR CONTRACT_STACK_SIZE "1 * 1024 * 1024")
ELSEIF (${PDO_MEMORY_CONFIG} STREQUAL "MEDIUM")
  MATH(EXPR CONTRACT_HEAP_SIZE "4 * 1024 * 1024")
  MATH(EXPR CONTRACT_STACK_SIZE "2 * 1024 * 1024")
ELSEIF (${PDO_MEMORY_CONFIG} STREQUAL "LARGE")
  MATH(EXPR CONTRACT_HEAP_SIZE "8 * 1024 * 1024")
  MATH(EXPR CONTRACT_STACK_SIZE "4 * 1024 * 1024")
ELSE()
  MESSAGE(FATAL_ERROR "Invalid memory size; ${PDO_MEMORY_CONFIG}")
ENDIF()

# This determines the reserved memory size in the enclave, that is,
# the interpreters linear memory is currently stored in the enclave
# reserve memory. The padding is expected to be constant over the
# size of the contract.
MATH(EXPR CONTRACT_PADDING "1 * 1024 * 1024")
MATH(EXPR CONTRACT_MAXIMUM_SIZE "${CONTRACT_HEAP_SIZE} + ${CONTRACT_STACK_SIZE} + ${CONTRACT_PADDING}")

# Global heap is used for the management structures that store WAMR
# state information, not specific to the module or contract size. This
# memory is allocated from the enclave heap/stack. Note that it is
# difficult (and not particularly well documented) to determine what
# this number should be. The current computation represents a best
# guess and will need to be adjusted as appropriate.
MATH(EXPR CONTRACT_GLOBAL_HEAP_SIZE "4 * 1024 * 1024 + ${CONTRACT_MAXIMUM_SIZE}")

# State cache size is one of the main consumers of memory, probably impacts
# both enclave stack and heap, the state data block size is probably not
# important enough to provide variable sizing. Should probably just set it
# to 8K and leave it.
MATH(EXPR STATE_CACHE_SIZE "4 * 1024 * 1024")
MATH(EXPR STATE_DATA_BLOCK_SIZE "8 * 1024")

# The worker stack and heap sizes are per-thread allocations. The heap
# size must be large enough to hold the state cache, the global heap, and
# some padding. For now, the stack size is a fraction of the heap.
MATH(EXPR ENCLAVE_WORKER_HEAP "${STATE_CACHE_SIZE} + ${CONTRACT_GLOBAL_HEAP_SIZE} + ${CONTRACT_MAXIMUM_SIZE}")
MATH(EXPR ENCLAVE_WORKER_STACK "${ENCLAVE_WORKER_HEAP} / 4")

# The WAMR requirement for reserved memory is the WAMR heap size plus
# the total memory required by the WASM module that is loaded. We can
# set a limit here and trust the wawaka module to enforce the
# limit. Note that the contract heap is double counted (explicitly and
# again in the contract maximum size). This is a reflection of how the
# contract interpreter handles reserved memory allocation.
MATH(EXPR ENCLAVE_WORKER_RESERVED_SIZE "${CONTRACT_HEAP_SIZE} + ${CONTRACT_MAXIMUM_SIZE}")

# The worker thread count corresponds roughly to the expected number
# of concurrent workers in the enclave, each will allocate memory to
# the contract interpreter.
SET(ENCLAVE_WORKER_THREADS "2")

# Heap padding is memory expected to be used by the enclave outside
# the interpreter (that is, it does not depend on the configuration of
# the interpreter).
MATH(EXPR ENCLAVE_STACK_PADDING "4 * 1024 * 1024")
MATH(EXPR ENCLAVE_HEAP_PADDING "4 * 1024 * 1024")

# And the final numbers allocate enough space for each worker plus the shared padding
MATH(EXPR ENCLAVE_STACK_SIZE "${ENCLAVE_WORKER_THREADS} * ${ENCLAVE_WORKER_STACK} + ${ENCLAVE_STACK_PADDING}")
MATH(EXPR ENCLAVE_HEAP_SIZE "${ENCLAVE_WORKER_THREADS} * ${ENCLAVE_WORKER_HEAP} + ${ENCLAVE_HEAP_PADDING}")
MATH(EXPR ENCLAVE_RESERVED_SIZE "${ENCLAVE_WORKER_THREADS} * ${ENCLAVE_WORKER_RESERVED_SIZE}")
