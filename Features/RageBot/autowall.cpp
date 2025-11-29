#include "../../Precompiled.h"

float CalcLengthMod(TraceData_t* pTraceData) {
	using fnCalcLengthMod = float(__fastcall*)(TraceData_t*);
	static fnCalcLengthMod CalcLengthModFunc = reinterpret_cast<fnCalcLengthMod>(
		Memory::FindPattern(CLIENT_DLL, X("40 53 48 83 EC ? 48 8B D9 48 81 C1 ? ? ? ? E8 ? ? ? ? 48 63 83"))
		);

	return CalcLengthModFunc(pTraceData);
}

//bool CAutowall::CFunctions::CFireBullet::CanHit(const Vector vecSrc, const Vector vecEnd, float flMinDmg) {
//	bool bReturn = FireBullet(vecSrc, vecEnd, Globals::m_pLocalPlayerPawn);
//	if (bReturn && (LocalPlayerData::m_pWeaponBaseVData->m_nDamage() < flMinDmg))
//		bReturn = false;
//
//	return bReturn;
//}

bool CAutowall::CFunctions::CFireBullet::FireBullet(Vector start, Vector end, penetration_data_t* pen_data, C_CSPlayerPawn* target) {
	auto weapon_data = LocalPlayerData::m_pWeaponBaseVData;

	C_CSPlayerPawn* local_player = Globals::m_pLocalPlayerPawn;
	if (!local_player)
		return false;

	TraceData_t trace_data = TraceData_t();
	trace_data.m_ArrPointer = &trace_data.m_Arr;

	Vector direction = end - start;
	Vector end_pos = direction * weapon_data->m_flRange();

	TraceFilter_t filter(0x1C300B, local_player, nullptr, 3);
	Interfaces::m_pVPhys2World->TraceToExit(&trace_data, start, end_pos, filter, 4);

	HandleBulletDataObj_t handle_bullet_data{
		static_cast<float>(weapon_data->m_nDamage()),
		weapon_data->m_flPenetration(),
		weapon_data->m_flRange(),
		weapon_data->m_flRangeModifier(),
		4,
		false
	};

	float max_range = weapon_data->m_flRange();
	pen_data->m_damage = static_cast<float>(weapon_data->m_nDamage());

	for (int i = 0; i < trace_data.m_nSurfaces; i++) {
		UpdateValue_t* modulate_values = reinterpret_cast<UpdateValue_t* const>(reinterpret_cast<std::uintptr_t>(trace_data.pointer_update_value) + i * sizeof(UpdateValue_t));

		GameTrace_t game_trace{};
		Interfaces::m_pVPhys2World->InitTraceInfo(&game_trace);
		Interfaces::m_pVPhys2World->GetTraceInfo(&trace_data, &game_trace, 0.0f, reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(trace_data.m_Arr) + sizeof(trace_array_element_t) * (modulate_values->m_HandleIdx & ENT_ENTRY_MASK)));

		if (game_trace.m_pHitEntity && game_trace.m_pHitEntity->IsPlayerPawn()) {
			if (pen_data->m_damage < 1.0f)
				continue;

			g_Autowall->m_Functions.m_ScaleDamage.ScaleDamage(target, LocalPlayerData::m_pWeaponBaseVData, game_trace.GetHitgroup(), pen_data->m_damage);
			pen_data->m_hitbox = game_trace.GetHitboxId();
			pen_data->m_penetrated = i == 0;

			return true;
		}

		if (Interfaces::m_pVPhys2World->HandleBulletPenetration(&trace_data, &handle_bullet_data, modulate_values, static_cast<int>(Globals::m_pLocalPlayerPawn->m_iTeamNum())))
			return false;

		*reinterpret_cast<DWORD*>(reinterpret_cast<QWORD>(modulate_values) + 12) = 4;
		pen_data->m_damage = handle_bullet_data.m_flDmg;
	}

	return false;
}

int CAutowall::CalculateDamage(C_CSPlayerPawn* pTargetPawn, C_CSWeaponBaseGun* pBaseGun, Vector vecEnd)
{
	return 0;
	//return g_Autowall->m_Functions.m_FireBullet.FireBullet( Globals::m_pLocalPlayerPawn->GetEyePosition( true )/*g_PredictionSystem->GetEyePos()*/, vecEnd, pTargetPawn );
}

struct TraceResult_t
{
	Vector m_vecEndPos;
	char unk1[28];
};

