#pragma once

#define FUNCTION_NAME "wasmer.function"

struct WasmerFunction {
    struct wasmer_export_func_t* function;
    int owner;
};

bool is_function(lua_State* L, int index);
void function_from_export(lua_State* L, wasmer_export_func_t* function, int index);
WasmerFunction* to_function (lua_State *L, int index);
void register_function(lua_State* L);
