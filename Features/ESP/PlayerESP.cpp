#include "../../Precompiled.h"
#include <unordered_map>

inline bool GetEntityBoundingBox(C_BaseEntity* pEntity, Box_t* pBox)
{
	if (!pEntity)
		return false;

	CCollisionProperty* pCollision = pEntity->m_pCollision();
	if (pCollision == nullptr)
		return false;

	CGameSceneNode* pGameSceneNode = pEntity->m_pGameSceneNode();
	if (pGameSceneNode == nullptr)
		return false;

	const CTransform nodeToWorldTransform = pGameSceneNode->m_nodeToWorld();
	const Matrix3x4_t matTransform = nodeToWorldTransform.quatOrientation.ToMatrix(nodeToWorldTransform.vecPosition);

	const Vector vecMins = pCollision->m_vecMins();
	const Vector vecMaxs = pCollision->m_vecMaxs();

	float flLeft = std::numeric_limits<float>::max();
	float flTop = std::numeric_limits<float>::max();
	float flRight = std::numeric_limits<float>::lowest();
	float flBottom = std::numeric_limits<float>::lowest();

	for (int i = 0; i < 8; ++i)
	{
		const Vector vecPoint
		{
			i & 1 ? vecMaxs.x : vecMins.x,
			i & 2 ? vecMaxs.y : vecMins.y,
			i & 4 ? vecMaxs.z : vecMins.z
		};

		const Vector vecTransformed = vecPoint.Transform(matTransform);
		ImVec2 vecScreen;
		if (!Draw::WorldToScreen(vecTransformed, vecScreen))
			return false;

		flLeft = std::min(flLeft, vecScreen.x);
		flTop = std::min(flTop, vecScreen.y);
		flRight = std::max(flRight, vecScreen.x);
		flBottom = std::max(flBottom, vecScreen.y);
	}

	pBox->left = flLeft;
	pBox->top = flTop;
	pBox->right = flRight;
	pBox->bottom = flBottom;
	pBox->width = flRight - flLeft;
	pBox->height = flBottom - flTop;
	return true;
}

ImVec2 RotateVertex(const Vector& p, const ImVec2& v, float angle)
{
	float c = std::cos(M_DEG2RAD(angle));
	float s = std::sin(M_DEG2RAD(angle));

	return {
		p.x + (v.x - p.x) * c - (v.y - p.y) * s,
		p.y + (v.x - p.x) * s + (v.y - p.y) * c
	};
}

void DrawOOF(C_CSPlayerPawn* pPawn) {

	if (!pPawn->m_pGameSceneNode())
		return;

	ImColor triangleColor = Config::c(g_Variables.m_Visuals.m_colOOFArrows).GetVec4();

	float baseSize = 8.0f;
	float screenDistance = 150.0f;

	Vector vecViewOrigin, vecTargetPos, vecDelta;
	ImVec2 vecScreenPos;
	Vector vecOffScreenPos;
	float flLeeWayX, flLeeWayY, flRadius, flOffScreenRotation;
	bool bIsOnScreen;
	std::array<ImVec2, 3> arrVecVerts;

	auto GetOffScreenData = [](const Vector& vecDelta, float flRadiusX, float flRadiusY, Vector& vecOutOffScreenPos, float& flOutRot) {
		QAngle view_angles(Interfaces::m_pInput->GetViewAngles());
		Vector fwd, right, up(0.f, 0.f, 1.f);

		Math::AngleVector(view_angles, fwd);
		fwd.z = 0.f;
		fwd.Normalize();

		right = up.CrossProduct(fwd);
		float front = vecDelta.DotProduct(fwd);
		float side = vecDelta.DotProduct(right);

		vecOutOffScreenPos.x = flRadiusX * -side;
		vecOutOffScreenPos.y = flRadiusY * -front;

		flOutRot = M_RAD2DEG(std::atan2(vecOutOffScreenPos.x, vecOutOffScreenPos.y) + M_PI);

		float yaw_rad = M_DEG2RAD(-flOutRot);
		float sa = std::sin(yaw_rad);
		float ca = std::cos(yaw_rad);

		vecOutOffScreenPos.x = (ImGui::GetIO().DisplaySize.x / 2.f) + (flRadiusX * sa);
		vecOutOffScreenPos.y = (ImGui::GetIO().DisplaySize.y / 2.f) - (flRadiusY * ca);
		};

	vecTargetPos = pPawn->m_pGameSceneNode()->m_vecAbsOrigin();
	bIsOnScreen = Draw::WorldToScreen(vecTargetPos, vecScreenPos);

	flLeeWayX = ImGui::GetIO().DisplaySize.x / 18.f;
	flLeeWayY = ImGui::GetIO().DisplaySize.y / 18.f;

	if (!bIsOnScreen ||
		vecScreenPos.x < -flLeeWayX ||
		vecScreenPos.x >(ImGui::GetIO().DisplaySize.x + flLeeWayX) ||
		vecScreenPos.y < -flLeeWayY ||
		vecScreenPos.y >(ImGui::GetIO().DisplaySize.y + flLeeWayY)) {

		vecViewOrigin = Globals::m_pLocalPlayerPawn->m_pGameSceneNode()->m_vecAbsOrigin();
		vecDelta = (vecTargetPos - vecViewOrigin).Normalized();

		float dist = (vecTargetPos - vecViewOrigin).Length();
		float triangleSize = baseSize * std::clamp(600.f / dist, 0.8f, 1.2f);

		flRadius = screenDistance * (ImGui::GetIO().DisplaySize.y / 480.f);

		float flRadiusX = screenDistance * static_cast<float>(Config::i(g_Variables.m_Visuals.m_bOOFArrowsWidth)) / 100.f;
		float flRadiusY = screenDistance * static_cast<float>(Config::i(g_Variables.m_Visuals.m_bOOFArrowsHeight)) / 100.f;

		GetOffScreenData(vecDelta, flRadiusX, flRadiusY, vecOffScreenPos, flOffScreenRotation);

		flOffScreenRotation = -flOffScreenRotation;

		float height = triangleSize * 1.732f;

		arrVecVerts[0] = { vecOffScreenPos.x, vecOffScreenPos.y - triangleSize };
		arrVecVerts[1] = { vecOffScreenPos.x - triangleSize, vecOffScreenPos.y + (height / 2) };
		arrVecVerts[2] = { vecOffScreenPos.x + triangleSize, vecOffScreenPos.y + (height / 2) };

		for (auto& vert : arrVecVerts)
			vert = RotateVertex(vecOffScreenPos, vert, flOffScreenRotation);

		std::vector<ImVec2> vecPoints(arrVecVerts.begin(), arrVecVerts.end());
		Draw::AddPolygon(vecPoints, Config::c(g_Variables.m_Visuals.m_colOOFArrows));
	}
}

