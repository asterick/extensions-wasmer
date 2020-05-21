#pragma once

#define IMPORT_NAME "wasmer.import"

struct WasmerImport {
    struct wasmer_instance_t* instance;
    int index;
    int ref_count;
    int* refs;
};

bool is_import(lua_State* L, int index);
WasmerImport* to_import (lua_State *L, int index);
int import_module(lua_State* L);
void register_import(lua_State* L);
