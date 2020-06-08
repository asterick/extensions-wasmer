#define LIB_NAME "Wasmer"

#include <dmsdk/sdk.h>

#include "extension.h"

dmExtension::Result AppInitialize(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

dmExtension::Result Initialize(dmExtension::Params* params)
{
    wasm_register_extension(params->m_L);
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalize(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

dmExtension::Result Finalize(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

// Defold SDK uses a macro for setting up extension entry points:
DM_DECLARE_EXTENSION(wasmer, LIB_NAME, AppInitialize, AppFinalize, Initialize, NULL, NULL, Finalize)
