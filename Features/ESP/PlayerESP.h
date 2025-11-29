#pragma once
inline std::vector<OnShotSkeleton_t> s_vecOnShotSkeletons;

namespace BoneJointList
{
	inline std::vector<DWORD> Trunk = { EBones::NECK,EBones::SPINE, EBones::PELVIS };
	inline std::vector<DWORD> LeftArm = { EBones::NECK,  EBones::LEFT_UPPER_ARM, EBones::LEFT_LOWER_ARM, EBones::LEFT_HAND };
	inline std::vector<DWORD> RightArm = { EBones::NECK, EBones::RIGHT_UPPER_ARM,EBones::RIGHT_LOWER_ARM, EBones::RIGHT_HAND };
	inline std::vector<DWORD> LeftLeg = { EBones::PELVIS, EBones::LEFT_UPPER_LEG , EBones::LEFT_LOWER_LEG, EBones::LEFT_ANKLE };
	inline std::vector<DWORD> RightLeg = { EBones::PELVIS, EBones::RIGHT_UPPER_LEG , EBones::RIGHT_LOWER_LEG, EBones::RIGHT_ANKLE };
	inline std::vector<std::vector<DWORD>> List = { Trunk, LeftArm, RightArm, LeftLeg, RightLeg };
}

enum EPaddingDirection : unsigned int
{
	DIR_LEFT = 0,
	DIR_TOP,
	DIR_RIGHT,
	DIR_BOTTOM,
	DIR_MAX
};

struct Box_t
{
	float left = 0.f, top = 0.f, right = 0.f, bottom = 0.f, width = 0.f, height = 0.f;
};

struct Context_t
{
	Box_t m_Box = { };
	std::array<float, DIR_MAX> m_arrPadding = { 0, 0, 0, 0 };
};

struct FlagObjects_t
{
	FlagObjects_t(bool bHelmet, bool bDefuser, bool bScoping, bool bDefusing, bool bFlashed, int iMoney)
	{
		this->m_bHasHelmet = bHelmet;
		this->m_bHasDefuser = bDefuser;
		this->m_bIsScoping = bScoping;
		this->m_bIsDefusing = bDefusing;
		this->m_bFlashed = bFlashed;

		this->m_iMoney = iMoney;
	}

	bool m_bHasHelmet = false;
	bool m_bHasHeavyArmor = false;
	bool m_bHasDefuser = false;
	bool m_bIsScoping = false;
	bool m_bIsDefusing = false;
	bool m_bFlashed = false;

	int m_iMoney = 0;
};

struct hit_marker_t {
	double time;
	float alpha;
	float scale;
};

struct DamageIndicator_t {
	int m_iDamage;
	bool m_bInitialized; 
	float m_flEraseTime;
	float m_flLastUpdate;
	C_CSPlayerPawn* m_pPlayer;
	Vector m_vecPosition;
	ImVec2 m_vecScreenPos;
};

struct DroppedWeapon_t {
	C_CSWeaponBaseGun* weapon;
	bool is_invalid;
	CBaseHandle handle;
	Vector abs_origin;
	Vector mins;
	Vector maxs;
	std::string weapon_name;
	int item_index;
};

namespace PlayerESP
{
	void Run(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, CRecordTrack* pRecordTrack);

	void DrawHealthBar(C_CSPlayerPawn* pPawn, ImVec2 vecMin, ImVec2 vecMax, Color colColor, Color colOutline);

	void DrawArmorBar(C_CSPlayerPawn* pPawn, ImVec2 vecMin, ImVec2 vecMax, Color colColor, Color colOutline);

	void DrawName(CCSPlayerController* pEntity, ImVec2 vecPosition, Color colColor, Color colOutline);

	void DrawWeapon(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, ImVec2 vecPosition, Color colColor, Color colOutline);

	void DrawFlags(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, ImVec2 vecPosition, Color colColor, Color colOutline);

	void DrawWeaponIcon(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, ImVec2 vecPosition, Color colColor, Color colOutline);

	void DrawSkeleton(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, ImVec2 vecMin, ImVec2 vecMax, Color colColor, Color colOutline, BoneData_t* bones);

	void DrawBox(CCSPlayerController* pEntity, ImVec2 vecMin, ImVec2 vecMax, Color colColor, Color colOutline);

	inline std::vector<hit_marker_t> m_hit_markers;
	inline std::vector<DamageIndicator_t> m_vecDamageIndicators;

	void HitMarker();

	void DrawDamageIndicators();

	void OnShotSkeleton();

	inline unsigned int m_uIndex = 0U;
	inline Context_t ctx = { };
}