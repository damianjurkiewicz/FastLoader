#pragma once
#include <wtypes.h>

class FastLoader
{
private:
    HINSTANCE handle;

   
    void ParseModloader();
    void HandleVanillaDataFiles(); 

public:
    FastLoader(HINSTANCE pluginHandle);
};

