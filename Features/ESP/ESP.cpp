#include "ESP.h"
#include "../Offsets.h"
#include "../Globals.h"
#include "../imgui/imgui.h"
#include <cmath>
#include <cstring>

namespace ESP {
    std::vector<PlayerInfo> g_Players;
    float g_LocalPos[3] = {0, 0, 0};
    float g_ViewMatrix[16] = {0};
    
    template<typename T>
    T Read(uintptr_t addr) {
        if (!addr) return T{};
        __try {
            return *reinterpret_cast<T*>(addr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return T{};
        }
    }
    
    bool WorldToScreen(float* pos, float* screen) {
        float w = g_ViewMatrix[3] * pos[0] + g_ViewMatrix[7] * pos[1] + g_ViewMatrix[11] * pos[2] + g_ViewMatrix[15];
        
        if (w < 0.001f) return false;
        
        float invW = 1.0f / w;
        
        screen[0] = (g_ViewMatrix[0] * pos[0] + g_ViewMatrix[4] * pos[1] + g_ViewMatrix[8] * pos[2] + g_ViewMatrix[12]) * invW;
        screen[1] = (g_ViewMatrix[1] * pos[0] + g_ViewMatrix[5] * pos[1] + g_ViewMatrix[9] * pos[2] + g_ViewMatrix[13]) * invW;
        
        ImGuiIO& io = ImGui::GetIO();
        screen[0] = (io.DisplaySize.x * 0.5f) + (screen[0] * io.DisplaySize.x * 0.5f);
        screen[1] = (io.DisplaySize.y * 0.5f) - (screen[1] * io.DisplaySize.y * 0.5f);
        
        return true;
    }
    
    void UpdatePlayers() {
        g_Players.clear();
        
        if (!g_ClientBase) return;
        
        // Read ViewMatrix
        for (int i = 0; i < 16; i++) {
            g_ViewMatrix[i] = Read<float>(g_ClientBase + Offsets::client_dll::dwViewMatrix + i * 4);
        }
        
        // Get local player
        uintptr_t localPawn = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        uintptr_t localController = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwLocalPlayerController);
        if (!localController) return;
        
        int localTeam = Read<int>(localPawn + Offsets::C_BaseEntity::m_iTeamNum);
        
        // Get local position
        uintptr_t localSceneNode = Read<uintptr_t>(localPawn + Offsets::C_BaseEntity::m_pGameSceneNode);
        if (localSceneNode) {
            g_LocalPos[0] = Read<float>(localSceneNode + Offsets::CGameSceneNode::m_vecAbsOrigin);
            g_LocalPos[1] = Read<float>(localSceneNode + Offsets::CGameSceneNode::m_vecAbsOrigin + 4);
            g_LocalPos[2] = Read<float>(localSceneNode + Offsets::CGameSceneNode::m_vecAbsOrigin + 8);
        }
        
        // Get entity list
        uintptr_t entityList = Read<uintptr_t>(g_ClientBase + Offsets::client_dll::dwEntityList);
        if (!entityList) return;
        
        // Iterate through players (1-64)
        for (int i = 1; i <= 64; i++) {
            uintptr_t listEntry = Read<uintptr_t>(entityList + (8 * ((i & 0x7FFF) >> 9)) + 16);
            if (!listEntry) continue;
            
            uintptr_t controller = Read<uintptr_t>(listEntry + 120 * (i & 0x1FF));
            if (!controller) continue;
            if (controller == localController) continue;
            
            // Check if alive
            bool isAlive = Read<bool>(controller + Offsets::CCSPlayerController::m_bPawnIsAlive);
            if (!isAlive) continue;
            
            // Get pawn handle
            uint32_t pawnHandle = Read<uint32_t>(controller + Offsets::CCSPlayerController::m_hPlayerPawn);
            if (!pawnHandle) continue;
            
            uintptr_t listEntry2 = Read<uintptr_t>(entityList + (8 * ((pawnHandle & 0x7FFF) >> 9)) + 16);
            if (!listEntry2) continue;
            
            uintptr_t pawn = Read<uintptr_t>(listEntry2 + 120 * (pawnHandle & 0x1FF));
            if (!pawn) continue;
            
            PlayerInfo player;
            player.valid = true;
            
            // Team check
            int team = Read<int>(pawn + Offsets::C_BaseEntity::m_iTeamNum);
            player.isEnemy = (team != localTeam);
            
            // Health
            player.health = Read<int>(pawn + Offsets::C_BaseEntity::m_iHealth);
            if (player.health <= 0 || player.health > 100) continue;
            
            // Position
            uintptr_t sceneNode = Read<uintptr_t>(pawn + Offsets::C_BaseEntity::m_pGameSceneNode);
            if (!sceneNode) continue;
            
            player.posX = Read<float>(sceneNode + Offsets::CGameSceneNode::m_vecAbsOrigin);
            player.posY = Read<float>(sceneNode + Offsets::CGameSceneNode::m_vecAbsOrigin + 4);
            player.posZ = Read<float>(sceneNode + Offsets::CGameSceneNode::m_vecAbsOrigin + 8);
            
            // Distance
            float dx = player.posX - g_LocalPos[0];
            float dy = player.posY - g_LocalPos[1];
            float dz = player.posZ - g_LocalPos[2];
            player.distance = sqrtf(dx*dx + dy*dy + dz*dz) / 100.0f; // Convert to meters
            
            if (player.distance > cfg::esp_max_distance) continue;
            
            // Name
            uintptr_t nameAddr = Read<uintptr_t>(controller + Offsets::CCSPlayerController::m_sSanitizedPlayerName);
            if (nameAddr) {
                for (int j = 0; j < 63; j++) {
                    player.name[j] = Read<char>(nameAddr + j);
                    if (player.name[j] == 0) break;
                }
            }
            
            // Armor
            player.armor = Read<int>(pawn + Offsets::C_CSPlayerPawn::m_ArmorValue);
            
            g_Players.push_back(player);
        }
    }

