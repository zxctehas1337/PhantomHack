#pragma once
#include <Windows.h>
#include <vector>
#include <string>

namespace ESP {
    struct PlayerInfo {
        bool valid = false;
        bool isEnemy = true;
        float posX = 0, posY = 0, posZ = 0;
        float screenX = 0, screenY = 0;
        int health = 100;
        int armor = 0;
        char name[64] = {0};
        float distance = 0;
        bool isVisible = false;
    };
    
    extern std::vector<PlayerInfo> g_Players;
    extern float g_LocalPos[3];
    extern float g_ViewMatrix[16];
    
    void UpdatePlayers();
    void RenderESP();
    bool WorldToScreen(float* pos, float* screen);
}
