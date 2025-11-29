#include "../../Precompiled.h"

std::vector<EHitBoxes> CLegitbot::LegitbotHB_To_HB(ELegitbotHitboxes nLegitbotHB)
{
	std::vector<EHitBoxes> vecToReturn;

	switch (nLegitbotHB)
	{
	case LEGITHITBOX_HEAD:
		vecToReturn.push_back(EHitBoxes::HITBOX_HEAD);
		break;
	case LEGITHITBOX_CHEST:
		vecToReturn.push_back(EHitBoxes::HITBOX_CHEST);
		break;
	case LEGITHITBOX_STOMACH:
		vecToReturn.push_back(EHitBoxes::HITBOX_STOMACH);
		break;
	case LEGITHITBOX_PELVIS:
		vecToReturn.push_back(EHitBoxes::HITBOX_PELVIS);
		break;
	case LEGITHITBOX_ARMS:
		vecToReturn.push_back(EHitBoxes::HITBOX_LEFT_FOREARM);
		vecToReturn.push_back(EHitBoxes::HITBOX_RIGHT_FOREARM);
		vecToReturn.push_back(EHitBoxes::HITBOX_LEFT_UPPER_ARM);
		vecToReturn.push_back(EHitBoxes::HITBOX_RIGHT_UPPER_ARM);
		break;
	case LEGITHITBOX_LEGS:
		vecToReturn.push_back(EHitBoxes::HITBOX_LEFT_CALF);
		vecToReturn.push_back(EHitBoxes::HITBOX_RIGHT_CALF);
		vecToReturn.push_back(EHitBoxes::HITBOX_LEFT_THIGH);
		vecToReturn.push_back(EHitBoxes::HITBOX_RIGHT_THIGH);
		break;
	case LEGITHITBOX_FEET:
		vecToReturn.push_back(EHitBoxes::HITBOX_LEFT_FOOT);
		vecToReturn.push_back(EHitBoxes::HITBOX_RIGHT_FOOT);
		break;
	default:
		break;
	}

	return vecToReturn;
}

