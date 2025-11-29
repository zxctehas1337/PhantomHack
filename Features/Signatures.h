#pragma once
#include <Windows.h>
#include <vector>

namespace Signatures {
    
    // Поиск паттерна в памяти
    uintptr_t FindPattern(uintptr_t moduleBase, size_t moduleSize, const char* pattern, const char* mask);
    
    // Поиск паттерна в модуле
    uintptr_t FindPatternInModule(const char* moduleName, const char* pattern, const char* mask);
    
    // CS2 сигнатуры (обновляются регулярно)
    namespace CS2 {
        // FOV функции
        constexpr const char* GetFOV_Pattern = "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xD9";
        constexpr const char* GetFOV_Mask = "xxxx?xxxxxxxx";
        
        // ConVar поиск
        constexpr const char* FindConVar_Pattern = "\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20";
        constexpr const char* FindConVar_Mask = "xxxx?xxxx?xxxxx";
        
        // ViewMatrix (для ESP)
        constexpr const char* ViewMatrix_Pattern = "\x48\x8D\x0D\x00\x00\x00\x00\x48\x C1\xE0\x06";
        constexpr const char* ViewMatrix_Mask = "xxx????xxxx";
    }
    
    // Инициализация сигнатур
    bool InitializeSignatures();
    
    // Найденные адреса
    extern uintptr_t g_GetFOVAddress;
    extern uintptr_t g_FindConVarAddress;
    extern uintptr_t g_ViewMatrixAddress;
}