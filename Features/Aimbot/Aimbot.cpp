#include "Aimbot.h"
#include "../Offsets.h"
#include "../Globals.h"
#include "../ESP/ESP.h"
#include <cmath>
#include <algorithm>

namespace Aimbot {
    
    // Bone indices for CS2
    enum BoneIndex {
        BONE_HEAD = 6,
        BONE_NECK = 5,
        BONE_CHEST = 4,
        BONE_STOMACH = 3,
        BONE_PELVIS = 0
    };
    
    template<typename T>
    T Read(uintptr_t addr) {
        if (!addr) return T{};
        __try {
            return *reinterpret_cast<T*>(addr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return T{};
        }
    }
    
    void Write(uintptr_t addr, float value) {
        if (!addr) return;
        __try {
            *reinterpret_cast<float*>(addr) = value;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return;
        }
    }
    
    void CalcAngle(float* src, float* dst, float* angles) {
        float delta[3] = { dst[0] - src[0], dst[1] - src[1], dst[2] - src[2] };
        float hyp = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);
        
        angles[0] = asinf(-delta[2] / sqrtf(hyp * hyp + delta[2] * delta[2])) * 57.295779513082f;
        angles[1] = atanf(delta[1] / delta[0]) * 57.295779513082f;
        angles[2] = 0.0f;
        
        if (delta[0] >= 0.0f) angles[1] += 180.0f;
    }
    