void PlayerESP::Run(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, CRecordTrack* pRecordTrack)
{

	if (!Config::b(g_Variables.m_Visuals.m_bEnableESP))
		return;
	if (!pEntity || !pPawn)
		return;
	if (pEntity == Globals::m_pLocalPlayerController)
		return;
	if (!Globals::m_pLocalPlayerPawn->IsEnemy(pPawn))
		return;
	if (!pEntity->m_bPawnIsAlive())
		return;

	if (Config::b(g_Variables.m_Visuals.m_bEnableOOFArrows) && Globals::m_pLocalPlayerController->m_bPawnIsAlive())
		DrawOOF(pPawn);

	ctx = { };

	if (!GetEntityBoundingBox(pPawn, &ctx.m_Box))
		return;

	const float boxMidX = ctx.m_Box.left + (ctx.m_Box.width * 0.5f);
	const float boxBottom = ctx.m_Box.bottom;
	const float bottomPadding = ctx.m_arrPadding.at(DIR_BOTTOM);
	const bool isImmune = pPawn->m_bGunGameImmunity();

	const Color colBoxESP = Config::c(g_Variables.m_Visuals.m_colBoxESP);
	const Color colESPOutline = Config::c(g_Variables.m_Visuals.m_colESPOutline);
	const Color colSkeletonESP = Config::c(g_Variables.m_Visuals.m_colSkeletonEsp);
	const Color colBacktrackSkeletonESP = Config::c(g_Variables.m_Visuals.m_colBacktrackSkeletonEsp);
	const Color colNameESP = Config::c(g_Variables.m_Visuals.m_colNameEsp);
	const Color colWeaponESP = Config::c(g_Variables.m_Visuals.m_colWeaponEsp);
	const Color colWeaponIconESP = Config::c(g_Variables.m_Visuals.m_colWeaponIcon);
	const Color colArmorBar = Config::c(g_Variables.m_Visuals.m_colArmorBar);
	const Color white(255, 255, 255, 255);

	const ImVec2 boxTopLeft(ctx.m_Box.left, ctx.m_Box.top);
	const ImVec2 boxBottomRight(ctx.m_Box.right, ctx.m_Box.bottom);

	BoneData_t* pBoneArray = nullptr;
	if (Config::b(g_Variables.m_Visuals.m_bEnableSkeletonESP)) {
		pBoneArray = pPawn->m_pGameSceneNode()->GetSkeletonInstance()->m_modelState().m_pBoneArray;
	}

	CLagRecord* pFirstRecord = nullptr;
	CLagRecord* pLastRecord = nullptr;
	if (Config::b(g_Variables.m_Visuals.m_bEnableBacktrackSkeletonESP)) {
		if (!pRecordTrack->m_deqRecords.empty()) {
			pFirstRecord = pRecordTrack->GetFirstRecord();
			pLastRecord = pRecordTrack->GetLastRecord();
		}
	}

	const float baseOffset = 4.0f;
	const float pingNameGap = 8.0f;

	if (Config::vb(g_Variables.m_Visuals.m_vecFlags).at(FLAG_PING) && Config::b(g_Variables.m_Visuals.m_bEditFlags))
	{
		const int iPing = pEntity->m_iPing();
		Color pingColor;

		if (iPing <= 20) 
			pingColor = Color(98, 217, 109, 255);
		else if (iPing <= 50)
			pingColor = Color(230, 206, 137);
		else if (iPing <= 70) 
			pingColor = Color(230, 156, 110, 255);
		else 
			pingColor = Color(222, 59, 59, 255);

		const std::string strPing = std::to_string(iPing) + "ms";
		const ImVec2 vecPingSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.0f, strPing.c_str());

		float pingYPos = ctx.m_Box.top - vecPingSize.y - baseOffset;

		if (Config::b(g_Variables.m_Visuals.m_bEnableNameESP)) {
			pingYPos -= pingNameGap;
		}

		Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, ImVec2(boxMidX - vecPingSize.x * 0.5f, pingYPos), strPing.c_str(), pingColor, DRAW_TEXT_DROPSHADOW, colESPOutline);

		ctx.m_arrPadding.at(DIR_TOP) += vecPingSize.y + (Config::b(g_Variables.m_Visuals.m_bEnableNameESP) ? pingNameGap : baseOffset);
	}

	if (Config::b(g_Variables.m_Visuals.m_bEnableBoxESP))
		DrawBox(pEntity, boxTopLeft, boxBottomRight, colBoxESP, Color(0, 0, 0, 170));

	if (Config::b(g_Variables.m_Visuals.m_bEnableSkeletonESP) && pBoneArray)
		DrawSkeleton(pEntity, pPawn, boxTopLeft, boxBottomRight, colSkeletonESP, colESPOutline, pBoneArray);

	if (Config::b(g_Variables.m_Visuals.m_bEnableBacktrackSkeletonESP) && pFirstRecord && pLastRecord)
	{
		float flDelta = (pFirstRecord->m_Bones->vecPosition - pLastRecord->m_Bones->vecPosition).Length2D();
		if (flDelta > 0.18f)
		{
			DrawSkeleton(pEntity, pPawn, boxTopLeft, boxBottomRight, colBacktrackSkeletonESP, colESPOutline, pLastRecord->m_Bones);
		}
	}

	if (Config::b(g_Variables.m_Visuals.m_bEnableHealthESP)) {
		const ImVec2 healthBarMin(ctx.m_Box.left - 6.0f, ctx.m_Box.top - 1.0f);
		const ImVec2 healthBarMax(ctx.m_Box.left - 2.0f, ctx.m_Box.bottom + 1.0f);
		DrawHealthBar(pPawn, healthBarMin, healthBarMax, white, Color(0, 0, 0, 170));
	}

	if (Config::b(g_Variables.m_Visuals.m_bEnableArmorESP)) {
		const float leftPadding = ctx.m_arrPadding.at(DIR_LEFT);
		const float boxBottom = ctx.m_Box.bottom;
		const float barHeight = 2.0f;
		const float barWidth = ctx.m_Box.width;

		const ImVec2 armorBarMin(ctx.m_Box.left, boxBottom + 2.0f);
		const ImVec2 armorBarMax(ctx.m_Box.left + barWidth, boxBottom + 2.0f + barHeight);

		DrawArmorBar(pPawn, armorBarMin, armorBarMax, colArmorBar, colESPOutline);
		ctx.m_arrPadding.at(DIR_BOTTOM) += barHeight + 2.0f;
	}

	if (Config::b(g_Variables.m_Visuals.m_bEnableNameESP))
	{
		const std::string strName = pEntity->m_sSanitizedPlayerName();

		bool hasNonAscii = false;
		for (char c : strName) {
			if (static_cast<unsigned char>(c) > 127) {
				hasNonAscii = true;
				break;
			}
		}

		std::string displayName = hasNonAscii ? "xxx" : strName;

		const ImVec2 vecNameSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.0f, displayName.c_str());

		Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, ImVec2(boxMidX - vecNameSize.x * 0.5f, ctx.m_Box.top - baseOffset - vecNameSize.y), displayName.c_str(), colNameESP, DRAW_TEXT_DROPSHADOW, colESPOutline);

		ctx.m_arrPadding.at(DIR_TOP) += vecNameSize.y + baseOffset;
	}

	const ImVec2 weaponPos(boxMidX, boxBottom + bottomPadding);
	if (Config::b(g_Variables.m_Visuals.m_bEnableWeaponESP))
		DrawWeapon(pEntity, pPawn, weaponPos, colWeaponESP, colESPOutline);

	if (Config::b(g_Variables.m_Visuals.m_bEnableWeaponIconESP))
		DrawWeaponIcon(pEntity, pPawn, weaponPos, colWeaponIconESP, colESPOutline);

	DrawFlags(pEntity, pPawn, ImVec2(ctx.m_Box.right + 2.0f, ctx.m_Box.top), white, colESPOutline);
}

