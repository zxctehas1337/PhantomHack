#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <map>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"

#include "../Features/Offsets.h"
#include "../Features/Globals.h"
#include "../Features/ESP/ESP.h"
#include "../Features/BunnyHop/BunnyHop.h"
#include "../Features/FOV/FOV.h"
#include "../Features/NoScope/NoScope.h"
#include "../Features/Visuals/Watermark.h"
#include "../Features/Visuals/Glow.h"
#include "../Features/Aimbot/Aimbot.h"
#include "../Features/Misc/Triggerbot.h"
#include "../Features/Misc/AutoShot.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Globals (объявляем extern, определения в GlobalVars.cpp)
extern uintptr_t g_ClientBase;
extern int g_ScreenWidth;
extern int g_ScreenHeight;
HWND g_GameWnd = nullptr;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dContext = nullptr;
ID3D11RenderTargetView* g_pRenderTarget = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
WNDPROC g_OriginalWndProc = nullptr;
bool g_Initialized = false;
bool g_ShowMenu = true;
bool g_Running = true;
HANDLE g_Console = nullptr;
float g_MenuAlpha = 0.0f;
float g_BlurAlpha = 0.0f;
int g_CurrentTab = 0;
ImVec2 g_MenuPos = ImVec2(-1, -1);

typedef HRESULT(__stdcall* tPresent)(IDXGISwapChain*, UINT, UINT);
tPresent oPresent = nullptr;

// Theme System
enum Theme { THEME_MIDNIGHT = 0, THEME_SNOW = 1 };
int g_CurrentTheme = THEME_MIDNIGHT;

struct ThemeColors {
    ImVec4 accent, accentHover, accentActive, windowBg, childBg, border, text, textDisabled;
};

ThemeColors g_Themes[2] = {
    { // MIDNIGHT
        ImVec4(0.20f, 0.40f, 0.80f, 1.0f), ImVec4(0.25f, 0.50f, 0.90f, 1.0f), ImVec4(0.15f, 0.35f, 0.70f, 1.0f),
        ImVec4(0.08f, 0.08f, 0.12f, 0.97f), ImVec4(0.10f, 0.10f, 0.15f, 1.0f), ImVec4(0.20f, 0.40f, 0.80f, 0.40f),
        ImVec4(0.95f, 0.95f, 0.98f, 1.0f), ImVec4(0.45f, 0.50f, 0.60f, 1.0f)
    },
    { // SNOW
        ImVec4(0.25f, 0.45f, 0.85f, 1.0f), ImVec4(0.30f, 0.55f, 0.95f, 1.0f), ImVec4(0.20f, 0.40f, 0.75f, 1.0f),
        ImVec4(0.96f, 0.96f, 0.98f, 0.97f), ImVec4(0.92f, 0.92f, 0.95f, 1.0f), ImVec4(0.25f, 0.45f, 0.85f, 0.40f),
        ImVec4(0.10f, 0.10f, 0.15f, 1.0f), ImVec4(0.50f, 0.50f, 0.55f, 1.0f)
    }
};

// Config (объявляем extern, определения в GlobalVars.cpp)
namespace cfg {
    // ESP
    extern bool esp_enabled, esp_box, esp_box_outline, esp_name;
    extern bool esp_health, esp_healthbar, esp_armor, esp_armorbar;
    extern bool esp_distance, esp_snaplines, esp_skeleton, esp_head;
    extern bool esp_visible_check, glow_enabled;
    extern int esp_box_type, esp_snapline_pos;
    extern float esp_max_distance, misc_flash_alpha;
    extern float esp_color[4];
    extern float esp_team_color[4];
    extern float esp_visible_color[4];
    extern float glow_color[4];
    // Misc
    extern bool misc_bhop, misc_autostrafe, misc_noflash, misc_watermark;
    extern bool misc_speedometer;
    // Aimbot - Legit
    extern bool aim_legit_enabled, aim_legit_vischeck;
    extern float aim_legit_fov, aim_legit_smooth;
    extern int aim_legit_bone;
    // Aimbot - Rage
    extern bool aim_rage_enabled, aim_rage_silent, aim_rage_autoshoot;
    extern float aim_rage_fov;
    extern int aim_rage_bone;
    // Triggerbot
    extern bool trigger_enabled;
    extern int trigger_delay;
    // AutoShot (penetration)
    extern bool autoshot_enabled;
    extern int autoshot_delay;
    // FOV
    extern bool fov_enabled;
    extern float fov_value;
    extern float viewmodel_fov;
    // NoScope
    extern bool noscope_enabled;
    extern bool noscope_remove_zoom;
    // World
    extern bool world_noflash;
    extern float world_flash_alpha;
    extern bool world_nosmoke;
    extern bool world_nightmode;
    extern float world_nightmode_intensity;
    extern bool world_fullbright;
    extern bool world_nofog;
    extern bool world_color_enabled;
    extern float world_color[4];
    extern bool world_skybox_enabled;
    extern int world_skybox_type;
}

template<typename T> T Read(uintptr_t addr) {
    if (!addr) return T{};
    __try { return *reinterpret_cast<T*>(addr); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return T{}; }
}

