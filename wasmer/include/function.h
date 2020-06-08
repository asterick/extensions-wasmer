#pragma once

#define FUNCTION_NAME "wasmer.function"

struct WasmerFunction {
    const struct wasmer_export_func_t* function;
    int owner;

    const char* name;
    int name_length;
    uint32_t param_count;
    wasmer_value_tag* param_type;
    wasmer_value_t* param_value;
    uint32_t return_count;
    wasmer_value_t* return_value;
};

bool is_function(lua_State* L, int index);
void function_from_export(lua_State* L, const char* name, int name_length, const wasmer_export_func_t* function, int index);
WasmerFunction* to_function (lua_State *L, int index);
void register_function(lua_State* L);
