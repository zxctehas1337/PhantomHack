#include "Ragebot.h"
#include "../threading/Threader.h"

constexpr Vector vWorldUpVector = Vector(0, 0, 1);

using namespace NRagebot;

std::vector<Vector> CMultipointManager::HeadPoints(HitboxData_t hitbox) {

	float flScaledRadius = hitbox.m_flRadius / 100.f * g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale;
	constexpr int iNumCirclePoints = 8;

	Vector vecMaxs = hitbox.m_vMaxs;
	Vector vCenter = hitbox.m_vCenter;

	std::vector<Vector> vecPoints;
	vecPoints.reserve(iNumCirclePoints);

	Vector vecForward = (vCenter - m_vExtrapShootPos).Normalized();
	Vector vecRight = Vector(-vecForward.y, vecForward.x, 0).Normalized();

	for (int i = 0; i < iNumCirclePoints; i++) {
		float flAngle = M_2PI * (float(i) / float(iNumCirclePoints));
		float flSin, flCos;
		DirectX::XMScalarSinCos(&flSin, &flCos, flAngle);

		Vector v2DCirclePoint = vecMaxs + (vecRight * flCos + vWorldUpVector * flSin) * flScaledRadius;
		Vector vPerpCirclePoint = vecMaxs + (vecForward * flCos + vWorldUpVector * flSin) * flScaledRadius;

		vecPoints.push_back(v2DCirclePoint);
		vecPoints.push_back(vPerpCirclePoint);
	}

	return vecPoints;
}

std::vector<Vector> CMultipointManager::MinimalPoints(HitboxData_t hitbox) {

	float flScaledRadius = hitbox.m_flRadius / 100.f * std::clamp(g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale * 1.15f, 1.f, 100.f);

	Vector vCenter = hitbox.m_vCenter;

	std::vector<Vector> vecPoints;
	vecPoints.reserve(2);

	Vector vecForward = (vCenter - m_vExtrapShootPos).Normalized();
	Vector vecRight = Vector(-vecForward.y, vecForward.x, 0).Normalized();

	vecPoints.push_back(vCenter + vecRight * flScaledRadius);
	vecPoints.push_back(vCenter - vecRight * flScaledRadius);

	return vecPoints;
}

std::vector<Vector> CMultipointManager::GenerateMultipoints(HitboxData_t hitbox) {
	switch (hitbox.m_nHitboxIndex) {
	case HITBOX_HEAD:
		return this->HeadPoints(hitbox);
	default:
		return this->MinimalPoints(hitbox);
	}
}

float SpeedRatioCalc(float flCurrentSpeed2D, float flAccurateSpeed, float flAdjustedFullSpeed)
{
	float v6 = flCurrentSpeed2D - flAccurateSpeed;
	float v7 = flAdjustedFullSpeed - flAccurateSpeed;

	float v9 = v6 / v7;
	return std::clamp(v9, 0.0f, 1.0f);
}

bool CAccuracy::HasMaximumAccuracy()
{
	CONVAR(weapon_accuracy_nospread);

	if (weapon_accuracy_nospread->GetBool())
		return true;


	if (LocalPlayerData::m_nWeaponDefinitionIndex == WEAPON_SSG_08) {

		Vector vCam = Globals::m_pLocalPlayerPawn->m_pCameraServices()->m_vClientScopeInaccuracy();

		if (vCam.x <= 1e-6f)
			vCam.x = 0.f;

		if (vCam.x <= 0.00089f)
			return true;

	}



	return false;

}

int GetRandomSeedFast(float flPitch, float flYaw, int nTickCount)
{
	struct HashBuffer
	{
		float flPitch;
		float flYaw;
		int nTick;
	};

	HashBuffer buffer;
	buffer.flPitch = flPitch;
	buffer.flYaw = flYaw;
	buffer.nTick = nTickCount;

	CSHA1 sha1;
	sha1.Reset();
	sha1.Update((const unsigned char*)&buffer, sizeof(buffer));
	sha1.Final();

	return *(int*)&sha1.m_digest[0];
}

float RoundHalf(float flValue)
{
	return floor(flValue + flValue) * 0.5f;
}

