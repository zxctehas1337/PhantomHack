#include "menu.h"
#include "hooks.h"
#include "../Features/ESP/ESP.h"
#include "../Features/Globals.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include <iostream>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Menu {
    Settings g_Settings;
    bool g_ShowMenu = true;
    bool g_Initialized = false;
    int g_ScreenWidth = 1920;
    int g_ScreenHeight = 1080;
    
    void RenderMenu();
    
    void Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context) {
        if (g_Initialized) return;
        
        printf("[PhantomHack] Initializing ImGui...\n");
        
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        
        style.WindowRounding = 8.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.95f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.05f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.10f, 0.30f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.50f, 0.20f, 0.80f, 0.50f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.10f, 0.20f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.20f, 0.60f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.50f, 0.25f, 0.75f, 0.67f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.40f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.30f, 0.90f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.40f, 1.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.40f, 0.20f, 0.60f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.25f, 0.75f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.30f, 0.90f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.40f, 0.20f, 0.60f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.25f, 0.75f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.30f, 0.90f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.10f, 0.30f, 0.86f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.50f, 0.25f, 0.75f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.40f, 0.20f, 0.60f, 1.00f);
        
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(device, context);
        
        g_Initialized = true;
        printf("[PhantomHack] ImGui initialized!\n");
    }
    
    void Shutdown() {
        if (!g_Initialized) return;
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_Initialized = false;
    }
    
    void Render() {
        if (!g_Initialized) return;
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ESP::UpdatePlayers();
        ESP::RenderESP();
        if (g_ShowMenu) RenderMenu();
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void RenderMenu() {
        ImGui::SetNextWindowSize(ImVec2(550, 480), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("PhantomHack | CS2", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);
        
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "PhantomHack");
        ImGui::SameLine();
        ImGui::TextDisabled("v2.0");
        ImGui::Separator();
        
        if (ImGui::BeginTabBar("MainTabs")) {
            
            // LEGITBOT TAB (DISABLED)
            if (ImGui::BeginTabItem("Legitbot")) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[ Feature not available yet ]");
                ImGui::Spacing();
                
                static bool dummy_legit = false;
                ImGui::BeginDisabled(true);
                ImGui::Checkbox("Enable Legitbot", &dummy_legit);
                ImGui::Separator();
                
                ImGui::Text("Aiming:");
                static float dummy_fov = 5.0f;
                static float dummy_smooth = 5.0f;
                static float dummy_rcs = 0.0f;
                ImGui::SliderFloat("FOV##legit", &dummy_fov, 1.0f, 30.0f, "%.1f");
                ImGui::SliderFloat("Smooth##legit", &dummy_smooth, 1.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("RCS##legit", &dummy_rcs, 0.0f, 100.0f, "%.0f%%");
                
                ImGui::Separator();
                ImGui::Text("Target:");
                static int dummy_bone = 0;
                static int dummy_key = 0;
                const char* bones[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis" };
                ImGui::Combo("Hitbox##legit", &dummy_bone, bones, IM_ARRAYSIZE(bones));
                const char* keys[] = { "Left Mouse", "Right Mouse", "Mouse 4", "Mouse 5", "Shift" };
                ImGui::Combo("Activation Key", &dummy_key, keys, IM_ARRAYSIZE(keys));
                
                ImGui::EndDisabled();
                ImGui::PopStyleVar();
                
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Coming soon...");
                ImGui::EndTabItem();
            }
            
            // RAGEBOT TAB (DISABLED)
            if (ImGui::BeginTabItem("Ragebot")) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[ Feature not available yet ]");
                ImGui::Spacing();
                
                static bool dummy_rage = false;
                ImGui::BeginDisabled(true);
                ImGui::Checkbox("Enable Ragebot", &dummy_rage);
                ImGui::Separator();
                
                ImGui::Text("Aiming:");
                static float dummy_rage_fov = 180.0f;
                static int dummy_hitchance = 70;
                static int dummy_mindmg = 10;
                ImGui::SliderFloat("FOV##rage", &dummy_rage_fov, 1.0f, 180.0f, "%.0f");
                ImGui::SliderInt("Hitchance", &dummy_hitchance, 0, 100, "%d%%");
                ImGui::SliderInt("Min Damage", &dummy_mindmg, 1, 100);
                
                ImGui::Separator();
                ImGui::Text("Target:");
                static int dummy_rage_bone = 0;
                static int dummy_priority = 0;
                const char* bones[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis" };
                ImGui::Combo("Hitbox##rage", &dummy_rage_bone, bones, IM_ARRAYSIZE(bones));
                const char* priorities[] = { "FOV", "Distance", "Health" };
                ImGui::Combo("Priority", &dummy_priority, priorities, IM_ARRAYSIZE(priorities));
                
                ImGui::EndDisabled();
                ImGui::PopStyleVar();
                
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Coming soon...");
                ImGui::EndTabItem();
            }
            
            // ESP TAB
            if (ImGui::BeginTabItem("ESP")) {
                ImGui::Checkbox("Enable ESP", &cfg::esp_enabled);
                ImGui::Separator();
                
                if (cfg::esp_enabled) {
                    ImGui::Text("Display Options:");
                    ImGui::Checkbox("Box", &cfg::esp_box);
                    ImGui::SameLine(150);
                    ImGui::Checkbox("Name", &cfg::esp_name);
                    ImGui::Checkbox("Health", &cfg::esp_health);
                    ImGui::SameLine(150);
                    ImGui::Checkbox("Distance", &cfg::esp_distance);
                    ImGui::Checkbox("Health Bar", &cfg::esp_healthbar);
                    ImGui::SameLine(150);
                    ImGui::Checkbox("Snaplines", &cfg::esp_snaplines);
                    
                    ImGui::Separator();
                    ImGui::SliderFloat("Max Distance", &cfg::esp_max_distance, 50.0f, 2000.0f, "%.0f m");
                    ImGui::ColorEdit4("ESP Color", cfg::esp_color);
                }
                ImGui::EndTabItem();
            }
            
            // MISC TAB
            if (ImGui::BeginTabItem("Misc")) {
                ImGui::Text("Movement:");
                ImGui::Checkbox("Bunny Hop", &cfg::misc_bhop);
                ImGui::SameLine(200);
                ImGui::Checkbox("Auto Strafe", &cfg::misc_autostrafe);
                
                ImGui::Separator();
                ImGui::Text("Triggerbot:");
                ImGui::Checkbox("Enable Triggerbot", &cfg::trigger_enabled);
                if (cfg::trigger_enabled) {
                    ImGui::SliderInt("Trigger Delay", &cfg::trigger_delay, 0, 200, "%d ms");
                }
                
                ImGui::Separator();
                ImGui::Text("Visuals:");
                ImGui::Checkbox("NoScope", &cfg::noscope_enabled);
                ImGui::Checkbox("Watermark", &cfg::misc_watermark);
                ImGui::EndTabItem();
            }
            
            // VISUALS TAB
            if (ImGui::BeginTabItem("Visuals")) {
                ImGui::Text("Glow:");
                ImGui::Checkbox("Enable Glow", &cfg::glow_enabled);
                if (cfg::glow_enabled) {
                    ImGui::ColorEdit4("Glow Color", cfg::glow_color);
                }
                
                ImGui::Separator();
                ImGui::Text("FOV Changer:");
                ImGui::Checkbox("Enable FOV", &cfg::fov_enabled);
                if (cfg::fov_enabled) {
                    ImGui::SliderFloat("Camera FOV", &cfg::fov_value, 90.0f, 150.0f, "%.0f");
                    ImGui::SliderFloat("Viewmodel FOV", &cfg::viewmodel_fov, 68.0f, 120.0f, "%.0f");
                }
                ImGui::EndTabItem();
            }
            
            // WORLD TAB
            if (ImGui::BeginTabItem("World")) {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "World Modifiers");
                ImGui::Separator();
                
                // Flash removal - WORKS
                ImGui::Text("Anti-Effects:");
                ImGui::Checkbox("No Flash", &cfg::world_noflash);
                if (cfg::world_noflash) {
                    ImGui::SameLine();
                    ImGui::SliderFloat("Max Alpha##flash", &cfg::world_flash_alpha, 0.0f, 100.0f, "%.0f");
                }
                
                // These need ConVar/hooks - show as coming soon
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                ImGui::BeginDisabled(true);
                
                ImGui::Checkbox("No Smoke", &cfg::world_nosmoke);
                ImGui::SameLine();
                ImGui::TextDisabled("(coming soon)");
                
                ImGui::Checkbox("No Fog", &cfg::world_nofog);
                ImGui::SameLine();
                ImGui::TextDisabled("(coming soon)");
                
                ImGui::Separator();
                ImGui::Text("Lighting:");
                
                ImGui::Checkbox("Night Mode", &cfg::world_nightmode);
                if (cfg::world_nightmode) {
                    ImGui::SliderFloat("Intensity##night", &cfg::world_nightmode_intensity, 0.0f, 1.0f, "%.2f");
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(coming soon)");
                
                ImGui::Checkbox("Fullbright", &cfg::world_fullbright);
                ImGui::SameLine();
                ImGui::TextDisabled("(coming soon)");
                
                ImGui::Separator();
                ImGui::Text("Skybox:");
                
                ImGui::Checkbox("Custom Skybox", &cfg::world_skybox_enabled);
                if (cfg::world_skybox_enabled) {
                    const char* skyboxes[] = { "Default", "Night", "Sunset", "Galaxy", "Clear Blue", "Stormy" };
                    ImGui::Combo("Skybox Type", &cfg::world_skybox_type, skyboxes, IM_ARRAYSIZE(skyboxes));
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(coming soon)");
                
                ImGui::Separator();
                ImGui::Text("World Color:");
                
                ImGui::Checkbox("Enable Color Mod", &cfg::world_color_enabled);
                if (cfg::world_color_enabled) {
                    ImGui::ColorEdit4("World Tint", cfg::world_color);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(coming soon)");
                
                ImGui::EndDisabled();
                ImGui::PopStyleVar();
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "* No Flash is fully working!");
                
                ImGui::EndTabItem();
            }
            
            // SETTINGS TAB
            if (ImGui::BeginTabItem("Settings")) {
                ImGui::Text("Menu Settings");
                ImGui::Separator();
                
                static float menuAlpha = 0.95f;
                ImGui::SliderFloat("Menu Opacity", &menuAlpha, 0.5f, 1.0f);
                ImGui::GetStyle().Alpha = menuAlpha;
                
                ImGui::Separator();
                ImGui::BulletText("INSERT - Toggle Menu");
                ImGui::BulletText("END - Unload Cheat");
                ImGui::Text("Build: %s %s", __DATE__, __TIME__);
                ImGui::Text("Players: %d", (int)ESP::g_Players.size());
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        if (ImGui::Button("Hide Menu")) g_ShowMenu = false;
        ImGui::SameLine();
        if (ImGui::Button("Unload")) { }
        ImGui::End();
        
        if (g_Settings.demo_window) ImGui::ShowDemoWindow(&g_Settings.demo_window);
        if (g_Settings.about_window) ImGui::ShowAboutWindow(&g_Settings.about_window);
    }
    
    void ToggleMenu() {
        g_ShowMenu = !g_ShowMenu;
        printf("[PhantomHack] Menu %s\n", g_ShowMenu ? "opened" : "closed");
    }
    
    bool WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
            ToggleMenu();
            return true;
        }
        if (g_ShowMenu && g_Initialized) {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                return true;
        }
        return false;
    }
}
