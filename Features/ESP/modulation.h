#pragma once

struct Skybox_t {
    std::string strSkyboxPath;
    std::string strSkyboxName;
    std::string strRawSkyboxName;

    CStrongHandle<CMaterial2> pSkyboxMaterial;

    bool bPreCached = false;
    bool bInitialized = false;

    void InitSkybox();
    void DeleteSkybox() const;
};

class CModulation
{
private:
    static constexpr const char* arrSkyboxes[] = {
    "materials/skybox/cs_italy_s2_skybox_sunset_2_exr_e56cedf6.vtex",
    "materials/skybox/sky_overcast_01_exr_da4019b1.vtex",
    "materials/skybox/tests/src/lightingtest_sky_night_exr_2c5e8c62.vtex",
    "materials/skybox/sky_hr_aztec_02_exr_f84f8de9.vtex",
    "materials/skybox/jungle_cube_pfm_bc16d813.vtex",
    "materials/skybox/sky_csgo_cloudy01_cube_pfm_f9a0b177.vtex",
    "materials/skybox/sky_de_overpass_01_exr_f8534391.vtex",
    "materials/skybox/sky_de_nuke_exr_f04e84b2.vtex",
    "materials/skybox/sky_cs_office_45_0_exr_d0152542.vtex",
    "materials/skybox/sky_de_annubis_exr_2c5e0b53.vtex",
    "materials/skybox/sky_de_train03_exr_4fdb8a38.vtex",
    "materials/skybox/sky_de_vertigo_exr_c70a3937.vtex"
    };

public:
    std::vector<Skybox_t> vecSkyboxes;
    unsigned long long nSelectedSkybox = ~1U;

    void LoadDefaultSkyboxes();
    void UpdateSkyboxes();
    void PrecacheSkyboxes();

    void Exposure(C_CSPlayerPawn* pPawn);
    void Exposure(C_PostProcessingVolume* pVolume);
};

inline std::unique_ptr<CModulation> g_WorldModulation = std::make_unique<CModulation>();