static std::unordered_map<void*, float> healthAnimationMap;

void PlayerESP::DrawHealthBar(C_CSPlayerPawn* pPawn, ImVec2 vecMin, ImVec2 vecMax, Color colColor, Color colOutline)
{
	void* entityKey = static_cast<void*>(pPawn);

	float& currentHealth = healthAnimationMap[entityKey];
	if (currentHealth == 0.0f)
		currentHealth = static_cast<float>(pPawn->m_iHealth());

	const int targetHealth = pPawn->m_iHealth();
	const float targetHealthFactor = static_cast<float>(targetHealth) * 0.01f;

	const float healthTransitionSpeed = 8.0f;
	currentHealth += (static_cast<float>(targetHealth) - currentHealth) * std::min(ImGui::GetIO().DeltaTime * healthTransitionSpeed, 1.0f);

	const float currentHealthFactor = std::max(currentHealth * 0.01f, 0.0f);
	const float flHue = (currentHealthFactor * 120.f) / 360.f;
	const float flHeight = vecMax.y - vecMin.y;

	const ImVec2 healthBarMin = ImVec2(vecMin.x + 1.0f, (vecMax.y + 1.0f) - flHeight * currentHealthFactor);
	const ImVec2 healthBarMax = ImVec2(vecMax.x - 1.0f, vecMax.y - 1.0f);

	Draw::AddRect(vecMin, vecMax, Color(30, 30, 30, 100), DRAW_RECT_FILLED | DRAW_RECT_ALIGNED, colOutline);

	Draw::AddRect(healthBarMin, healthBarMax, Config::c(g_Variables.m_Visuals.m_colHealthBarEsp), DRAW_RECT_FILLED | DRAW_RECT_ALIGNED, colOutline);

	ctx.m_arrPadding.at(DIR_LEFT) += 7.0f;

	if (targetHealth <= 0)
		healthAnimationMap.erase(entityKey);
}

