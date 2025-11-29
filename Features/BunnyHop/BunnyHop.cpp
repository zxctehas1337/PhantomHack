#include "BunnyHop.h"
#include "../Offsets.h"
#include "../Globals.h"
#include <cmath>

namespace BunnyHop {
    
    float g_CurrentSpeed = 0.0f;
    
    template<typename T>
    T Read(uintptr_t addr) {
        if (!addr) return T{};
        __try {
            return *reinterpret_cast<T*>(addr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return T{};
        }
    }
    
    void RunBhop() {
        if (!cfg::misc_bhop) return;
        
        // Простой BunnyHop через keybd_event
        static bool wasSpacePressed = false;
        bool isSpacePressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        
        if (isSpacePressed && !wasSpacePressed) {
            // Начинаем прыжки
            keybd_event(VK_SPACE, 0, 0, 0);
        } else if (isSpacePressed && wasSpacePressed) {
            // Продолжаем прыжки - отпускаем и сразу нажимаем
            keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
            Sleep(1);
            keybd_event(VK_SPACE, 0, 0, 0);
        } else if (!isSpacePressed && wasSpacePressed) {
            // Прекращаем прыжки
            keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
        }
        
        wasSpacePressed = isSpacePressed;
        
        // Обновляем скорость если есть доступ к игре
        if (g_ClientBase) {
            uintptr_t localPawn = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerPawn);
            if (localPawn) {
                float velX = Read<float>(localPawn + Offsets::C_BaseEntity::m_vecAbsVelocity);
                float velY = Read<float>(localPawn + Offsets::C_BaseEntity::m_vecAbsVelocity + 4);
                g_CurrentSpeed = sqrtf(velX * velX + velY * velY);
            } else {
                g_CurrentSpeed = 0.0f;
            }
        }
    }
    
    float GetSpeed() {
        return g_CurrentSpeed;
    }
}
