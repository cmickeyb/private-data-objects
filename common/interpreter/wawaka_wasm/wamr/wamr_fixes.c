#include "wasm_export.h"
#include "wasm_runtime.h"
#include "platform_internal.h"

/* Basic functionality copied from wasm_get_module_mem_consumption in
   core/iwasm/interpreter/wasm_runtime.c. This function is not exposed
   outside the interpreter module, so we need to duplicate it here. It
   has been modified to return only the total memory consumption.
 */

uint32 wasm_get_module_total_mem_consumption(const WASMModule *module)
{
    uint32 i, size;
    WASMModuleMemConsumption memory_consumption;
    WASMModuleMemConsumption *mem_conspn = &memory_consumption;

    memset(mem_conspn, 0, sizeof(WASMModuleMemConsumption));

    mem_conspn->module_struct_size = sizeof(WASMModule);

    mem_conspn->types_size = sizeof(WASMFuncType *) * module->type_count;
    for (i = 0; i < module->type_count; i++) {
        WASMFuncType *type = module->types[i];
        size = offsetof(WASMFuncType, types)
               + sizeof(uint8) * (type->param_count + type->result_count);
        mem_conspn->types_size += size;
    }

    mem_conspn->imports_size = sizeof(WASMImport) * module->import_count;

    mem_conspn->functions_size =
        sizeof(WASMFunction *) * module->function_count;
    for (i = 0; i < module->function_count; i++) {
        WASMFunction *func = module->functions[i];
        WASMFuncType *type = func->func_type;
        size = sizeof(WASMFunction) + func->local_count
               + sizeof(uint16) * (type->param_count + func->local_count);
#if WASM_ENABLE_FAST_INTERP != 0
        size +=
            func->code_compiled_size + sizeof(uint32) * func->const_cell_num;
#endif
        mem_conspn->functions_size += size;
    }

    mem_conspn->tables_size = sizeof(WASMTable) * module->table_count;
    mem_conspn->memories_size = sizeof(WASMMemory) * module->memory_count;
    mem_conspn->globals_size = sizeof(WASMGlobal) * module->global_count;
    mem_conspn->exports_size = sizeof(WASMExport) * module->export_count;

    mem_conspn->table_segs_size =
        sizeof(WASMTableSeg) * module->table_seg_count;
    for (i = 0; i < module->table_seg_count; i++) {
        WASMTableSeg *table_seg = &module->table_segments[i];
        mem_conspn->tables_size +=
            sizeof(InitializerExpression *) * table_seg->value_count;
    }

    mem_conspn->data_segs_size = sizeof(WASMDataSeg *) * module->data_seg_count;
    for (i = 0; i < module->data_seg_count; i++) {
        mem_conspn->data_segs_size += sizeof(WASMDataSeg);
    }

    if (module->const_str_list) {
        StringNode *node = module->const_str_list, *node_next;
        while (node) {
            node_next = node->next;
            mem_conspn->const_strs_size +=
                sizeof(StringNode) + strlen(node->str) + 1;
            node = node_next;
        }
    }

    mem_conspn->total_size += mem_conspn->module_struct_size;
    mem_conspn->total_size += mem_conspn->types_size;
    mem_conspn->total_size += mem_conspn->imports_size;
    mem_conspn->total_size += mem_conspn->functions_size;
    mem_conspn->total_size += mem_conspn->tables_size;
    mem_conspn->total_size += mem_conspn->memories_size;
    mem_conspn->total_size += mem_conspn->globals_size;
    mem_conspn->total_size += mem_conspn->exports_size;
    mem_conspn->total_size += mem_conspn->table_segs_size;
    mem_conspn->total_size += mem_conspn->data_segs_size;
    mem_conspn->total_size += mem_conspn->const_strs_size;

    return mem_conspn->total_size;
}