#define FL_ONGROUND (1 << 0)

void SetColor(int color) { SetConsoleTextAttribute(g_Console, color); }

void PrintLogo() {
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("\n");
    printf("    ____  _                 _                   _   _            _    \n");
    printf("   |  _ \\| |__   __ _ _ __ | |_ ___  _ __ ___ | | | | __ _  ___| | __\n");
    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("   | |_) | '_ \\ / _` | '_ \\| __/ _ \\| '_ ` _ \\| |_| |/ _` |/ __| |/ /\n");
    printf("   |  __/| | | | (_| | | | | || (_) | | | | | |  _  | (_| | (__|   < \n");
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("   |_|   |_| |_|\\__,_|_| |_|\\__\\___/|_| |_| |_|_| |_|\\__,_|\\___|_|\\_\\\n");
    printf("\n");
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf("    ============================================================\n");
    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("                    CS2 Internal Cheat v3.0\n");
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf("    ============================================================\n\n");
}

void Log(const char* type, const char* msg) {
    time_t now = time(nullptr); tm* t = localtime(&now);
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf("  [%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
    if (strcmp(type, "OK") == 0) { SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY); printf("[  OK  ] "); }
    else if (strcmp(type, "INFO") == 0) { SetColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY); printf("[ INFO ] "); }
    else if (strcmp(type, "WARN") == 0) { SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); printf("[ WARN ] "); }
    else if (strcmp(type, "ERR") == 0) { SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY); printf("[ ERR  ] "); }
    else if (strcmp(type, "HOOK") == 0) { SetColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY); printf("[ HOOK ] "); }
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf("%s\n", msg);
}

void ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    ThemeColors& t = g_Themes[g_CurrentTheme];
    s.WindowPadding = ImVec2(12, 12); s.WindowRounding = 8.0f; s.FramePadding = ImVec2(8, 5);
    s.FrameRounding = 4.0f; s.ItemSpacing = ImVec2(10, 6); s.GrabRounding = 4.0f;
    s.ScrollbarRounding = 4.0f; s.WindowBorderSize = 1.0f; s.ChildBorderSize = 1.0f;
    s.TabRounding = 4.0f; s.WindowTitleAlign = ImVec2(0.5f, 0.5f); s.ChildRounding = 6.0f;
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = t.text; c[ImGuiCol_TextDisabled] = t.textDisabled;
    c[ImGuiCol_WindowBg] = t.windowBg; c[ImGuiCol_ChildBg] = t.childBg;
    c[ImGuiCol_PopupBg] = t.windowBg; c[ImGuiCol_Border] = t.border;
    c[ImGuiCol_FrameBg] = ImVec4(t.childBg.x * 0.8f, t.childBg.y * 0.8f, t.childBg.z * 0.8f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(t.accent.x * 0.5f, t.accent.y * 0.5f, t.accent.z * 0.5f, 0.4f);
    c[ImGuiCol_FrameBgActive] = ImVec4(t.accent.x * 0.6f, t.accent.y * 0.6f, t.accent.z * 0.6f, 0.6f);
    c[ImGuiCol_TitleBg] = t.accent; c[ImGuiCol_TitleBgActive] = t.accentHover;
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(t.accent.x, t.accent.y, t.accent.z, 0.6f);
    c[ImGuiCol_ScrollbarBg] = t.childBg;
    c[ImGuiCol_ScrollbarGrab] = ImVec4(t.textDisabled.x, t.textDisabled.y, t.textDisabled.z, 0.5f);
    c[ImGuiCol_ScrollbarGrabHovered] = t.textDisabled; c[ImGuiCol_ScrollbarGrabActive] = t.accent;
    c[ImGuiCol_CheckMark] = t.accent; c[ImGuiCol_SliderGrab] = t.accent; c[ImGuiCol_SliderGrabActive] = t.accentHover;
    c[ImGuiCol_Button] = ImVec4(t.accent.x, t.accent.y, t.accent.z, 0.7f);
    c[ImGuiCol_ButtonHovered] = t.accentHover; c[ImGuiCol_ButtonActive] = t.accentActive;
    c[ImGuiCol_Header] = ImVec4(t.accent.x, t.accent.y, t.accent.z, 0.4f);
    c[ImGuiCol_HeaderHovered] = ImVec4(t.accent.x, t.accent.y, t.accent.z, 0.6f);
    c[ImGuiCol_HeaderActive] = ImVec4(t.accent.x, t.accent.y, t.accent.z, 0.8f);
    c[ImGuiCol_Separator] = ImVec4(t.border.x, t.border.y, t.border.z, 0.5f);
    c[ImGuiCol_Tab] = t.childBg; c[ImGuiCol_TabHovered] = t.accentHover; c[ImGuiCol_TabActive] = t.accent;
}

bool MidnightButton(const char* label, bool selected, ImVec2 size) {
    ThemeColors& t = g_Themes[g_CurrentTheme];
    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, t.accent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, t.accentHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, t.accentActive);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(t.childBg.x * 0.9f, t.childBg.y * 0.9f, t.childBg.z * 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(t.accent.x * 0.5f, t.accent.y * 0.5f, t.accent.z * 0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, t.accent);
        ImGui::PushStyleColor(ImGuiCol_Text, t.textDisabled);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleVar(); ImGui::PopStyleColor(4);
    return clicked;
}

