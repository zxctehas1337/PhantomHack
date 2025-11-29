#pragma once
#include <Windows.h>

namespace Aimbot {
    void RunAimbot();
    void RunLegitAimbot();
    void RunRageAimbot();
    bool IsVisible(float* start, float* end);
    float GetFOV(float* viewAngles, float* aimAngles);
    void CalcAngle(float* src, float* dst, float* angles);
    float GetDistance(float* src, float* dst);
}
