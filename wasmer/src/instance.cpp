#include <dmsdk/sdk.h>

#include "wasmer.hh"
#include "extension.h"
#include "memory.h"
#include "function.h"
#include "instance.h"

struct import_entry
{
    const char* name;
    size_t name_length;
    int ref;
    wasmer_import_export_kind tag;
    
    union {
        wasmer_memory_t* memory;
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

static int build_imports(lua_State* L, int index, wasmer_import_t*& imports, int*& refs)
{
    // No imports
    if (lua_isnil(L, index)) {
        return 0;
    }

    struct import_module* modules = NULL;
    int count = 0;

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
                        
            // TODO: HANDLE FUNC (Wasmer and Lua), TABLE, GLOBAL

            if (is_memory(L, -1)) {
                // Reference and hold pointer
                lua_pushvalue(L, -1);
                entry->ref = luaL_ref(L, LUA_REGISTRYINDEX);
                entry->tag = wasmer_import_export_kind::WASM_MEMORY;
                entry->memory = to_memory(L, -1)->mem;
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
        return 0;
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
                imports[target].tag = entry->tag;
                imports[target].value.memory = entry->memory;
                refs[target] = entry->ref;
                target++;

                struct import_entry* cleanup = entry;
                entry = entry->next;
                delete cleanup;
            }
            
            {
                struct import_module* cleanup = module;
                module = module->next;
                delete cleanup;
            }
        }
    }

    return count;
}

static void build_exports(lua_State* L, wasmer_instance_t* instance, int* refs, int ref_count)
{
    // Create our export index
    lua_newtable(L);

    // Create our userdata object
    WasmerInstance* inst = (WasmerInstance*) lua_newuserdata(L, sizeof(WasmerInstance));
    luaL_getmetatable(L, INSTANCE_NAME);
    lua_setmetatable(L, -2);

    wasmer_instance_exports(instance, &inst->exports);
   
    inst->instance = instance;
    inst->refs = refs;
    inst->ref_count = ref_count;

    // Start iterating over the index
    int exports_len = wasmer_exports_len(inst->exports);
    
    for (int i = 0; i < exports_len; i++) {
        wasmer_export_t* item = wasmer_exports_get(inst->exports, i);
        wasmer_import_export_kind export_kind = wasmer_export_kind(item);
        wasmer_byte_array name_bytes = wasmer_export_name(item);
        
        switch (export_kind) {
            case wasmer_import_export_kind::WASM_MEMORY:
                {
                    wasmer_memory_t* memory;
                    wasmer_result_t res = wasmer_export_to_memory(item, &memory);
                    
                    if (res != wasmer_result_t::WASMER_OK) {
                        dmLogInfo("Cannot created exported memory %s type %i", name_bytes.bytes, export_kind);
                        continue ;
                    }

                    lua_pushlstring(L, (const char*)name_bytes.bytes, (size_t)name_bytes.bytes_len);
                    memory_from_export(L, memory, -2);
                    lua_settable(L, -4);
                }
                break ;
            case wasmer_import_export_kind::WASM_FUNCTION:
                {
                    const wasmer_export_func_t* funct = wasmer_export_to_func(item);
                    lua_pushlstring(L, (const char*)name_bytes.bytes, (size_t)name_bytes.bytes_len);
                    function_from_export(L, funct, -2);
                    lua_settable(L, -4);
                }
                break ;
            // TODO: case wasmer_import_export_kind::WASM_TABLE:
            // TODO: case wasmer_import_export_kind::WASM_GLOBAL:
            default:
                dmLogInfo("Cannot handle '%s' type %i", name_bytes.bytes, export_kind);
                continue ;
        }
    }
    
    // Discard userdata
    lua_pop(L, 1);
}

int instance_module(lua_State* L)
{
    // Load bytecode for module
    size_t bytecode_len;
    const uint8_t* bytecode = (const uint8_t*)luaL_checklstring(L, 1, &bytecode_len);

    // Build import table
    wasmer_import_t DUMMY_IMPORT = {};
    wasmer_import_t* imports = &DUMMY_IMPORT;
    int* refs;
    int import_count = build_imports(L, 2, imports, refs);

    // Create our instance
    wasmer_instance_t *instance = NULL;
    wasmer_exports_t* exports = NULL;

    wasmer_result_t res = wasmer_instantiate(
        &instance,
        (uint8_t*) bytecode,
        (uint32_t) bytecode_len,
        imports,
        import_count
    );

    // -- Release Import table (unneeded)
    if (import_count > 0) {
        const uint8_t* last_module = NULL;
        for (int i = 0; i < import_count; i++) {
            if (imports[i].module_name.bytes != last_module) {
                last_module = imports[i].module_name.bytes;
                delete last_module;
            }
            delete imports[i].import_name.bytes;
        }
        delete imports;
    }

    // Attempt to locate exports
    if (res != wasmer_result_t::WASMER_OK) {
        for (int i = 0; i < import_count; i++) {
            luaL_unref(L, LUA_REGISTRYINDEX, refs[i]);
        }
        delete refs;

        lua_pushnil(L);
        wasm_pusherror(L);
        return 2;
    }

    build_exports(L, instance, refs, import_count);

    return 1;
}

bool is_instance(lua_State* L, int index)
{
    lua_getmetatable(L, index);
    luaL_getmetatable(L, INSTANCE_NAME);
    bool test = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    return test;

   return 0;
}

WasmerInstance* to_instance (lua_State *L, int index)
{
    WasmerInstance* instance = (WasmerInstance *)lua_touserdata(L, index);
    if (instance == NULL) luaL_typerror(L, index, INSTANCE_NAME);
    return instance;
}

static int instance_gc(lua_State* L)
{
    WasmerInstance* instance = to_instance(L, 1);

    // Release references        
    for (int i = 0; i < instance->ref_count; i++) {
        luaL_unref(L, LUA_REGISTRYINDEX, instance->refs[i]);
    }
    delete instance->refs;
    wasmer_exports_destroy(instance->exports);
    wasmer_instance_destroy(instance->instance);
    
    return 0;
}

static int instance_tostring(lua_State* L)
{
    WasmerInstance* instance = to_instance(L, 1);
    lua_pushfstring(L, INSTANCE_NAME "(0x%x)",(int)instance);

    return 1;
}

static const luaL_reg instance_meta[] =
{
    {"__gc",       instance_gc},
    {"__tostring", instance_tostring},
    {0, 0}
};

void register_instance(lua_State* L)
{
    luaL_newmetatable(L, INSTANCE_NAME);
    luaL_register(L, NULL, instance_meta);
    lua_pop(L, 1);
}
