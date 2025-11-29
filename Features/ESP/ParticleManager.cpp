#include "../../Precompiled.h"

constexpr const char* EMBER_EFFECT_PATH = "bin/falling_ember1.vpcf";
constexpr const char* EMBER_EFFECT2_PATH = "bin/falling_ember2.vpcf";
constexpr const char* SNOW_EFFECT_PATH = "bin/falling_snow1.vpcf";
constexpr const char* STARS_EFFECT_PATH = "bin/nomove_stars.vpcf";


struct TracerStartInfo
{
    Vector vEyePos;
    float flTime;
};
std::unordered_map<CCSPlayerController*, TracerStartInfo> m_mapTracerStartCache;


void CParticleMgr::OnEvent(CGameEvent* pEvent)
{
    CCSPlayerController* pController = reinterpret_cast<CCSPlayerController*>(pEvent->GetPlayerController(X("userid")));
    C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pEvent->GetPlayerPawn(X("userid")));
    if (!pController || !pPawn)
        return;

    Vector vecImpactPosition = Vector(pEvent->GetFloat(X("x")), pEvent->GetFloat(X("y")), pEvent->GetFloat(X("z")));
    if (vecImpactPosition.IsZero())
        return;

    float currentTime = Interfaces::m_pGlobalVariables->m_flCurrentTime;

    if (pController->m_bIsLocalPlayerController())
    {
        if (Config::b(g_Variables.m_WorldEffects.m_bDrawImpacts))
        {
            Interfaces::m_pClient->GetDebugOverlay()->RenderWithoutDots(true);
            Interfaces::m_pClient->GetDebugOverlay()->AddBoxOverlay(
                vecImpactPosition,
                Vector(-1.f, -1.f, -1.f),
                Vector(1.f, 1.f, 1.f),
                QAngle(0.0f, 0.0f, 0.0f),
                Config::c(g_Variables.m_WorldEffects.m_colDrawImpactsColor),
                Config::i(g_Variables.m_Visuals.m_nHitEffectsDurration)
            );
        }

        if (Config::b(g_Variables.m_WorldEffects.m_bLocalBulletTracers))
        {
            TracerStartInfo& info = m_mapTracerStartCache[pController];

            /* if (currentTime - info.flTime > LocalPlayerData::m_pWeaponBaseVData->m_flCycleTime()[LocalPlayerData::m_pWeapon->m_weaponMode()] )
             {*/
            info.vEyePos = Globals::m_pLocalPlayerPawn->GetEyePosition(true);
            info.flTime = currentTime;
            // }

            this->AddParticle2Point(
                X("particles/entity/spectator_utility_trail.vpcf"),
                info.vEyePos,
                vecImpactPosition,
                Config::c(g_Variables.m_WorldEffects.m_colLocalBulletTracers),
                Config::i(g_Variables.m_Visuals.m_nHitEffectsDurration),
                false
            );
        }
    }

    else if (pPawn->IsEnemy(Globals::m_pLocalPlayerPawn))
    {
        if (Config::b(g_Variables.m_WorldEffects.m_bEnemyBulletTracers))
        {
            TracerStartInfo& info = m_mapTracerStartCache[pController];

            if (currentTime - info.flTime > 0.1f)
            {
                info.vEyePos = pPawn->GetEyePosition();
                info.flTime = currentTime;
            }

            this->AddParticle2Point(
                X("particles/entity/spectator_utility_trail.vpcf"),
                info.vEyePos,
                vecImpactPosition,
                Config::c(g_Variables.m_WorldEffects.m_colEnemyBulletTracers),
                Config::i(g_Variables.m_Visuals.m_nHitEffectsDurration),
                false
            );
        }
    }
}

