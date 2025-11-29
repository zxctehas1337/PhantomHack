#include "../../Precompiled.h"

uint32_t NoSpread::ComputeRandomSeed(QAngle* angViewAngles, std::uint32_t nPlayerTickCount) {
    using fn_t = uint32_t(__fastcall*)(void* a1, QAngle* angViewAngles, std::uint32_t nPlayerTickCount);
    static fn_t fn = (fn_t)(Memory::FindPattern(CLIENT_DLL, X("48 89 5C 24 08 57 48 81 EC F0 00")));
    return fn(nullptr, angViewAngles, nPlayerTickCount);
}

CalcSpreadOutput_t NoSpread::CalculateSpread(int nRandomSeed, float flInAccuracy, float flSpread)
{
    using func_t = void(__fastcall*)(int16_t, int, int, std::uint32_t, float, float, float, float*, float*);
    static func_t pFn = (func_t)(Memory::FindPattern(CLIENT_DLL, X("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 63 EA")));

    CalcSpreadOutput_t vecOut;
    pFn(Interfaces::m_pGameResourceService->pGameEntitySystem->Get<C_CSWeaponBaseGun>(Globals::m_pLocalPlayerPawn->m_pWeaponServices()->m_hActiveWeapon())->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex(), 1, 0, nRandomSeed, flInAccuracy, flSpread, 0.f, &vecOut.x, &vecOut.y);

    return vecOut;
}

Vector2D NoSpread::CalculateSpreadBasic(int nSeed, float flInaccuracy, float flSpread, float flRecoilIndex) {
    Vector2D spread;

    using func_t = void(__fastcall*)(int16_t, int, int, std::uint32_t, float, float, float, float*, float*);
    static func_t pFn = (func_t)(Memory::FindPattern(CLIENT_DLL, X("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 63 EA")));

    pFn(LocalPlayerData::m_nWeaponDefinitionIndex, LocalPlayerData::m_pWeaponBaseVData->m_nNumBullets(), LocalPlayerData::m_pWeapon->m_weaponMode(), nSeed, flInaccuracy, flSpread, flRecoilIndex, &spread.x, &spread.y);

    return spread;
}

NoSpreadResult NoSpread::NoSpread(QAngle& angle)
{
    NoSpreadResult NoSpreadResult{};
    NoSpreadResult.bFound = false;


    unsigned int nCurrentTick = Globals::m_pLocalPlayerController->m_nTickBase();
    float flInAccuracy = 0;
    float flSpread = 0;

    for (int i = 0; i < 720; i++)
    {
        QAngle angTemp = QAngle((float)i / 0.5f, angle.y, 0.0f);

        int nTempSeed = ComputeRandomSeed(&angTemp, nCurrentTick);
        auto fuckingnigger = CalculateSpread(nTempSeed + 1, flInAccuracy, flSpread);
        Vector2D vecSpread = Vector2D(fuckingnigger.x, fuckingnigger.y);

        QAngle angNoSpreadView = angle;
        angNoSpreadView.x += DirectX::XMConvertToDegrees(atan(sqrt((vecSpread.x * vecSpread.x) + (vecSpread.y * vecSpread.y))));
        angNoSpreadView.z = -DirectX::XMConvertToDegrees(atan2(vecSpread.x, vecSpread.y));

        if (ComputeRandomSeed(&angNoSpreadView, nCurrentTick) == nTempSeed)
        {
            NoSpreadResult.angAdjusted = angNoSpreadView;
            NoSpreadResult.iSeed = nTempSeed;
            NoSpreadResult.bFound = true;
            break;
        }
    }

    return NoSpreadResult;
}
