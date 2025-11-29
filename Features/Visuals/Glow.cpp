#include "Glow.h"
#include "../Offsets.h"
#include "../Globals.h"
#include "../ESP/ESP.h"

namespace Glow {
    
    template<typename T>
    T Read(uintptr_t addr) {
        if (!addr) return T{};
        __try {
            return *reinterpret_cast<T*>(addr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return T{};
        }
    }
    
    template<typename T>
    void Write(uintptr_t addr, T value) {
        if (!addr) return;
        __try {
            *reinterpret_cast<T*>(addr) = value;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return;
        }
    }
    
    // Оффсеты для Glow
    constexpr uintptr_t m_Glow = 0xCB0;           // CGlowProperty offset in C_BaseModelEntity
    constexpr uintptr_t m_fGlowColor = 0x8;       // Vector (RGB)
    constexpr uintptr_t m_iGlowType = 0x30;       // int32
    constexpr uintptr_t m_nGlowRange = 0x38;      // int32
    constexpr uintptr_t m_nGlowRangeMin = 0x3C;   // int32
    constexpr uintptr_t m_glowColorOverride = 0x40; // Color (RGBA)
    constexpr uintptr_t m_bGlowing = 0x51;        // bool
    
    void SetGlow(uintptr_t pawn, float r, float g, float b) {
        if (!pawn) return;
        
        uintptr_t glowProp = pawn + m_Glow;
        
        // Устанавливаем цвет свечения (Vector RGB)
        Write<float>(glowProp + m_fGlowColor, r);
        Write<float>(glowProp + m_fGlowColor + 4, g);
        Write<float>(glowProp + m_fGlowColor + 8, b);
        
        // Тип свечения (3 = всегда видно)
        Write<int>(glowProp + m_iGlowType, 3);
        
        // Дальность
        Write<int>(glowProp + m_nGlowRange, 99999);
        Write<int>(glowProp + m_nGlowRangeMin, 0);
        
        // Color override (RGBA как uint32)
        uint8_t colorR = (uint8_t)(r * 255.0f);
        uint8_t colorG = (uint8_t)(g * 255.0f);
        uint8_t colorB = (uint8_t)(b * 255.0f);
        uint32_t color = (255 << 24) | (colorB << 16) | (colorG << 8) | colorR;
        Write<uint32_t>(glowProp + m_glowColorOverride, color);
        
        // Включаем свечение
        Write<bool>(glowProp + m_bGlowing, true);
    }
    
    void DisableGlow(uintptr_t pawn) {
        if (!pawn) return;
        
        uintptr_t glowProp = pawn + m_Glow;
        Write<int>(glowProp + m_iGlowType, 0);
        Write<bool>(glowProp + m_bGlowing, false);
    }
    
    void RunGlow() {
        if (!cfg::glow_enabled || !g_ClientBase) return;
        
        uintptr_t localPawn = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        int localTeam = Read<uint8_t>(localPawn + Offsets::C_BaseEntity::m_iTeamNum);
        
        uintptr_t entityList = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwEntityList);
        if (!entityList) return;
        
        for (int i = 1; i <= 64; i++) {
            uintptr_t listEntry = Read<uintptr_t>(entityList + (8 * ((i & 0x7FFF) >> 9) + 16));
            if (!listEntry) continue;
            
            uintptr_t controller = Read<uintptr_t>(listEntry + 120 * (i & 0x1FF));
            if (!controller) continue;
            
            bool isAlive = Read<bool>(controller + Offsets::CCSPlayerController::m_bPawnIsAlive);
            if (!isAlive) continue;
            
            uint32_t pawnHandle = Read<uint32_t>(controller + Offsets::CCSPlayerController::m_hPlayerPawn);
            if (!pawnHandle) continue;
            
            uintptr_t listEntry2 = Read<uintptr_t>(entityList + (8 * ((pawnHandle & 0x7FFF) >> 9) + 16));
            if (!listEntry2) continue;
            
            uintptr_t pawn = Read<uintptr_t>(listEntry2 + 120 * (pawnHandle & 0x1FF));
            if (!pawn || pawn == localPawn) continue;
            
            int team = Read<uint8_t>(pawn + Offsets::C_BaseEntity::m_iTeamNum);
            bool isEnemy = (team != localTeam);
            
            if (isEnemy) {
                // Враги - красный glow
                SetGlow(pawn, cfg::glow_color[0], cfg::glow_color[1], cfg::glow_color[2]);
            }
        }
    }
}