void CParticleMgr::AddParticlePoints(const char* szParticlePath, std::vector<Vector> vecPoints, Color colColor, float flTime, bool bCustom)
{
    CBeamInfo objParticleInfo = CBeamInfo();

    if (bCustom)
    {
        CBufferString objParticleName(szParticlePath, 'fcpv');
        Interfaces::m_pResourceSystem->BlockingLoadResourceByName(&objParticleName, "");
    }

    Interfaces::m_pParticleManager->CreateParticleEffect(&objParticleInfo.m_nEffectIndex, szParticlePath);
    // Create the particle effect.
    Interfaces::m_pParticleManager->SetParticleSetting(objParticleInfo.m_nEffectIndex, EParticleSetting::PARTICLE_SETTING_COLOR, &colColor);

    for (int i = 0; i < vecPoints.size(); i++)
    {
        // Create new particle info
        CParticleInformation particle_info{};
        particle_info.m_flTime = flTime;
        particle_info.m_flWidth = 2.f;
        particle_info.m_flAlpha = 1.f;

        // create particle effect for this stage/position
        Interfaces::m_pParticleManager->SetParticleSetting(objParticleInfo.m_nEffectIndex, EParticleSetting::PARTICLE_SETTING_INFO, &particle_info);

        objParticleInfo.m_vecPositions = new Vector[i + 1];
        objParticleInfo.m_flTimes = new float[i + 1];

        for (int j{}; j < i + 1; j++) {
            objParticleInfo.m_vecPositions[j] = vecPoints[j];
            objParticleInfo.m_flTimes[j] = INTERVAL_PER_TICK * float(j);
        }
        objParticleInfo.m_ParticleData.m_vecPosition = objParticleInfo.m_vecPositions;
        objParticleInfo.m_ParticleData.m_flTimes2 = objParticleInfo.m_flTimes;

        Interfaces::m_pParticleSystemMgr->CreateSnapshot(&objParticleInfo.m_hSnapsotParticle);

        Interfaces::m_pParticleManager->UnknownFunction(objParticleInfo.m_nEffectIndex, 0, &objParticleInfo.m_hSnapsotParticle);

        objParticleInfo.m_hSnapsotParticle->Draw(i + 1, &objParticleInfo.m_ParticleData);

        delete[] objParticleInfo.m_vecPositions;
        delete[] objParticleInfo.m_flTimes;
    }

    m_deqPointParticlesAdded.push_back({ objParticleInfo.m_nEffectIndex , Interfaces::m_pGlobalVariables->m_flCurrentTime, flTime });
}

void CParticleMgr::AddParticle2Point(const char* szParticlePath, Vector vStart, Vector vEnd, Color colColor, float flTime, bool bCustom)
{
    CBeamInfo objParticleInfo = CBeamInfo();

    if (bCustom)
    {
        CBufferString objParticleName(szParticlePath, 'fcpv');
        Interfaces::m_pResourceSystem->BlockingLoadResourceByName(&objParticleName, "");
    }

    Interfaces::m_pParticleManager->CreateParticleEffect(&objParticleInfo.m_nEffectIndex, szParticlePath);

    CParticleColor tColor = CParticleColor(static_cast<float>(colColor.Get<COLOR_R>()), static_cast<float>(colColor.Get<COLOR_G>()), static_cast<float>(colColor.Get<COLOR_B>()));
    Interfaces::m_pParticleManager->SetParticleSetting(objParticleInfo.m_nEffectIndex, EParticleSetting::PARTICLE_SETTING_COLOR, &tColor);

    objParticleInfo.m_ParticleData = CParticleData();
    Vector vDirection = (vEnd - vStart);
    Vector vMiddle = vStart + (vDirection * 0.5f);

    CParticleInformation  particleInfo = CParticleInformation();
    particleInfo.m_flWidth = 0.5f;
    particleInfo.m_flTime = flTime;
    particleInfo.m_flAlpha = colColor.a() / 255.f;
    Interfaces::m_pParticleManager->SetParticleSetting(objParticleInfo.m_nEffectIndex, EParticleSetting::PARTICLE_SETTING_INFO, &particleInfo);

    const int totalPoints = 5;
    std::vector<Vector> vecPositions;
    vecPositions.reserve(totalPoints);

    vecPositions.push_back(vStart);
    vecPositions.push_back(vStart.Lerp(vMiddle, 0.05f));

    vecPositions.push_back(vMiddle);
    vecPositions.push_back(vMiddle.Lerp(vEnd, 0.95f));
    vecPositions.push_back(vEnd);

    objParticleInfo.m_ParticleData.m_vecPosition = vecPositions.data();

    Interfaces::m_pParticleSystemMgr->CreateSnapshot(&objParticleInfo.m_hSnapsotParticle);

    Interfaces::m_pParticleManager->UnknownFunction(objParticleInfo.m_nEffectIndex, 0, &objParticleInfo.m_hSnapsotParticle);

    objParticleInfo.m_hSnapsotParticle->Draw(vecPositions.size(), &objParticleInfo.m_ParticleData);

    m_deqPointParticlesAdded.push_back({ objParticleInfo.m_nEffectIndex , Interfaces::m_pGlobalVariables->m_flCurrentTime, flTime });
}

void CParticleMgr::AddParticle1Point(const char* szParticlePath, Vector vPosition, Color colColor, float flTime, bool bCustom)
{
    CBeamInfo objParticleInfo = CBeamInfo();

    if (bCustom)
    {
        CBufferString objParticleName(szParticlePath, 'fcpv');
        Interfaces::m_pResourceSystem->BlockingLoadResourceByName(&objParticleName, "");
    }

    Interfaces::m_pParticleManager->CreateParticleEffect(&objParticleInfo.m_nEffectIndex, szParticlePath);

    CParticleColor tColor = CParticleColor(static_cast<float>(colColor.Get<COLOR_R>()), static_cast<float>(colColor.Get<COLOR_G>()), static_cast<float>(colColor.Get<COLOR_B>()));
    Interfaces::m_pParticleManager->SetParticleSetting(objParticleInfo.m_nEffectIndex, EParticleSetting::PARTICLE_SETTING_COLOR, &tColor);

    Interfaces::m_pParticleManager->SetParticleSetting(objParticleInfo.m_nEffectIndex, EParticleSetting::PARTICLE_SETTING_POSITION, &vPosition);

    m_deqPointParticlesAdded.push_back({ objParticleInfo.m_nEffectIndex, Interfaces::m_pGlobalVariables->m_flCurrentTime, flTime });
}

