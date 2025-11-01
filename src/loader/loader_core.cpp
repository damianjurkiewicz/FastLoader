#include "pch.h"
#include "loader_core.h"
#include "audio.h"
#include "cargrp.h"
#include "objdat.h"
#include "additionaltxd.h"
#include "default_audio_data.h" // This include should now work

FastLoader::FastLoader(HINSTANCE pluginHandle)
{
    handle = pluginHandle;
    IsPluginNameValid(); 
}


bool FastLoader::IsPluginNameValid()
{
    MessageBox(
        NULL,
        "Asi no 3'.",
        MODNAME,
        MB_OK | MB_ICONINFORMATION
    );
    return true;
}




 
