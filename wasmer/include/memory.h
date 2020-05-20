#pragma once

#define MEMORY_NAME "wasmer.memory"

struct WasmerMemory {
	struct wasmer_memory_t* mem;
};

void register_memory(lua_State* L);
WasmerMemory* to_memory (lua_State *L, int index);
