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
    WasmerFunction* function = (WasmerFunction *)lua_touserdata(L, index);
    if (mem == NULL) luaL_typerror(L, index, FUNCTION_NAME);
    return mem;
}

void function_from_export(lua_State* L, wasmer_export_func_t* func, int index)
{
    lua_pushvalue(L, index);
    int owner = luaL_ref(L, LUA_REGISTRYINDEX);
 
    WasmerFunction* function = (WasmerFunction*) lua_newuserdata(L, sizeof(WasmerFunction));
    luaL_getmetatable(L, FUNCTION_NAME);
    lua_setmetatable(L, -2);
    function->function = funct;
    function->owner = owner;
}

static int function_gc(lua_State* L)
{
    WasmerFunction* function = to_function(L, 1);

    luaL_unref(L, LUA_REGISTRYINDEX, function->owner);

    return 0;
}

static int function_tostring(lua_State* L)
{
    WasmerFunction* function = to_function(L, 1);
    lua_pushfstring(L, FUNCTION_NAME "(%i)", (int)function);

    return 1;
}

static int function_call(lua_State* L)
{
    return 0;
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
