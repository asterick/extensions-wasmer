#include <dmsdk/sdk.h>

#include "wasmer.hh"
#include "extension.h"
#include "memory.h"
#include "import.h"

int import_module(lua_State* L)
{
    lua_pushnil(L);

    const uint8_t* bytecode = NULL;
    size_t bytecode_len;

    while (lua_next(L, 1) != 0)
    {
        const char* key = luaL_checkstring(L, -2);

        dmLogInfo("%i\n", key != NULL);
        
        //if (is_memory(L, -1)) {
            //dmLogInfo("Import memory: %s", key);
        //} else if (lua_istable(L, -1)) {
            // IMPORT EXTERNAL MODULE HERE
        //}

        // Drop value for next iteration
        lua_pop(L, 1);
    }

    return 0;
}
