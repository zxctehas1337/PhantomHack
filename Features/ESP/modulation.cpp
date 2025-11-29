 #include "../../Precompiled.h"

#include "StringForSkyboxes.h"

void Skybox_t::InitSkybox() {
    if (bInitialized)
        return;

    if (strSkyboxPath.empty())
        return;

    std::string strSkyMaterial =
        R"#(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d}
        format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
        {
            shader = "sky.vfx"
            g_flBrightnessExposureBias = 0
            g_flRenderOnlyExposureBias = 0
            g_tSkyTexture = resource:")#" + strSkyboxPath + R"#("
        })#";

    if (g_Chams) {
        pSkyboxMaterial = g_Chams->CreateMaterial(X("materials/dev/primary_white.vmat"), strSkyMaterial.c_str());
        bInitialized = pSkyboxMaterial && pSkyboxMaterial.m_pBinding;
    }
}

void Skybox_t::DeleteSkybox() const {
    if (!bInitialized || !pSkyboxMaterial || !pSkyboxMaterial.m_pBinding)
        return;

    if (Interfaces::m_pResourceHandleUtils) {
        Interfaces::m_pResourceHandleUtils->DeleteResource(pSkyboxMaterial.m_pBinding);
    }
}

void CModulation::LoadDefaultSkyboxes() {
    for (auto& skybox : vecSkyboxes) {
        skybox.DeleteSkybox();
    }
    vecSkyboxes.clear();

    for (const std::string& strSkyboxPath : arrSkyboxes) {
        if (strSkyboxPath.empty())
            continue;

        size_t cPrefixPos = strSkyboxPath.find("materials/skybox/");
        if (cPrefixPos == std::string::npos)
            continue;

        std::string strSkyboxName = strSkyboxPath.substr(cPrefixPos + 17);

        size_t cSkyPos = strSkyboxPath.find("sky_");
        if (cSkyPos != std::string::npos)
            strSkyboxName = strSkyboxPath.substr(cSkyPos + 4);

        size_t cFirstUnderscore = strSkyboxName.find('_');
        size_t cSecondUnderscore = strSkyboxName.find('_', cFirstUnderscore + 1);
        if (cSecondUnderscore != std::string::npos)
            strSkyboxName = strSkyboxName.substr(0, cSecondUnderscore);
        else if (cFirstUnderscore != std::string::npos)
            strSkyboxName = strSkyboxName.substr(0, cFirstUnderscore);

        if (strSkyboxName != "de_dust2")
            strSkyboxName.erase(std::remove_if(strSkyboxName.begin(), strSkyboxName.end(), ::isdigit), strSkyboxName.end());

        if (strSkyboxName.length() > 32)
            strSkyboxName = strSkyboxName.substr(0, 29) + "...";

        if (strSkyboxName.empty())
            strSkyboxName = "unnamed_" + std::to_string(vecSkyboxes.size());

        Skybox_t pSkybox;
        pSkybox.strSkyboxPath = strSkyboxPath;
        pSkybox.strSkyboxName = strSkyboxName;
        pSkybox.strRawSkyboxName = strSkyboxPath.substr(cPrefixPos + 17);
        pSkybox.InitSkybox();

        if (pSkybox.bInitialized)
            vecSkyboxes.push_back(pSkybox);
    }

    if (nSelectedSkybox >= static_cast<unsigned long long>(vecSkyboxes.size()))
        nSelectedSkybox = 0;
}

