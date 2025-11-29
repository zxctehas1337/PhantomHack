#pragma once
#include <Windows.h>

#define SHARED_MEMORY_NAME L"PhantomHack_SharedMem"
#define SHARED_MEMORY_SIZE 4096

// Настройки чита - общие для External (меню) и Internal (функции)
struct CheatSettings {
    // Статус
    bool initialized;
    bool running;
    bool menu_open;
    
    // Aimbot
    bool aimbot_enabled;
    float aimbot_fov;
    float aimbot_smooth;
    bool aimbot_visible_only;
    int aimbot_bone; // 0=head, 1=neck, 2=chest
    int aimbot_key;
    
    // Triggerbot
    bool triggerbot_enabled;
    int triggerbot_delay;
    int triggerbot_key;
    
    // ESP (рисуется External)
    bool esp_enabled;
    bool esp_box;
    bool esp_name;
    bool esp_health;
    bool esp_distance;
    float esp_max_distance;
    float esp_color[4];
    
    // Visuals
    bool glow_enabled;
    float glow_color[4];
    
    // Misc
    bool bhop_enabled;
    bool no_flash_enabled;
    bool radar_hack_enabled;
    
    // Данные от Internal
    int local_health;
    int local_team;
    bool in_game;
};

inline CheatSettings* GetSharedSettings() {
    static HANDLE hMapFile = nullptr;
    static CheatSettings* pSettings = nullptr;
    
    if (!pSettings) {
        hMapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEMORY_NAME);
        if (!hMapFile) {
            hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, SHARED_MEMORY_SIZE, SHARED_MEMORY_NAME);
            if (hMapFile) {
                pSettings = (CheatSettings*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);
                if (pSettings) {
                    ZeroMemory(pSettings, sizeof(CheatSettings));
                    // Дефолтные значения
                    pSettings->aimbot_fov = 5.0f;
                    pSettings->aimbot_smooth = 5.0f;
                    pSettings->aimbot_key = VK_LBUTTON;
                    pSettings->triggerbot_delay = 50;
                    pSettings->triggerbot_key = VK_XBUTTON1;
                    pSettings->esp_max_distance = 500.0f;
                    pSettings->esp_color[0] = 1.0f;
                    pSettings->esp_color[1] = 0.0f;
                    pSettings->esp_color[2] = 0.0f;
                    pSettings->esp_color[3] = 1.0f;
                    pSettings->glow_color[0] = 1.0f;
                    pSettings->glow_color[3] = 1.0f;
                }
            }
        } else {
            pSettings = (CheatSettings*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);
        }
    }
    return pSettings;
}
