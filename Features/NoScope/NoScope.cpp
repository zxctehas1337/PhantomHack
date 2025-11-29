#include "NoScope.h"
#include "../Offsets.h"
#include "../Globals.h"
#include <cmath>

namespace NoScope {
    
    bool g_NoScopeActive = false;
    bool g_WasScoped = false;
    float g_OriginalScopeAlpha = 1.0f;
    
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
    
    bool IsSniperWeapon(int weaponID) {
        // ID снайперских винтовок в CS2
        switch (weaponID) {
            case 9:   // AWP
            case 11:  // G3SG1
            case 38:  // SCAR-20
            case 40:  // SSG 08 (Scout)
                return true;
            default:
                return false;
        }
    }
    
    void RunNoScope() {
        if (!cfg::noscope_enabled || !g_ClientBase) return;
        
        // Получаем локального игрока
        uintptr_t localPawn = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        // Получаем текущее оружие
        uintptr_t weaponServices = Read<uintptr_t>(localPawn + 0x1130); // m_pWeaponServices
        if (!weaponServices) return;
        
        uintptr_t activeWeapon = Read<uintptr_t>(weaponServices + 0x58); // m_hActiveWeapon
        if (!activeWeapon) return;
        
        // Получаем ID оружия
        int weaponID = Read<int>(activeWeapon + 0x1C); // m_iItemDefinitionIndex
        
        // Проверяем является ли оружие снайперкой
        if (!IsSniperWeapon(weaponID)) {
            // Если не снайперка, сбрасываем состояние
            if (g_WasScoped) {
                ResetScope();
                g_WasScoped = false;
            }
            return;
        }
        
        // Проверяем в прицеле ли игрок
        bool isScoped = Read<bool>(localPawn + 0x23E8); // m_bIsScoped
        
        if (isScoped && !g_WasScoped) {
            // Игрок только что прицелился - убираем прицел
            
            // Метод 1: Убираем альфа-канал прицела
            uintptr_t hudScope = Read<uintptr_t>(g_ClientBase + 0x1E2C000); // Примерный оффсет HUD
            if (hudScope) {
                Write<float>(hudScope + 0x50, 0.0f); // Альфа прицела = 0
            }
            
            // Метод 2: Убираем затемнение экрана
            Write<float>(localPawn + 0x23EC, 0.0f); // m_flScopeAlpha = 0
            
            // Метод 3: Убираем зум (опционально)
            if (cfg::noscope_remove_zoom) {
                uintptr_t cameraServices = Read<uintptr_t>(localPawn + 0x1150);
                if (cameraServices) {
                    Write<float>(cameraServices + 0x214, 90.0f); // Сбрасываем FOV на 90
                }
            }
            
            g_WasScoped = true;
            g_NoScopeActive = true;
            
        } else if (!isScoped && g_WasScoped) {
            // Игрок перестал прицелиться - восстанавливаем
            ResetScope();
            g_WasScoped = false;
            g_NoScopeActive = false;
        }
    }
    
    void ResetScope() {
        if (!g_ClientBase) return;
        
        uintptr_t localPawn = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        // Восстанавливаем альфа прицела
        Write<float>(localPawn + 0x23EC, 1.0f); // m_flScopeAlpha = 1.0
        
        // Восстанавливаем HUD прицела
        uintptr_t hudScope = Read<uintptr_t>(g_ClientBase + 0x1E2C000);
        if (hudScope) {
            Write<float>(hudScope + 0x50, 1.0f); // Альфа прицела = 1.0
        }
    }
}