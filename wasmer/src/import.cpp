#include <dmsdk/sdk.h>

#include "wasmer.hh"
#include "extension.h"
#include "memory.h"
#include "import.h"

struct import_entry
{
    const char* name;
    size_t name_length;
    int ref;
    wasmer_import_export_kind tag;
    
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

// Copy a lua string into a buffer, and append a null-terminator
static void alloc_copy(const void** buffer, size_t length) {
    void* new_ptr = malloc(length + 1);
    memcpy(new_ptr, *buffer, length);
    ((uint8_t*)new_ptr)[length] = 0;
    *buffer = new_ptr;
}

int build_imports(lua_State* L, int index, wasmer_import_t*& imports, int*& refs)
{
    int count;
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
                        
            // TODO: HANDLE FUNC (C and Lua), TABLE, GLOBAL
            if (is_memory(L, -1)) {
                // Reference and hold pointer
                lua_pushvalue(L, -1);
                entry->tag = wasmer_import_export_kind::WASM_MEMORY;
                entry->ref = dmScript::Ref(L, LUA_REGISTRYINDEX);
                entry->memory = to_memory(L, -1);
            } else {
                dmLogInfo("Cannot import %s.%s", module->name, entry->name);
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

    imports = new wasmer_import_t[count];
    refs = new int[count];
    int target = 0;
    
    // Iterate over found imports
    {
        struct import_module* module = modules;
        while (module != NULL) {
            struct import_entry* entry = module->entries;

            while(entry != NULL) {
                imports[target].module_name.bytes = (const uint8_t*) module->name;
                imports[target].module_name.bytes_len = module->name_length;
                imports[target].import_name.bytes = (const uint8_t*) entry->name;
                imports[target].import_name.bytes_len = entry->name_length;
                refs[target] = entry->ref;
                imports[target].tag = entry->tag;
                
                switch (entry->tag) {
                    case wasmer_import_export_kind::WASM_MEMORY:
                        imports[target].value.memory = entry->memory->mem;
                        break ;
                    default:
                        dmLogError("Unhandled tag: %i", (int)entry->tag);
                }
                
                struct import_entry* cleanup = entry;
                entry = entry->next;
                delete cleanup;
                target++;
            }
            
            {
                struct import_module* cleanup = module;
                module = module->next;
                delete cleanup;
            }
        }
    }

    return imports;
}

int import_module(lua_State* L)
{
    lua_pushnil(L);

    const uint8_t* bytecode = NULL;
    size_t bytecode_len;

    bytecode = (const uint8_t*)luaL_checklstring(L, 1, &bytecode_len);

    // Build import table
    wasmer_import_t* imports;
    int* refs;
    int import_count = build_imports(L, 2, imports, refs);

    // TODO: CREATE INSTANCE
    // TODO: RELEASE ALL STRINGS IN IMPORTS

    return 0;
}