void PlayerESP::DrawArmorBar(C_CSPlayerPawn* pPawn, ImVec2 vecMin, ImVec2 vecMax, Color colColor, Color colOutline)
{
	const int iArmor = pPawn->m_ArmorValue();
	const float flFactor = static_cast<float>(iArmor) * 0.01f;
	const float flWidth = vecMax.x - vecMin.x;
	const float flFilledWidth = flWidth * flFactor;

	Draw::AddRect(vecMin, vecMax, Color(40, 40, 40, 100), DRAW_RECT_FILLED | DRAW_RECT_ALIGNED, colOutline);

	ImVec2 filledMax = ImVec2(vecMin.x + flFilledWidth, vecMax.y);

	Color startColor = Config::c(g_Variables.m_Visuals.m_colArmorBar);
	Color endColor = Config::c(g_Variables.m_Visuals.m_colArmorBarGradient);

	if (g_Variables.m_Visuals.m_bArmorBarGradient)
	{
		const int gradientSteps = 16;
		const float segmentWidth = flFilledWidth / gradientSteps;

		for (int i = 0; i < gradientSteps; i++)
		{
			float progress = static_cast<float>(i) / (gradientSteps - 1);
			Color segmentColor = Color(
				static_cast<uint8_t>(startColor.r() + (endColor.r() - startColor.r()) * progress),
				static_cast<uint8_t>(startColor.g() + (endColor.g() - startColor.g()) * progress),
				static_cast<uint8_t>(startColor.b() + (endColor.b() - startColor.b()) * progress),
				static_cast<uint8_t>(startColor.a() + (endColor.a() - startColor.a()) * progress)
			);

			ImVec2 segmentMin = ImVec2(
				vecMin.x + i * segmentWidth,
				vecMin.y
			);

			ImVec2 segmentMax = ImVec2(
				vecMin.x + (i + 1) * segmentWidth,
				vecMax.y
			);

			if (i == gradientSteps - 1)
				segmentMax.x = filledMax.x;

			Draw::AddRect(segmentMin, segmentMax, segmentColor, DRAW_RECT_FILLED | DRAW_RECT_ALIGNED);
		}
	}
	else
	{
		Draw::AddRect(vecMin, filledMax, startColor, DRAW_RECT_FILLED | DRAW_RECT_ALIGNED);
	}
}

