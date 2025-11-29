#pragma once
#include <Windows.h>

#define FL_ONGROUND (1 << 0)

namespace BunnyHop {
    void RunBhop();
    float GetSpeed();
    extern float g_CurrentSpeed;
}