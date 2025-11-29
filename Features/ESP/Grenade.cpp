#include "../../Precompiled.h"

struct Grenade_t
{
	C_BaseEntity* entity;
	std::vector<Vector> positions;
	float addTime;
};

class CGrenade
{
public:
	bool CheckGrenades(C_BaseEntity* ent)
	{
		for (Grenade_t grenade : vecGrenades)
		{
			if (grenade.entity == ent)
				return false;
		}

		return true;
	}
	void AddGrenade(Grenade_t grenade)
	{
		vecGrenades.push_back(grenade);
	}

	void UpdatePosition(C_BaseEntity* ent, Vector position)
	{
		vecGrenades.at(FindGrenade(ent)).positions.push_back(position);
	}

	void Draw();
private:
	std::vector<Grenade_t> vecGrenades;
	int FindGrenade(C_BaseEntity* ent)
	{
		for (size_t i = 0; i < vecGrenades.size(); i++)
		{
			if (vecGrenades.at(i).entity == ent)
				return i;
		}

		return 0;
	}
};

void CGrenade::Draw()
{
	for (size_t i = 0; i < vecGrenades.size(); i++)
	{
		if (vecGrenades.at(i).addTime + 20.f < Interfaces::m_pGlobalVariables->m_flRealTime)
		{
			continue;
		}
		if (vecGrenades.at(i).addTime + 2.5f < Interfaces::m_pGlobalVariables->m_flRealTime)
		{
			if (vecGrenades.at(i).positions.size() < 1) continue;

			vecGrenades.at(i).positions.erase(vecGrenades.at(i).positions.begin());
		}

		for (size_t j = 1; j < vecGrenades.at(i).positions.size(); j++)
		{
			ImVec2 imSPosition;
			ImVec2 imSLastPosition;

			bool bResult1 = Draw::WorldToScreen(vecGrenades.at(i).positions.at(j), imSPosition);
			bool bResult2 = Draw::WorldToScreen(vecGrenades.at(i).positions.at(j - 1), imSLastPosition);

			if (bResult1 && bResult2)
				Draw::AddLine(imSPosition, imSLastPosition, Color(255,255,255));
		}
	}
}

void GrenadeESP::Draw(EntityObject_t& object)
{
	static CGrenade GrenadeClass;

	C_BaseEntity* pEntity = object.m_pEntity;
	if (!pEntity)
		return;

	CGameSceneNode* pGameSceneNode = pEntity->m_pGameSceneNode();
	if (!pGameSceneNode || !pGameSceneNode->GetSkeletonInstance())
		return;

	Vector pos = pGameSceneNode->m_vecAbsOrigin();
	ImVec2 vecScreen;
	Draw::WorldToScreen(pos, vecScreen);
	SchemaClassInfoData_t* grenadeSchema;
	pEntity->GetSchemaClassInfo(&grenadeSchema);

	const char* szGrenadeName = "UNKNOWN";
	FNV1A_t uHashedName = FNV1A::Hash(grenadeSchema->szName);

	if (uHashedName == FNV1A::HashConst("C_SmokeGrenadeProjectile"))
		szGrenadeName = "SMOKE";
	else if (uHashedName == FNV1A::HashConst("C_FlashbangProjectile"))
		szGrenadeName = "FLASHBANG";
	else if (uHashedName == FNV1A::HashConst("C_HEGrenadeProjectile"))
		szGrenadeName = "HE GRENADE";
	else if (uHashedName == FNV1A::HashConst("C_MolotovProjectile"))
		szGrenadeName = "MOLOTOV";
	else if (uHashedName == FNV1A::HashConst("C_DecoyProjectile"))
		szGrenadeName = "DECOY";
	else if (uHashedName == FNV1A::HashConst("C_Inferno"))
		szGrenadeName = "INFERNO";

	if (Config::b(g_Variables.m_WorldVisuals.m_bDrawGrenadeNames))
		Draw::AddText(Fonts::ESP, 10.0f, vecScreen, szGrenadeName, Color(255, 255, 255, 255), DRAW_TEXT_OUTLINE);

	if (Config::b(g_Variables.m_WorldVisuals.m_bDrawGrenadeTracers))
	{
		if (GrenadeClass.CheckGrenades(pEntity))
		{
			Grenade_t grenade;
			grenade.entity = pEntity;
			grenade.addTime = Interfaces::m_pGlobalVariables->m_flRealTime;

			GrenadeClass.AddGrenade(grenade);
		}
		else
		{
			GrenadeClass.UpdatePosition(pEntity, pos);
		}

		GrenadeClass.Draw();
	}
}

void GrenadeESP::CreateMove()
{

}
