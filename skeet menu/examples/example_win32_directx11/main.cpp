#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

#include <d3d11.h>
#include <D3DX11.h>
#pragma comment (lib, "d3dx11.lib")

#include "bytes/bites.h"
#include <misc/freetype/imgui_freetype.h>

#include "bytes/sex.hpp"
#include <map>
#include <imgui_internal.h>

ID3D11ShaderResourceView* BG = nullptr;
ID3D11ShaderResourceView* BG2 = nullptr;

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int tab = 0;

bool enableaim = false;
bool aimlock = false;
bool visiblecheck = false;
bool playerlock = false;
bool triggerbot = false;
bool rcsenable = false;

int selecthitbox = 0;

int fov = 0;
float smooth = 0;
float sensivity = 0;
int preferience = 0;

bool espBox = false;
bool espName = false;
bool espHeath = false;
bool espGlow = false;
bool espBomb = false;

bool chamsVisible = false;
int chamsIntensity = 0;

bool chamsInvisible = false;
int chamsInvisIntensity = 0;

bool modelGlow = false;
int modelGlowIntensity = 0;

bool nightmode = false;
bool removeflash = false;
bool removelegs = false;

bool viewmodelEditor = false;
float viewF = 0;
float viewX = 0;
float viewY = 0;
float viewZ = 0;

bool knifeChanger = false;
int combo_knife = 0;

void CustomSlider(const char* label, float* v, float v_min, float v_max, bool checker_bool)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    const ImGuiID id = window->GetID(label);
    static std::map<ImGuiID, int> anim_state;
    auto state = anim_state.find(id);

    //if (state == anim_state.end())
       // anim_state[id] = 45;

    //const float original_pos = ImGui::GetCursorPosX();

    //ImGui::SetCursorPosX(original_pos);
    auto posSlider = ImGui::GetCursorScreenPos();
    if (checker_bool)
    {
        ImGui::SliderFloat(label, v, v_min, v_max, "%.0f");
    }
    else
    {
        ImGui::BeginDisabled(true);
        ImGui::SliderFloat(label, v, v_min, v_max, "");
        ImGui::EndDisabled();
        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(posSlider.x + 16, posSlider.y), ImVec2(posSlider.x + 180, posSlider.y + 37), ImColor(22, 22, 22, 155));
    }


    //if (checker_bool)
    //{
        //if (anim_state[id] > 0)
            //anim_state[id] = ImMax(0, anim_state[id] - 8);

        //ImGui::SetCursorPosX(original_pos - anim_state[id]);
    //}
    //else
    //{
        //anim_state[id] = 245;
        //ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(posSlider.x + 16, posSlider.y), ImVec2(posSlider.x + 180, posSlider.y + 40), ImColor(22, 22, 22, 155));
    //}
}

void CustomSliderInt(const char* label, int* v, float v_min, float v_max, bool checker_bool)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    const ImGuiID id = window->GetID(label);
    static std::map<ImGuiID, int> anim_state;
    auto state = anim_state.find(id);

    //if (state == anim_state.end())
       // anim_state[id] = 45;

    //const float original_pos = ImGui::GetCursorPosX();

    //ImGui::SetCursorPosX(original_pos);
    auto posSlider = ImGui::GetCursorScreenPos();
    if (checker_bool)
    {
        ImGui::SliderInt(label, v, v_min, v_max);
    }
    else
    {
        ImGui::BeginDisabled(true);
        ImGui::SliderInt(label, v, v_min, v_max, "");
        ImGui::EndDisabled();
        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(posSlider.x + 16, posSlider.y), ImVec2(posSlider.x + 180, posSlider.y + 37), ImColor(22, 22, 22, 155));
    }


    //if (checker_bool)
    //{
        //if (anim_state[id] > 0)
            //anim_state[id] = ImMax(0, anim_state[id] - 8);

        //ImGui::SetCursorPosX(original_pos - anim_state[id]);
    //}
    //else
    //{
        //anim_state[id] = 245;
        //ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(posSlider.x + 16, posSlider.y), ImVec2(posSlider.x + 180, posSlider.y + 40), ImColor(22, 22, 22, 155));
    //}
}

