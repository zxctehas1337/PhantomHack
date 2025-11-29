#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <cstdio>
#include "../Shared/shared_data.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Globals
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
HWND g_hWnd = nullptr;
HWND g_GameWnd = nullptr;
bool g_ShowMenu = true;
bool g_Running = true;
int g_Width = 1920;
int g_Height = 1080;

CheatSettings* g_Settings = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowPadding = ImVec2(15, 15);
    style.ItemSpacing = ImVec2(8, 6);
    
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.95f);
    c[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.05f, 0.15f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.10f, 0.30f, 1.00f);
    c[ImGuiCol_Border] = ImVec4(0.50f, 0.20f, 0.80f, 0.50f);
    c[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.10f, 0.20f, 0.54f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.20f, 0.60f, 0.40f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.50f, 0.25f, 0.75f, 0.67f);
    c[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.40f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.30f, 0.90f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.40f, 1.00f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.40f, 0.20f, 0.60f, 0.40f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.25f, 0.75f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.30f, 0.90f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.40f, 0.20f, 0.60f, 0.31f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.25f, 0.75f, 0.80f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.30f, 0.90f, 1.00f);
    c[ImGuiCol_Tab] = ImVec4(0.20f, 0.10f, 0.30f, 0.86f);
    c[ImGuiCol_TabHovered] = ImVec4(0.50f, 0.25f, 0.75f, 0.80f);
    c[ImGuiCol_TabActive] = ImVec4(0.40f, 0.20f, 0.60f, 1.00f);
}

void RenderMenu() {
    if (!g_Settings) return;
    
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("PhantomHack | CS2", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);
    
    ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "PhantomHack");
    ImGui::SameLine();
    ImGui::TextDisabled("v1.0 | %s", g_Settings->initialized ? "Internal OK" : "Waiting for DLL...");
    ImGui::Separator();
    
    if (ImGui::BeginTabBar("Tabs")) {
        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::Checkbox("Enable Aimbot", &g_Settings->aimbot_enabled);
            ImGui::SliderFloat("FOV", &g_Settings->aimbot_fov, 1.0f, 30.0f);
            ImGui::SliderFloat("Smooth", &g_Settings->aimbot_smooth, 1.0f, 20.0f);
            ImGui::Checkbox("Visible Only", &g_Settings->aimbot_visible_only);
            
            const char* bones[] = { "Head", "Neck", "Chest" };
            ImGui::Combo("Bone", &g_Settings->aimbot_bone, bones, 3);
            
            ImGui::Separator();
            ImGui::Checkbox("Enable Triggerbot", &g_Settings->triggerbot_enabled);
            ImGui::SliderInt("Trigger Delay (ms)", &g_Settings->triggerbot_delay, 0, 200);
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("ESP")) {
            ImGui::Checkbox("Enable ESP", &g_Settings->esp_enabled);
            ImGui::Checkbox("Box", &g_Settings->esp_box);
            ImGui::Checkbox("Name", &g_Settings->esp_name);
            ImGui::Checkbox("Health", &g_Settings->esp_health);
            ImGui::Checkbox("Distance", &g_Settings->esp_distance);
            ImGui::SliderFloat("Max Distance", &g_Settings->esp_max_distance, 100.0f, 2000.0f);
            ImGui::ColorEdit4("ESP Color", g_Settings->esp_color);
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Visuals")) {
            ImGui::Checkbox("Glow ESP", &g_Settings->glow_enabled);
            ImGui::ColorEdit4("Glow Color", g_Settings->glow_color);
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::Checkbox("Bunny Hop", &g_Settings->bhop_enabled);
            ImGui::Checkbox("No Flash", &g_Settings->no_flash_enabled);
            ImGui::Checkbox("Radar Hack", &g_Settings->radar_hack_enabled);
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Info")) {
            ImGui::Text("PhantomHack CS2");
            ImGui::Separator();
            ImGui::BulletText("INSERT - Toggle Menu");
            ImGui::BulletText("END - Exit");
            ImGui::Separator();
            ImGui::Text("Internal: %s", g_Settings->initialized ? "Connected" : "Not connected");
            ImGui::Text("In Game: %s", g_Settings->in_game ? "Yes" : "No");
            if (g_Settings->in_game) {
                ImGui::Text("Health: %d", g_Settings->local_health);
                ImGui::Text("Team: %d", g_Settings->local_team);
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}
