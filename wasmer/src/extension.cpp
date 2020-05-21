#include <dmsdk/sdk.h>

#include "wasmer.hh"
#include "extension.h"
#include "memory.h"
#include "import.h"

struct luaL_const {
    const char* name;
    double val;
};

static const luaL_reg extension_methods[] =
{
    { "import", import_module },
    { 0, 0 }
};

static const luaL_const extension_consts[] = {
    { "i32", (double) wasmer_value_tag::WASM_I32 },
    { "i64", (double) wasmer_value_tag::WASM_I64 },
    { "f32", (double) wasmer_value_tag::WASM_F32 },
    { "f64", (double) wasmer_value_tag::WASM_F64 },
    { 0, 0 }
};

void wasm_pusherror(lua_State* L)
{  
    int error_length = wasmer_last_error_length();
    char* error_message = (char*) malloc(error_length);

    wasmer_last_error_message(error_message, error_length);
    lua_pushlstring(L, error_message, error_length);
    free(error_message);
}

void wasm_register_extension(lua_State* L)
{
    int top = lua_gettop(L);

    luaL_register(L, EXTENSION_NAME, extension_methods);
    register_import(L);
    register_function(L);

    lua_pushstring(L, "memory");
    register_memory(L);
    lua_settable(L, -3);

    const luaL_const* cons = &extension_consts[0];
    while (cons->name) {
        lua_pushstring(L, cons->name);
        lua_pushnumber(L, cons->val);
        lua_settable(L, -3);
        cons++;
    }

    lua_pop(L, 1);
    
    assert(top == lua_gettop(L));
}
