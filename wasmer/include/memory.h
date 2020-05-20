#pragma once

#define MEMORY_NAME "wasmer.memory"

struct WasmerMemory {
	struct wasmer_memory_t* mem;
};

WasmerMemory* to_memory (lua_State *L, int index);
bool is_memory(lua_State* L, int index);
void register_memory(lua_State* L);
