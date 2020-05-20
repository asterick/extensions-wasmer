#include <dmsdk/sdk.h>

#include "wasmer.hh"
#include "extension.h"
#include "memory.h"

bool is_memory(lua_State* L, int index)
{
    return luaL_checkudata(L, index, MEMORY_NAME) != NULL;
}

WasmerMemory* to_memory (lua_State *L, int index)
{
    WasmerMemory* mem = (WasmerMemory *)lua_touserdata(L, index);
    if (mem == NULL) luaL_typerror(L, index, MEMORY_NAME);
    return mem;
}

static int memory_new(lua_State* L)
{
    int top = lua_gettop(L);
    
    // Verify we were passed a table
    DM_LUA_STACK_CHECK(L, 1);
    luaL_checktype(L, 1, LUA_TTABLE);

    wasmer_limits_t limits = {
        .max = { .has_some = false }
    };

    // Define limits
    lua_pushstring(L, "minimum");
    lua_gettable(L, 1);
    limits.min = luaL_checkint(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "maximum");
    lua_gettable(L, 1);
    if (!lua_isnil(L, -1)) {
        limits.max.some = luaL_checkint(L, -1);
        limits.max.has_some = true;
    }
    lua_pop(L, 1);
   
    // Create the memory object
    wasmer_memory_t* mem;
    wasmer_result_t res = wasmer_memory_new(&mem, limits);

    if (res != wasmer_result_t::WASMER_OK) {
        lua_pushnil(L);
        wasm_pusherror(L);

        dmLogInfo("%i\n", top - lua_gettop(L));
        return 2;
    }

    // Create mapped user data object
    WasmerMemory* memory = (WasmerMemory*) lua_newuserdata(L, sizeof(WasmerMemory));
    luaL_getmetatable(L, MEMORY_NAME);
    lua_setmetatable(L, -2);
    memory->mem = mem;

    return 1;
}

static int memory_grow(lua_State* L)
{
    WasmerMemory* memory = to_memory(L, 1);

    wasmer_result_t res = wasmer_memory_grow(memory->mem, luaL_checkint(L, 2));

    if (res != wasmer_result_t::WASMER_OK) {
        wasm_pusherror(L);
        return 1;
    }

    return 0;
}

static int memory_gc(lua_State* L)
{
    WasmerMemory* memory = to_memory(L, 1);

    if (memory->mem) wasmer_memory_destroy(memory->mem);

    return 0;
}

static int memory_length(lua_State* L)
{
    WasmerMemory* memory = to_memory(L, 1);
    lua_pushinteger(L, wasmer_memory_data_length(memory->mem));

    return 1;
}

static int memory_tostring(lua_State* L)
{
    WasmerMemory* memory = to_memory(L, 1);
    lua_pushfstring(L, MEMORY_NAME "(%i)", wasmer_memory_data_length(memory->mem));

    return 1;
}

template <typename T>
static int memory_get(lua_State* L)
{
    WasmerMemory* memory = to_memory(L, 1);
    uint8_t* data = (uint8_t*) wasmer_memory_data(memory->mem);
    int size = wasmer_memory_data_length(memory->mem);
    int index = luaL_checkint(L, 2);

    luaL_argcheck(L, 0 <= index && (index + sizeof(T)) <= size, 2, "index out of range");

    T value;
    memcpy(&value, &data[index], sizeof(value));
    lua_pushnumber(L, (lua_Number)value);
    return 1;
}

template <typename T>
static int memory_set(lua_State* L)
{
    WasmerMemory* memory = to_memory(L, 1);
    uint8_t* data = (uint8_t*) wasmer_memory_data(memory->mem);
    int size = wasmer_memory_data_length(memory->mem);
    int index = luaL_checkint(L, 2);

    luaL_argcheck(L, 0 <= index && (index + sizeof(T)) <= size, 2, "index out of range");

    T value = (T)luaL_checknumber(L, 3);   
    memcpy(&data[index], &value, sizeof(value));

    return 0;
}

static int memory_read(lua_State* L)
{
    WasmerMemory* memory = to_memory(L, 1);
    uint8_t* data = (uint8_t*) wasmer_memory_data(memory->mem);
    int size = wasmer_memory_data_length(memory->mem);
    int index = luaL_checkint(L, 2);
    int length = luaL_checkint(L, 3);

    luaL_argcheck(L, 0 <= index && (index + length) <= size, 2, "index out of range");

    lua_pushlstring(L, (const char*)&data[index], length);
    
    return 1;
}

static int memory_write(lua_State* L)
{
    WasmerMemory* memory = to_memory(L, 1);
    uint8_t* data = (uint8_t*) wasmer_memory_data(memory->mem);
    int size = wasmer_memory_data_length(memory->mem);
    int index = luaL_checkint(L, 2);
    size_t length;
    const char* str = luaL_checklstring(L, 3, &length);

    luaL_argcheck(L, 0 <= index && (index + length) <= size, 2, "index out of range");

    memcpy(&data[index], str, length);

    return 0;
}

// Functions exposed to Lua
static const luaL_reg memory_methods[] =
{
    {"new", memory_new},
    {"grow", memory_grow},
    {"read", memory_read},
    {"write", memory_write},

    {"setI8", memory_set<int8_t>},
    {"setI16", memory_set<int16_t>},
    {"setI32", memory_set<int32_t>},
    {"setU8", memory_set<uint8_t>},
    {"setU16", memory_set<uint16_t>},
    {"setU32", memory_set<uint32_t>},
    {"setF32", memory_set<float>},
    {"setF64", memory_set<double>},

    {"getI8", memory_get<int8_t>},
    {"getI16", memory_get<int16_t>},
    {"getI32", memory_get<int32_t>},
    {"getU8", memory_get<uint8_t>},
    {"getU16", memory_get<uint16_t>},
    {"getU32", memory_get<uint32_t>},
    {"getF32", memory_get<float>},
    {"getF64", memory_get<double>},
    {0, 0}
};

static int memory_get(lua_State* L)
{
    // Basic numeric index
    if (lua_isnumber(L, 2)) {
        return memory_get<uint8_t>(L);
    }

    // Attempt to locate the function we need
    const char* name = luaL_checkstring(L, 2);
    const luaL_reg* seek = &memory_methods[0];

    while (seek->name) {
        if (strcmp(seek->name, name)) {
            seek++;
            continue ;
        }

        lua_pushcfunction(L, seek->func);
        return 1;
    }

    return 0;
}

static const luaL_reg memory_meta[] =
{
    {"__gc",       memory_gc},
    {"__tostring", memory_tostring},
    {"__len",      memory_length},
    {"__newindex", memory_set<uint8_t>},
    {"__index",    memory_get},
    {0, 0}
};

void register_memory(lua_State* L)
{
    lua_newtable(L);
    luaL_register(L, NULL, memory_methods);
    luaL_newmetatable(L, MEMORY_NAME);
    luaL_register(L, NULL, memory_meta);
    lua_pop(L, 1);
}