    float GetFOV(float* viewAngles, float* aimAngles) {
        float delta[3];
        delta[0] = aimAngles[0] - viewAngles[0];
        delta[1] = aimAngles[1] - viewAngles[1];
        delta[2] = 0.0f;
        
        while (delta[1] > 180.0f) delta[1] -= 360.0f;
        while (delta[1] < -180.0f) delta[1] += 360.0f;
        while (delta[0] > 89.0f) delta[0] -= 180.0f;
        while (delta[0] < -89.0f) delta[0] += 180.0f;
        
        return sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);
    }
    
    float GetDistance(float* src, float* dst) {
        float delta[3] = { dst[0] - src[0], dst[1] - src[1], dst[2] - src[2] };
        return sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
    }
    
    bool IsVisible(float* start, float* end) {
        // Simplified - would need proper ray tracing
        return true;
    }
    
    float GetBoneHeight(int boneIndex) {
        switch (boneIndex) {
            case 0: return 72.0f;  // Head
            case 1: return 65.0f;  // Neck
            case 2: return 50.0f;  // Chest
            case 3: return 40.0f;  // Stomach
            case 4: return 30.0f;  // Pelvis
            default: return 50.0f;
        }
    }
    
    // ==================== LEGITBOT ====================
    void RunLegitAimbot() {
        if (!cfg::aim_legit_enabled || !g_ClientBase) return;
        
        // Check activation key
        bool keyPressed = false;
        switch (cfg::aim_legit_key) {
            case 0: keyPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; break;
            case 1: keyPressed = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0; break;
            case 2: keyPressed = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0; break;
            case 3: keyPressed = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0; break;
            case 4: keyPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0; break;
            default: keyPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; break;
        }
        if (!keyPressed) return;
        
        uintptr_t localPawn = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        float viewAngles[3];
        viewAngles[0] = Read<float>(localPawn + Offsets::C_CSPlayerPawnBase::m_angEyeAngles);
        viewAngles[1] = Read<float>(localPawn + Offsets::C_CSPlayerPawnBase::m_angEyeAngles + 4);
        viewAngles[2] = 0.0f;
        
        float eyePos[3] = { ESP::g_LocalPos[0], ESP::g_LocalPos[1], ESP::g_LocalPos[2] + 64.0f };
        
        float bestFOV = cfg::aim_legit_fov;
        float bestAngles[3] = {0, 0, 0};
        bool foundTarget = false;
        
        for (const auto& player : ESP::g_Players) {
            if (!player.valid || !player.isEnemy) continue;
            
            // Visibility check
            if (cfg::aim_legit_vischeck && !IsVisible(eyePos, (float*)&player.posX)) continue;
            
            float targetPos[3] = { player.posX, player.posY, player.posZ };
            targetPos[2] += GetBoneHeight(cfg::aim_legit_bone);
            
            float aimAngles[3];
            CalcAngle(eyePos, targetPos, aimAngles);
            
            float fov = GetFOV(viewAngles, aimAngles);
            if (fov < bestFOV) {
                bestFOV = fov;
                bestAngles[0] = aimAngles[0];
                bestAngles[1] = aimAngles[1];
                foundTarget = true;
            }
        }
        
        if (foundTarget) {
            // Apply smoothing
            float smooth = cfg::aim_legit_smooth;
            if (smooth < 1.0f) smooth = 1.0f;
            
            float deltaX = (bestAngles[0] - viewAngles[0]) / smooth;
            float deltaY = bestAngles[1] - viewAngles[1];
            
            while (deltaY > 180.0f) deltaY -= 360.0f;
            while (deltaY < -180.0f) deltaY += 360.0f;
            deltaY /= smooth;
            
            // RCS (Recoil Control System)
            if (cfg::aim_legit_rcs > 0.0f) {
                float punchX = Read<float>(localPawn + 0x1584); // m_aimPunchAngle
                float punchY = Read<float>(localPawn + 0x1588);
                deltaX -= punchX * 2.0f * (cfg::aim_legit_rcs / 100.0f);
                deltaY -= punchY * 2.0f * (cfg::aim_legit_rcs / 100.0f);
            }
            
            float newAngles[3] = { viewAngles[0] + deltaX, viewAngles[1] + deltaY, 0.0f };
            
            // Clamp angles
            if (newAngles[0] > 89.0f) newAngles[0] = 89.0f;
            if (newAngles[0] < -89.0f) newAngles[0] = -89.0f;
            while (newAngles[1] > 180.0f) newAngles[1] -= 360.0f;
            while (newAngles[1] < -180.0f) newAngles[1] += 360.0f;
            
            Write(localPawn + Offsets::C_CSPlayerPawnBase::m_angEyeAngles, newAngles[0]);
            Write(localPawn + Offsets::C_CSPlayerPawnBase::m_angEyeAngles + 4, newAngles[1]);
        }
    }
    
    // ==================== RAGEBOT ====================
    static DWORD lastShotTime = 0;
    
    void RunRageAimbot() {
        if (!cfg::aim_rage_enabled || !g_ClientBase) return;
        
        uintptr_t localPawn = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        float viewAngles[3];
        viewAngles[0] = Read<float>(localPawn + Offsets::C_CSPlayerPawnBase::m_angEyeAngles);
        viewAngles[1] = Read<float>(localPawn + Offsets::C_CSPlayerPawnBase::m_angEyeAngles + 4);
        viewAngles[2] = 0.0f;
        
        float eyePos[3] = { ESP::g_LocalPos[0], ESP::g_LocalPos[1], ESP::g_LocalPos[2] + 64.0f };
        
        // Target selection based on priority
        float bestValue = FLT_MAX;
        float bestAngles[3] = {0, 0, 0};
        bool foundTarget = false;
        int bestHealth = INT_MAX;
        
        for (const auto& player : ESP::g_Players) {
            if (!player.valid || !player.isEnemy) continue;
            
            float targetPos[3] = { player.posX, player.posY, player.posZ };
            targetPos[2] += GetBoneHeight(cfg::aim_rage_bone);
            
            float aimAngles[3];
            CalcAngle(eyePos, targetPos, aimAngles);
            
            float fov = GetFOV(viewAngles, aimAngles);
            if (fov > cfg::aim_rage_fov) continue;
            
            float distance = GetDistance(eyePos, targetPos);
            
            bool isBetter = false;
            int priority = cfg::aim_rage_priority;
            if (priority == 0) { // FOV
                isBetter = fov < bestValue;
                if (isBetter) bestValue = fov;
            } else if (priority == 1) { // Distance
                isBetter = distance < bestValue;
                if (isBetter) bestValue = distance;
            } else if (priority == 2) { // Health
                isBetter = player.health < bestHealth;
                if (isBetter) bestHealth = player.health;
            } else {
                isBetter = fov < bestValue;
                if (isBetter) bestValue = fov;
            }
            
            if (isBetter) {
                bestAngles[0] = aimAngles[0];
                bestAngles[1] = aimAngles[1];
                foundTarget = true;
            }
        }
        
        if (foundTarget) {
            // Instant aim (no smoothing for rage)
            float newAngles[3] = { bestAngles[0], bestAngles[1], 0.0f };
            
            // Clamp
            if (newAngles[0] > 89.0f) newAngles[0] = 89.0f;
            if (newAngles[0] < -89.0f) newAngles[0] = -89.0f;
            while (newAngles[1] > 180.0f) newAngles[1] -= 360.0f;
            while (newAngles[1] < -180.0f) newAngles[1] += 360.0f;
            
            // Silent aim - only set angles if not silent
            if (!cfg::aim_rage_silent) {
                Write(localPawn + Offsets::C_CSPlayerPawnBase::m_angEyeAngles, newAngles[0]);
                Write(localPawn + Offsets::C_CSPlayerPawnBase::m_angEyeAngles + 4, newAngles[1]);
            }
            
            // Auto shoot
            if (cfg::aim_rage_autoshoot) {
                DWORD currentTime = GetTickCount();
                if (currentTime - lastShotTime > 50) { // 50ms delay between shots
                    INPUT input = {0};
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    SendInput(1, &input, sizeof(INPUT));
                    Sleep(10);
                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &input, sizeof(INPUT));
                    lastShotTime = currentTime;
                }
            }
            
            // Auto scope for snipers
            if (cfg::aim_rage_autoscope) {
                // Check if holding sniper and not scoped
                // Would need weapon ID check here
            }
        }
    }
    
    void RunAimbot() {
        // Run both - they check their own enabled flags
        RunLegitAimbot();
        RunRageAimbot();
    }
}