void CLegitbot::CreateMove()
{
	if (!Config::b(g_Variables.m_Legitbot.m_bEnabled))
		return;

	if (LocalPlayerData::m_pWeapon->m_iClip1() <= 0)
		return;

	if (LocalPlayerData::m_pWeapon->m_nNextPrimaryAttackTick() > Interfaces::m_pGlobalVariables->m_nTickCount)
		return;

	if (Config::vb(g_Variables.m_Legitbot.m_vbConditions).at(ELegitbotConditions::CONDITION_NO_FLASH) &&
		Globals::m_pLocalPlayerPawn->m_flFlashAlpha() > 0.f
		)
		return;

	float flFov = static_cast<float>(Config::i(g_Variables.m_Legitbot.m_iFov)) / 4.f;
	int iSmoothness = Config::i(g_Variables.m_Legitbot.m_iSmoothness);

	LegitbotPlayer_t objBestPlayer = { nullptr, nullptr, { 0, 0, 0 }, 0, INT_MAX };

	for (auto& objPlayer : this->GetPlayers())
	{
		CModel* pModel = objPlayer.m_pPawn->m_pGameSceneNode()->GetSkeletonInstance()->m_modelState().m_modelHandle;
		if (!pModel)
			continue;

		std::vector<CHitBox*> vecHitboxes;

		for (int i = 0; i < ELegitbotHitboxes::LEGITHITBOX_MAX; i++)
		{
			if (!Config::vb(g_Variables.m_Legitbot.m_vbHitboxes).at(i))
				continue;

			auto vecSelectedHitboxes = LegitbotHB_To_HB((ELegitbotHitboxes)i);
			for (auto& nHitboxIndex : vecSelectedHitboxes)
			{
				vecHitboxes.push_back(pModel->GetHitBox(nHitboxIndex));
			}
		}

		GameTrace_t trace = GameTrace_t();
		TraceFilter_t filter = TraceFilter_t(0x1C3003, Globals::m_pLocalPlayerPawn, nullptr, 4);
		Ray_t ray = Ray_t();

		QAngle aBestHitboxAngle = { };
		Vector vBestHitboxPosition = { };
		float flBestHitboxDistance = FLT_MAX;

		for (auto& pHitbox : vecHitboxes)
		{
			Vector vHitboxPos = objPlayer.m_pPawn->m_pGameSceneNode()->GetBonePosition(HitboxToBoneIndex(pHitbox->m_nHitboxIndex));
			QAngle aHitboxAngle = Math::CalcAngle(Globals::m_pLocalPlayerPawn->GetEyePosition(true), vHitboxPos);
			float flDistance = Math::GetFOV(Interfaces::m_pInput->GetViewAngles(), aHitboxAngle);
			if (flDistance < flBestHitboxDistance)
			{
				flBestHitboxDistance = flDistance;
				vBestHitboxPosition = vHitboxPos;
				aBestHitboxAngle = aHitboxAngle;
			}
		}

		if (vBestHitboxPosition.IsZero() || aBestHitboxAngle.IsZero() || vBestHitboxPosition.IsZero())
			continue;

		Interfaces::m_pVPhys2World->TraceShape(&ray, Globals::m_pLocalPlayerPawn->GetEyePosition(), vBestHitboxPosition, &filter, &trace);
		if (trace.m_pHitEntity != objPlayer.m_pPawn)
			continue;

		if (Config::vb(g_Variables.m_Legitbot.m_vbConditions).at(ELegitbotConditions::CONDITION_VISIBLE) &&
			!trace.IsVisible()
			)
			continue;

		if (Config::vb(g_Variables.m_Legitbot.m_vbConditions).at(ELegitbotConditions::CONDITION_NO_SMOKE) &&
			!Utilities::LineGoesThroughSmoke(Globals::m_pLocalPlayerPawn->GetEyePosition(), vBestHitboxPosition, 0.9f))
			continue;

		if (flBestHitboxDistance > flFov)
			continue;
	
		objPlayer.m_iFov_CenterToAimpos = flBestHitboxDistance;
		objPlayer.m_aShootAngle = aBestHitboxAngle;

		if (flBestHitboxDistance < objBestPlayer.m_iFov_CenterToAimpos)
			objBestPlayer = objPlayer;
	}

	if (!objBestPlayer.m_pController || 
		!objBestPlayer.m_pPawn ||
		objBestPlayer.m_aShootAngle.IsZero())
		return;

	QAngle aCurrentAngle = Interfaces::m_pInput->GetViewAngles();
	QAngle aDelta = objBestPlayer.m_aShootAngle - aCurrentAngle;
	QAngle aSmoothedAngle = aCurrentAngle + (aDelta / iSmoothness);

	Interfaces::m_pInput->SetViewAngle(aSmoothedAngle);
}

std::vector<CLegitbot::LegitbotPlayer_t> CLegitbot::GetPlayers()
{
	std::vector<CLegitbot::LegitbotPlayer_t> vecOut;

	for (auto& objEntity : g_Entities->m_vecPlayersOnly)
	{
		CCSPlayerController* pController = (CCSPlayerController*)objEntity.m_pEntity;
		if (!pController)
			continue;

		C_CSPlayerPawn* pPawn = pController->m_hPlayerPawn().Get();
		if (!pPawn)
			continue;

		if (!pController->m_bPawnIsAlive())
			continue;

		if (!Globals::m_pLocalPlayerPawn->IsEnemy(pPawn))
			continue;

		LegitbotPlayer_t objLegitbotPlayer = { };

		objLegitbotPlayer.m_pController = pController;
		objLegitbotPlayer.m_pPawn = pPawn;

		objLegitbotPlayer.m_iHealth = objLegitbotPlayer.m_pPawn->m_iHealth();

		vecOut.emplace_back(objLegitbotPlayer);
	}

	return vecOut;
}