int GetRandomSeed(const QAngle& angAngles, int nTickCount)
{
	const float flPitch = RoundHalf(std::remainder(angAngles.x, 360.0f));
	const float flYaw = RoundHalf(std::remainder(angAngles.y, 360.0f));
	return GetRandomSeedFast(flPitch, flYaw, nTickCount);
}

Vector2D GetSpread(const int nSeed, const float flInaccuracy, const float flSpread)
{
	const float flRecoilIndex = Globals::m_pLocalPlayerPawn->m_pWeaponServices()->m_hActiveWeapon().Get()->m_flRecoilIndex();
	Vector2D vSpread = { 0.f, 0.f };
	static auto fnCalcSpread = reinterpret_cast<void(__fastcall*)(int16_t, int, int, std::uint32_t, float, float, float, float*, float*)>(Memory::FindPattern(CLIENT_DLL, X("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 63 EA")));
	fnCalcSpread(Globals::m_pLocalPlayerPawn->m_pWeaponServices()->m_hActiveWeapon().Get()->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex(), Globals::m_pLocalPlayerPawn->m_pWeaponServices()->m_hActiveWeapon().Get()->GetWeaponBaseVData()->m_nNumBullets(), false ? 1 : 0, nSeed, flInaccuracy, flSpread, flRecoilIndex, &vSpread.x, &vSpread.y);
	return vSpread;
}

static std::vector<Vector> CalculateSpreadInternal(int seed, float accuracy, float spread, float recoil_index, int item_def_idx, int num_bullets)
{
	std::vector<Vector> spread_results(num_bullets);
	static auto fnCalcSpread = reinterpret_cast<void(__fastcall*)(int16_t, int, int, std::uint32_t, float, float, float, float*, float*)>(Memory::FindPattern(CLIENT_DLL, X("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 63 EA")));
	fnCalcSpread(
		item_def_idx,
		num_bullets,
		0,
		seed + 1,
		accuracy,
		spread,
		recoil_index,
		reinterpret_cast<float*>(&spread_results[0].x),
		reinterpret_cast<float*>(&spread_results[0].y)
	);

	return spread_results;
}

bool CAccuracy::IsAccurate()
{
	const float flWeaponRange = LocalPlayerData::m_pWeaponBaseVData->m_flRange();
	float flWeaponInaccuracy = LocalPlayerData::m_pWeapon->GetInaccuracy().flTotalInaccuracy;
	float flWeaponSpread = LocalPlayerData::m_pWeapon->GetSpread();
	float flRecoilIndex = LocalPlayerData::m_pWeapon->m_flRecoilIndex();
	int nNumBullets = LocalPlayerData::m_pWeaponBaseVData->m_nNumBullets();

	Vector vForward, vRight, vUp;
	g_Rage->m_Data.m_LastTarget.m_angToTarget.ToDirections(&vForward, &vRight, &vUp);

	const int iHitChance = g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance;
	constexpr int nWantedHitCount = 256;
	const int nNeededHits = static_cast<int>(nWantedHitCount * (iHitChance / 100.f));

	int nHits = 0;

	GameTrace_t GameTrace = GameTrace_t();
	TraceFilter_t Filter = TraceFilter_t(TRACE_MASK_AUTOWALL, Globals::m_pLocalPlayerPawn, nullptr, 4);
	Ray_t Ray = Ray_t();

	auto fnCheckHit = [&](Vector vSpread)
		{
			Vector vDirection = vForward + vRight * vSpread.x + vUp * vSpread.y;
			vDirection.NormalizeInPlace();
			Vector vEnd = m_vExtrapShootPos + (vDirection * flWeaponRange);
			return Interfaces::m_pVPhys2World->ClipRayToEntity(&Ray, m_vExtrapShootPos, vEnd, g_Rage->m_Data.m_LastTarget.m_pLagRecord->m_pPawn, &Filter, &GameTrace) &&
				GameTrace.m_pHitEntity == g_Rage->m_Data.m_LastTarget.m_pLagRecord->m_pPawn /*&& GameTrace.m_pHitboxData->m_nHitboxId == g_Rage->m_Data.m_LastTarget.m_hitboxData.m_nHitboxIndex*/;
		};

	for (int nSeed = 0; nSeed < nWantedHitCount; nSeed++)
	{
		Vector vSpread = CalculateSpreadInternal(nSeed, flWeaponInaccuracy, flWeaponSpread, flRecoilIndex, LocalPlayerData::m_nWeaponDefinitionIndex, nNumBullets)[0];

		if (fnCheckHit(vSpread))
			++nHits;

		const float flCurrentHitChance = (static_cast<float>(nHits) / static_cast<float>(nSeed + 1)) * 100.f;
		if (flCurrentHitChance >= static_cast<float>(iHitChance))
			return true;
	}

	return false;
}

