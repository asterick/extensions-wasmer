#pragma once

#define EXTENSION_NAME "wasmer"

void wasm_register_extension(lua_State* L);
void wasm_pusherror(lua_State* L);
