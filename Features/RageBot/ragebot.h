#pragma once
#include <vector>
#include "../../Precompiled.h"

namespace NRagebot {

	inline Vector m_vShootPos = {};
	inline Vector m_vExtrapShootPos = {};
	inline bool m_bHoldingMinOverride = false;
	inline bool m_bHoldingHitchanceOverride = false;

	struct ScanResult_t {
		CLagRecord* m_pLagRecord = nullptr;
		HitboxData_t m_hitboxData = {};
		Vector m_vPoint = Vector(0, 0, 0);
		float m_flWorldDist = FLT_MAX;
		float m_flDamage = -1;
		int m_iPlayerMinDmg = 0;
		float m_flDamageThreshold = 0.f;
		QAngle m_angToTarget = QAngle(0, 0, 0);

		bool HasValidTarget() {
			return this->m_flDamage >= m_flDamageThreshold && m_pLagRecord && !m_vPoint.IsZero();
		}
	};

	inline std::vector<Vector> debugPoints = {};

	class CMultipointManager {
	public:
		std::vector<Vector> GenerateMultipoints(HitboxData_t hitbox);
	private:
		std::vector<Vector> HeadPoints(HitboxData_t head);
		std::vector<Vector> MinimalPoints(HitboxData_t hitbox);
	};

	class CAccuracy {
	public:
		bool IsAccurate();
		void CacheSpreadTable();
		bool HasMaximumAccuracy( );
		Vector2D m_SpreadTable[512];
		float m_LastInaccuracy = -1.f;
		float m_LastSpread = -1.f;
		float m_LastRecoilIndex = -1.f;
	};

	class CScan {
	public:
		bool IsBetterCandidate(ScanResult_t candidate, ScanResult_t current);
		bool IsWithinFov(const Vector& vStartPos, const Vector& vTargetPos, const QAngle& angView, float flFovDegrees);
		ScanResult_t ProcessRecord(CLagRecord* pRecord);
		ScanResult_t ProcessTarget(CRecordTrack* pRecordTrack);
		ScanResult_t GetBestTarget();
		int m_nLastSequenceScan;
	};

	class CRage {
	public:
		struct RageData_t {
			bool m_bShouldInvalidateAngle = false;
			bool m_bIsAccurate = false;
			bool m_bWants2DMoveHalt = false;
			ScanResult_t m_LastTarget = {};
		};

		bool m_bBadWeapon = false;
		RageData_t m_Data = { };

		bool SetConfig();
		void HandleKeybinds();
		void OverrideInputHistory();
		void ConsiderAttack();
		bool AddAttack(CUserCmd* pCmd);
		void RemoveAttack(CUserCmd* pCmd);
		void OnCreateMove();
		void KnifeBot();
		void AutoRevolver();
		void ZeusBot();

		bool m_bForceHeadshot = false;
		bool m_bForceBodyaim = false;
	};

	inline auto g_Points = std::make_unique<CMultipointManager>();
	inline auto g_Accuracy = std::make_unique<CAccuracy>();
	inline auto g_Scan = std::make_unique<CScan>();
	inline auto g_Rage = std::make_unique<CRage>();
}

#define AUTOWALL_POINT(pPawn, vPoint, pen_data) g_Autowall->m_Functions.m_FireBullet.FireBullet(NRagebot::m_vExtrapShootPos, vPoint, pen_data, pPawn)