    void RenderESP() {
        if (!cfg::esp_enabled) return;
        
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImGuiIO& io = ImGui::GetIO();
        
        for (auto& player : g_Players) {
            if (!player.valid) continue;
            if (!player.isEnemy) continue; // Only enemies
            
            // Get screen position for feet and head
            float feetPos[3] = { player.posX, player.posY, player.posZ };
            float headPos[3] = { player.posX, player.posY, player.posZ + 72.0f };
            
            float feetScreen[2], headScreen[2];
            if (!WorldToScreen(feetPos, feetScreen)) continue;
            if (!WorldToScreen(headPos, headScreen)) continue;
            
            // Calculate box dimensions
            float height = feetScreen[1] - headScreen[1];
            float width = height * 0.4f;
            
            float left = headScreen[0] - width * 0.5f;
            float right = headScreen[0] + width * 0.5f;
            float top = headScreen[1];
            float bottom = feetScreen[1];
            
            // Colors
            ImU32 boxColor = IM_COL32(
                (int)(cfg::esp_color[0] * 255),
                (int)(cfg::esp_color[1] * 255),
                (int)(cfg::esp_color[2] * 255),
                (int)(cfg::esp_color[3] * 255)
            );
            ImU32 outlineColor = IM_COL32(0, 0, 0, 200);
            ImU32 whiteColor = IM_COL32(255, 255, 255, 255);
            
            // Box ESP
            if (cfg::esp_box) {
                // Outline
                draw->AddRect(ImVec2(left - 1, top - 1), ImVec2(right + 1, bottom + 1), outlineColor);
                draw->AddRect(ImVec2(left + 1, top + 1), ImVec2(right - 1, bottom - 1), outlineColor);
                // Main box
                draw->AddRect(ImVec2(left, top), ImVec2(right, bottom), boxColor);
            }
            
            // Health bar
            if (cfg::esp_healthbar) {
                float healthPercent = player.health / 100.0f;
                float barHeight = height * healthPercent;
                
                // Health color (green to red)
                int r = (int)((1.0f - healthPercent) * 255);
                int g = (int)(healthPercent * 255);
                ImU32 healthColor = IM_COL32(r, g, 0, 255);
                
                // Background
                draw->AddRectFilled(ImVec2(left - 6, top), ImVec2(left - 2, bottom), IM_COL32(0, 0, 0, 150));
                // Health
                draw->AddRectFilled(ImVec2(left - 5, bottom - barHeight), ImVec2(left - 3, bottom), healthColor);
                // Border
                draw->AddRect(ImVec2(left - 6, top), ImVec2(left - 2, bottom), outlineColor);
            }
            
            // Name
            if (cfg::esp_name && player.name[0] != 0) {
                ImVec2 textSize = ImGui::CalcTextSize(player.name);
                float textX = headScreen[0] - textSize.x * 0.5f;
                float textY = top - textSize.y - 2;
                
                // Shadow
                draw->AddText(ImVec2(textX + 1, textY + 1), outlineColor, player.name);
                draw->AddText(ImVec2(textX, textY), whiteColor, player.name);
            }
            
            // Health text
            if (cfg::esp_health) {
                char healthStr[16];
                sprintf_s(healthStr, "%d HP", player.health);
                ImVec2 textSize = ImGui::CalcTextSize(healthStr);
                float textX = right + 4;
                float textY = top;
                
                draw->AddText(ImVec2(textX + 1, textY + 1), outlineColor, healthStr);
                draw->AddText(ImVec2(textX, textY), whiteColor, healthStr);
            }
            
            // Distance
            if (cfg::esp_distance) {
                char distStr[32];
                sprintf_s(distStr, "%.0fm", player.distance);
                ImVec2 textSize = ImGui::CalcTextSize(distStr);
                float textX = headScreen[0] - textSize.x * 0.5f;
                float textY = bottom + 2;
                
                draw->AddText(ImVec2(textX + 1, textY + 1), outlineColor, distStr);
                draw->AddText(ImVec2(textX, textY), whiteColor, distStr);
            }
            
            // Snaplines
            if (cfg::esp_snaplines) {
                float startX = io.DisplaySize.x * 0.5f;
                float startY = io.DisplaySize.y;
                
                draw->AddLine(ImVec2(startX, startY), ImVec2(feetScreen[0], feetScreen[1]), boxColor, 1.0f);
            }
        }
    }
}