void CParticleMgr::CreateParticle(unsigned int& nEffectIndex, const char* szParticlePath)
{
    if (!szParticlePath || !Interfaces::m_pResourceSystem || !Interfaces::m_pParticleManager)
        return;

    CBufferString emberName(szParticlePath, 'fcpv');
    Interfaces::m_pResourceSystem->BlockingLoadResourceByName(&emberName, "");

    unsigned int uEffectIndex = 0;
    Interfaces::m_pParticleManager->CreateParticleEffect(&uEffectIndex, szParticlePath);

    if (uEffectIndex != 0)
    {
        m_vecEffectIndexes.push_back(uEffectIndex);
        nEffectIndex = uEffectIndex;
    }
}

void CParticleMgr::ReleaseParticles()
{
    if (!Interfaces::m_pParticleManager)
        return;

    for (unsigned int& uEffectIndex : m_vecEffectIndexes)
    {
        if (uEffectIndex != 0)
        {
            Interfaces::m_pParticleManager->DestroyParticle(uEffectIndex, true, true);
            Interfaces::m_pParticleManager->ReleaseParticleIndex(uEffectIndex);
        }
    }

    if (!m_vecEffectIndexes.empty())
        m_vecEffectIndexes.clear();
}

void CParticleMgr::UpdateParticles()
{
    if (!Globals::m_pLocalPlayerPawn || !Interfaces::m_pGameRules || !Interfaces::m_pParticleManager)
        return;

    // POINT PARTICLES
    if (!m_deqPointParticlesAdded.empty())
    {
        for (std::deque<PointParticle_t>::iterator it = m_deqPointParticlesAdded.begin(); it != m_deqPointParticlesAdded.end(); )
        {
            if (it->m_flParticleTimeBeforeDelete + it->m_flCurrentTimeGlobal < Interfaces::m_pGlobalVariables->m_flCurrentTime)
            {
                Interfaces::m_pParticleManager->DestroyParticle(it->m_uiEffectIndex, 1, 1);
                Interfaces::m_pParticleManager->ReleaseParticleIndex(it->m_uiEffectIndex);
                it = m_deqPointParticlesAdded.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // WORLD PARTICLES
    int iSelectedEffect = Config::i(g_Variables.m_WorldEffects.m_iSelectedCustomWorldParticles);

    if (iSelectedEffect == WORLD_PARTICLES_NONE)
    {
        this->ReleaseParticles();
        return;
    }

    float flCurrentRoundStartTime = Interfaces::m_pGameRules->m_fRoundStartTime();

    if (iSelectedEffect != m_iLastEffectCached || flCurrentRoundStartTime != m_flLastReloadTime)
    {
        this->ReleaseParticles();

        m_iLastEffectCached = iSelectedEffect;
        m_flLastReloadTime = flCurrentRoundStartTime;

        unsigned int uEffectIndex1 = 0;
        unsigned int uEffectIndex2 = 0;
        switch (iSelectedEffect) {
        case ECustomWorldParticles::WORLD_PARTICLES_ASHES:
            this->CreateParticle(uEffectIndex1, EMBER_EFFECT_PATH);
            this->CreateParticle(uEffectIndex2, EMBER_EFFECT2_PATH);
            break;
        case ECustomWorldParticles::WORLD_PARTICLES_SNOW:
            this->CreateParticle(uEffectIndex1, SNOW_EFFECT_PATH);
            break;
        case ECustomWorldParticles::WORLD_PARTICLES_STARS:
            this->CreateParticle(uEffectIndex1, STARS_EFFECT_PATH);
            break;
        }
    }

    Vector vDensity(Config::i(g_Variables.m_WorldEffects.m_iSelectedCustomWorldParticleDensity), 0, 0);
    for (const unsigned int& uEffectIndex : m_vecEffectIndexes)
    {
        if (uEffectIndex != 0)
        {
            Interfaces::m_pParticleManager->SetParticleSetting(
                uEffectIndex,
                EParticleSetting::PARTICLE_SETTING_POSITION,
                &Globals::m_pLocalPlayerPawn->m_pGameSceneNode()->m_vecAbsOrigin()
            );

            Interfaces::m_pParticleManager->SetParticleSetting(
                uEffectIndex,
                EParticleSetting::PARTICLE_SETTING_DENSITY,
                &vDensity
            );
        }
    }
}