void RenderBlurBackground() {
    if (g_BlurAlpha <= 0.01f) return;
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    
    // Multi-layer blur effect
    for (int i = 0; i < 3; i++) {
        float alpha = g_BlurAlpha * (0.3f + i * 0.2f);
        ImU32 blurColor = IM_COL32(5, 5, 15, (int)(alpha * 200));
        draw->AddRectFilled(ImVec2(0, 0), ImVec2((float)g_ScreenWidth, (float)g_ScreenHeight), blurColor);
    }
    
    // Gradient overlay
    ThemeColors& t = g_Themes[g_CurrentTheme];
    ImU32 gradientColor = IM_COL32((int)(t.accent.x * 255 * 0.3f), (int)(t.accent.y * 255 * 0.3f), (int)(t.accent.z * 255 * 0.3f), (int)(g_BlurAlpha * 30));
    draw->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2((float)g_ScreenWidth, (float)g_ScreenHeight),
        gradientColor, IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), gradientColor);
    
    // PhantomHack logo in center (semi-transparent)
    int logoAlpha = (int)(g_BlurAlpha * 50); // Semi-transparent
    
    const char* phantom = "Phantom";
    const char* hack = "Hack";
    
    ImVec2 phantomSize = ImGui::CalcTextSize(phantom);
    ImVec2 hackSize = ImGui::CalcTextSize(hack);
    float totalWidth = phantomSize.x + hackSize.x;
    
    float logoX = (g_ScreenWidth - totalWidth) * 0.5f;
    float logoY = g_ScreenHeight * 0.5f - 50.0f;
    
    // Draw logo with shadow
    draw->AddText(ImVec2(logoX + 2, logoY + 2), IM_COL32(0, 0, 0, logoAlpha / 2), phantom);
    draw->AddText(ImVec2(logoX + phantomSize.x + 2, logoY + 2), IM_COL32(0, 0, 0, logoAlpha / 2), hack);
    draw->AddText(ImVec2(logoX, logoY), IM_COL32(255, 255, 255, logoAlpha), phantom);
    draw->AddText(ImVec2(logoX + phantomSize.x, logoY), IM_COL32(255, 60, 60, logoAlpha), hack);
}

void 
RenderESPPreview(ImVec2 pos, ImVec2 size) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ThemeColors& t = g_Themes[g_CurrentTheme];
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
        IM_COL32((int)(t.childBg.x * 255 * 0.5f), (int)(t.childBg.y * 255 * 0.5f), (int)(t.childBg.z * 255 * 0.5f), 200), 6.0f);
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
        IM_COL32((int)(t.border.x * 255), (int)(t.border.y * 255), (int)(t.border.z * 255), 100), 6.0f);
    
    float playerX = pos.x + size.x * 0.5f, playerY = pos.y + size.y * 0.3f;
    float playerH = size.y * 0.5f, playerW = playerH * 0.4f;
    ImU32 espCol = ImGui::ColorConvertFloat4ToU32(ImVec4(cfg::esp_color[0], cfg::esp_color[1], cfg::esp_color[2], cfg::esp_color[3]));
    float x = playerX - playerW / 2, y = playerY;
    
    if (cfg::esp_box) {
        if (cfg::esp_box_type == 0) {
            if (cfg::esp_box_outline) draw->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + playerW + 1, y + playerH + 1), IM_COL32(0, 0, 0, 200), 0, 0, 3.0f);
            draw->AddRect(ImVec2(x, y), ImVec2(x + playerW, y + playerH), espCol, 0, 0, 1.5f);
        } else {
            float cornerLen = playerH * 0.25f;
            ImVec2 tl(x, y), tr(x + playerW, y), bl(x, y + playerH), br(x + playerW, y + playerH);
            draw->AddLine(tl, ImVec2(tl.x + cornerLen, tl.y), espCol, 2.0f); draw->AddLine(tl, ImVec2(tl.x, tl.y + cornerLen), espCol, 2.0f);
            draw->AddLine(tr, ImVec2(tr.x - cornerLen, tr.y), espCol, 2.0f); draw->AddLine(tr, ImVec2(tr.x, tr.y + cornerLen), espCol, 2.0f);
            draw->AddLine(bl, ImVec2(bl.x + cornerLen, bl.y), espCol, 2.0f); draw->AddLine(bl, ImVec2(bl.x, bl.y - cornerLen), espCol, 2.0f);
            draw->AddLine(br, ImVec2(br.x - cornerLen, br.y), espCol, 2.0f); draw->AddLine(br, ImVec2(br.x, br.y - cornerLen), espCol, 2.0f);
        }
    }
    if (cfg::esp_healthbar) {
        float barX = x - 6.0f;
        draw->AddRectFilled(ImVec2(barX - 3, y - 1), ImVec2(barX, y + playerH + 1), IM_COL32(0, 0, 0, 200));
        draw->AddRectFilled(ImVec2(barX - 2, y + playerH * 0.3f), ImVec2(barX - 1, y + playerH), IM_COL32(0, 255, 0, 255));
    }
    if (cfg::esp_armorbar) {
        float barX = x - 10.0f;
        draw->AddRectFilled(ImVec2(barX - 3, y - 1), ImVec2(barX, y + playerH + 1), IM_COL32(0, 0, 0, 200));
        draw->AddRectFilled(ImVec2(barX - 2, y + playerH * 0.5f), ImVec2(barX - 1, y + playerH), IM_COL32(0, 128, 255, 255));
    }
    if (cfg::esp_head) {
        float headRadius = playerH * 0.12f;
        draw->AddCircle(ImVec2(playerX, y + headRadius), headRadius, espCol, 0, 1.5f);
    }
    if (cfg::esp_snaplines) {
        float endY = (cfg::esp_snapline_pos == 0) ? pos.y : (cfg::esp_snapline_pos == 1) ? pos.y + size.y * 0.5f : pos.y + size.y;
        draw->AddLine(ImVec2(playerX, y + playerH), ImVec2(pos.x + size.x * 0.5f, endY), espCol, 1.0f);
    }
    float textY = y - 4.0f;
    if (cfg::esp_name) {
        textY -= 12.0f;
        const char* name = "Player";
        ImVec2 textSize = ImGui::CalcTextSize(name);
        float textX = playerX - textSize.x / 2;
        draw->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 200), name);
        draw->AddText(ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), name);
    }
    textY = y + playerH + 3.0f;
    if (cfg::esp_health) {
        const char* hp = "70 HP";
        ImVec2 sz = ImGui::CalcTextSize(hp);
        float tx = playerX - sz.x / 2;
        draw->AddText(ImVec2(tx + 1, textY + 1), IM_COL32(0, 0, 0, 200), hp);
        draw->AddText(ImVec2(tx, textY), IM_COL32(0, 255, 0, 255), hp);
        textY += 12.0f;
    }
    if (cfg::esp_distance) {
        const char* dist = "25m";
        ImVec2 sz = ImGui::CalcTextSize(dist);
        float tx = playerX - sz.x / 2;
        draw->AddText(ImVec2(tx + 1, textY + 1), IM_COL32(0, 0, 0, 200), dist);
        draw->AddText(ImVec2(tx, textY), IM_COL32(255, 255, 255, 200), dist);
    }
}

