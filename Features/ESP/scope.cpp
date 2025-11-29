#pragma once
#include "../../Precompiled.h"

void Scope::DrawCrosshair() {
    static float flScopeAnimationProgress;
    static bool bIsCurrentlyScoped;
    static int iTargetScopeLength;
    static float flAnimationSpeed = 0.09f;
    static int iLastScopeLength = Config::i(g_Variables.m_Visuals.m_iScopeLength);
    static bool bForceUpdate = false;

    if (!Globals::m_pLocalPlayerController->m_bPawnIsAlive())
        return;

    bool bIsScoped = Globals::m_pLocalPlayerPawn->m_bIsScoped();
    int iLineLength = Config::i(g_Variables.m_Visuals.m_iScopeLength);

    if (iLineLength != iLastScopeLength) {
        iLastScopeLength = iLineLength;
        bForceUpdate = true;
    }
    if (bIsScoped && !bIsCurrentlyScoped) {
        iTargetScopeLength = iLineLength;
        bForceUpdate = false;
    }
    else if (!bIsScoped && bIsCurrentlyScoped) {
        iTargetScopeLength = 0;
        bForceUpdate = false;
    }
    else if (bForceUpdate && bIsScoped) {
        iTargetScopeLength = iLineLength;
        bForceUpdate = false;
    }

    if (bForceUpdate && bIsScoped)
        flScopeAnimationProgress = static_cast<float>(iLineLength);
    else {
        if (static_cast<int>(flScopeAnimationProgress) != iTargetScopeLength) {
            float flDiff = static_cast<float>(iTargetScopeLength) - flScopeAnimationProgress;
            flScopeAnimationProgress += flDiff * flAnimationSpeed;

            if (fabsf(flDiff) < 1.0f || (flDiff > 0 && flScopeAnimationProgress > iTargetScopeLength) || (flDiff < 0 && flScopeAnimationProgress < iTargetScopeLength))
                flScopeAnimationProgress = static_cast<float>(iTargetScopeLength);
        }
    }

    bIsCurrentlyScoped = bIsScoped;

    int iCurrentLineLength = static_cast<int>(flScopeAnimationProgress);
    if (iCurrentLineLength <= 0 && !bIsScoped)
        return;

    int iGap = Config::i(g_Variables.m_Visuals.m_iScopeGap);
    ImDrawList* pDrawList = ImGui::GetBackgroundDrawList();
    ImVec2 vecScreenSize = ImGui::GetIO().DisplaySize;
    ImVec2 vecScreenCenter = ImVec2(vecScreenSize.x * 0.5f, vecScreenSize.y * 0.5f);

    Color colInside = Config::c(g_Variables.m_Visuals.m_colScope);
    Color colOutside = Config::c(g_Variables.m_Visuals.m_colScopeOutSide);

    int iRawScopeThickness = Config::i(g_Variables.m_Visuals.m_iScopeThickness);
    float fScaledThickness = 1.0f + (static_cast<float>(iRawScopeThickness - 1) / 25.0f);

    pDrawList->AddRectFilledMultiColor(ImVec2(vecScreenCenter.x - fScaledThickness * 0.5f, vecScreenCenter.y - iGap * 0.5f - iCurrentLineLength), ImVec2(vecScreenCenter.x + fScaledThickness * 0.5f, vecScreenCenter.y - iGap * 0.5f), colOutside.GetU32(), colOutside.GetU32(), colInside.GetU32(), colInside.GetU32());

    pDrawList->AddRectFilledMultiColor(ImVec2(vecScreenCenter.x - fScaledThickness * 0.5f, vecScreenCenter.y + iGap * 0.5f), ImVec2(vecScreenCenter.x + fScaledThickness * 0.5f, vecScreenCenter.y + iGap * 0.5f + iCurrentLineLength), colInside.GetU32(), colInside.GetU32(), colOutside.GetU32(), colOutside.GetU32());

    pDrawList->AddRectFilledMultiColor(ImVec2(vecScreenCenter.x - iGap * 0.5f - iCurrentLineLength, vecScreenCenter.y - fScaledThickness * 0.5f), ImVec2(vecScreenCenter.x - iGap * 0.5f, vecScreenCenter.y + fScaledThickness * 0.5f), colOutside.GetU32(), colInside.GetU32(), colInside.GetU32(), colOutside.GetU32());

    pDrawList->AddRectFilledMultiColor(ImVec2(vecScreenCenter.x + iGap * 0.5f, vecScreenCenter.y - fScaledThickness * 0.5f), ImVec2(vecScreenCenter.x + iGap * 0.5f + iCurrentLineLength, vecScreenCenter.y + fScaledThickness * 0.5f), colInside.GetU32(), colOutside.GetU32(), colOutside.GetU32(), colInside.GetU32());
}

void Scope::DrawCrosshairOverlay() {
    if (!Globals::m_pLocalPlayerController->m_bPawnIsAlive())
        return;

    if (!Globals::m_pLocalPlayerPawn->m_bIsScoped())
        return;

    ImDrawList* pDrawList = ImGui::GetBackgroundDrawList();
    const auto display{ ImGui::GetIO().DisplaySize };

    pDrawList->AddLine(ImVec2(0, display.y / 2), ImVec2(display.x, display.y / 2), Color(0, 0, 0).GetU32(), 1.0f);
    pDrawList->AddLine(ImVec2(display.x / 2, 0), ImVec2(display.x / 2, display.y), Color(0, 0, 0).GetU32(), 1.0f);
}

__int64(__fastcall* o52BEE0)(C_CSPlayerPawn* pawn, __int64 unk);
__int64 __fastcall hk52BEE0(C_CSPlayerPawn* pawn, __int64 unk) {
    if (!pawn)
        return o52BEE0(pawn, unk);

    if (pawn->GetRefEHandle().GetEntryIndex() != Globals::m_pLocalPlayerPawn->GetRefEHandle().GetEntryIndex())
        return o52BEE0(pawn, unk);

    if (Config::i(g_Variables.m_Visuals.m_iRemoveScope) == SCOPE_ALL && pawn->m_bIsScoped() && Interfaces::m_pEngine->IsInGame() && Interfaces::m_pEngine->IsConnected())
        return NULL;

    return o52BEE0(pawn, unk);
}

void Scope::Hook()
{
    void* func = Memory::GetAbsoluteAddress(Memory::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 80 7C 24 34 ? 74 ?"), 0x1, 0x0);
    if (MH_CreateHook(func, hk52BEE0, (LPVOID*)&o52BEE0) != MH_OK)
        return;

    if (MH_EnableHook(func) != MH_OK)
        return;
}

void Scope::Run()
{
    if (!Globals::m_pLocalPlayerPawn)
        return;

    if (!Interfaces::m_pEngine->IsInGame() && !Interfaces::m_pEngine->IsConnected())
        return;

    if (Config::i(g_Variables.m_Visuals.m_iRemoveScope) == SCOPE_DEFAULT)
        return;

    else if (Config::i(g_Variables.m_Visuals.m_iRemoveScope) == SCOPE_OVERLAY)
        Scope::DrawCrosshairOverlay();

    else if (Config::i(g_Variables.m_Visuals.m_iRemoveScope) == SCOPE_GRADIENT)
        Scope::DrawCrosshair();

    else if (Config::i(g_Variables.m_Visuals.m_iRemoveScope) == SCOPE_ALL)
        return;
}