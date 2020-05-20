#include <dmsdk/sdk.h>

#include "wasmer.hh"
#include "extension.h"
#include "memory.h"
#include "import.h"

struct import_entry
{
    const char* name;
    size_t name_length;
    int reference;

    union {
        WasmerMemory* memory;
        void* ptr;
    };

    struct import_entry* next;
};

struct import_module
{
    const char* name;
    size_t name_length;
    struct import_entry* entries;

    struct import_module* next;
};

static void alloc_copy(const void** buffer, size_t length) {
    void* new_ptr = malloc(length);
    memcpy(new_ptr, *buffer, length);
    *buffer = new_ptr;
}

wasmer_import_t* build_imports(lua_State* L, int index, int& count)
{
    struct import_module* modules = NULL;
    count = 0;

    // No imports
    if (lua_isnil(L, index)) {
        return NULL;
    }
    
    lua_pushnil(L);
    while (lua_next(L, index) != 0)
    {
        // Chain here
        struct import_module* module = new struct import_module;
        module->name = luaL_checklstring(L, -2, &module->name_length);
        module->entries = NULL;

        lua_pushnil(L);
        while (lua_next(L, -2) != 0)
        {
            struct import_entry* entry = new struct import_entry;
            entry->name = luaL_checklstring(L, -2, &entry->name_length);
            entry->ptr = NULL;
                        
            if (is_memory(L, -1)) {
                // Reference and hold pointer
                lua_pushvalue(L, -1);
                entry->ref = dmScript::Ref(L, LUA_REGISTRYINDEX);
                entry->memory = to_memory(L, -1);

                dmLogInfo("Found memory");
            }

            if (entry->ptr != NULL) {
                alloc_copy((const void**)&entry->name, entry->name_length);
                entry->next = module->entries;
                module->entries = entry;
                count++;
            } else {
                delete entry;
            }
            
            lua_pop(L, 1);
        }

        // Insert into chain
        if (module->entries != NULL) {
            alloc_copy((const void**)&module->name, module->name_length);
            module->next = modules;
            modules = module;
        } else {
            delete module;
        }

        lua_pop(L, 1);
    }

    // We no longer need this
    if (count == 0) {
        return NULL;
    }
    
    // TODO: WORK IT
    
    return NULL;
}

int import_module(lua_State* L)
{
    lua_pushnil(L);

    const uint8_t* bytecode = NULL;
    size_t bytecode_len;

    bytecode = (const uint8_t*)luaL_checklstring(L, 1, &bytecode_len);

    // Build import table
    int import_count;
    wasmer_import_t* imports = build_imports(L, 2, import_count);

    return 0;
}