void PlayerESP::DrawWeapon(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, ImVec2 vecPosition, Color colColor, Color colOutline)
{
	C_BasePlayerWeapon* pActiveWeapon = pPawn->m_pWeaponServices()->m_hActiveWeapon().Get();
	if (!pActiveWeapon)
		return;

	C_AttributeContainer* pAttributeContainer = pActiveWeapon->m_AttributeManager();
	if (!pAttributeContainer)
		return;

	C_EconItemView* pEconItemView = pAttributeContainer->m_Item();
	if (!pEconItemView)
		return;

	CEconItemDefinition* pEconItemDefinition = pEconItemView->GetStaticData();
	if (!pEconItemDefinition)
		return;

	std::string strWeaponName = Interfaces::m_pLocalize->FindSafe(pEconItemDefinition->m_pszItemBaseName());
	const ImVec2 vecNameSize = Fonts::ESP->CalcTextSizeA(10.0f, FLT_MAX, 0.0f, strWeaponName.c_str());
	Draw::AddText(Fonts::ESP, 10.0f, ImVec2(vecPosition.x - vecNameSize.x * 0.5f - 0.75f, vecPosition.y + 2.0f), strWeaponName.c_str(), colColor, DRAW_TEXT_DROPSHADOW, colOutline);

	float offsetY = +20.f;


	ctx.m_arrPadding.at(DIR_BOTTOM) += vecNameSize.y - offsetY;
}

void PlayerESP::DrawFlags(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, ImVec2 vecPosition, Color colColor, Color colOutline)
{
	CCSPlayer_ItemServices* pItemServices = pPawn->m_pItemServices();
	CCSPlayerController_InGameMoneyServices* pInGameMoneyServices = pEntity->m_pInGameMoneyServices();
	if (!pItemServices || !pInGameMoneyServices)
		return;

	if (!Config::b(g_Variables.m_Visuals.m_bEditFlags))
		return;

	int iFlagsHeight = 0;
	bool bIsFlashed = pPawn->m_flFlashAlpha() > 0.0f;
	FlagObjects_t flagObjects{ pItemServices->m_bHasHelmet(), pItemServices->m_bHasDefuser(), pPawn->m_bIsScoped(), pPawn->m_bIsDefusing(), bIsFlashed, pInGameMoneyServices->m_iAccount() };

	if (Config::vb(g_Variables.m_Visuals.m_vecFlags).at(FLAG_MONEY))
	{
		const std::string strText = std::to_string(flagObjects.m_iMoney).append(X("$"));
		const ImVec2 vecTextSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.0f, strText.c_str());
		Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, ImVec2(vecPosition.x, vecPosition.y + iFlagsHeight), strText.c_str(), Color(141, 227, 66, 255), DRAW_TEXT_OUTLINE, colOutline);
		iFlagsHeight += vecTextSize.y - 2.0f;
	}

	if (Config::Get<std::vector<bool>>(g_Variables.m_Visuals.m_vecFlags).at(FLAG_ARMOR) && pPawn->IsArmored(HITGROUP_CHEST))
	{
		const std::string strText = flagObjects.m_bHasHelmet ? X("HK") : X("K");
		const ImVec2 vecTextSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.0f, strText.c_str());
		Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, ImVec2(vecPosition.x, vecPosition.y + iFlagsHeight), strText.c_str(), colColor, DRAW_TEXT_OUTLINE, colOutline);
		iFlagsHeight += vecTextSize.y - 2.0f;
	}

	if (Config::Get<std::vector<bool>>(g_Variables.m_Visuals.m_vecFlags).at(FLAG_KIT) && flagObjects.m_bHasDefuser)
	{
		const std::string strText = X("KIT");
		const ImVec2 vecTextSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.0f, strText.c_str());
		Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, ImVec2(vecPosition.x, vecPosition.y + iFlagsHeight), strText.c_str(), Color(63, 151, 224, 255), DRAW_TEXT_OUTLINE, colOutline);
		iFlagsHeight += vecTextSize.y - 2.0f;
	}

	if (Config::Get<std::vector<bool>>(g_Variables.m_Visuals.m_vecFlags).at(FLAG_DEFUSING) && flagObjects.m_bIsDefusing)
	{
		const std::string strText = X("DEFUSING");
		const ImVec2 vecTextSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.0f, strText.c_str());
		Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, ImVec2(vecPosition.x, vecPosition.y + iFlagsHeight), strText.c_str(), Color(63, 151, 224, 255), DRAW_TEXT_OUTLINE, colOutline);
		iFlagsHeight += vecTextSize.y - 2.0f;
	}

	if (Config::Get<std::vector<bool>>(g_Variables.m_Visuals.m_vecFlags).at(FLAG_ZOOM) && flagObjects.m_bIsScoping)
	{
		const std::string strText = X("ZOOM");
		const ImVec2 vecTextSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.0f, strText.c_str());
		Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, ImVec2(vecPosition.x, vecPosition.y + iFlagsHeight), strText.c_str(), colColor, DRAW_TEXT_OUTLINE, colOutline);
		iFlagsHeight += vecTextSize.y - 2.0f;
	}
}