bool CScan::IsBetterCandidate(ScanResult_t candidate, ScanResult_t current) {

	if (candidate.m_flDamage <= 0)
		return false;

	if (current.m_flDamage <= 0)
		return true;

	int targetingPriority = g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType;
	switch (targetingPriority) {
	case 0:
		return candidate.m_flDamage > current.m_flDamage;
	case 1:
		return candidate.m_flWorldDist < current.m_flWorldDist;
	case 2:
		return candidate.m_pLagRecord->m_pPawn->m_iHealth() < current.m_pLagRecord->m_pPawn->m_iHealth();
	default:
		return candidate.m_flDamage > current.m_flDamage;
	}
}

bool CScan::IsWithinFov(const Vector& vStartPos, const Vector& vTargetPos, const QAngle& angView, float flFovDegrees) {
	if (flFovDegrees >= 180.0f)
		return true;

	QAngle angToTarget = Math::CalcAngle(vStartPos, vTargetPos);

	angToTarget = Math::NormalizeAnglesReturn(angToTarget);
	QAngle normalizedView = Math::NormalizeAnglesReturn(angView);

	float flDeltaYaw = std::abs(angToTarget.y - normalizedView.y);

	if (flDeltaYaw > 180.0f)
		flDeltaYaw = 360.0f - flDeltaYaw;

	return flDeltaYaw <= (flFovDegrees * 0.5f);
}

struct BestHitboxPoint_t {
	Vector m_vPoint = {};
	float m_flDamage = 0.f;
	float m_flAlignment = -1.f;
	float m_flSize = 0.f;
	HitboxData_t m_Hitbox = {};
};

ScanResult_t CScan::ProcessRecord(CLagRecord* pRecord) {
	ScanResult_t objCandidate = {};
	objCandidate.m_pLagRecord = pRecord;
	objCandidate.m_flWorldDist = (m_vExtrapShootPos - pRecord->m_vOrigin).Length();
	int nDamageThreshold = m_bHoldingMinOverride
		? g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride
		: ((g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage > 100)
			? (g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage - 100 + pRecord->m_pPawn->m_iHealth())
			: g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage);
	objCandidate.m_flDamageThreshold = nDamageThreshold;
	BestHitboxPoint_t globalBest = {};
	for (const auto& hitbox : pRecord->m_arrTargetHitboxes) {
		const Vector vDirToCenter = (hitbox.m_vCenter - m_vExtrapShootPos).Normalized();
		Vector vAxis = hitbox.m_vMaxs - hitbox.m_vMins;
		float flSegmentLength = vAxis.Length();
		Vector vAxisDir = flSegmentLength > 0.f ? (vAxis / flSegmentLength) : Vector{};
		BestHitboxPoint_t bestInHitbox = {};
		penetration_data_t pen_data{};
		if (!g_Autowall->m_Functions.m_FireBullet.FireBullet(m_vExtrapShootPos, hitbox.m_vCenter, &pen_data, pRecord->m_pPawn))
			continue;
		float flDmgCenter = pen_data.m_damage;
		if (flDmgCenter >= nDamageThreshold) {
			float flAlignment = 1.0f;
			float flSize = hitbox.m_flRadius;
			bestInHitbox = {
				hitbox.m_vCenter,
				flDmgCenter,
				flAlignment,
				flSize,
				hitbox
			};
		}
		if (hitbox.m_bMultipoint) {
			for (const auto& vMP : g_Points->GenerateMultipoints(hitbox)) {
				penetration_data_t pen_data_mp{};
				if (!g_Autowall->m_Functions.m_FireBullet.FireBullet(m_vExtrapShootPos, vMP, &pen_data_mp, pRecord->m_pPawn))
					continue;
				float flDmg = pen_data_mp.m_damage;
				if (flDmg >= nDamageThreshold) {
					Vector dirToMP = (vMP - m_vExtrapShootPos).Normalized();
					float flAlignment = dirToMP.DotProduct(vDirToCenter);
					float flSinAng = vAxisDir.CrossProduct(dirToMP).Length();
					float flSize = hitbox.m_flRadius + 0.5f * flSegmentLength * flSinAng;
					if (flDmg > bestInHitbox.m_flDamage ||
						(fabs(flDmg - bestInHitbox.m_flDamage) < 1e-6f &&
							(flAlignment > bestInHitbox.m_flAlignment ||
								(fabs(flAlignment - bestInHitbox.m_flAlignment) < 1e-6f && flSize > bestInHitbox.m_flSize)))) {
						bestInHitbox = {
							vMP,
							flDmg,
							flAlignment,
							flSize,
							hitbox
						};
					}
				}
			}
		}
		if (bestInHitbox.m_flDamage > globalBest.m_flDamage ||
			(fabs(bestInHitbox.m_flDamage - globalBest.m_flDamage) < 1e-6f &&
				bestInHitbox.m_flAlignment > globalBest.m_flAlignment)) {
			globalBest = bestInHitbox;
		}
	}
	objCandidate.m_vPoint = globalBest.m_vPoint;
	objCandidate.m_flDamage = globalBest.m_flDamage;
	objCandidate.m_hitboxData = globalBest.m_Hitbox;
	return objCandidate;
}