unsigned __int8 sub_61C8A0(void* a1, void* a2, __int64 a3, unsigned __int8 a4)
{
	using sub_61C8A0Fn = unsigned __int8(__fastcall*)(void*, void*, __int64, unsigned __int8);
	static sub_61C8A0Fn sub_61C8A0 = reinterpret_cast<sub_61C8A0Fn>(
		Memory::GetAbsoluteAddress(Memory::FindPattern(CLIENT_DLL, X("E8 ? ? ? ? 48 8B 5C 24 ? 48 8B 6C 24 ? 48 8B 74 24 ? 0F 28 74 24 ? 48 83 C4 ? 5F C3 CC CC CC 48 89 5C 24")), 0x1, 0x0)
		);

	return sub_61C8A0(a1, a2, a3, a4);
}

Vector* NormalizeVectorInPlace(Vector* vector)
{
	using NormalizeVectorInPlaceFn = Vector * (__fastcall*)(Vector*);
	static NormalizeVectorInPlaceFn NormalizeVectorInPlace = reinterpret_cast<NormalizeVectorInPlaceFn>(
		Memory::GetAbsoluteAddress(Memory::FindPattern(CLIENT_DLL, X("E8 ? ? ? ? F2 0F 10 0B 4C 8B C3")), 0x1, 0x0)
		);

	return NormalizeVectorInPlace(vector);
}

bool FillTraceData(TraceData_t* pTraceData, TraceResult_t* pTraceResult)
{
	using FillTraceDataFn = bool(__fastcall*)(TraceData_t*, TraceResult_t*);
	static FillTraceDataFn FillTraceDataFunc = reinterpret_cast<FillTraceDataFn>(
		Memory::FindPattern(CLIENT_DLL, X("40 55 56 41 54 41 55 41 57 48 8B EC"))
		);

	return FillTraceDataFunc(pTraceData, pTraceResult);
}

static bool CheckHitboxesWrapper(CVPhys2World* pVPhys2World, TraceData_t* pTraceData, Vector* vecStart, TraceResult_t* pTraceResult, int64_t* pTraceMask, int nMagicNumber)
{
	using CheckHitboxesWrapperFn = char(__fastcall*)(CVPhys2World*, TraceData_t*, Vector*, TraceResult_t*, int64_t*, int);
	static CheckHitboxesWrapperFn CheckHitboxesWrapperFunc = reinterpret_cast<CheckHitboxesWrapperFn>(
		Memory::FindPattern(CLIENT_DLL, X("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 09"))
		);

	return CheckHitboxesWrapperFunc(pVPhys2World, pTraceData, vecStart, pTraceResult, pTraceMask, nMagicNumber);
}

