#include <dmsdk/sdk.h>

#include "wasmer.hh"
#include "extension.h"
#include "memory.h"
#include "import.h"

static const luaL_reg extension_methods[] =
{
    { "import", import_module },
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

    lua_pushstring(L, "memory");
    register_memory(L);
    lua_settable(L, -3);

    lua_pop(L, 1);
    
    assert(top == lua_gettop(L));
}