void RenderMenu() {
    ThemeColors& t = g_Themes[g_CurrentTheme];
    if (g_ShowMenu) {
        if (g_MenuAlpha < 1.0f) g_MenuAlpha += 0.06f;
        if (g_BlurAlpha < 1.0f) g_BlurAlpha += 0.04f;
        g_MenuAlpha = min(g_MenuAlpha, 1.0f); g_BlurAlpha = min(g_BlurAlpha, 1.0f);
    } else {
        if (g_MenuAlpha > 0.0f) g_MenuAlpha -= 0.08f;
        if (g_BlurAlpha > 0.0f) g_BlurAlpha -= 0.06f;
        g_MenuAlpha = max(g_MenuAlpha, 0.0f); g_BlurAlpha = max(g_BlurAlpha, 0.0f);
    }
    RenderBlurBackground();
    if (g_MenuAlpha <= 0.0f) return;
    
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_MenuAlpha);
    ImVec2 menuSize(700, 480);
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    if (g_MenuPos.x < 0) g_MenuPos = ImVec2((screenSize.x - menuSize.x) * 0.5f, (screenSize.y - menuSize.y) * 0.5f);
    ImGui::SetNextWindowPos(g_MenuPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(menuSize, ImGuiCond_Always);
    
    ImGui::Begin("##Midnight", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(0)) {
        g_MenuPos.x += ImGui::GetIO().MouseDelta.x;
        g_MenuPos.y += ImGui::GetIO().MouseDelta.y;
    }
    
    // Header with PhantomHack logo (bigger)
    ImGui::SetWindowFontScale(1.4f);
    float headerX = (ImGui::GetWindowWidth() - ImGui::CalcTextSize("PhantomHack").x) * 0.5f;
    ImGui::SetCursorPosX(headerX);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Phantom");
    ImGui::SameLine(0, 0);
    ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "Hack");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextDisabled("v3.0");
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    
    ImGui::BeginChild("##tabs", ImVec2(120, 0), true);
    if (MidnightButton("AIMBOT", g_CurrentTab == 0, ImVec2(-1, 35))) g_CurrentTab = 0; ImGui::Spacing();
    if (MidnightButton("ESP", g_CurrentTab == 1, ImVec2(-1, 35))) g_CurrentTab = 1; ImGui::Spacing();
    if (MidnightButton("VISUALS", g_CurrentTab == 2, ImVec2(-1, 35))) g_CurrentTab = 2; ImGui::Spacing();
    if (MidnightButton("MISC", g_CurrentTab == 3, ImVec2(-1, 35))) g_CurrentTab = 3; ImGui::Spacing();
    if (MidnightButton("SKINS", g_CurrentTab == 4, ImVec2(-1, 35))) g_CurrentTab = 4; ImGui::Spacing();
    if (MidnightButton("CONFIG", g_CurrentTab == 5, ImVec2(-1, 35))) g_CurrentTab = 5;
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextDisabled("  INSERT"); ImGui::TextDisabled("  Toggle Menu");
    ImGui::Spacing(); ImGui::TextDisabled("  END"); ImGui::TextDisabled("  Unload");
    ImGui::EndChild();
    
    ImGui::SameLine();
    ImGui::BeginChild("##content", ImVec2(0, 0), true);
    
    switch (g_CurrentTab) {
    case 0: { // AIMBOT
        ImGui::TextColored(t.accent, "AIMBOT"); ImGui::Separator(); ImGui::Spacing();
        
        // Left column - Legit
        ImGui::BeginChild("##aim_l", ImVec2(180, 0), true);
        ImGui::Text("Legit"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Enable Aimbot##legit", &cfg::aim_legit_enabled);
        if (cfg::aim_legit_enabled) {
            ImGui::Spacing();
            ImGui::SliderFloat("FOV##legit", &cfg::aim_legit_fov, 1.0f, 30.0f, "%.1f");
            ImGui::SliderFloat("Smooth##legit", &cfg::aim_legit_smooth, 1.0f, 20.0f, "%.1f");
            const char* bones[] = {"Head", "Neck", "Chest"};
            ImGui::Combo("Target Bone##legit", &cfg::aim_legit_bone, bones, 3);
            ImGui::Checkbox("Visible Check##legit", &cfg::aim_legit_vischeck);
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Middle column - Rage
        ImGui::BeginChild("##aim_m", ImVec2(180, 0), true);
        ImGui::Text("Rage"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Enable Aimbot##rage", &cfg::aim_rage_enabled);
        if (cfg::aim_rage_enabled) {
            ImGui::Spacing();
            ImGui::SliderFloat("FOV##rage", &cfg::aim_rage_fov, 30.0f, 180.0f, "%.0f");
            const char* bones[] = {"Head", "Neck", "Chest"};
            ImGui::Combo("Target Bone##rage", &cfg::aim_rage_bone, bones, 3);
            ImGui::Checkbox("Silent Aim", &cfg::aim_rage_silent);
            ImGui::Checkbox("Auto Shoot", &cfg::aim_rage_autoshoot);
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Right column - Other
        ImGui::BeginChild("##aim_r", ImVec2(0, 0), true);
        ImGui::Text("Other"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Enable Triggerbot", &cfg::trigger_enabled);
        if (cfg::trigger_enabled) {
            ImGui::Spacing();
            ImGui::SliderInt("Delay (ms)", &cfg::trigger_delay, 0, 200);
        }
        
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        
        ImGui::Text("Status");
        ImGui::TextColored(cfg::aim_legit_enabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), "Legit: %s", cfg::aim_legit_enabled ? "ON" : "OFF");
        ImGui::TextColored(cfg::aim_rage_enabled ? ImVec4(1, 0.5f, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), "Rage: %s", cfg::aim_rage_enabled ? "ON" : "OFF");
        ImGui::TextColored(cfg::trigger_enabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), "Trigger: %s", cfg::trigger_enabled ? "ON" : "OFF");
        ImGui::EndChild();
    } break;
        
    case 1: { // ESP
        ImGui::TextColored(t.accent, "ESP"); ImGui::Separator(); ImGui::Spacing();
        ImGui::BeginChild("##esp_l", ImVec2(220, 0), true);
        ImGui::Text("Player ESP"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Enable ESP", &cfg::esp_enabled); ImGui::Spacing();
        ImGui::Checkbox("Box", &cfg::esp_box);
        if (cfg::esp_box) {
            ImGui::SameLine(); ImGui::Checkbox("Outline", &cfg::esp_box_outline);
            const char* boxTypes[] = {"Normal", "Corner"};
            ImGui::Combo("Box Type", &cfg::esp_box_type, boxTypes, 2);
        }
        ImGui::Checkbox("Name", &cfg::esp_name);
        ImGui::Checkbox("Health Text", &cfg::esp_health);
        ImGui::Checkbox("Health Bar", &cfg::esp_healthbar);
        ImGui::Checkbox("Armor Bar", &cfg::esp_armorbar);
        ImGui::Checkbox("Distance", &cfg::esp_distance);
        ImGui::Checkbox("Head Circle", &cfg::esp_head);
        ImGui::Checkbox("Snaplines", &cfg::esp_snaplines);
        if (cfg::esp_snaplines) {
            const char* snapPos[] = {"Top", "Center", "Bottom"};
            ImGui::Combo("Line Pos", &cfg::esp_snapline_pos, snapPos, 3);
        }
        ImGui::Spacing(); ImGui::Text("Colors");
        ImGui::ColorEdit4("ESP Color", cfg::esp_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##esp_r", ImVec2(0, 0), true);
        ImGui::Text("Preview"); ImGui::Separator();
        ImVec2 previewPos = ImGui::GetCursorScreenPos();
        ImVec2 previewSize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 10);
        RenderESPPreview(previewPos, previewSize);
        ImGui::EndChild();
    } break;
        
    case 2: { // VISUALS
        ImGui::TextColored(t.accent, "VISUALS"); ImGui::Separator(); ImGui::Spacing();
        
        // Left column - FOV & Glow
        ImGui::BeginChild("##vis_l", ImVec2(250, 0), true);
        ImGui::Text("FOV Changer"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Enable FOV", &cfg::fov_enabled);
        if (cfg::fov_enabled) {
            ImGui::SliderFloat("Camera FOV", &cfg::fov_value, 90.0f, 150.0f, "%.0f");
            ImGui::SliderFloat("Viewmodel FOV", &cfg::viewmodel_fov, 68.0f, 120.0f, "%.0f");
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "ЭКСПЕРИМЕНТАЛЬНО");
            ImGui::TextDisabled("Может не работать");
        }
        
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("NoScope"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Enable NoScope", &cfg::noscope_enabled);
        if (cfg::noscope_enabled) {
            ImGui::Checkbox("Remove Zoom", &cfg::noscope_remove_zoom);
            ImGui::TextDisabled("Убирает прицел у снайперок");
            ImGui::TextDisabled("AWP, Scout, G3SG1, SCAR-20");
        }
        
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Glow"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Enable Glow", &cfg::glow_enabled);
        ImGui::ColorEdit4("Glow Color", cfg::glow_color, ImGuiColorEditFlags_NoInputs);
        
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Removals"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("No Flash", &cfg::misc_noflash);
        if (cfg::misc_noflash) ImGui::SliderFloat("Flash Alpha", &cfg::misc_flash_alpha, 0.0f, 1.0f);
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Right column - World
        ImGui::BeginChild("##vis_r", ImVec2(0, 0), true);
        ImGui::Text("World"); ImGui::Separator(); ImGui::Spacing();
        
        ImGui::Checkbox("No Flash", &cfg::world_noflash);
        if (cfg::world_noflash) {
            ImGui::SliderFloat("Flash Alpha", &cfg::world_flash_alpha, 0.0f, 100.0f, "%.0f");
        }
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "* No Flash works!");
        
        ImGui::Spacing();
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        ImGui::BeginDisabled(true);
        ImGui::Checkbox("No Smoke", &cfg::world_nosmoke);
        ImGui::Checkbox("Night Mode", &cfg::world_nightmode);
        ImGui::Checkbox("Fullbright", &cfg::world_fullbright);
        ImGui::Checkbox("No Fog", &cfg::world_nofog);
        ImGui::EndDisabled();
        ImGui::PopStyleVar();
        ImGui::TextDisabled("(coming soon)");
        
        ImGui::EndChild();
    } break;
        
    case 3: { // MISC
        ImGui::TextColored(t.accent, "MISC"); ImGui::Separator(); ImGui::Spacing();
        
        // Left column - Movement
        ImGui::BeginChild("##misc_l", ImVec2(180, 0), true);
        ImGui::Text("Movement"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Bunny Hop", &cfg::misc_bhop);
        if (cfg::misc_bhop) {
            ImGui::Checkbox("Auto Strafe", &cfg::misc_autostrafe);
            ImGui::Checkbox("Speedometer", &cfg::misc_speedometer);
            ImGui::TextDisabled("Hold SPACE to bhop");
        }
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("AutoShot"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Enable AutoShot", &cfg::autoshot_enabled);
        if (cfg::autoshot_enabled) {
            ImGui::SliderInt("Delay (ms)", &cfg::autoshot_delay, 50, 500);
            ImGui::TextDisabled("Shoots through walls");
            ImGui::TextDisabled("if penetration possible");
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Middle column - Other
        ImGui::BeginChild("##misc_m", ImVec2(180, 0), true);
        ImGui::Text("Other"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Checkbox("Watermark", &cfg::misc_watermark);
        ImGui::Spacing();
        ImGui::Text("Theme"); ImGui::Separator(); ImGui::Spacing();
        const char* themes[] = {"Midnight (Dark Blue)", "Snow (White)"};
        if (ImGui::Combo("##Theme", &g_CurrentTheme, themes, 2)) ApplyTheme();
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Right column - Status
        ImGui::BeginChild("##misc_r", ImVec2(0, 0), true);
        ImGui::Text("Status"); ImGui::Separator(); ImGui::Spacing();
        ImGui::TextDisabled("client.dll: %s", g_ClientBase ? "OK" : "N/A");
        if (g_ClientBase) ImGui::TextDisabled("Base: 0x%llX", g_ClientBase);
        ImGui::TextDisabled("Game Window: %s", g_GameWnd ? "OK" : "N/A");
        ImGui::TextDisabled("Players: %d", (int)ESP::g_Players.size());
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Performance");
        ImGui::TextDisabled("FPS: %.0f", ImGui::GetIO().Framerate);
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Features");
        ImGui::TextColored(cfg::esp_enabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), "ESP: %s", cfg::esp_enabled ? "ON" : "OFF");
        ImGui::TextColored(cfg::aim_legit_enabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), "Legit: %s", cfg::aim_legit_enabled ? "ON" : "OFF");
        ImGui::TextColored(cfg::aim_rage_enabled ? ImVec4(1, 0.5f, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), "Rage: %s", cfg::aim_rage_enabled ? "ON" : "OFF");
        ImGui::TextColored(cfg::misc_bhop ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), "BHop: %s", cfg::misc_bhop ? "ON" : "OFF");
        ImGui::TextColored(cfg::noscope_enabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), "NoScope: %s", cfg::noscope_enabled ? "ON" : "OFF");
        ImGui::EndChild();
    } break;
        
    case 4: { // SKINS
        ImGui::TextColored(t.accent, "SKINCHANGER"); ImGui::Separator();
        ImGui::BeginChild("##skins", ImVec2(0, 0), true);
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 textSize = ImGui::CalcTextSize("Coming soon...");
        ImGui::SetCursorPos(ImVec2((windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f));
        ImGui::TextColored(t.textDisabled, "Coming soon...");
        ImGui::EndChild();
    } break;
        
    case 5: { // CONFIG
        ImGui::TextColored(t.accent, "CONFIG"); ImGui::Separator(); ImGui::Spacing();
        ImGui::BeginChild("##cfg", ImVec2(0, 0), true);
        static int selCfg = 0;
        const char* cfgs[] = {"default", "legit", "rage"};
        ImGui::Combo("Config", &selCfg, cfgs, 3);
        ImGui::Spacing();
        if (ImGui::Button("Load", ImVec2(100, 30))) {} ImGui::SameLine();
        if (ImGui::Button("Save", ImVec2(100, 30))) {} ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(100, 30))) { cfg::esp_enabled = false; cfg::aim_legit_enabled = false; cfg::aim_rage_enabled = false; }
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "PhantomHack v3.0");
        ImGui::TextDisabled("Build: %s", __DATE__);
        ImGui::EndChild();
    } break;
    }
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();
}


