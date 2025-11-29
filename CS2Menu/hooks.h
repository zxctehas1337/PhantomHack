#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace Hooks {
    bool Initialize();
    void Shutdown();
    void RenderLoop();
    void RenderFrame();
    
    // DirectX 11
    extern HWND g_hWnd;
    extern HWND g_OverlayWnd;
    extern ID3D11Device* g_pd3dDevice;
    extern ID3D11DeviceContext* g_pd3dDeviceContext;
    extern IDXGISwapChain* g_pSwapChain;
    extern ID3D11RenderTargetView* g_mainRenderTargetView;
    
    extern bool g_Running;
    
    // Типы
    typedef HRESULT(__stdcall* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
    
    extern Present oPresent;
    extern WNDPROC oWndProc;
    
    // Функции
    HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    LRESULT __stdcall hkWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}