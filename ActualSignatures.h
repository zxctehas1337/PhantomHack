// Актуальные сигнатуры CS2 (29.11.2025)
// Источник: cs2-dumper + сообществоfdfscxsfdfxsdfsfdfddfssfs

#pragma once

namespace Signatures {
    
    // Поиск паттерна в памяти
    uintptr_t FindPattern(uintptr_t moduleBase, size_t moduleSize, const char* pattern, const char* mask);
    uintptr_t FindPatternInModule(const char* moduleName, const char* pattern, const char* mask);
    
    namespace CS2 {
        
        // ============ FOV СИГНАТУРЫ ============
        
        // GetFOV функция (проверено 29.11.2025)
        constexpr const char* GetFOV_Pattern = "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8B\xF9\xF3\x0F\x10\x87";
        constexpr const char* GetFOV_Mask = "xxxxxxxxxxxxxxxxx";
        
        // Альтернативная GetFOV сигнатура
        constexpr const char* GetFOV_Alt_Pattern = "\xF3\x0F\x10\x87\x14\x02\x00\x00\xC3";
        constexpr const char* GetFOV_Alt_Mask = "xxxxxxxxx";
        
        // GetViewmodelFOV функция
        constexpr const char* ViewmodelFOV_Pattern = "\xF3\x0F\x10\x87\x18\x02\x00\x00\xC3";
        constexpr const char* ViewmodelFOV_Mask = "xxxxxxxxx";
        
        // ============ CONVAR СИГНАТУРЫ ============
        
        // FindConVar функция
        constexpr const char* FindConVar_Pattern = "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x48\x8B\xF2";
        constexpr const char* FindConVar_Mask = "xxxxxxxxxxxxxxxxxx";
        
        // ConVar fov_cs_debug (строка)
        constexpr const char* FOVConVar_String = "fov_cs_debug";
        
        // ConVar viewmodel_fov (строка)  
        constexpr const char* ViewmodelConVar_String = "viewmodel_fov";
        
        // ============ ESP СИГНАТУРЫ ============
        
        // ViewMatrix
        constexpr const char* ViewMatrix_Pattern = "\x48\x8D\x0D\x00\x00\x00\x00\x48\xC1\xE0\x06";
        constexpr const char* ViewMatrix_Mask = "xxx????xxxx";
        
        // EntityList
        constexpr const char* EntityList_Pattern = "\x48\x8B\x0D\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x8B\xFA";
        constexpr const char* EntityList_Mask = "xxx????xxxx?xx";
        
        // LocalPlayerPawn
        constexpr const char* LocalPlayer_Pattern = "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\x48";
        constexpr const char* LocalPlayer_Mask = "xxx????xxxx?xxx";
        
        // ============ AIMBOT СИГНАТУРЫ ============
        
        // GetBonePosition
        constexpr const char* BonePosition_Pattern = "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x40";
        constexpr const char* BonePosition_Mask = "xxxxxxxxxxxxxxxxxxxx";
        
        // IsVisible (трассировка лучей)
        constexpr const char* IsVisible_Pattern = "\x55\x48\x8B\xEC\x48\x83\xEC\x70\x48\x89\x4D\xF0";
        constexpr const char* IsVisible_Mask = "xxxxxxxxxxxx";
        
        // ============ MOVEMENT СИГНАТУРЫ ============
        
        // SetButtons (для bhop)
        constexpr const char* SetButtons_Pattern = "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x8B\xDA";
        constexpr const char* SetButtons_Mask = "xxxxxxxxxxxxxxxxx";
        
        // GetFlags
        constexpr const char* GetFlags_Pattern = "\x8B\x81\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC";
        constexpr const char* GetFlags_Mask = "xx????xxxxxxxxxx";
    }
    
    // ============ ИЗВЕСТНЫЕ ОФФСЕТЫ ============
    
    namespace KnownOffsets {
        // Эти оффсеты нужно обновлять после каждого патча
        // Источник: https://github.com/a2x/cs2-dumper
        
        constexpr uintptr_t dwLocalPlayerPawn = 0x1BECF38;    // Обновлено 29.11.2025
        constexpr uintptr_t dwEntityList = 0x1D11D78;        // Обновлено 29.11.2025  
        constexpr uintptr_t dwViewMatrix = 0x1E30450;        // Обновлено 29.11.2025
        constexpr uintptr_t dwViewAngles = 0x1E3A880;        // Обновлено 29.11.2025
        
        // Оффсеты в структурах
        constexpr uintptr_t m_iHealth = 0x34C;
        constexpr uintptr_t m_iTeamNum = 0x3EB;
        constexpr uintptr_t m_fFlags = 0x3F8;
        constexpr uintptr_t m_vecAbsOrigin = 0xD0;           // В CGameSceneNode
        constexpr uintptr_t m_pGameSceneNode = 0x330;
        
        // FOV оффсеты
        constexpr uintptr_t m_pCameraServices = 0x1150;
        constexpr uintptr_t m_iFOV = 0x214;                  // В CameraServices
        constexpr uintptr_t m_iFOVStart = 0x218;            // Viewmodel FOV
    }
}

// ============ ФУНКЦИИ ПОИСКА ============

namespace SignatureFinder {
    
    // Инициализация всех сигнатур
    bool InitializeAllSignatures();
    
    // Поиск конкретных функций
    uintptr_t FindGetFOV();
    uintptr_t FindViewmodelFOV();
    uintptr_t FindConVarSystem();
    uintptr_t FindViewMatrix();
    
    // Найденные адреса (заполняются при инициализации)
    extern uintptr_t g_GetFOVAddress;
    extern uintptr_t g_ViewmodelFOVAddress;
    extern uintptr_t g_ConVarSystemAddress;
    extern uintptr_t g_ViewMatrixAddress;
    
    // Статус поиска
    extern bool g_SignaturesInitialized;
}