LRESULT CALLBACK hkWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (g_ShowMenu && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp)) return true;
    return CallWindowProcA(g_OriginalWndProc, hWnd, msg, wp, lp);
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwap, UINT sync, UINT flags) {
    if (!g_Initialized) {
        if (SUCCEEDED(pSwap->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
            g_pd3dDevice->GetImmediateContext(&g_pd3dContext);
            g_pSwapChain = pSwap;
            DXGI_SWAP_CHAIN_DESC desc; pSwap->GetDesc(&desc);
            g_GameWnd = desc.OutputWindow;
            ID3D11Texture2D* pBack;
            pSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBack);
            g_pd3dDevice->CreateRenderTargetView(pBack, nullptr, &g_pRenderTarget);
            pBack->Release();
            g_OriginalWndProc = (WNDPROC)SetWindowLongPtrA(g_GameWnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
            ApplyTheme();
            ImGui_ImplWin32_Init(g_GameWnd);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);
            g_Initialized = true;
            Log("OK", "Present hooked successfully!");
            Log("INFO", "Menu initialized - Press INSERT to toggle");
        }
    }
    if (g_Initialized) {
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            g_ShowMenu = !g_ShowMenu;
            Log("INFO", g_ShowMenu ? "Menu opened" : "Menu closed");
        }
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = g_ShowMenu;
        DXGI_SWAP_CHAIN_DESC desc; pSwap->GetDesc(&desc);
        g_ScreenWidth = desc.BufferDesc.Width;
        g_ScreenHeight = desc.BufferDesc.Height;
        ESP::UpdatePlayers();
        BunnyHop::RunBhop();
        FOV::RunFOV();
        NoScope::RunNoScope();
        Aimbot::RunAimbot();
        Triggerbot::RunTriggerbot();
        AutoShot::RunAutoShot();
        Glow::RunGlow();
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ESP::RenderESP();
        Watermark::RenderWatermark();
        
        // Speedometer - когда bhop и speedometer включены
        if (cfg::misc_bhop && cfg::misc_speedometer) {
            ImDrawList* draw = ImGui::GetBackgroundDrawList();
            ImFont* font = ImGui::GetFont();
            float speed = BunnyHop::GetSpeed();
            
            // Позиция внизу по центру
            float posX = g_ScreenWidth / 2.0f;
            float posY = g_ScreenHeight - 120.0f;
            
            // Цвет в зависимости от скорости
            ImU32 speedColor;
            if (speed < 250.0f) speedColor = IM_COL32(255, 255, 255, 255);
            else if (speed < 285.0f) speedColor = IM_COL32(255, 255, 0, 255);
            else if (speed < 300.0f) speedColor = IM_COL32(0, 255, 0, 255);
            else speedColor = IM_COL32(0, 255, 255, 255);
            
            char speedText[32];
            sprintf_s(speedText, "%.0f", speed);
            
            // Размер шрифта 1.8x (меньше)
            float baseSize = ImGui::GetFontSize();
            float fontSize = baseSize * 1.8f;
            
            ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, speedText);
            float textX = posX - textSize.x / 2.0f;
            
            // Обводка
            draw->AddText(font, fontSize, ImVec2(textX + 1, posY + 1), IM_COL32(0, 0, 0, 255), speedText);
            draw->AddText(font, fontSize, ImVec2(textX - 1, posY - 1), IM_COL32(0, 0, 0, 255), speedText);
            draw->AddText(font, fontSize, ImVec2(textX + 1, posY - 1), IM_COL32(0, 0, 0, 255), speedText);
            draw->AddText(font, fontSize, ImVec2(textX - 1, posY + 1), IM_COL32(0, 0, 0, 255), speedText);
            // Основной текст
            draw->AddText(font, fontSize, ImVec2(textX, posY), speedColor, speedText);
            
            // Подпись "u/s"
            float smallSize = baseSize * 1.0f;
            ImVec2 unitSize = font->CalcTextSizeA(smallSize, FLT_MAX, 0.0f, "u/s");
            float unitX = posX - unitSize.x / 2.0f;
            float unitY = posY + fontSize + 2.0f;
            
            draw->AddText(font, smallSize, ImVec2(unitX + 1, unitY + 1), IM_COL32(0, 0, 0, 200), "u/s");
            draw->AddText(font, smallSize, ImVec2(unitX, unitY), IM_COL32(180, 180, 180, 255), "u/s");
        }
        
        RenderMenu();
        ImGui::Render();
        g_pd3dContext->OMSetRenderTargets(1, &g_pRenderTarget, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    return oPresent(pSwap, sync, flags);
}