ScanResult_t CScan::ProcessTarget(CRecordTrack* pRecordTrack) {
	CLagRecord* pNewestRecord = pRecordTrack->GetFirstRecord();
	CLagRecord* pOldestRecord = pRecordTrack->GetLastRecord();

	if (!pNewestRecord || !pOldestRecord)
		return {};

	const bool bEnableBacktrack = Config::vb(g_Variables.m_Ragebot.m_vbRageHistoryAim).at(ERageHistoryAimModes::RAGE_HISTORY_BT_ENABLED);
	const bool bBacktrackOnly = Config::vb(g_Variables.m_Ragebot.m_vbRageHistoryAim).at(ERageHistoryAimModes::RAGE_HISTORY_BT_ONLY);

	const float flPositionDelta = (pNewestRecord->m_vOrigin - pOldestRecord->m_vOrigin).Length2D();
	const bool bShouldScanBacktrack = flPositionDelta > 0.18f;

	auto frontResult = pNewestRecord->WithAppliedState([&] { return this->ProcessRecord(pNewestRecord); });

	if ((!frontResult.HasValidTarget() && bEnableBacktrack && bShouldScanBacktrack) || bBacktrackOnly) {
		return pOldestRecord->WithAppliedState([&] { return this->ProcessRecord(pOldestRecord); });
	}

	return frontResult;
}

ScanResult_t CScan::GetBestTarget() {

	std::vector<ScanResult_t> results(g_Entities->m_vecPlayersOnly.size());

	std::atomic<size_t> resultIndex{ 0 };

	for (size_t i = 0; i < g_Entities->m_vecPlayersOnly.size(); ++i) {
		CRecordTrack* pTrack = g_Entities->m_vecPlayersOnly[i].GetRecordTrack();
		if (!pTrack || !pTrack->m_bRageMatters)
			continue;

		g_Threader->EnqueueJob([this, pTrack, &results, i]() {
			results[i] = this->ProcessTarget(pTrack);
			});
	}

	g_Threader->WaitUntilFinished();

	ScanResult_t objBestResult{};
	for (const auto& result : results) {
		if (g_Scan->IsBetterCandidate(result, objBestResult)) {
			objBestResult = result;
		}
	}

	objBestResult.m_angToTarget = Math::CalcAngle(m_vShootPos, objBestResult.m_vPoint)/* - Globals::m_pLocalPlayerPawn->GetRemovedAimPunch()*/;


	return objBestResult;
}

void CRage::HandleKeybinds() {
	m_bHoldingMinOverride = Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iMinDamageOverrideKey));

	static bool bLastHeadshotState = false;
	static bool bLastBodyaimState = false;

	bool bCurrentHeadshotState = Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iForceHeadshotKey));
	bool bCurrentBodyaimState = Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iForceBodyaimKey));

	if (bCurrentHeadshotState && !bLastHeadshotState) {
		m_bForceHeadshot = !m_bForceHeadshot;
		m_bForceBodyaim = false;
	}

	if (bCurrentBodyaimState && !bLastBodyaimState) {
		m_bForceBodyaim = !m_bForceBodyaim;
		m_bForceHeadshot = false;
	}

	bLastHeadshotState = bCurrentHeadshotState;
	bLastBodyaimState = bCurrentBodyaimState;
}

