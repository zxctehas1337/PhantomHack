#include "Overlay.h"

namespace {
    //watermark
    float g_window_alpha_w = 0.f;
    float g_window_height_w = 35.f;
    std::chrono::steady_clock::time_point g_last_update_w;
    float g_window_width_w = 180.f;
    ImVec2 g_window_pos_w = ImVec2(50, 50);

    //kb list
    float g_window_alpha = 0.f;
    float g_window_height = 30.f;
    float g_bind_alpha[5] = { 0.f, 0.f, 0.f, 0.f, 0.f };
    std::chrono::steady_clock::time_point g_last_update;
    float g_window_width = 165.f;
    ImVec2 g_window_pos = ImVec2(50, 50);
    bool g_window_pos_initialized = false;

    //spec list
    float g_spec_alpha = 0.f;
    float g_spec_height = 30.f;
    std::chrono::steady_clock::time_point g_spec_last_update;
    float g_spec_width = 165.f;
    ImVec2 g_spec_pos = ImVec2(50, 50);
    bool g_spec_pos_initialized = false;
}

void COverlay::Watermark() {
    if (!Config::vb(g_Variables.m_Gui.m_vbOverlay).at(OVERLAY_WATERMARK))
        return;

    static float last_update_time = 0.f;
    static int fps = 0, ping = 0, speed = 0, frame_count = 0;
    float current_time = ImGui::GetTime();

    if (current_time - last_update_time >= 1.f) {
        fps = frame_count;
        frame_count = 0;
        ping = Interfaces::m_pEngine->IsInGame() && Globals::m_pLocalPlayerController ? Globals::m_pLocalPlayerController->m_iPing() : 0;
        last_update_time = current_time;
    }

    static float last_speed_update = 0.f;
    if (current_time - last_speed_update >= 0.2f) {
        if (Interfaces::m_pEngine->IsInGame() && Globals::m_pLocalPlayerPawn) {
            speed = static_cast<int>(Globals::m_pLocalPlayerPawn->m_vecAbsVelocity().Length2D());
        }
        else {
            speed = 0;
        }
        last_speed_update = current_time;
    }

    frame_count++;
    auto now = std::chrono::steady_clock::now();
    float delta_time = std::min(std::chrono::duration<float>(now - g_last_update_w).count(), 0.1f);
    g_last_update_w = now;

    time_t current_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char time_str_buffer[26];
    ctime_s(time_str_buffer, sizeof(time_str_buffer), &current_time_t);
    std::string time_str = std::string(time_str_buffer).substr(11, 5) + " TIME";
    std::string speed_text = std::to_string(speed) + " SPEED";
    std::string fps_text = std::to_string(fps) + " FPS";
    std::string ping_text = std::to_string(ping) + " PING";

    g_window_alpha_w = ImLerp(g_window_alpha_w, 1.f, delta_time * 12.f);
    if (g_window_alpha_w <= 0.01f)
        return;

    ImFont* font = Fonts::overlay3;
    float main_width = 0.f, offset = 10.f;

    if (font) {
        ImVec2 fps_size = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, fps_text.c_str());
        ImVec2 ping_size = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, ping_text.c_str());
        ImVec2 time_size = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, time_str.c_str());
        ImVec2 speed_size = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, speed_text.c_str());
        main_width = fps_size.x + 4 + ping_size.x + 4 + time_size.x + 4 + speed_size.x + 30.f;
    }

    float total_width = main_width + 10.f;
    static ImVec2 watermark_pos = ImVec2(ImGui::GetIO().DisplaySize.x - offset, 10);

    g_window_width_w = ImLerp(g_window_width_w, total_width, delta_time * 10.f);
    g_window_height_w = ImLerp(g_window_height_w, 20.f, delta_time * 10.f);

    auto draw_list = ImGui::GetBackgroundDrawList();
    if (g_window_alpha_w > 0.01f) {
        ImVec2 main_pos = ImVec2(watermark_pos.x - main_width, watermark_pos.y);

        draw_list->AddRectFilled({ main_pos.x, main_pos.y }, { main_pos.x + main_width, main_pos.y + g_window_height_w }, IM_COL32(20, 20, 20, 200 * g_window_alpha_w));
        draw_list->AddRect({ main_pos.x, main_pos.y }, { main_pos.x + main_width, main_pos.y + g_window_height_w }, IM_COL32(100, 100, 100, 50 * g_window_alpha_w), 0.f, 0, 1.f);

        if (font) {
            ImVec2 fps_size = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, fps_text.c_str());
            ImVec2 ping_size = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, ping_text.c_str());
            ImVec2 time_size = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, time_str.c_str());
            ImVec2 speed_size = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, speed_text.c_str());

            float x_offset = main_pos.x + 7;
            float text_y = main_pos.y + (g_window_height_w - fps_size.y) * 0.5f;

            draw_list->AddText(font, 16.f, { x_offset, text_y }, IM_COL32(220, 220, 220, 255 * g_window_alpha_w), fps_text.c_str());
            x_offset += fps_size.x + 4;

            draw_list->AddLine(ImVec2(x_offset, text_y + (fps_size.y * 0.2f)), ImVec2(x_offset, text_y + (fps_size.y * 0.8f)), IM_COL32(220, 220, 220, 200 * g_window_alpha_w), 1.0f);
            x_offset += 5;

            draw_list->AddText(font, 16.f, { x_offset, text_y }, IM_COL32(220, 220, 220, 255 * g_window_alpha_w), ping_text.c_str());
            x_offset += ping_size.x + 4;

            draw_list->AddLine(ImVec2(x_offset, text_y + (ping_size.y * 0.2f)), ImVec2(x_offset, text_y + (ping_size.y * 0.8f)), IM_COL32(220, 220, 220, 200 * g_window_alpha_w), 1.0f);
            x_offset += 5;

            draw_list->AddText(font, 16.f, { x_offset, text_y }, IM_COL32(220, 220, 220, 255 * g_window_alpha_w), time_str.c_str());
            x_offset += time_size.x + 4;

            draw_list->AddLine(ImVec2(x_offset, text_y + (time_size.y * 0.2f)), ImVec2(x_offset, text_y + (time_size.y * 0.8f)), IM_COL32(220, 220, 220, 200 * g_window_alpha_w), 1.0f);
            x_offset += 5;

            draw_list->AddText(font, 16.f, { x_offset, text_y }, IM_COL32(220, 220, 220, 255 * g_window_alpha_w), speed_text.c_str());

            if (x_offset + speed_size.x > main_pos.x + main_width - 10) {
                main_width = x_offset + speed_size.x - main_pos.x + 10;
            }
        }
    }
}

