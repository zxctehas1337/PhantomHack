#pragma once
#include <Windows.h>

// Global variables shared between dllmain.cpp and Features
extern uintptr_t g_ClientBase;
extern int g_ScreenWidth;
extern int g_ScreenHeight;
extern HWND g_GameWnd;

// Config namespace
namespace cfg {
    // ESP
    extern bool esp_enabled;
    extern bool esp_box;
    extern bool esp_box_outline;
    extern int esp_box_type;
    extern bool esp_name;
    extern bool esp_health;
    extern bool esp_healthbar;
    extern bool esp_armor;
    extern bool esp_armorbar;
    extern bool esp_distance;
    extern bool esp_snaplines;
    extern int esp_snapline_pos;
    extern bool esp_head;
    extern float esp_max_distance;
    extern float esp_color[4];
    
    // Glow
    extern bool glow_enabled;
    extern float glow_color[4];
    
    // Misc
    extern bool misc_bhop;
    extern bool misc_autostrafe;
    extern bool misc_watermark;
    
    // ==================== LEGITBOT ====================
    extern bool aim_legit_enabled;
    extern float aim_legit_fov;        // FOV radius (1-30)
    extern float aim_legit_smooth;     // Smoothing (1-20)
    extern bool aim_legit_vischeck;    // Visibility check
    extern int aim_legit_bone;         // Target bone (0=head, 1=neck, 2=chest, 3=stomach, 4=pelvis)
    extern int aim_legit_key;          // Activation key (0=LMB, 1=RMB, 2=Mouse4, 3=Mouse5, 4=Shift)
    extern float aim_legit_rcs;        // Recoil control (0-100%)
    extern bool aim_legit_standalone;  // Work without shooting
    
    // ==================== RAGEBOT ====================
    extern bool aim_rage_enabled;
    extern float aim_rage_fov;         // FOV radius (1-180)
    extern bool aim_rage_silent;       // Silent aim
    extern bool aim_rage_autoshoot;    // Auto shoot
    extern bool aim_rage_autoscope;    // Auto scope for snipers
    extern int aim_rage_bone;          // Target bone
    extern int aim_rage_priority;      // Target priority (0=FOV, 1=Distance, 2=Health)
    extern int aim_rage_hitchance;     // Hit chance (0-100%)
    extern int aim_rage_mindamage;     // Minimum damage
    
    // Triggerbot
    extern bool trigger_enabled;
    extern int trigger_delay;
    
    // AutoShot
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
