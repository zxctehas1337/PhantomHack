#include "../Features/Globals.h"

// Global variables
uintptr_t g_ClientBase = 0;
int g_ScreenWidth = 1920;
int g_ScreenHeight = 1080;

// Config variables
namespace cfg {
    // ESP
    bool esp_enabled = true;
    bool esp_box = true;
    bool esp_box_outline = true;
    int esp_box_type = 0;
    bool esp_name = true;
    bool esp_health = true;
    bool esp_healthbar = true;
    bool esp_armor = false;
    bool esp_armorbar = false;
    bool esp_distance = true;
    bool esp_snaplines = false;
    bool esp_skeleton = false;
    bool esp_head = false;
    bool esp_visible_check = false;
    int esp_snapline_pos = 2;
    float esp_max_distance = 500.0f;
    float misc_flash_alpha = 0.0f;
    float esp_color[4] = { 0.20f, 0.60f, 1.0f, 1.0f };
    float esp_team_color[4] = { 0.3f, 0.5f, 1.0f, 1.0f };
    float esp_visible_color[4] = { 0.3f, 1.0f, 0.3f, 1.0f };
    
    // Glow
    bool glow_enabled = false;
    float glow_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    
    // Misc
    bool misc_bhop = true;
    bool misc_autostrafe = false;
    bool misc_noflash = false;
    bool misc_watermark = true;
    bool misc_speedometer = true;
    
    // ==================== LEGITBOT ====================
    bool aim_legit_enabled = false;
    float aim_legit_fov = 5.0f;
    float aim_legit_smooth = 5.0f;
    bool aim_legit_vischeck = true;
    int aim_legit_bone = 0;           // Head by default
    int aim_legit_key = 0;            // LMB by default
    float aim_legit_rcs = 0.0f;       // No RCS by default
    bool aim_legit_standalone = false;
    
    // ==================== RAGEBOT ====================
    bool aim_rage_enabled = false;
    float aim_rage_fov = 180.0f;
    bool aim_rage_silent = false;
    bool aim_rage_autoshoot = false;
    bool aim_rage_autoscope = false;
    int aim_rage_bone = 0;            // Head by default
    int aim_rage_priority = 0;        // FOV by default
    int aim_rage_hitchance = 70;
    int aim_rage_mindamage = 10;
    
    // Triggerbot
    bool trigger_enabled = false;
    int trigger_delay = 50;
    
    // AutoShot
    bool autoshot_enabled = false;
    int autoshot_delay = 100;
    
    // FOV
    bool fov_enabled = false;
    float fov_value = 120.0f;
    float viewmodel_fov = 90.0f;
    
    // NoScope
    bool noscope_enabled = false;
    bool noscope_remove_zoom = false;
    
    // World
    bool world_noflash = false;
    float world_flash_alpha = 0.0f;
    bool world_nosmoke = false;
    bool world_nightmode = false;
    float world_nightmode_intensity = 0.5f;
    bool world_fullbright = false;
    bool world_nofog = false;
    bool world_color_enabled = false;
    float world_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool world_skybox_enabled = false;
    int world_skybox_type = 0;
}
