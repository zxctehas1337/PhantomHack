#include "AutoShot.h"
#include "../Offsets.h"
#include "../Globals.h"
#include <cstdint>

namespace AutoShot {
    
    template<typename T>
    T Read(uintptr_t addr) {
        if (!addr) return T{};
        __try {
            return *reinterpret_cast<T*>(addr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return T{};
        }
    }
    
    void RunAutoShot() {
        if (!cfg::autoshot_enabled) return;
        
        static DWORD lastShot = 0;
        DWORD now = GetTickCount();
        
        // Check delay between shots
        if (now - lastShot < (DWORD)cfg::autoshot_delay) return;
        
        // Simple AutoShot: shoot when key is pressed (using Mouse5 by default)
        static bool wasKeyPressed = false;
        bool isKeyPressed = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
        
        if (isKeyPressed && !wasKeyPressed) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            Sleep(10);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            lastShot = now;
        } else if (isKeyPressed && wasKeyPressed && (now - lastShot >= (DWORD)cfg::autoshot_delay)) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            Sleep(10);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            lastShot = now;
        }
        
        wasKeyPressed = isKeyPressed;
    }
}
