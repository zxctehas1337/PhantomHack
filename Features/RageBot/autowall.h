#pragma once

class CAutowall
{
public:
	class CRebuiltFunctions
	{
	public:
		class CTraceToExit
		{
		public:
			void PenTest(C_CSPlayerPawn* pTarget, Vector vShootPos, Vector vPoint);

			bool TraceToExit(TraceData_t* pTraceData, Vector& vecStart, Vector& vecEnd, TraceFilter_t mTraceFilter, int iPenCount, C_CSPlayerPawn* pTargetPawn);
		}; CTraceToExit m_TraceToExit;

		class CTestSurfaces
		{
		public:
			void TestSurfaces(TraceData_t* pTraceData, double flDamage, double flPenetration, double flRangeModifier, int iPenCount, int nTeamNum, __int64 sv_showimpacts_penetration);
		}; CTestSurfaces m_TestSurfaces;
	}; CRebuiltFunctions m_RebuiltFunctions;

	class CFunctions
	{
	public:
		class CScaleDamage
		{
		public:
			float ScaleDamage(C_CSPlayerPawn* pPawn, CCSWeaponBaseVData* pWeaponVData, int iHitGroup, float& iDamage);
		}; CScaleDamage m_ScaleDamage;


		class CFireBullet
		{
		public:
			bool FireBullet(Vector vecStart, Vector vecEnd, penetration_data_t* hitbox, C_CSPlayerPawn* pTargetPawn);
			//bool CanHit(const Vector vecSource, const Vector vecEnd, float flMinimumDamage);
		}; CFireBullet m_FireBullet;

		class CTestSurfaces
		{
		public:
			void TestSurfaces(TraceData_t* pTraceData, float flDamage, float flPenetration, float flRangeModifier, int nPenetrationCount, int nTeamNum)
			{
				using fnTestSurfaces = void(__fastcall*)(TraceData_t*, float, float, float, int, int, __int64);
				static const fnTestSurfaces oTestSurfaces = reinterpret_cast<fnTestSurfaces>(Memory::FindPattern(CLIENT_DLL, X("40 53 57 41 56 48 83 EC 50 8B")));

				return oTestSurfaces(pTraceData, flDamage, flPenetration, flRangeModifier, nPenetrationCount, nTeamNum, 0);
			}
		}; CTestSurfaces m_TestSurfaces;

	}; CFunctions m_Functions;

	class CStructs
	{
	public:
	}; CStructs m_Structs;

	int CalculateDamage(C_CSPlayerPawn* pTargetPawn, C_CSWeaponBaseGun* pBaseGun, Vector vecEnd);

	int MaxPossibleDamageToHitbox(C_CSPlayerPawn* pTargetPawn, int iHitGroup, float iWeaponDamage) {

		auto copy = iWeaponDamage;

		m_Functions.m_ScaleDamage.ScaleDamage(pTargetPawn, LocalPlayerData::m_pWeaponBaseVData, iHitGroup, copy);
		return copy;
	}
};

inline std::unique_ptr<CAutowall> g_Autowall = std::make_unique<CAutowall>();