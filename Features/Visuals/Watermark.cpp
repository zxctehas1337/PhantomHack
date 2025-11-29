#define _CRT_SECURE_NO_WARNINGS
#include "Watermark.h"
#include "../../CS2Menu/imgui/imgui.h"
#include "../Globals.h"
#include <ctime>
#include <cstdio>

namespace Watermark {
    
    void RenderWatermark() {
        if (!cfg::misc_watermark) return;
        
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        
        time_t now = time(nullptr);
        tm* t = localtime(&now);
        char timeStr[64];
        sprintf_s(timeStr, "PhantomHack | %02d:%02d:%02d | %d FPS", 
            t->tm_hour, t->tm_min, t->tm_sec, (int)ImGui::GetIO().Framerate);
        
        ImVec2 textSize = ImGui::CalcTextSize(timeStr);
        float x = g_ScreenWidth - textSize.x - 10;
        float y = 10;
        
        draw->AddRectFilled(ImVec2(x - 5, y - 2), ImVec2(x + textSize.x + 5, y + textSize.y + 2), 
            IM_COL32(20, 20, 25, 200), 4.0f);
        draw->AddRect(ImVec2(x - 5, y - 2), ImVec2(x + textSize.x + 5, y + textSize.y + 2), 
            IM_COL32(230, 115, 40, 255), 4.0f);
        draw->AddText(ImVec2(x, y), IM_COL32(255, 255, 255, 255), timeStr);
    }
}