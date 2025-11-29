#include "hooks.h"
#include "menu.h"
#include "../Features/World/World.h"
#include "../Features/BunnyHop/BunnyHop.h"
#include <iostream>
#include <dwmapi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

namespace Hooks {
    HWND g_hWnd = nullptr;
    HWND g_OverlayWnd = nullptr;
    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
    IDXGISwapChain* g_pSwapChain = nullptr;
    ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
    
    Present oPresent = nullptr;
    WNDPROC oWndProc = nullptr;
    
    bool g_Running = true;
    int g_Width = 1920;
    int g_Height = 1080;
    
    LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (Menu::WndProc(hWnd, msg, wParam, lParam))
            return 0;
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    
    bool Initialize() {
        printf("[PhantomHack] Initializing overlay...\n");
        
        // Найти окно CS2
        g_hWnd = FindWindowA("SDL_app", nullptr);
        if (!g_hWnd) {
            printf("[PhantomHack] CS2 window not found!\n");
            return false;
        }
        
        RECT rc;
        GetClientRect(g_hWnd, &rc);
        g_Width = rc.right - rc.left;
        g_Height = rc.bottom - rc.top;
        
        printf("[PhantomHack] Game window: %dx%d\n", g_Width, g_Height);
        
        // Регистрация класса окна
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = OverlayWndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"PhantomOverlay";
        RegisterClassExW(&wc);
        
        // Создание оверлей окна
        g_OverlayWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
            L"PhantomOverlay", L"",
            WS_POPUP,
            0, 0, g_Width, g_Height,
            nullptr, nullptr, wc.hInstance, nullptr
        );
        
        if (!g_OverlayWnd) {
            printf("[PhantomHack] Failed to create overlay!\n");
            return false;
        }
        
        SetLayeredWindowAttributes(g_OverlayWnd, RGB(0, 0, 0), 255, LWA_ALPHA);
        
        // Margins для прозрачности
        MARGINS margins = { -1 };
        DwmExtendFrameIntoClientArea(g_OverlayWnd, &margins);
        
        ShowWindow(g_OverlayWnd, SW_SHOWNOACTIVATE);
        
        printf("[PhantomHack] Overlay window created!\n");
        
        // Создание D3D11
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = g_Width;
        sd.BufferDesc.Height = g_Height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_OverlayWnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pd3dDeviceContext
        );
        
        if (FAILED(hr)) {
            printf("[PhantomHack] D3D11 failed: 0x%lx\n", hr);
            return false;
        }
        
        ID3D11Texture2D* pBackBuffer;
        g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
        
        printf("[PhantomHack] D3D11 initialized!\n");
        
        // Инициализация ImGui
        Menu::Initialize(g_OverlayWnd, g_pd3dDevice, g_pd3dDeviceContext);
        
        printf("[PhantomHack] Ready!\n");
        return true;
    }
    
    void RenderFrame() {
        if (!g_Running || !g_OverlayWnd) return;
        
        // Обновляем позицию оверлея
        if (g_hWnd && IsWindow(g_hWnd)) {
            RECT rc;
            GetWindowRect(g_hWnd, &rc);
            SetWindowPos(g_OverlayWnd, HWND_TOPMOST, rc.left, rc.top, 
                rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE);
        }
        
        // Рендер
        const float clear[4] = { 0, 0, 0, 0 };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
        
        Menu::Render();
        
        g_pSwapChain->Present(1, 0);
    }
    
    void RenderLoop() {
        printf("[PhantomHack] Render loop started\n");
        
        while (g_Running) {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            
            // Run features
            World::RunWorld();
            BunnyHop::RunBhop();
            
            RenderFrame();
            Sleep(16);
        }
    }
    
    void Shutdown() {
        g_Running = false;
        
        Menu::Shutdown();
        
        if (g_mainRenderTargetView) g_mainRenderTargetView->Release();
        if (g_pSwapChain) g_pSwapChain->Release();
        if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
        if (g_pd3dDevice) g_pd3dDevice->Release();
        if (g_OverlayWnd) DestroyWindow(g_OverlayWnd);
        
        UnregisterClassW(L"PhantomOverlay", GetModuleHandleW(nullptr));
    }
    
    // Не используются
    HRESULT __stdcall hkPresent(IDXGISwapChain* p, UINT s, UINT f) { return S_OK; }
    LRESULT __stdcall hkWndProc(HWND h, UINT m, WPARAM w, LPARAM l) { return 0; }
}