void COverlay::KeybindList() {
    if (!Config::vb(g_Variables.m_Gui.m_vbOverlay).at(OVERLAY_KEYBIND))
        return;

    const bool is_menu_open = Gui::m_bOpen;
    const auto now = std::chrono::steady_clock::now();
    const float delta_time = std::min(std::chrono::duration<float>(now - g_last_update).count(), 0.1f);
    g_last_update = now;

    auto get_bind_type = [](int mode) -> const char* {
        switch (mode) {
        case HOLD: return "[hold]";
        case TOGGLE: return "[toggle]";
        case ALWAYS_ON: return "[always]";
        default: return "[none]";
        }
        };

    struct KeybindInfo {
        const char* name;
        bool* enabled;
        KeyBind_t* bind;
        float* value;
    };

    static bool edgejump = true;

    std::vector<KeybindInfo> keybinds = {
        {"Damage override", &Config::b(g_Variables.m_Ragebot.m_bRagebotEnabled), &Config::kb(g_Variables.m_Ragebot.m_iMinDamageOverrideKey), nullptr},
        {"Hitchance override", &Config::b(g_Variables.m_Ragebot.m_bRagebotEnabled), &Config::kb(g_Variables.m_Ragebot.m_iHitchanceOverrideKey), nullptr},
        {"Force bodyaim", &Config::b(g_Variables.m_Ragebot.m_bRagebotEnabled), &Config::kb(g_Variables.m_Ragebot.m_iForceBodyaimKey), nullptr},
        {"Force headshot", &Config::b(g_Variables.m_Ragebot.m_bRagebotEnabled), &Config::kb(g_Variables.m_Ragebot.m_iForceHeadshotKey), nullptr},
        {"Manual right", &Config::b(g_Variables.m_Ragebot.m_bAntiaim), &Config::kb(g_Variables.m_Ragebot.m_iRightAntiaimKeybind), nullptr},
        {"Manual left", &Config::b(g_Variables.m_Ragebot.m_bAntiaim), &Config::kb(g_Variables.m_Ragebot.m_iLeftAntiaimKeybind), nullptr},
        {"Auto retreat", &Config::b(g_Variables.m_Ragebot.m_bAutoPeek), &Config::kb(g_Variables.m_Ragebot.m_kbAutoPeek), nullptr},
        {"Auto direction", &Config::b(g_Variables.m_Ragebot.m_bAntiaim), &Config::kb(g_Variables.m_Ragebot.m_bFreestanding), nullptr},
        {"Fake pitch", &Config::b(g_Variables.m_Ragebot.m_bAntiaim), &Config::kb(g_Variables.m_Ragebot.m_iFakePitch), nullptr},
        {"Mouse override", &Config::b(g_Variables.m_Ragebot.m_bAntiaim), &Config::kb(g_Variables.m_Ragebot.m_bMouseOverride), nullptr},
        {"Edge jump", &edgejump, &Config::kb(g_Variables.m_Movement.m_kEdgeJump), nullptr},
    };

    std::vector<KeybindInfo> active_keybinds;
    for (const auto& kb : keybinds) {
        if (*kb.enabled && Input::HandleInput(*kb.bind)) {
            active_keybinds.push_back(kb);
        }
    }

    bool should_show_window = is_menu_open || !active_keybinds.empty();
    float target_alpha = should_show_window ? 1.f : 0.f;
    g_window_alpha = ImLerp(g_window_alpha, target_alpha, delta_time * 12.f);

    if (g_window_alpha <= 0.01f && !should_show_window)
        return;

    const float item_height = 22;
    float target_height = 36 + (active_keybinds.empty() ? 0 : active_keybinds.size() * item_height);

    g_window_height = ImLerp(g_window_height, target_height, delta_time * 10.f);

    float max_text_width = 220;
    ImFont* font = Fonts::overlay3;

    for (const auto& kb : active_keybinds) {
        float total_width = font->CalcTextSizeA(16, FLT_MAX, 0.0f, (std::string(kb.name) + (kb.value ? std::to_string((int)*kb.value) : get_bind_type(kb.bind->m_iMode))).c_str()).x + 24;
        if (total_width > max_text_width)
            max_text_width = total_width;
    }

    g_window_width = ImLerp(g_window_width, max_text_width, delta_time * 10.f);

    if (!g_window_pos_initialized) {
        g_window_pos = ImVec2(50, 50);
        g_window_pos_initialized = true;
    }

    ImGui::SetNextWindowBgAlpha(160 * g_window_alpha / 255.f);
    ImGui::SetNextWindowSize(ImVec2(g_window_width, g_window_height));
    ImGui::SetNextWindowPos(g_window_pos, ImGuiCond_Once);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize;
    if (!is_menu_open) flags |= ImGuiWindowFlags_NoInputs;

    ImGui::Begin("Keybinds", nullptr, flags);

    if (g_window_alpha > 0.01f) {
        g_window_pos = ImGui::GetWindowPos();
        auto draw_list = ImGui::GetWindowDrawList();
        ImVec2 window_pos = ImGui::GetWindowPos();

        draw_list->AddRectFilled(
            window_pos,
            ImVec2(window_pos.x + g_window_width, window_pos.y + g_window_height),
            IM_COL32(0, 0, 0, static_cast<int>(160 * g_window_alpha)),
            6
        );

        draw_list->AddRectFilledMultiColor(
            window_pos,
            ImVec2(window_pos.x + g_window_width, window_pos.y + 3),
            IM_COL32(143, 154, 255, static_cast<int>(255 * g_window_alpha)),
            IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, 0),
            IM_COL32(143, 154, 255, static_cast<int>(255 * g_window_alpha))
        );

        ImVec2 text_size = font->CalcTextSizeA(16, FLT_MAX, 0.0f, "Binds");
        draw_list->AddText(
            font, 16,
            ImVec2(window_pos.x + (g_window_width - text_size.x) * 0.5f, window_pos.y + 10),
            IM_COL32(255, 255, 255, static_cast<int>(255 * g_window_alpha)),
            "Binds"
        );

        ImVec2 binds_pos(window_pos.x + 6, window_pos.y + 34);
        for (const auto& kb : active_keybinds) {
            draw_list->AddText(
                font, 16,
                ImVec2(binds_pos.x, binds_pos.y),
                IM_COL32(255, 255, 255, static_cast<int>(255 * g_window_alpha)),
                kb.name
            );

            const char* type_str = kb.value ? std::to_string((int)*kb.value).c_str() : get_bind_type(kb.bind->m_iMode);
            ImVec2 type_size = font->CalcTextSizeA(16, FLT_MAX, 0.0f, type_str);
            draw_list->AddText(
                font, 16,
                ImVec2(window_pos.x + g_window_width - type_size.x - 6, binds_pos.y),
                IM_COL32(200, 200, 200, static_cast<int>(160 * g_window_alpha)),
                type_str
            );

            binds_pos.y += item_height;
        }
    }

    ImGui::End();
}