constexpr const char* WeaponToFontLetter(const char* weapon) {

	if (weapon == nullptr)
		return "";

	if (strcmp(weapon, "weapon_knife_ct") == 0) return "]";
	if (strcmp(weapon, "weapon_knife_t") == 0) return "[";
	if (strcmp(weapon, "weapon_knife") == 0) return "]";
	if (strcmp(weapon, "knife") == 0) return "]";

	if (strcmp(weapon, "weapon_deagle") == 0) return "A";
	if (strcmp(weapon, "weapon_elite") == 0) return "B";
	if (strcmp(weapon, "weapon_fiveseven") == 0) return "C";
	if (strcmp(weapon, "weapon_glock") == 0) return "D";
	if (strcmp(weapon, "weapon_revolver") == 0) return "J";
	if (strcmp(weapon, "weapon_hkp2000") == 0) return "E";
	if (strcmp(weapon, "weapon_p250") == 0) return "F";
	if (strcmp(weapon, "weapon_usp_silencer") == 0) return "G";
	if (strcmp(weapon, "weapon_tec9") == 0) return "H";
	if (strcmp(weapon, "weapon_cz75a") == 0) return "I";

	if (strcmp(weapon, "weapon_mac10") == 0) return "K";
	if (strcmp(weapon, "weapon_ump45") == 0) return "L";
	if (strcmp(weapon, "weapon_bizon") == 0) return "M";
	if (strcmp(weapon, "weapon_mp7") == 0) return "N";
	if (strcmp(weapon, "weapon_mp9") == 0) return "R";
	if (strcmp(weapon, "weapon_p90") == 0) return "O";

	if (strcmp(weapon, "weapon_galilar") == 0) return "Q";
	if (strcmp(weapon, "weapon_famas") == 0) return "R";
	if (strcmp(weapon, "weapon_m4a1_silencer") == 0) return "T";
	if (strcmp(weapon, "weapon_m4a1") == 0) return "S";
	if (strcmp(weapon, "weapon_aug") == 0) return "U";
	if (strcmp(weapon, "weapon_sg556") == 0) return "V";
	if (strcmp(weapon, "weapon_ak47") == 0) return "W";

	if (strcmp(weapon, "weapon_g3sg1") == 0) return "X";
	if (strcmp(weapon, "weapon_scar20") == 0) return "Y";
	if (strcmp(weapon, "weapon_awp") == 0) return "Z";
	if (strcmp(weapon, "weapon_ssg08") == 0) return "a";

	if (strcmp(weapon, "weapon_xm1014") == 0) return "b";
	if (strcmp(weapon, "weapon_sawedoff") == 0) return "c";
	if (strcmp(weapon, "weapon_mag7") == 0) return "d";
	if (strcmp(weapon, "weapon_nova") == 0) return "e";

	if (strcmp(weapon, "weapon_negev") == 0) return "f";
	if (strcmp(weapon, "weapon_m249") == 0) return "g";

	if (strcmp(weapon, "weapon_taser") == 0) return "h";

	if (strcmp(weapon, "weapon_flashbang") == 0) return "i";
	if (strcmp(weapon, "weapon_hegrenade") == 0) return "j";
	if (strcmp(weapon, "weapon_smokegrenade") == 0) return "k";
	if (strcmp(weapon, "weapon_molotov") == 0) return "l";
	if (strcmp(weapon, "weapon_decoy") == 0) return "m";
	if (strcmp(weapon, "weapon_incgrenade") == 0) return "n";

	if (strcmp(weapon, "weapon_c4") == 0) return "o";

	return "";
}

void PlayerESP::DrawWeaponIcon(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, ImVec2 vecPosition, Color colColor, Color colOutline)
{
	if (!pEntity->GetPlayerWeapon(pEntity->m_hPlayerPawn().Get())) return;
	if (!pEntity->GetPlayerWeapon(pEntity->m_hPlayerPawn().Get())->GetWeaponBaseVData()) return;
	if (!pPawn) return;

	const char* weaponLetter = pEntity->GetPlayerWeapon(pPawn)->GetWeaponBaseVData()->m_szName();
	if (!weaponLetter) return;

	const char* weaponFontLetter = WeaponToFontLetter(weaponLetter);
	ImVec2 vecTextSize = Fonts::GunIcons->CalcTextSizeA(12, FLT_MAX, 0.0f, weaponFontLetter);


	float offsetY = 14.f;
	if (Config::b(g_Variables.m_Visuals.m_bEnableWeaponESP))
		offsetY = +24.f;

	Draw::AddText(Fonts::GunIcons, 12, ImVec2(vecPosition.x - vecTextSize.x * 0.5f, vecPosition.y - vecTextSize.y + offsetY), weaponFontLetter, colColor, DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 100));
	ctx.m_arrPadding.at(DIR_BOTTOM) += vecTextSize.y - offsetY;
}

