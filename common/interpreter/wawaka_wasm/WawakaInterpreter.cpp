/* Copyright 2018 Intel Corporation
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

#include <unistd.h>
#include <ctype.h>

#include <exception>
#include <string>
#include <map>

#include "packages/base64/base64.h"
#include "packages/parson/parson.h"

#include "basic_kv.h"
#include "crypto.h"
#include "error.h"
#include "log.h"
#include "pdo_error.h"
#include "types.h"

#include "InvocationHelpers.h"
#include "WawakaInterpreter.h"

namespace pc = pdo::contracts;
namespace pe = pdo::error;
namespace pstate = pdo::state;

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#if USE_WAWAKA_OPT
const std::string WawakaInterpreter::identity_ = "wawaka-opt";
#else
const std::string WawakaInterpreter::identity_ = "wawaka";
#endif

// TODO: Produce verifiable info about runtime built into this enclave
// See issue #255
std::string pdo::contracts::GetInterpreterIdentity(void)
{
    return WawakaInterpreter::identity_;
}

pc::ContractInterpreter* pdo::contracts::CreateInterpreter(void)
{
    return new WawakaInterpreter();
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// function definitions for the Wasm interpreter
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
extern "C" {
#include "wasm_export.h"
#include "wasm_runtime.h"

extern uint32 wasm_get_module_total_mem_consumption(const WASMModule *module);

//#include "sample.h"

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
void wasm_logger(unsigned int level, const char *msg, const int value)
{
    SAFE_LOG(level, "%s; %d", msg, value);
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
int wasm_printer(const char *msg)
{
    SAFE_LOG(PDO_LOG_ERROR, msg);
    return 0; // for updated WAMR os_print_function_t
}

} /* extern "C" */

