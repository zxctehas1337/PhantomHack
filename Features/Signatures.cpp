#include "Signatures.h"
#include <iostream>

namespace Signatures {
    
    uintptr_t g_GetFOVAddress = 0;
    uintptr_t g_FindConVarAddress = 0;
    uintptr_t g_ViewMatrixAddress = 0;
    
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
        if (!module) return 0;
        
        MODULEINFO moduleInfo;
        if (!GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(moduleInfo))) {
            return 0;
        }
        
        return FindPattern((uintptr_t)module, moduleInfo.SizeOfImage, pattern, mask);
    }
    
    bool InitializeSignatures() {
        printf("[Signatures] Searching for CS2 signatures...\n");
        
        // Поиск GetFOV функции
        g_GetFOVAddress = FindPatternInModule("client.dll", CS2::GetFOV_Pattern, CS2::GetFOV_Mask);
        if (g_GetFOVAddress) {
            printf("[Signatures] GetFOV found at: 0x%llX\n", g_GetFOVAddress);
        } else {
            printf("[Signatures] GetFOV not found!\n");
        }
        
        // Поиск FindConVar функции
        g_FindConVarAddress = FindPatternInModule("client.dll", CS2::FindConVar_Pattern, CS2::FindConVar_Mask);
        if (g_FindConVarAddress) {
            printf("[Signatures] FindConVar found at: 0x%llX\n", g_FindConVarAddress);
        } else {
            printf("[Signatures] FindConVar not found!\n");
        }
        
        // Поиск ViewMatrix
        g_ViewMatrixAddress = FindPatternInModule("client.dll", CS2::ViewMatrix_Pattern, CS2::ViewMatrix_Mask);
        if (g_ViewMatrixAddress) {
            printf("[Signatures] ViewMatrix found at: 0x%llX\n", g_ViewMatrixAddress);
        } else {
            printf("[Signatures] ViewMatrix not found!\n");
        }
        
        return (g_GetFOVAddress != 0);
    }
}