bool CRage::SetConfig()
{
	if (!Config::b(g_Variables.m_Ragebot.m_bRagebotEnabled)) {
		this->m_Data = {};
		return false;
	}

	auto nigger = GetWeaponMenuType((EItemDefinitionIndexes)LocalPlayerData::m_nWeaponDefinitionIndex);
	if (nigger == NONE) {
		this->m_Data = {};
		this->m_bBadWeapon = true;
		return false;
	}

	if (!LocalPlayerData::m_bRageNeedsConfigUpdate)
		return true;

	LocalPlayerData::m_bRageNeedsConfigUpdate = false;

	switch (nigger) {
	case LIGHT_PISTOL:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_LIGHT_PISTOL_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_LIGHT_PISTOL_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_LIGHT_PISTOL_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_LIGHT_PISTOL_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_LIGHT_PISTOL_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_LIGHT_PISTOL_iMaxPointscale);
		break;
	case DEAGLE:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_DEAGLE_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_DEAGLE_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_DEAGLE_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_DEAGLE_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_DEAGLE_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_DEAGLE_iMaxPointscale);
		break;
	case REVOLVER:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_REVOLVER_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_REVOLVER_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_REVOLVER_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_REVOLVER_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_REVOLVER_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_REVOLVER_iMaxPointscale);
		break;
	case SMG:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_SMG_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_SMG_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_SMG_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_SMG_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_SMG_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_SMG_iMaxPointscale);
		break;
	case LMG:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_LMG_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_LMG_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_LMG_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_LMG_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_LMG_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_LMG_iMaxPointscale);
		break;
	case AR:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_AR_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_AR_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_AR_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_AR_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_AR_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_AR_iMaxPointscale);
		break;
	case SHOTGUN:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_SHOTGUN_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_SHOTGUN_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_SHOTGUN_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_SHOTGUN_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_SHOTGUN_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_SHOTGUN_iMaxPointscale);
		break;
	case SCOUT:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_SCOUT_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_SCOUT_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_SCOUT_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_SCOUT_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_SCOUT_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_SCOUT_iMaxPointscale);
		break;
	case AUTOSNIPER:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_AUTOSNIPER_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_AUTOSNIPER_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_AUTOSNIPER_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_AUTOSNIPER_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_AUTOSNIPER_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_AUTOSNIPER_iMaxPointscale);
		break;
	case AWP:
		g_Variables.m_Ragebot.m_ActiveConfig.m_iHitchance = Config::i(g_Variables.m_Ragebot.m_AWP_iHitchance);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage = Config::i(g_Variables.m_Ragebot.m_AWP_iMinDamage);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride = Config::i(g_Variables.m_Ragebot.m_AWP_iMinDamageOverride);
		g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes = Config::vhb<MenuRageHitbox_t>(g_Variables.m_Ragebot.m_AWP_vecHitBoxes);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iTargettingType = Config::i(g_Variables.m_Ragebot.m_AWP_iTargetPriority);
		g_Variables.m_Ragebot.m_ActiveConfig.m_iMaxPointscale = Config::i(g_Variables.m_Ragebot.m_AWP_iMaxPointscale);
		break;
	}
	this->m_bBadWeapon = false;
	return true;
}