// Main code
int main(int, char**)
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImFontConfig font_config;

    font_config.FontBuilderFlags |=
        ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_MonoHinting;

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdana.ttf", 12, &font_config, io.Fonts->GetGlyphRangesCyrillic());

    D3DX11_IMAGE_LOAD_INFO iInfo;
    ID3DX11ThreadPump* threadPump{ nullptr };

    D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, background, sizeof(background), &iInfo, threadPump, &BG, 0);
    D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, background2, sizeof(background2), &iInfo, threadPump, &BG2, 0);
    D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, checkboxBG, sizeof(checkboxBG), &iInfo, threadPump, &widgets::check_box, 0);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        auto size = ImVec2(276 /* widgets::dpi_scale */, 415 /* widgets::dpi_scale */);

        ImGui::SetNextWindowSize(size);
        ImGui::Begin("RAZECLUB", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        {
            auto draw = ImGui::GetWindowDrawList();
            auto pos = ImGui::GetWindowPos();

            draw->AddImage(BG, pos, ImVec2(pos.x + size.x, pos.y + size.y));

            ImGui::SetCursorPos(ImVec2(68, 16));
            ImGui::BeginChild("LeftPanel", ImVec2(185, 375), true);
            {
                ImGui::Checkbox("Enabled", &enableaim);

                ImGui::Combo("Hitbox", &selecthitbox, "-\0Head\0Chest\0Stomath\0Pelvis\0", 6);

                ImGui::Checkbox("Aimlock", &aimlock);
                ImGui::Checkbox("Visible check", &visiblecheck);
                ImGui::Checkbox("Player lock", &playerlock);

                CustomSliderInt("Aimbot FOV", &fov, 0, 15, enableaim);

                CustomSlider("Aimbot smooth", &smooth, 0, 30, enableaim);

                CustomSlider("Aimbot sensivity", &sensivity, 0, 10, enableaim);

                CustomSliderInt("Aimbot preferience", &preferience, 0, 3, enableaim);

                ImGui::Checkbox("Trigger bot", &triggerbot);
                ImGui::Checkbox("Recoil control system", &rcsenable);
            }
            ImGui::EndChild();
        }
        ImGui::End();

        auto size2 = ImVec2(472 /* widgets::dpi_scale */, 415 /* widgets::dpi_scale */);

        ImGui::SetNextWindowSize(size2);
        ImGui::Begin("RAZECLUB2", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        {
            auto draw = ImGui::GetWindowDrawList();
            auto pos = ImGui::GetWindowPos();

            draw->AddImage(BG2, pos, ImVec2(pos.x + size2.x, pos.y + size2.y));

            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::BeginChild("LeftIcons", ImVec2(55, 415), true);
            {
                if (ImGui::InvisibleButton("Legitbot", ImVec2(55, 45)))
                    tab = 0;
                if (ImGui::InvisibleButton("Visuals", ImVec2(55, 35)))
                    tab = 1;
                if (ImGui::InvisibleButton("Misc", ImVec2(55, 45)))
                    tab = 2;
                if (ImGui::InvisibleButton("Skins", ImVec2(55, 35)))
                    tab = 3;
                if (ImGui::InvisibleButton("Configs", ImVec2(55, 45)))
                    tab = 4;
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(73, 23));
            ImGui::BeginChild("LeftPanel", ImVec2(185, 375));
            {
                switch (tab)
                {
                case 0:
                    ImGui::Checkbox("Esp box", &espBox);
                    ImGui::Checkbox("Esp name", &espName);
                    ImGui::Checkbox("Esp health", &espHeath);
                    ImGui::Checkbox("Esp glow", &espGlow);
                    ImGui::Checkbox("Esp bomb", &espBomb);

                    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

                    ImGui::Checkbox("Chams visible", &chamsVisible);
                    ImGui::SliderInt("Visible chams intensity", &chamsIntensity, 0, 255);

                    ImGui::Checkbox("Chams invisible", &chamsInvisible);
                    ImGui::SliderInt("Invisible chams intensity", &chamsInvisIntensity, 0, 255);

                    ImGui::Checkbox("Model glow", &modelGlow);
                    ImGui::SliderInt("Model glow intensity", &modelGlowIntensity, 0, 255);

                    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

                    ImGui::Checkbox("Nightmode", &nightmode);
                    ImGui::Checkbox("Remove flash", &removeflash);
                    ImGui::Checkbox("Remove legs", &removelegs);
                    break;
                }
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(271, 23));
            ImGui::BeginChild("RightPanel", ImVec2(185, 375));
            {
                switch (tab)
                {
                case 0:
                    ImGui::Checkbox("Viewmodel editor", &viewmodelEditor);
                    CustomSlider("Viewmodel FOV", &viewF, -60, 60, viewmodelEditor);
                    CustomSlider("Viewmodel X", &viewX, -3, 3, viewmodelEditor);
                    CustomSlider("Viewmodel Y", &viewY, -3, 3, viewmodelEditor);
                    CustomSlider("Viewmodel Z", &viewZ, -3, 3, viewmodelEditor);

                    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

                    ImGui::Checkbox("Knife changer", &knifeChanger);
                    ImGui::Combo("#combo", &combo_knife, "-\0Karambit\0M9 Bayonet", 5);

                    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

                    ImGui::Combo("#dpi", &widgets::dpi_select, "dpi 1 0.75\0dpi 1\0dpi 1.25\0dpi 1.5", 5);
                    break;
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
