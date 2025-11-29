#pragma once

class CLegitbot
{
public:
	struct LegitbotPlayer_t
	{
		CCSPlayerController* m_pController;
		C_CSPlayerPawn* m_pPawn;

		QAngle m_aShootAngle;

		int m_iHealth;
		int m_iFov_CenterToAimpos;
	};
public:
	std::vector<EHitBoxes> LegitbotHB_To_HB(ELegitbotHitboxes nLegitbotHB);

	void CreateMove();
	std::vector<LegitbotPlayer_t> GetPlayers();
};
inline const auto g_Legitbot = std::make_unique<CLegitbot>();