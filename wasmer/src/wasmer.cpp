#define LIB_NAME "Wasmer"

#include <dmsdk/sdk.h>

#include "extension.h"

dmExtension::Result AppInitialize(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializeMyExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result Initialize(dmExtension::Params* params)
{
    wasm_register_extension(params->m_L);

    dmLogInfo("Registered %s Extension\n", LIB_NAME);
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalize(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizeMyExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result Finalize(dmExtension::Params* params)
{
    dmLogInfo("FinalizeMyExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result OnUpdate(dmExtension::Params* params)
{
    //dmLogInfo("OnUpdateMyExtension\n");
    return dmExtension::RESULT_OK;
}

void OnEvent(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            //dmLogInfo("OnEvent - EVENT_ID_ACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            //dmLogInfo("OnEvent - EVENT_ID_DEACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            //dmLogInfo("OnEvent - EVENT_ID_ICONIFYAPP\n");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            //dmLogInfo("OnEvent - EVENT_ID_DEICONIFYAPP\n");
            break;
        default:
            dmLogWarning("OnEvent - Unknown event id\n");
            break;
    }
}

// Defold SDK uses a macro for setting up extension entry points:
DM_DECLARE_EXTENSION(wasmer, LIB_NAME, AppInitialize, AppFinalize, Initialize, OnUpdate, OnEvent, Finalize)
