#pragma once
#include <Windows.h>

namespace World {
    void Initialize();
    void RunWorld();
    void ApplyNightMode();
    void ApplyFullbright();
    void ApplyNoFog();
    void ApplyNoSky();
    void ApplyCustomSky();
    void RemoveSmoke();
    void RemoveFlash();
    void ApplyWorldColor();
    void Reset();
}
