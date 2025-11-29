#pragma once

// Обновленные офсеты CS2 (2025-11-25)
// Generated using https://github.com/a2x/cs2-dumper

namespace Offsets {
    namespace client_dll {
        constexpr uintptr_t dwLocalPlayerPawn = 0x1BECF38;
        constexpr uintptr_t dwLocalPlayerController = 0x1E1BC58;
        constexpr uintptr_t dwEntityList = 0x1D11D78;
        constexpr uintptr_t dwViewMatrix = 0x1E30450;
        constexpr uintptr_t dwViewAngles = 0x1E3A880;
        constexpr uintptr_t dwGlobalVars = 0x1BE21C0;
        constexpr uintptr_t dwGameRules = 0x1E2F490;
        constexpr uintptr_t dwPlantedC4 = 0x1E34C68;
        constexpr uintptr_t dwGlowManager = 0x1E2C338;
    }
    
    namespace C_BaseEntity {
        constexpr uintptr_t m_iHealth = 0x34C;
        constexpr uintptr_t m_iMaxHealth = 0x348;
        constexpr uintptr_t m_iTeamNum = 0x3EB;
        constexpr uintptr_t m_fFlags = 0x3F8;
        constexpr uintptr_t m_lifeState = 0x354;
        constexpr uintptr_t m_vecAbsVelocity = 0x3FC;
        constexpr uintptr_t m_pGameSceneNode = 0x330;
    }
    
    namespace CGameSceneNode {
        constexpr uintptr_t m_vecAbsOrigin = 0xD0;
        constexpr uintptr_t m_angAbsRotation = 0xDC;
    }
    
    namespace C_CSPlayerPawn {
        constexpr uintptr_t m_ArmorValue = 0x23F4;
    }
    
    namespace C_CSPlayerPawnBase {
        constexpr uintptr_t m_angEyeAngles = 0x1578;
        constexpr uintptr_t m_iIDEntIndex = 0x1450;
        constexpr uintptr_t m_hOriginalController = 0x1660;
    }
    
    namespace CCSPlayerController {
        constexpr uintptr_t m_bPawnIsAlive = 0x904;
        constexpr uintptr_t m_hPlayerPawn = 0x8FC;
        constexpr uintptr_t m_sSanitizedPlayerName = 0x850;
        constexpr uintptr_t m_szClan = 0x848;
        constexpr uintptr_t m_iPawnHealth = 0x908;
        constexpr uintptr_t m_iPawnArmor = 0x90C;
    }
}