void PlayerESP::DrawSkeleton(CCSPlayerController* pEntity, C_CSPlayerPawn* pPawn, ImVec2 vecMin, ImVec2 vecMax, Color colColor, Color colOutline, BoneData_t* bones)
{
	BoneData_t hPrevious, hCurrent;
	for (auto& vecBones : BoneJointList::List)
	{
		hPrevious.vecPosition = Vector(0, 0, 0);
		for (DWORD dwBoneIndex : vecBones)
		{
			hCurrent = bones[dwBoneIndex];
			if (hPrevious.vecPosition == Vector(0, 0, 0))
			{
				hPrevious = hCurrent;
				continue;
			}

			ImVec2 previousSP, currentSP;

			if (Draw::WorldToScreen(hPrevious.vecPosition, previousSP) && Draw::WorldToScreen(hCurrent.vecPosition, currentSP))
			{
				Draw::AddLine(previousSP, currentSP, colColor, 1.f);
			}

			hPrevious = hCurrent;
		}
	}
}

void PlayerESP::DrawBox(CCSPlayerController* pEntity, ImVec2 vecMin, ImVec2 vecMax, Color colColor, Color colOutline)
{
	Draw::AddRect(vecMin, vecMax, colColor, DRAW_RECT_OUTLINE | DRAW_RECT_BORDER | DRAW_RECT_ALIGNED, colOutline, 0.0f);
}

void PlayerESP::HitMarker() {
	if (!Config::b(g_Variables.m_Misc.m_bHitMarker))
		return;

	const float current_time = ImGui::GetTime();
	const float fade_duration = 0.6f;
	const float max_size = 5.0f;
	const float thickness = 1.5f;
	const float gap = 5.0f;
	const float max_scale = 1.2f;
	const float min_scale = 0.7f;

	m_hit_markers.erase(std::remove_if(m_hit_markers.begin(), m_hit_markers.end(),
		[current_time, fade_duration](const hit_marker_t& marker) {
			return (current_time - marker.time) > fade_duration;
		}), m_hit_markers.end());

	ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

	for (auto& marker : m_hit_markers) {
		float elapsed = current_time - marker.time;
		float alpha = 1.0f - (elapsed / fade_duration);
		float t = elapsed / fade_duration;
		float scale = max_scale - (max_scale - min_scale) * (t * t * t);
		scale = std::clamp(scale, min_scale, max_scale);

		ImU32 color = Config::c(g_Variables.m_Misc.m_colHitMarker).GetU32(alpha);

		float scaled_size = max_size * scale;
		ImGui::GetBackgroundDrawList()->AddLine(
			ImVec2(center.x - gap, center.y - gap),
			ImVec2(center.x - gap - scaled_size, center.y - gap - scaled_size),
			color, thickness
		);
		ImGui::GetBackgroundDrawList()->AddLine(
			ImVec2(center.x - gap, center.y + gap),
			ImVec2(center.x - gap - scaled_size, center.y + gap + scaled_size),
			color, thickness
		);
		ImGui::GetBackgroundDrawList()->AddLine(
			ImVec2(center.x + gap, center.y - gap),
			ImVec2(center.x + gap + scaled_size, center.y - gap - scaled_size),
			color, thickness
		);
		ImGui::GetBackgroundDrawList()->AddLine(
			ImVec2(center.x + gap, center.y + gap),
			ImVec2(center.x + gap + scaled_size, center.y + gap + scaled_size),
			color, thickness
		);
	}
}

//void PlayerESP::WeaponESP() {
//	if (!Interfaces::m_pEngine->IsConnected() || !Interfaces::m_pEngine->IsInGame())
//		return;
//
//	if (!Config::b(g_Variables.m_WorldVisuals.m_bWeaponEsp))
//		return;
//
//	for (auto& it : m_vecDroppedWeapons) {
//		ImVec2 out;
//
//		if (it.is_invalid)
//			continue;
//
//		if (!Draw::WorldToScreen(it.weapon->m_pGameSceneNode()->m_vecAbsOrigin(), out))
//			continue;
//
//		float multiplicator = 1.f;
//
//		if (Globals::m_pLocalPlayerPawn->m_iHealth() >= 1) {
//			auto delta_ = Globals::m_pLocalPlayerPawn->m_pGameSceneNode()->m_vecAbsOrigin().DistTo(it.abs_origin);
//			if (delta_ > 500.f) {
//				auto delta = 400.f - std::clamp(delta_ - 500.f, 0.f, 400.f);
//				multiplicator = delta / 400.f;
//			}
//		}
//
//		ImU32 color = IM_COL32(255, 255, 255, (int)(255 * multiplicator));
//
//		const char* text = it.weapon_name.c_str();
//		ImVec2 text_size = ImGui::CalcTextSize(text);
//
//		Vector text_pos = Vector(out.x, out.y);
//
//		Vector text_center_pos = Vector(text_pos.x - text_size.x / 2, text_pos.y - text_size.y / 2);
//
//		Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, ImVec2(text_center_pos.x, text_center_pos.y), it.weapon_name, Color(255, 255, 255, 255), DRAW_TEXT_OUTLINE, Color(0, 0, 0, 255));
//	}
//}

