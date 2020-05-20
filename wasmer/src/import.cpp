#include <dmsdk/sdk.h>

#include "wasmer.hh"
#include "extension.h"
#include "memory.h"
#include "import.h"

struct wasmer_import_chain_t
{
    wasmer_import_t import;
    int ref;
    struct wasmer_import_chain_t* next;
}

wasmer_import_t* build_imports(lua_State* L, int index, int& count)
{
    while (lua_next(L, index) != 0)
    {
        const char* key = luaL_checkstring(L, -2);
        dmLogInfo("%s\n", key);

        while (lua_next(L, -1) != 0)
        {
            const char* key2 = luaL_checkstring(L, -2);

            dmLogInfo("%s\n", key, key2);
            lua_pop(L, 1);
        }
        
        lua_pop(L, 1);
    }
}

int import_module(lua_State* L)
{
    lua_pushnil(L);

    const uint8_t* bytecode = NULL;
    size_t bytecode_len;

    bytecode = (const uint8_t*)luaL_checklstring(L, 1, &bytecode_len);

    // Build import table
    wasmer_import_t* imports = NULL;
    int import_count = 0;

    if (lua_istable(L, 2)) {
        imports = build_imports(L, 2, import_count);
    }

    return 0;
}
