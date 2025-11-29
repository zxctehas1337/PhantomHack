#pragma once
#include <Windows.h>

namespace NoScope {
    void RunNoScope();
    void ResetScope();
    
    // Состояние NoScope
    extern bool g_NoScopeActive;
    extern bool g_WasScoped;
    extern float g_OriginalScopeAlpha;
}