void CRage::OverrideInputHistory()
{
	if (!this->m_Data.m_LastTarget.HasValidTarget())
		return;

	for (int i = 0; i < Globals::m_pCmd->m_csgoUserCmd.input_history_size(); i++)
	{
		CSGOInputHistoryEntryPB* pInputEntry = Globals::m_pCmd->m_csgoUserCmd.mutable_input_history(i);
		if (!pInputEntry)
			continue;

		pInputEntry->set_player_tick_count(Globals::m_pLocalPlayerController->m_nTickBase());
		pInputEntry->set_render_tick_count(this->m_Data.m_LastTarget.m_pLagRecord->m_nTick + g_LagCompensation->m_InterpolationTiming.m_iTick);

		pInputEntry->set_player_tick_fraction(0.f);
		pInputEntry->set_render_tick_fraction(0.f);

		if (pInputEntry->has_cl_interp())
			pInputEntry->mutable_cl_interp()->set_frac(0.f);

		if (pInputEntry->has_sv_interp0()) {
			pInputEntry->mutable_sv_interp0()->set_frac(0.f);
			pInputEntry->mutable_sv_interp0()->set_src_tick(0);
			pInputEntry->mutable_sv_interp0()->set_dst_tick(0);
		}

		if (pInputEntry->has_sv_interp1()) {
			pInputEntry->mutable_sv_interp1()->set_frac(0.f);
			pInputEntry->mutable_sv_interp1()->set_src_tick(0);
			pInputEntry->mutable_sv_interp1()->set_dst_tick(0);
		}

		pInputEntry->mutable_view_angles()->set_x(this->m_Data.m_LastTarget.m_angToTarget.x);
		pInputEntry->mutable_view_angles()->set_y(this->m_Data.m_LastTarget.m_angToTarget.y);

	}
}

void CRage::ConsiderAttack()
{
	if (!this->m_Data.m_LastTarget.HasValidTarget())
		return;

	if (!this->m_Data.m_bIsAccurate)
		return;

	auto& pCmd = Globals::m_pCmd;

	if (!(Interfaces::m_pInput->nKeyboardPressed & IN_ATTACK))
		Interfaces::m_pInput->nKeyboardPressed |= IN_ATTACK;

	if (!(pCmd->m_nButtons.m_nValue & IN_ATTACK))
		pCmd->m_nButtons.m_nValue |= IN_ATTACK;

	Globals::m_pCmd->m_csgoUserCmd.mutable_base()->set_client_tick(Globals::m_pLocalPlayerController->m_nTickBase());

	int nEntryIdx = pCmd->m_csgoUserCmd.input_history_size() - 1;
	pCmd->m_csgoUserCmd.set_attack1_start_history_index(nEntryIdx);

	if (this->m_Data.m_bShouldInvalidateAngle)
	{
		pCmd->m_csgoUserCmd.mutable_base()->mutable_viewangles()->set_x(this->m_Data.m_LastTarget.m_angToTarget.x);
		pCmd->m_csgoUserCmd.mutable_base()->mutable_viewangles()->set_y(this->m_Data.m_LastTarget.m_angToTarget.y);
		g_Movement->StoreModelAngles();
	}

	if (!Config::b(g_Variables.m_Ragebot.m_bSilentAim))
		Interfaces::m_pInput->SetViewAngle(this->m_Data.m_LastTarget.m_angToTarget);
}

