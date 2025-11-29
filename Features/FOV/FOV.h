#pragma once
#include <Windows.h>

namespace FOV {
    void RunFOV();
    void ResetFOV();
    
    // Настройки FOV
    extern float g_OriginalFOV;
    extern float g_OriginalViewmodelFOV;
    extern bool g_FOVInitialized;
}