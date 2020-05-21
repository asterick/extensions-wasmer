#pragma once

#define INSTANCE_NAME "wasmer.instance"

struct WasmerInstance {
    struct wasmer_instance_t* instance;
    struct wasmer_exports_t* exports;
    int ref_count;
    int* refs;
};

bool is_instance(lua_State* L, int index);
WasmerInstance* to_instance (lua_State *L, int index);
int instance_module(lua_State* L);
void register_instance(lua_State* L);