void CRage::AutoRevolver() {
	if (!Interfaces::m_pEngine->IsInGame() || !Interfaces::m_pEngine->IsConnected())
		return;

	if (!Globals::m_pLocalPlayerController || !Globals::m_pLocalPlayerPawn)
		return;

	CPlayer_WeaponServices* pWeaponServices = Globals::m_pLocalPlayerPawn->m_pWeaponServices();
	if (!pWeaponServices)
		return;

	C_BasePlayerWeapon* pActiveWeapon = pWeaponServices->m_hActiveWeapon().Get();
	if (!pActiveWeapon)
		return;

	CCSWeaponBaseVData* pData = pActiveWeapon->GetWeaponBaseVData();
	if (!pData || !pData->m_bIsRevolver())
		return;

	const float flFPS = 1.0f / Interfaces::m_pGlobalVariables->m_flFrameTime;
	const bool bLowFPS = flFPS < 60.0f;

	const int nAttackTicks = bLowFPS ? 2 : 10;
	const int nAltAttackTicks = bLowFPS ? 5 : 20;
	const int nResetTicks = bLowFPS ? 8 : 20;

	static int ticks = 0;
	static int old_ticks = 0;

	if (!pData->m_bIsRevolver()) {
		ticks = 0;
		old_ticks = Interfaces::m_pGlobalVariables->m_nTickCount;
		return;
	}

	int32_t current_tick = Interfaces::m_pGlobalVariables->m_nTickCount;
	if (current_tick != old_ticks) {
		ticks++;
		old_ticks = current_tick;
	}

	if (ticks < nAttackTicks)
		Globals::m_pCmd->m_nButtons.m_nValue |= IN_ATTACK;
	else if (ticks < nAltAttackTicks)
		Globals::m_pCmd->m_nButtons.m_nValue |= IN_ATTACK2;

	if (ticks > nResetTicks)
		ticks = 0;

	/*test revolver, can be fixed. on community servers you`ll lost ammo*/

	//const int nCurrentTick = Interfaces::m_pGlobalVariables->m_nTickCount;
	//const bool bCanPrimaryAttack = nCurrentTick >= pActiveWeapon->m_nNextPrimaryAttackTick();
	//const bool bCanSecondaryAttack = nCurrentTick >= pActiveWeapon->m_nNextSecondaryAttackTick();
	//const int nCurrentSubTick = Globals::m_pCmd->m_csgoUserCmd.input_history_size() - 1;

	//if (!(Globals::m_pCmd->m_nButtons.m_nValue & (IN_ATTACK | IN_ATTACK2))) {
	//	if (!bCanSecondaryAttack) {
	//		Globals::m_pCmd->m_nButtons.SetButtonState(IN_ATTACK2, CInButtonState::EButtonState::IN_BUTTON_UP_DOWN);

	//		Globals::m_pCmd->m_csgoUserCmd.set_attack2_start_history_index(nCurrentSubTick);
	//	}
	//	else if (bCanPrimaryAttack) {
	//		Globals::m_pCmd->m_nButtons.SetButtonState(IN_ATTACK, CInButtonState::EButtonState::IN_BUTTON_UP_DOWN);

	//		Globals::m_pCmd->m_csgoUserCmd.set_attack1_start_history_index(nCurrentSubTick);
	//	}
	//}

	//Globals::m_pCmd->m_csgoUserCmd.set_attack3_start_history_index(nCurrentSubTick);
}

void CRage::OnCreateMove()
{
	if (!Config::b(g_Variables.m_Ragebot.m_bRagebotEnabled) || this->m_bBadWeapon)
	{
		this->m_Data = {};
		return;
	}

	AutoRevolver();

	if (Globals::m_pCmd->m_nCommandNumber == g_Scan->m_nLastSequenceScan)
		return;

	g_Scan->m_nLastSequenceScan = Globals::m_pCmd->m_nCommandNumber;

	if (!Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iForceHeadshotKey))) {
		m_bForceHeadshot = false;
	}

	if (!Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iForceBodyaimKey))) {
		m_bForceBodyaim = false;
	}

	if (LocalPlayerData::m_pWeapon->m_nNextPrimaryAttackTick() > Interfaces::m_pGlobalVariables->m_nTickCount)
	{
		this->m_Data = {};
		return;
	}

	bool bShouldExtrap = Globals::m_pLocalPlayerPawn->m_vecAbsVelocity().Length2D() > LocalPlayerData::m_pWeapon->GetMaxSpeed() * 0.34f;
	m_vExtrapShootPos = bShouldExtrap ? g_Movement->SimulatePosAtStop(m_vShootPos) : m_vShootPos;

	this->m_Data.m_LastTarget = g_Scan->GetBestTarget();

	if (!this->m_Data.m_LastTarget.HasValidTarget())
	{
		this->m_Data = {};
		return;
	}

	if ((Globals::m_pLocalPlayerPawn->m_pWeaponServices()->m_hActiveWeapon().Get()->GetWeaponBaseVData()->m_WeaponType() == WEAPONTYPE_SNIPER_RIFLE) && !(Globals::m_pCmd->m_nButtons.m_nValue & IN_ZOOM) && !(Globals::m_pLocalPlayerPawn->m_bIsScoped()) && (Globals::m_pLocalPlayerPawn->m_fFlags() & FL_ONGROUND))
		Globals::m_pCmd->m_nButtons.m_nValue |= IN_ZOOM;

	this->m_Data.m_bIsAccurate = this->m_Data.m_LastTarget.m_pLagRecord->WithAppliedState(([&] { return g_Accuracy->IsAccurate(); }));
	this->m_Data.m_bShouldInvalidateAngle = !this->m_Data.m_LastTarget.m_pLagRecord->WantsLagCompensationOnEntity(LocalPlayerData::m_vecAbsOrigin, g_Movement->m_angModelAngles);
	this->m_Data.m_bWants2DMoveHalt = Config::b(g_Variables.m_Ragebot.m_bAutostop);
}