namespace Interfaces
{
	class CCSHitboxSystem
	{
	public:
		__int64(__fastcall** ppfunc0)(__int64, TraceData_t*, __int64, Vector*, TraceResult_t*, int, int, __int64, int);
		MEM_PAD(0x28);
		int* m_pEntityList;
	public:
		bool HandleEntityList(TraceData_t* pTraceData, Vector* vecStart, TraceResult_t* pTraceResult, __int64 nTraceMask, int nMagicNumber)
		{
			using fnHandleEntityList = bool(__fastcall*)(CCSHitboxSystem*, TraceData_t*, Vector*, TraceResult_t*, __int64, int, int);
			static auto HandleEntityList_INTERNAL = reinterpret_cast<fnHandleEntityList>(Memory::GetAbsoluteAddress(Memory::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 84 C0 74 ? 48 63 03"), 0x1, 0x0));

			return HandleEntityList_INTERNAL(this, pTraceData, vecStart, pTraceResult, nTraceMask, 0, nMagicNumber);;
		}
	};

	CCSHitboxSystem* m_pHitboxSystem = nullptr;
}

static bool HandleEntityListRebuilt(C_CSPlayerPawn* pTargetPawn, Vector* vecStart, TraceResult_t* pTraceResult, TraceData_t* pTraceData, int nMagicNumber, int64_t* pTraceMask)
{
	using fnSetTraceData = __int64(__fastcall*)(TraceData_t*, void*);
	static fnSetTraceData SetTraceData = (fnSetTraceData)Memory::GetAbsoluteAddress(Memory::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 8B 85 ? ? ? ? 48 8D 54 24 ? F2 0F 10 45"), 0x1, 0x0);

	GameTrace_t objGameTrace = GameTrace_t();
	TraceFilter_t objFilter = TraceFilter_t(0x1C3003, Globals::m_pLocalPlayerPawn, nullptr, 4);
	Ray_t objRay = Ray_t();

	auto pCollisionProperty = Memory::CallVFunc<__int64, 57>(pTargetPawn);
	Matrix3x4_t v31 = *(Matrix3x4_t*)(pCollisionProperty + 0x18);

	Interfaces::m_pVPhys2World->TraceShape(&objRay, *vecStart, pTraceResult->m_vecEndPos, &objFilter, &objGameTrace);
	bool bHitPlayer = objGameTrace.m_pHitEntity == pTargetPawn;
	if (bHitPlayer)
	{
		SetTraceData(pTraceData, &v31);

		pTraceResult->m_vecEndPos = objGameTrace.m_vecEndPos;
	}
	return bHitPlayer;
}

bool __fastcall CheckHitboxesWrapperRebuilt(CVPhys2World* pVPhys2World, TraceData_t* pTraceData, Vector* vecStart, TraceResult_t* pTraceResult, uint64_t* pTraceMask, int nMagicNumber)
{
	using fn = bool(__fastcall*)(CVPhys2World*, TraceData_t*, Vector*, TraceResult_t*, uint64_t*, int);
	static fn x = (fn)Memory::FindPattern(CLIENT_DLL, "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 09");
	return x(pVPhys2World, pTraceData, vecStart, pTraceResult, pTraceMask, nMagicNumber);
}

bool CAutowall::CRebuiltFunctions::CTraceToExit::TraceToExit(TraceData_t* pTraceData, Vector& vecStart, Vector& vecEnd, TraceFilter_t mTraceFilter, int iPenCount, C_CSPlayerPawn* pTargetPawn)
{
	if (!pTraceData)
		return false;

	TraceResult_t objTraceResult = { };

	pTraceData->m_vecStart = vecStart;

	objTraceResult.m_vecEndPos = vecEnd;

	FillTraceData(pTraceData, &objTraceResult);

	CheckHitboxesWrapperRebuilt(Interfaces::m_pVPhys2World, pTraceData, &vecStart, &objTraceResult, &mTraceFilter.m_uTraceMask, -1.0f);

	return FillTraceData(pTraceData, &objTraceResult);
}

float CAutowall::CFunctions::CScaleDamage::ScaleDamage(C_CSPlayerPawn* pPawn, CCSWeaponBaseVData* pWeaponVData, int iHitGroup, float& flDamage)
{
	float flHeadDamageScale = 1.f;
	float flBodyDamageScale = 1.f;

	CCSPlayer_ItemServices* pItemServices = pPawn->m_pItemServices();
	if (!pItemServices)
		return -1;

	switch (pPawn->GetAssociatedTeam()) {
	case TEAM_TT:
		flHeadDamageScale = Convar::mp_damage_scale_t_head->GetFloat();
		flBodyDamageScale = Convar::mp_damage_scale_t_body->GetFloat();

		break;
	case TEAM_CT:
		flHeadDamageScale = Convar::mp_damage_scale_ct_head->GetFloat();
		flBodyDamageScale = Convar::mp_damage_scale_ct_body->GetFloat();

		break;
	}

	//if (pItemServices->m_bHasHeavyArmor())
	//	flHeadDamageScale *= 0.5f;

	switch (iHitGroup) {
	case HITGROUP_HEAD:
		flDamage *= pWeaponVData->m_flHeadshotMultiplier() * flHeadDamageScale;
		break;
	case HITGROUP_CHEST:
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
	case HITGROUP_NECK:
		flDamage *= 1.f * flBodyDamageScale;
		break;
	case HITGROUP_STOMACH:
		flDamage *= 1.25f * flBodyDamageScale;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		flDamage *= 0.75f * flBodyDamageScale;
		break;
	default:
		break;
	}

	if (!pPawn->IsArmored(iHitGroup))
		return flDamage;

	float heavy_armor_bonus = 1.0f;
	float armor_bonus = 0.5f;
	float armor_ratio = pWeaponVData->m_flArmorRatio() * 0.5f;

	//if (pItemServices->m_bHasHeavyArmor()) {
	//	heavy_armor_bonus = 0.25f;
	//	armor_bonus = 0.33f;
	//	armor_ratio *= 0.2f;
	//}

	float flDamageToHealth = flDamage * armor_ratio;
	float flDamageToArmor = (flDamage - flDamageToHealth) * heavy_armor_bonus * armor_bonus;

	float flArmorValue = pPawn->m_ArmorValue();
	if (flDamageToArmor > flArmorValue)
		flDamageToHealth = flDamage - flArmorValue / armor_bonus;

	return flDamageToHealth;
}