// Should be defined in WasmExtensions.cpp
extern bool InitializeNativeSymbols(RuntimeInitArgs& init_args);

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
void WawakaInterpreter::parse_response_string(
    int32 response_app,
    std::string& outResponse,
    bool& outStateChanged,
    std::map<std::string,std::string>& outDependencies)
{
    // Convert the wasm address for the result string into an
    // address in the native code
    uint64_t response_app_beg, response_app_end;

    pe::ThrowIf<pe::RuntimeError>(
        response_app == 0,
        report_interpreter_error("invalid result pointer", "no response"));

    pe::ThrowIf<pe::RuntimeError>(
        ! wasm_runtime_get_app_addr_range(wasm_module_inst_, response_app, &response_app_beg, &response_app_end),
        report_interpreter_error("invalid result pointer", "out of range"));

    pe::ThrowIf<pe::RuntimeError>(
        response_app_beg == response_app_end,
        report_interpreter_error("invalid result pointer", "empty response"));

    char *result = (char*)wasm_runtime_addr_app_to_native(wasm_module_inst_, response_app);
    pe::ThrowIfNull(result, report_interpreter_error("invalid result pointer", "invalid address"));

    const char *result_end = result + (response_app_end - response_app);

    // Not the most performant way to do this, but with no assumptions
    // about the size of the string, we have to walk the entire string
    for (char *p = result; (*p) != '\0'; p++)
        pe::ThrowIf<pe::RuntimeError>(
            p == result_end,
            report_interpreter_error("invalid result pointer", "unterminated string"));

    SAFE_LOG(PDO_LOG_DEBUG, "response string: %s", result);

    bool status;
    pc::parse_invocation_response(result, outResponse, status, outStateChanged, outDependencies);
    pe::ThrowIf<pe::ValueError>(
        ! status,
        report_interpreter_error("method evaluation failed", outResponse.c_str()));
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
const char* WawakaInterpreter::report_interpreter_error(
    const char* message,
    const char* error)
{
    error_msg_ = message;
    error_msg_.append("; ");
    error_msg_.append(error);
    return error_msg_.c_str();
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
void WawakaInterpreter::load_contract_code(
    const std::string& code)
{
    SAFE_LOG(PDO_LOG_DEBUG, "load contract code");

    char error_buf[128] = {0};
    binary_code_ = Base64EncodedStringToByteArray(code);

    // This is not the right check, but it is something to start with
    pe::ThrowIf<pe::RuntimeError>(
        binary_code_.size() > MAXIMUM_SIZE,
        "insufficient memory for the module");

    wasm_module_ = wasm_runtime_load((uint8*)binary_code_.data(), binary_code_.size(), error_buf, sizeof(error_buf));
    pe::ThrowIfNull(wasm_module_, "module load failed with error %s", error_buf);
    pe::ThrowIf<pe::RuntimeError>(
        wasm_runtime_get_module_package_type(wasm_module_) != Wasm_Module_Bytecode,
        "unsupported module type");

    uint32 module_memory = wasm_get_module_total_mem_consumption((const WASMModule*)wasm_module_);
    SAFE_LOG(PDO_LOG_DEBUG, "module memory consumption = %u", module_memory);

    pe::ThrowIf<pe::RuntimeError>(
        module_memory > MAXIMUM_SIZE,
        "insufficient interpreter memory; %u required, %u available", module_memory, MAXIMUM_SIZE);

    wasm_module_inst_ = wasm_runtime_instantiate(wasm_module_, STACK_SIZE, HEAP_SIZE, error_buf, sizeof(error_buf));
    pe::ThrowIfNull(wasm_module_inst_, "failed to instantiate the module; %s", error_buf);

    wasm_exec_env_ = wasm_runtime_create_exec_env(wasm_module_inst_, STACK_SIZE);
    pe::ThrowIfNull(wasm_exec_env_, "failed to create the wasm execution environment");

    SAFE_LOG(PDO_LOG_DEBUG, "contract code loaded");
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// current expects marshalled data
int32 WawakaInterpreter::initialize_contract(
    const std::string& env)
{
    uint8_t* buffer;
    wasm_function_inst_t wasm_func = NULL;
    int32 result = 0;

    SAFE_LOG(PDO_LOG_DEBUG, "wasm initialize_contract");

    wasm_func = wasm_runtime_lookup_function(wasm_module_inst_, "ww_initialize");
    if (wasm_func == NULL)
        wasm_func = wasm_runtime_lookup_function(wasm_module_inst_, "_ww_initialize");

    pe::ThrowIfNull(wasm_func, "Unable to locate the initialize function");

    uint32 argv[1] = {0}, buf_offset = 0;
    try {
        // might need to add a null terminator
        argv[0] = buf_offset = (int32)wasm_runtime_module_malloc(wasm_module_inst_, env.length() + 1, (void**)&buffer);
        pe::ThrowIf<pe::RuntimeError>(argv[0] == 0, "module malloc failed for some reason");

        memcpy(buffer, env.c_str(), env.length());
        buffer[env.length()] = '\0';

        pe::ThrowIf<pe::RuntimeError>(
            ! wasm_runtime_call_wasm(wasm_exec_env_, wasm_func, 1, argv),
            "initialize_contract: execution failed for some reason");

        SAFE_LOG(PDO_LOG_DEBUG, "RESULT=%u", argv[0]);
        result = argv[0];
    }
    catch (pe::RuntimeError& e) {
        SAFE_LOG(PDO_LOG_ERROR, "Exception: %s", e.what());
        const char *exception = wasm_runtime_get_exception(wasm_module_inst_);
        if (exception != NULL)
            SAFE_LOG(PDO_LOG_ERROR, "wasm exception = %s", exception);
        result = 0;
    }

    if (buf_offset)
        wasm_runtime_module_free(wasm_module_inst_, buf_offset);
    return result;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// current expects marshalled data
int32 WawakaInterpreter::evaluate_function(
    const std::string& args,
    const std::string& env)
{
    uint8_t* buffer;
    wasm_function_inst_t wasm_func = NULL;
    int32 result = 0;

    SAFE_LOG(PDO_LOG_DEBUG, "evaluate_function");
    pc::validate_invocation_request(args);

    wasm_func = wasm_runtime_lookup_function(wasm_module_inst_, "ww_dispatch");
    if (wasm_func == NULL)
        wasm_func = wasm_runtime_lookup_function(wasm_module_inst_, "_ww_dispatch");

    pe::ThrowIfNull(wasm_func, "Unable to locate the dispatch function");

    uint32 argv[2] = {0}, buf_offset0 = 0, buf_offset1 = 0;
    try {
        // might need to add a null terminator
        argv[0] = buf_offset0 = wasm_runtime_module_malloc(wasm_module_inst_, args.length() + 1, (void**)&buffer);
        pe::ThrowIf<pe::RuntimeError>(argv[0] == 0, "module malloc failed for some reason");

        memcpy(buffer, args.c_str(), args.length());
        buffer[args.length()] = '\0';

        argv[1] = buf_offset1 = wasm_runtime_module_malloc(wasm_module_inst_, env.length() + 1, (void**)&buffer);
        pe::ThrowIf<pe::RuntimeError>(argv[1] == 0, "module malloc failed for some reason");

        memcpy(buffer, env.c_str(), env.length());
        buffer[env.length()] = '\0';

        pe::ThrowIf<pe::RuntimeError>(
            ! wasm_runtime_call_wasm(wasm_exec_env_, wasm_func, 2, argv),
            "evaluate_function: execution failed for some reason");

        SAFE_LOG(PDO_LOG_DEBUG, "RESULT=%u", argv[0]);
        result = argv[0];
    }
    catch (pe::RuntimeError& e) {
        SAFE_LOG(PDO_LOG_ERROR, "Exception: %s", e.what());
        const char *exception = wasm_runtime_get_exception(wasm_module_inst_);
        if (exception != NULL)
            SAFE_LOG(PDO_LOG_ERROR, "wasm exception = %s", exception);
        result = 0;
    }

    if (buf_offset0)
        wasm_runtime_module_free(wasm_module_inst_, buf_offset0);
    if (buf_offset1)
        wasm_runtime_module_free(wasm_module_inst_, buf_offset1);
    return result;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
void WawakaInterpreter::Finalize(void)
{
    if (! initialized_)
        return;

    // Destroy the environment
    if (wasm_exec_env_ != NULL)
    {
        wasm_runtime_destroy_exec_env(wasm_exec_env_);
        wasm_exec_env_ = NULL;
    }

    if (wasm_module_inst_ != NULL)
    {
        wasm_runtime_deinstantiate(wasm_module_inst_);
        wasm_module_inst_ = NULL;
    }

    if (wasm_module_ != NULL)
    {
        wasm_runtime_unload(wasm_module_);
        wasm_module_ = NULL;
    }

    wasm_runtime_destroy();

    // release any heap resources
    global_mem_pool_buf_.resize(0);
    binary_code_.resize(0);

    // mark as uninitialized
    initialized_ = false;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
WawakaInterpreter::~WawakaInterpreter(void)
{
    Finalize();
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
void WawakaInterpreter::Initialize(void)
{
    SAFE_LOG(PDO_LOG_DEBUG, "initialize wawaka interpreter");
    SAFE_LOG(PDO_LOG_DEBUG, "interpreter memory profile: <%u, %u, %u>", STACK_SIZE, HEAP_SIZE, GLOBAL_HEAP_SIZE);

    // Make sure there are no lingering resources
    Finalize();

    // Initialize the code buffer and error message
    binary_code_.clear();
    error_msg_.clear();

    // Clear the kv_store_pool_
    kv_store_pool_.fill(nullptr);

    // Clear out the global memory pool
    global_mem_pool_buf_.resize(GLOBAL_HEAP_SIZE);

    // Configure the WAMR print function
    os_set_print_function(wasm_printer);

    // Construct the runtime arguments
    RuntimeInitArgs init_args;
    bool result;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.running_mode = Mode_Interp;

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_mem_pool_buf_.data();
    init_args.mem_alloc_option.pool.heap_size = global_mem_pool_buf_.size();

    init_args.max_thread_num = 1;

    InitializeNativeSymbols(init_args);

    result = wasm_runtime_full_init(&init_args);
    pe::ThrowIf<pe::RuntimeError>(! result, "failed to initialize wasm runtime environment");

    initialized_ = true;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
WawakaInterpreter::WawakaInterpreter(void)
{
    Initialize();
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
void WawakaInterpreter::create_initial_contract_state(
    const std::string& ContractID,
    const std::string& CreatorID,
    const pc::ContractCode& inContractCode,
    const pc::ContractMessage& inMessage,
    pstate::Basic_KV_Plus& inoutContractState
    )
{
    pstate::StateBlockId initialStateHash;
    initialStateHash.assign(initialStateHash.size(), 0); // this is probably not necessary

    // load the contract code
    load_contract_code(inContractCode.Code);

    // this doesn't really set thread local data since it is
    // not supported for sgx, it does however attach the data
    // to the module so we can use it in the extensions
    kv_store_pool_[0] = &inoutContractState;
    wasm_runtime_set_custom_data(wasm_module_inst_, kv_store_pool_.data());

    // serialize the environment parameter for the method
    std::string env;
    pc::create_invocation_environment(ContractID, CreatorID, inContractCode, inMessage, initialStateHash, env);

    // invoke the initialize function, later we can allow this to be passed with args
    int32 response_app = initialize_contract(env);

    std::string outMessageResult;
    bool outStateChangedFlag;
    std::map<std::string,std::string> outDependencies;
    parse_response_string(response_app, outMessageResult, outStateChangedFlag, outDependencies);

    // We could throw an exception if the store is not finalized
    // or we could just finalize and throw away the block id, which
    // effectively loses access to the kv store, seems like throwing
    // an exception is the right idea
    for (size_t i = 1; i < kv_store_pool_.size(); i++)
        pe::ThrowIf<pe::RuntimeError>(kv_store_pool_[i] != NULL, "failed to close contract KV store");

    // this should be in finally... later...
    wasm_runtime_set_custom_data(wasm_module_inst_, NULL);
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
void WawakaInterpreter::send_message_to_contract(
    const std::string& ContractID,
    const std::string& CreatorID,
    const pc::ContractCode& inContractCode,
    const pc::ContractMessage& inMessage,
    const pstate::StateBlockId& inContractStateHash,
    pstate::Basic_KV_Plus& inoutContractState,
    bool& outStateChangedFlag,
    std::map<std::string,std::string>& outDependencies,
    std::string& outMessageResult
    )
{
    // initialize the extensions library with the current state

    // load the contract code
    load_contract_code(inContractCode.Code);

    // set up the key value store information
    kv_store_pool_[0] = &inoutContractState;
    wasm_runtime_set_custom_data(wasm_module_inst_, kv_store_pool_.data());

    // serialize the environment parameter for the method
    std::string env;
    pc::create_invocation_environment(ContractID, CreatorID, inContractCode, inMessage, inContractStateHash, env);

    int32 response_app = evaluate_function(inMessage.Message, env);
    parse_response_string(response_app, outMessageResult, outStateChangedFlag, outDependencies);

    // We could throw an exception if the store is not finalized
    // or we could just finalize and throw away the block id, which
    // effectively loses access to the kv store, seems like throwing
    // an exception is the right idea
    for (size_t i = 1; i < kv_store_pool_.size(); i++)
        pe::ThrowIf<pe::RuntimeError>(kv_store_pool_[i] != NULL, "failed to close contract KV store");

    // this should be in finally... later...
    wasm_runtime_set_custom_data(wasm_module_inst_, NULL);
}
