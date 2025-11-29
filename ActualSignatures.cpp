#include "ActualSignatures.h"
#include <iostream>
#include <Psapi.h>

namespace SignatureFinder {
    
    uintptr_t g_GetFOVAddress = 0;
    uintptr_t g_ViewmodelFOVAddress = 0;
    uintptr_t g_ConVarSystemAddress = 0;
    uintptr_t g_ViewMatrixAddress = 0;
    bool g_SignaturesInitialized = false;
    
    uintptr_t FindPattern(uintptr_t moduleBase, size_t moduleSize, const char* pattern, const char* mask) {
        size_t patternLength = strlen(mask);
        
        for (size_t i = 0; i <= moduleSize - patternLength; i++) {
            bool found = true;
            for (size_t j = 0; j < patternLength; j++) {
                if (mask[j] != '?' && pattern[j] != *(char*)(moduleBase + i + j)) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return moduleBase + i;
            }
        }
        return 0;
    }
    
    uintptr_t FindPatternInModule(const char* moduleName, const char* pattern, const char* mask) {
        HMODULE module = GetModuleHandleA(moduleName);
        if (!module) {
            printf("[Signatures] Модуль %s не найден!\n", moduleName);
            return 0;
        }
        
        MODULEINFO moduleInfo;
        if (!GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(moduleInfo))) {
            printf("[Signatures] Не удалось получить информацию о модуле %s!\n", moduleName);
            return 0;
        }
        
        printf("[Signatures] Поиск в %s (база: 0x%llX, размер: 0x%X)...\n", 
               moduleName, (uintptr_t)module, moduleInfo.SizeOfImage);
        
        return FindPattern((uintptr_t)module, moduleInfo.SizeOfImage, pattern, mask);
    }
    
    uintptr_t FindGetFOV() {
        printf("[Signatures] Поиск GetFOV...\n");
        
        // Пробуем основную сигнатуру
        uintptr_t address = FindPatternInModule("client.dll", 
            Signatures::CS2::GetFOV_Pattern, 
            Signatures::CS2::GetFOV_Mask);
        
        if (address) {
            printf("[Signatures] GetFOV найден: 0x%llX\n", address);
            return address;
        }
        
        // Пробуем альтернативную сигнатуру
        address = FindPatternInModule("client.dll",
            Signatures::CS2::GetFOV_Alt_Pattern,
            Signatures::CS2::GetFOV_Alt_Mask);
        
        if (address) {
            printf("[Signatures] GetFOV (alt) найден: 0x%llX\n", address);
            return address;
        }
        
        printf("[Signatures] GetFOV НЕ НАЙДЕН!\n");
        return 0;
    }
    
    uintptr_t FindViewmodelFOV() {
        printf("[Signatures] Поиск ViewmodelFOV...\n");
        
        uintptr_t address = FindPatternInModule("client.dll",
            Signatures::CS2::ViewmodelFOV_Pattern,
            Signatures::CS2::ViewmodelFOV_Mask);
        
        if (address) {
            printf("[Signatures] ViewmodelFOV найден: 0x%llX\n", address);
        } else {
            printf("[Signatures] ViewmodelFOV НЕ НАЙДЕН!\n");
        }
        
        return address;
    }
    
    uintptr_t FindConVarSystem() {
        printf("[Signatures] Поиск ConVar системы...\n");
        
        uintptr_t address = FindPatternInModule("client.dll",
            Signatures::CS2::FindConVar_Pattern,
            Signatures::CS2::FindConVar_Mask);
        
        if (address) {
            printf("[Signatures] ConVar система найдена: 0x%llX\n", address);
        } else {
            printf("[Signatures] ConVar система НЕ НАЙДЕНА!\n");
        }
        
        return address;
    }
    
    uintptr_t FindViewMatrix() {
        printf("[Signatures] Поиск ViewMatrix...\n");
        
        uintptr_t address = FindPatternInModule("client.dll",
            Signatures::CS2::ViewMatrix_Pattern,
            Signatures::CS2::ViewMatrix_Mask);
        
        if (address) {
            // ViewMatrix обычно находится по относительному адресу
            uintptr_t realAddress = address + *(int*)(address + 3) + 7;
            printf("[Signatures] ViewMatrix найден: 0x%llX (реальный: 0x%llX)\n", address, realAddress);
            return realAddress;
        } else {
            printf("[Signatures] ViewMatrix НЕ НАЙДЕН!\n");
        }
        
        return address;
    }
    
    bool InitializeAllSignatures() {
        printf("\n[Signatures] ========== ПОИСК СИГНАТУР CS2 ==========\n");
        
        // Поиск всех сигнатур
        g_GetFOVAddress = FindGetFOV();
        g_ViewmodelFOVAddress = FindViewmodelFOV();
        g_ConVarSystemAddress = FindConVarSystem();
        g_ViewMatrixAddress = FindViewMatrix();
        
        // Подсчет найденных сигнатур
        int foundCount = 0;
        if (g_GetFOVAddress) foundCount++;
        if (g_ViewmodelFOVAddress) foundCount++;
        if (g_ConVarSystemAddress) foundCount++;
        if (g_ViewMatrixAddress) foundCount++;
        
        printf("\n[Signatures] ========== РЕЗУЛЬТАТ ==========\n");
        printf("[Signatures] Найдено сигнатур: %d/4\n", foundCount);
        
        if (foundCount > 0) {
            printf("[Signatures] ✅ Найденные сигнатуры:\n");
            if (g_GetFOVAddress) printf("  - GetFOV: 0x%llX\n", g_GetFOVAddress);
            if (g_ViewmodelFOVAddress) printf("  - ViewmodelFOV: 0x%llX\n", g_ViewmodelFOVAddress);
            if (g_ConVarSystemAddress) printf("  - ConVarSystem: 0x%llX\n", g_ConVarSystemAddress);
            if (g_ViewMatrixAddress) printf("  - ViewMatrix: 0x%llX\n", g_ViewMatrixAddress);
        }
        
        if (foundCount < 4) {
            printf("[Signatures] ❌ Не найденные сигнатуры:\n");
            if (!g_GetFOVAddress) printf("  - GetFOV\n");
            if (!g_ViewmodelFOVAddress) printf("  - ViewmodelFOV\n");
            if (!g_ConVarSystemAddress) printf("  - ConVarSystem\n");
            if (!g_ViewMatrixAddress) printf("  - ViewMatrix\n");
            
            printf("[Signatures] ⚠️  Обнови сигнатуры с GitHub!\n");
        }
        
        g_SignaturesInitialized = (foundCount > 0);
        printf("[Signatures] ==========================================\n\n");
        
        return g_SignaturesInitialized;
    }
}