#include "Triggerbot.h"
#include "../Offsets.h"
#include "../Globals.h"

namespace Triggerbot {
    
    template<typename T>
    T Read(uintptr_t addr) {
        if (!addr) return T{};
        __try {
            return *reinterpret_cast<T*>(addr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return T{};
        }
    }
    
    void RunTriggerbot() {
        if (!cfg::trigger_enabled || !g_ClientBase || !g_GameWnd) return;
        
        uintptr_t localPawn = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        int localTeam = Read<int>(localPawn + Offsets::C_BaseEntity::m_iTeamNum);
        
        // Get crosshair entity
        int crosshairId = Read<int>(localPawn + Offsets::C_CSPlayerPawnBase::m_iIDEntIndex);
        if (crosshairId <= 0 || crosshairId > 64) return;
        
        // Get entity list
        uintptr_t entityList = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwEntityList);
        if (!entityList) return;
        
        // Get target entity
        uintptr_t listEntry = Read<uintptr_t>(entityList + (8 * ((crosshairId & 0x7FFF) >> 9) + 16));
        if (!listEntry) return;
        
        uintptr_t targetPawn = Read<uintptr_t>(listEntry + 120 * (crosshairId & 0x1FF));
        if (!targetPawn) return;
        
        // Check if target is enemy
        int targetTeam = Read<int>(targetPawn + Offsets::C_BaseEntity::m_iTeamNum);
        if (targetTeam == localTeam) return;
        
        // Check if target is alive
        int targetHealth = Read<int>(targetPawn + Offsets::C_BaseEntity::m_iHealth);
        if (targetHealth <= 0) return;
        
        // Trigger with delay
        static DWORD lastShot = 0;
        DWORD now = GetTickCount();
        
        if (now - lastShot >= (DWORD)cfg::trigger_delay) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            Sleep(10);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            lastShot = now;
        }
    }
}