class VMTHook {
    void** vtable; void* original; int index; DWORD oldProt;
public:
    VMTHook(void* obj, int idx) : index(idx) { vtable = *(void***)obj; original = vtable[idx]; }
    template<typename T> T GetOriginal() { return (T)original; }
    void Hook(void* fn) { VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProt); vtable[index] = fn; VirtualProtect(&vtable[index], sizeof(void*), oldProt, &oldProt); }
    void Unhook() { VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProt); vtable[index] = original; VirtualProtect(&vtable[index], sizeof(void*), oldProt, &oldProt); }
};

VMTHook* g_PresentHook = nullptr;

bool SetupHook() {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, DefWindowProcW, 0, 0, GetModuleHandleW(0), 0, 0, 0, 0, L"X", 0 };
    RegisterClassExW(&wc);
    HWND hWnd = CreateWindowExW(0, L"X", 0, WS_OVERLAPPED, 0, 0, 2, 2, 0, 0, wc.hInstance, 0);
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE;
    ID3D11Device* dev; ID3D11DeviceContext* ctx; IDXGISwapChain* swap;
    if (FAILED(D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, D3D11_SDK_VERSION, &sd, &swap, &dev, 0, &ctx))) {
        DestroyWindow(hWnd); return false;
    }
    g_PresentHook = new VMTHook(swap, 8);
    oPresent = g_PresentHook->GetOriginal<tPresent>();
    g_PresentHook->Hook(hkPresent);
    swap->Release(); ctx->Release(); dev->Release();
    DestroyWindow(hWnd); UnregisterClassW(L"X", wc.hInstance);
    return true;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    AllocConsole();
    g_Console = GetStdHandle(STD_OUTPUT_HANDLE);
    FILE* f; freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleTitleA("PhantomHack | CS2 Internal");
    SMALL_RECT rect = {0, 0, 79, 35};
    SetConsoleWindowInfo(g_Console, TRUE, &rect);
    PrintLogo();
    Log("OK", "PhantomHack loaded successfully!");
    Log("INFO", "Waiting for CS2 window...");
    while (!FindWindowA("SDL_app", nullptr)) Sleep(100);
    Log("OK", "CS2 window found!");
    // Инициализируем глобальную переменную
    extern uintptr_t g_ClientBase;
    g_ClientBase = (uintptr_t)GetModuleHandleA("client.dll");
    if (g_ClientBase) {
        char buf[64]; sprintf(buf, "client.dll: 0x%llX", g_ClientBase);
        Log("OK", buf); Log("INFO", "Using static offsets");
    } else { Log("WARN", "client.dll not found - features disabled"); }
    Log("HOOK", "Setting up DirectX hook...");
    if (!SetupHook()) { Log("ERR", "Failed to setup hook!"); Sleep(3000); FreeLibraryAndExitThread((HMODULE)lpParam, 0); return 0; }
    Log("HOOK", "Present hook installed!");
    printf("\n");
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf("  ============================================================\n");
    SetColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("    [INSERT] Toggle Menu    [END] Unload Cheat\n");
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf("  ============================================================\n\n");
    while (g_Running) {
        if (GetAsyncKeyState(VK_END) & 1) { Log("INFO", "Unloading PhantomHack..."); g_Running = false; }
        Sleep(1);
    }
    if (g_PresentHook) { g_PresentHook->Unhook(); delete g_PresentHook; Log("HOOK", "Present hook removed"); }
    if (g_Initialized) {
        ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
        SetWindowLongPtrA(g_GameWnd, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
    }
    Log("OK", "PhantomHack unloaded. Goodbye!"); Sleep(1000);
    if (f) fclose(f); FreeConsole();
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(hModule); CreateThread(0, 0, MainThread, hModule, 0, 0); }
    return TRUE;
}