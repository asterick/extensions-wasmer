#include <dmsdk/sdk.h>

#include "wasmer.hh"
#include "extension.h"
#include "function.h"

bool is_function(lua_State* L, int index)
{
    lua_getmetatable(L, index);
    luaL_getmetatable(L, FUNCTION_NAME);
    bool test = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    return test;
}

WasmerFunction* to_function (lua_State *L, int index)
{
    WasmerFunction* funct = (WasmerFunction *)lua_touserdata(L, index);
    if (funct == NULL) luaL_typerror(L, index, FUNCTION_NAME);
    return funct;
}

void function_from_export(lua_State* L, const char* name, int name_length, const wasmer_export_func_t* funct, int index)
{
    lua_pushvalue(L, index);
    int owner = luaL_ref(L, LUA_REGISTRYINDEX);
 
    WasmerFunction* function = (WasmerFunction*) lua_newuserdata(L, sizeof(WasmerFunction));
    luaL_getmetatable(L, FUNCTION_NAME);
    lua_setmetatable(L, -2);
    function->function = funct;
    function->owner = owner;
    function->name = name;
    function->name_length = name_length;
    
    wasmer_export_func_params_arity(funct, &function->param_count);
    function->param_type = new wasmer_value_tag[function->param_count];
    function->param_value = new wasmer_value_t[function->param_count];
    wasmer_export_func_params(funct, function->param_type, function->param_count);

    for (unsigned int i = 0; i < function->param_count; i++) {
        function->param_value[i].tag = function->param_type[i];
    }

    wasmer_export_func_returns_arity(funct, &function->return_count);
    function->return_value = new wasmer_value_t[function->return_count];
}

static int function_gc(lua_State* L)
{
    WasmerFunction* function = to_function(L, 1);

    luaL_unref(L, LUA_REGISTRYINDEX, function->owner);
    
    delete function->param_type;
    delete function->param_value;
    delete function->return_value;

    return 0;
}

static int function_tostring(lua_State* L)
{
    WasmerFunction* function = to_function(L, 1);

    lua_pushstring(L, FUNCTION_NAME "('");
    lua_pushlstring(L, function->name, function->name_length);
    lua_pushstring(L, "')");
    lua_concat(L, 3);
        
    return 1;
}

static int function_call(lua_State* L)
{
    WasmerFunction* funct = to_function(L, 1);

    for (unsigned int i = 0; i < funct->param_count; i++) {
        switch (funct->param_type[i]) {
            case wasmer_value_tag::WASM_I32:
                funct->param_value[i].value.I32 = (int32_t)lua_tonumber(L, i + 2);
                break ;
            case wasmer_value_tag::WASM_I64:
                funct->param_value[i].value.I64 = (int64_t)lua_tonumber(L, i + 2);
                break ;
            case wasmer_value_tag::WASM_F32:
                funct->param_value[i].value.F32 = (float)lua_tonumber(L, i + 2);
                break ;
            case wasmer_value_tag::WASM_F64:
                funct->param_value[i].value.F64 = (double)lua_tonumber(L, i + 2);
                break ;            
        }
    }

    wasmer_export_func_call(
        funct->function,
        funct->param_value,
        funct->param_count,
        funct->return_value,
        funct->return_count
    );
    
    for (unsigned int i = 0; i < funct->return_count; i++) {
        switch (funct->return_value[i].tag) {
            case wasmer_value_tag::WASM_I32:
                lua_pushinteger(L, funct->return_value[i].value.I32);
                break ;
            case wasmer_value_tag::WASM_I64:
                lua_pushinteger(L, funct->return_value[i].value.I64);
                break ;
            case wasmer_value_tag::WASM_F32:
                lua_pushnumber(L, funct->return_value[i].value.F32);
                break ;
            case wasmer_value_tag::WASM_F64:
                lua_pushnumber(L, funct->return_value[i].value.F64);
                break ;            
        }
    }

    return funct->return_count;
}

static const luaL_reg function_meta[] =
{
    {"__gc",       function_gc},
    {"__tostring", function_tostring},
    {"__call",     function_call},
    {0, 0}
};

void register_function(lua_State* L)
{
    luaL_newmetatable(L, FUNCTION_NAME);
    luaL_register(L, NULL, function_meta);
    lua_pop(L, 1);
}