void PlayerESP::OnShotSkeleton() {
	if (Config::b(g_Variables.m_Visuals.m_bEnableOnShotSkeletonESP)) {
		auto AddOnShotSkeleton = [](CCSPlayerController* pController, C_CSPlayerPawn* pPawn) {
			if (!pController || !pPawn) return;

			OnShotSkeleton_t skeleton;
			skeleton.m_pController = pController;
			skeleton.m_pPawn = pPawn;
			skeleton.m_flTimeAdded = Interfaces::m_pGlobalVariables->m_flCurrentTime;

			if (auto pSkeleton = pPawn->m_pGameSceneNode()->GetSkeletonInstance()) {
				memcpy(skeleton.m_Matrix, pSkeleton->m_modelState().m_pBoneArray, sizeof(BoneData_t) * 128);
				s_vecOnShotSkeletons.push_back(skeleton);
			}
			};

		if (!s_vecOnShotSkeletons.empty()) {
			float flDuration = static_cast<float>(Config::i(g_Variables.m_Visuals.m_nHitEffectsDurration));
			float flCurrentTime = Interfaces::m_pGlobalVariables->m_flCurrentTime;

			for (auto it = s_vecOnShotSkeletons.begin(); it != s_vecOnShotSkeletons.end(); ) {
				if ((flCurrentTime - it->m_flTimeAdded) > flDuration) {
					it = s_vecOnShotSkeletons.erase(it);
				}
				else {
					bool bValid = false;
					for (int i = 0; i < 128; i++) {
						if (it->m_Matrix[i].vecPosition != Vector(0, 0, 0)) {
							bValid = true;
							break;
						}
					}

					if (bValid) {
						PlayerESP::DrawSkeleton(
							it->m_pController,
							it->m_pPawn,
							{ },
							{ },
							Config::c(g_Variables.m_Visuals.m_colOnShotSkeletonBones),
							{ },
							it->m_Matrix
						);
					}
					++it;
				}
			}
		}
	}
}

void PlayerESP::DrawDamageIndicators() {
	if (!Config::b(g_Variables.m_Visuals.m_bDamageIndicators))
		return;

	if (!Globals::m_pLocalPlayerController)
		return;

	float flCurrentTime = Interfaces::m_pGlobalVariables->m_flRealTime;

	for (int i = 0; i < m_vecDamageIndicators.size(); i++) {
		auto& indicator = m_vecDamageIndicators[i];

		float flTimeElapsed = flCurrentTime - indicator.m_flEraseTime;
		const float flDuration = 2.0f;

		if (flTimeElapsed > flDuration) {
			m_vecDamageIndicators.erase(m_vecDamageIndicators.begin() + i);
			continue;
		}

		//if (!indicator.m_bInitialized) {
		//	if (indicator.m_pPlayer) {
		//		indicator.m_vecPosition = indicator.m_pPlayer->GetBonePosition(indicator.m_pPlayer, HITBOX_HEAD);
		//		indicator.m_flEraseTime = flCurrentTime;
		//		indicator.m_bInitialized = true;
		//	}
		//}

		float flProgress = flTimeElapsed / flDuration;

		float flOffsetY = 100.0f * flProgress;
		Vector animatedPos = indicator.m_vecPosition;
		animatedPos.z += flOffsetY;

		ImVec2 screenPos;
		if (Draw::WorldToScreen(animatedPos, screenPos)) {
			std::string damageText = std::to_string(indicator.m_iDamage);
			Color textColor = Config::c(g_Variables.m_Visuals.m_colDamageIndicator);

			float flAlpha = 1.0f - (flProgress * flProgress);
			flAlpha = std::max(0.0f, std::min(1.0f, flAlpha));
			textColor.Set(textColor.r(), textColor.g(), textColor.b(), static_cast<int>(flAlpha * 255));

			ImVec2 textSize = Fonts::overlay3->CalcTextSizeA(Fonts::overlay3->FontSize, FLT_MAX, 0.0f, damageText.c_str());
			Draw::AddText(Fonts::overlay3, Fonts::overlay3->FontSize, ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y - textSize.y * 0.5f), damageText.c_str(), textColor, 0, Color(0, 0, 0, static_cast<int>(flAlpha * 255)));
		}
	}
}