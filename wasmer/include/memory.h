#pragma once

#define MEMORY_NAME "wasmer.memory"

struct WasmerMemory {
	struct wasmer_memory_t* mem;
	int owner;
};

WasmerMemory* to_memory (lua_State *L, int index);
void memory_from_export(lua_State* L, wasmer_memory_t* mem, int index);
bool is_memory(lua_State* L, int index);
void register_memory(lua_State* L);
