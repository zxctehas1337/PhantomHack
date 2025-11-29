#include "FOV.h"
#include "../Offsets.h"
#include "../Globals.h"
#include <cmath>

namespace FOV {
    
    float g_OriginalFOV = 90.0f;
    float g_OriginalViewmodelFOV = 68.0f;
    bool g_FOVInitialized = false;
    
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
    
    void RunFOV() {
        if (!cfg::fov_enabled || !g_ClientBase) return;
        
        // Простой подход - попробуем найти и изменить FOV в памяти
        uintptr_t localPawn = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        // Пробуем разные оффсеты для FOV
        static uintptr_t fovOffsets[] = {
            0x1150 + 0x214,  // CameraServices + m_iFOV
            0x1150 + 0x218,  // CameraServices + m_iFOVStart
            0x1578,          // m_angEyeAngles + FOV
            0x1570,          // Альтернативный оффсет
        };
        
        // Сохраняем оригинальные значения при первом запуске
        if (!g_FOVInitialized) {
            for (int i = 0; i < 4; i++) {
                float currentFOV = Read<float>(localPawn + fovOffsets[i]);
                if (currentFOV > 60.0f && currentFOV < 120.0f) {
                    g_OriginalFOV = currentFOV;
                    break;
                }
            }
            g_FOVInitialized = true;
        }
        
        // Применяем FOV ко всем возможным оффсетам
        for (int i = 0; i < 4; i++) {
            float currentFOV = Read<float>(localPawn + fovOffsets[i]);
            if (currentFOV > 60.0f && currentFOV < 120.0f) {
                Write<float>(localPawn + fovOffsets[i], cfg::fov_value);
            }
        }
        
        // Также пробуем через CameraServices
        uintptr_t cameraServices = Read<uintptr_t>(localPawn + 0x1150);
        if (cameraServices) {
            Write<float>(cameraServices + 0x214, cfg::fov_value);      // m_iFOV
            Write<float>(cameraServices + 0x218, cfg::viewmodel_fov);  // viewmodel FOV
        }
    }
    
    void ResetFOV() {
        // Заглушка
    }
}