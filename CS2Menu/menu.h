#pragma once
#include <Windows.h>
#include <d3d11.h>

namespace Menu {
    void Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();
    void Render();
    void ToggleMenu();
    bool WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Настройки меню
    struct Settings {
        // Базовые настройки
        bool demo_window = false;
        bool about_window = false;
        
        // Тестовые значения
        float test_float = 0.5f;
        int test_int = 50;
        bool test_bool = false;
        float test_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    };
    
    extern Settings g_Settings;
    extern bool g_ShowMenu;
    extern int g_ScreenWidth;
    extern int g_ScreenHeight;
}