void CModulation::UpdateSkyboxes() {
    std::string strSkyboxesDirectory = STR::GetDirectory(SkyboxModels);
    if (strSkyboxesDirectory.empty())
        return;

    for (size_t i = 0; i < vecSkyboxes.size(); ) {
        const Skybox_t& pSkybox = vecSkyboxes[i];
        std::string strFilePath = strSkyboxesDirectory + pSkybox.strRawSkyboxName;

        if (!std::filesystem::exists(strFilePath)) {
            pSkybox.DeleteSkybox();
            vecSkyboxes.erase(vecSkyboxes.begin() + i);
        }
        else {
            ++i;
        }
    }

    for (const auto& entry : std::filesystem::directory_iterator(strSkyboxesDirectory)) {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension().string() != ".vtex_c")
            continue;

        std::string strRawFileName = entry.path().filename().string();
        std::string strSkyboxPath = "materials/skybox/" + strRawFileName;

        size_t cExrPos = strRawFileName.find("_exr");
        size_t cCubePos = strRawFileName.find("_cube");
        size_t cCutPos = std::min(cExrPos, cCubePos);

        std::string strFileName;
        if (cCutPos != std::string::npos) {
            strFileName = strRawFileName.substr(0, cCutPos);
        }
        else {
            size_t dotPos = strRawFileName.find_last_of('.');
            strFileName = dotPos != std::string::npos ? strRawFileName.substr(0, dotPos) : strRawFileName;
        }

        if (strFileName.length() > 32)
            strFileName = strFileName.substr(0, 29) + "...";

        if (strFileName.empty())
            strFileName = "unnamed_" + std::to_string(vecSkyboxes.size());

        auto it = std::find_if(vecSkyboxes.begin(), vecSkyboxes.end(),
            [&](const Skybox_t& pSkybox) { return pSkybox.strSkyboxName == strFileName; });

        if (it == vecSkyboxes.end()) {
            Skybox_t pSkybox;
            pSkybox.strSkyboxPath = strSkyboxPath.substr(0, strSkyboxPath.size() - 2);
            pSkybox.strSkyboxName = strFileName;
            pSkybox.strRawSkyboxName = strRawFileName;
            pSkybox.InitSkybox();

            if (pSkybox.bInitialized)
                vecSkyboxes.push_back(pSkybox);
        }
    }

    if (nSelectedSkybox >= static_cast<unsigned long long>(vecSkyboxes.size()))
        nSelectedSkybox = 0;
}

void CModulation::PrecacheSkyboxes() {
    if (!Config::b(g_Variables.m_WorldEffects.m_bCustomSkybox))
        return;

    if (nSelectedSkybox == ~1ULL || nSelectedSkybox >= static_cast<unsigned long long>(vecSkyboxes.size()))
        return;

    Skybox_t& selectedSkybox = vecSkyboxes[nSelectedSkybox];
    if (!selectedSkybox.bInitialized || !selectedSkybox.pSkyboxMaterial || !selectedSkybox.pSkyboxMaterial.m_pBinding)
        return;

    if (!Interfaces::m_pResourceSystem)
        return;

    const char* szVtexPath = selectedSkybox.strSkyboxPath.c_str();
    if (!szVtexPath || selectedSkybox.strSkyboxPath.empty())
        return;

    selectedSkybox.bPreCached = true;
    CBufferString asfuhskybx = CBufferString(szVtexPath, 'xetv');
    Interfaces::m_pResourceSystem->LoadResource(asfuhskybx, "");
}

void CModulation::Exposure(C_CSPlayerPawn* pPawn) {
    if (!Interfaces::m_pEngine->IsInGame())
        return;

    static auto Update = reinterpret_cast<void(__fastcall*)(CPlayer_CameraServices*, int)>(Memory::FindPattern(CLIENT_DLL, X("48 89 5C 24 08 57 48 83 EC 20 8B FA 48 8B D9 E8 ?? ?? ?? ?? 84 C0 0F 84")));

    static float prev_exposure = -1;
    float exposure = static_cast<float>(Config::i(g_Variables.m_WorldVisuals.m_nWorldExposure));

    if (prev_exposure == exposure)
        return;

    if (!pPawn || !pPawn->m_pCameraServices())
        return;

    CPlayer_CameraServices* pCamera = pPawn->m_pCameraServices();
    if (!pCamera)
        return;

    C_PostProcessingVolume* pVolume = reinterpret_cast<C_PostProcessingVolume*>(Interfaces::m_pGameResourceService->pGameEntitySystem->Get(pCamera->m_hActivePostProcessingVolume().GetEntryIndex()));
    if (!pVolume)
        return;

    prev_exposure = exposure;

    this->Exposure(pVolume);
    Update(pCamera, 0);
}

void CModulation::Exposure(C_PostProcessingVolume* pVolume) {
    float exposure = Config::i(g_Variables.m_WorldVisuals.m_nWorldExposure) * 0.01f;

    pVolume->m_bExposureControl() = true;
    pVolume->m_flExposureFadeSpeedDown() = pVolume->m_flExposureFadeSpeedUp() = 0;
    pVolume->m_flMinExposure() = pVolume->m_flMaxExposure() = exposure;
}