void COverlay::SpectatorsList() {
    if (!Config::vb(g_Variables.m_Gui.m_vbOverlay).at(OVERLAY_SPECTATORS))
        return;

    const bool is_menu_open = Gui::m_bOpen;
    const auto now = std::chrono::steady_clock::now();
    const float delta_time = std::min(std::chrono::duration<float>(now - g_spec_last_update).count(), 0.1f);
    g_spec_last_update = now;

    std::vector<const char*> spectators;
    bool in_game = Interfaces::m_pEngine->IsInGame() && Interfaces::m_pEngine->IsConnected() && Globals::m_pLocalPlayerPawn;

    if (in_game) {
        for (EntityObject_t& object : g_Entities->m_vecEntities) {
            if (object.m_eType != EEntityType::ENTITY_PLAYER)
                continue;

            CCSPlayerController* pPlayerController = reinterpret_cast<CCSPlayerController*>(object.m_pEntity);
            if (!pPlayerController || pPlayerController->m_bPawnIsAlive())
                continue;

            C_CSObserverPawn* pPawn = pPlayerController->m_hObserverPawn().Get();
            if (!pPawn)
                continue;

            CPlayer_ObserverServices* pObserverServices = pPawn->m_pObserverServices();
            if (!pObserverServices)
                continue;

            C_BaseEntity* pObserverTarget = pObserverServices->m_hObserverTarget().Get();
            if (!pObserverTarget)
                continue;

            if (pObserverTarget->GetRefEHandle().GetEntryIndex() != Globals::m_pLocalPlayerPawn->GetRefEHandle().GetEntryIndex())
                continue;

            if (pPlayerController->m_sSanitizedPlayerName()) {
                spectators.push_back(pPlayerController->m_sSanitizedPlayerName());
            }
        }
    }

    bool should_show_window = is_menu_open || (!spectators.empty() && in_game);
    float target_alpha = should_show_window ? 1.f : 0.f;
    g_spec_alpha = ImLerp(g_spec_alpha, target_alpha, delta_time * 12.f);

    if (g_spec_alpha <= 0.01f && !should_show_window)
        return;

    const float item_height = 22;
    float target_height = 36 + (spectators.empty() ? 0 : spectators.size() * item_height);

    g_spec_height = ImLerp(g_spec_height, target_height, delta_time * 10.f);

    float max_text_width = 220;
    ImFont* font = Fonts::overlay3;

    if (in_game) {
        for (const auto& name : spectators) {
            float total_width = font->CalcTextSizeA(16, FLT_MAX, 0.0f, name).x + 24;
            if (total_width > max_text_width)
                max_text_width = total_width;
        }
    }

    g_spec_width = ImLerp(g_spec_width, max_text_width, delta_time * 10.f);

    if (!g_spec_pos_initialized) {
        g_spec_pos = ImVec2(310, 50);
        g_spec_pos_initialized = true;
    }

    ImGui::SetNextWindowBgAlpha(160 * g_spec_alpha / 255.f);
    ImGui::SetNextWindowSize(ImVec2(g_spec_width, g_spec_height));
    ImGui::SetNextWindowPos(g_spec_pos, ImGuiCond_Once);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize;
    if (!is_menu_open) flags |= ImGuiWindowFlags_NoInputs;

    ImGui::Begin("Spectators", nullptr, flags);

    if (g_spec_alpha > 0.01f) {
        g_spec_pos = ImGui::GetWindowPos();
        auto draw_list = ImGui::GetWindowDrawList();
        ImVec2 window_pos = ImGui::GetWindowPos();

        draw_list->AddRectFilled(
            window_pos,
            ImVec2(window_pos.x + g_spec_width, window_pos.y + g_spec_height),
            IM_COL32(0, 0, 0, static_cast<int>(160 * g_spec_alpha)),
            6
        );

        draw_list->AddRectFilledMultiColor(
            window_pos,
            ImVec2(window_pos.x + g_spec_width, window_pos.y + 3),
            IM_COL32(143, 154, 255, static_cast<int>(255 * g_spec_alpha)),
            IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, 0),
            IM_COL32(143, 154, 255, static_cast<int>(255 * g_spec_alpha))
        );

        ImVec2 text_size = font->CalcTextSizeA(16, FLT_MAX, 0.0f, "Spectators");
        draw_list->AddText(
            font, 16,
            ImVec2(window_pos.x + (g_spec_width - text_size.x) * 0.5f, window_pos.y + 10),
            IM_COL32(255, 255, 255, static_cast<int>(255 * g_spec_alpha)),
            "Spectators"
        );

        if (in_game && !spectators.empty()) {
            ImVec2 names_pos(window_pos.x + 6, window_pos.y + 34);
            for (const auto& name : spectators) {
                draw_list->AddText(
                    font, 16,
                    ImVec2(names_pos.x, names_pos.y),
                    IM_COL32(255, 255, 255, static_cast<int>(255 * g_spec_alpha)),
                    name
                );

                names_pos.y += item_height;
            }
        }
    }

    ImGui::End();
}