#include "../../Precompiled.h"

#define CONFIG_GET(type, path) Config::Get<type>(path);

#define RED_DEBUG_BOX(position) Interfaces::m_pClient->GetDebugOverlay()->AddBoxOverlay(position,   { 1, 1, 1 }, { -1, -1, -1 }, { 0, 0, 0 }, { 255, 0, 0     }, 0.1f)
#define GREEN_DEBUG_BOX(position) Interfaces::m_pClient->GetDebugOverlay()->AddBoxOverlay(position, { 1, 1, 1 }, { -1, -1, -1 }, { 0, 0, 0 }, { 0, 255, 0     }, 0.1f)
#define BLUE_DEBUG_BOX(position) Interfaces::m_pClient->GetDebugOverlay()->AddBoxOverlay(position,  { 1, 1, 1 }, { -1, -1, -1 }, { 0, 0, 0 }, { 0, 0, 255     }, 0.1f)
#define WHITE_DEBUG_BOX(position) Interfaces::m_pClient->GetDebugOverlay()->AddBoxOverlay(position, { 1, 1, 1 }, { -1, -1, -1 }, { 0, 0, 0 }, { 255, 255, 255 }, 0.1f)
#define BLACK_DEBUG_BOX(position) Interfaces::m_pClient->GetDebugOverlay()->AddBoxOverlay(position, { 1, 1, 1 }, { -1, -1, -1 }, { 0, 0, 0 }, { 0, 0, 0       }, 0.1f)

#define LEFT_ANGLE -90.f
#define RIGHT_ANGLE 90.f
#define NO_ANGLE 0.f

struct HighestThreat_t
{
	QAngle m_aAngle;
	C_CSPlayerPawn* m_pPawn;
};

static HighestThreat_t GetHighestThreat()
{
	Vector vLocalEyePos = Globals::m_pLocalPlayerPawn->GetEyePosition(true);
	QAngle aLocalAngles = g_Movement->m_angCameraAngles;
	Vector vViewForward, vViewRight, vViewUp;
	Math::AngleVectors(aLocalAngles, vViewForward, vViewRight, vViewUp);

	float flNearestFOV = FLT_MAX;
	QAngle aBestAngle = aLocalAngles;
	C_CSPlayerPawn* pBestPawn = nullptr;

	for (EntityObject_t& EntityObject : g_Entities->m_vecPlayersOnly) {
		CCSPlayerController* pController = static_cast<CCSPlayerController*>(EntityObject.m_pEntity);
		if (!pController || pController == Globals::m_pLocalPlayerController)
			continue;

		C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(Interfaces::m_pGameResourceService->pGameEntitySystem->Get(pController->m_hPawn()));
		if (!pPawn)
			continue;

		if (!pController->m_bPawnIsAlive() || !pPawn->IsEnemy(Globals::m_pLocalPlayerPawn))
			continue;

		Vector vecTarget = pPawn->GetEyePosition();
		Vector vDelta = vecTarget - vLocalEyePos;
		vDelta.NormalizeInPlace();

		float flDot = vViewForward.DotProduct(vDelta);
		float flFOV = M_RAD2DEG(acosf(flDot));

		if (flFOV < flNearestFOV)
		{
			flNearestFOV = flFOV;
			aBestAngle = Math::CalcAngle(vLocalEyePos, vecTarget);
			pBestPawn = pPawn;
		}
	}

	return { aBestAngle, pBestPawn };
}

static GameTrace_t TraceShapeWrapper(Vector vStart, Vector vEnd)
{
	GameTrace_t trace;
	TraceFilter_t filter(TRACE_MASK, Globals::m_pLocalPlayerPawn, nullptr, 4);
	Ray_t ray;

	Interfaces::m_pVPhys2World->TraceShape(&ray, vStart, vEnd, &filter, &trace);

	return trace;
}

static int GetFreeStandingYaw()
{
	if (!Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_bFreestanding)))
		return 0;

	HighestThreat_t objHighestThreat = GetHighestThreat();

	bool bEnemyValid = objHighestThreat.m_pPawn;

	const Vector vEyePos = NRagebot::m_vShootPos;
	const Vector vEnemyEyePos = bEnemyValid ? objHighestThreat.m_pPawn->GetEyePosition() : Vector(0.f, 0.f, 0.f);

	bool bAtTarget = Config::b(g_Variables.m_Ragebot.m_bAtTarget);
	QAngle aViewAngles = bAtTarget ? QAngle(0.f, objHighestThreat.m_aAngle.y, 0.f) : QAngle(0.f, Interfaces::m_pInput->GetViewAngles().y, 0.f);

	Matrix3x4_t mViewMatrix = {};
	Math::AngleMatrix(aViewAngles, mViewMatrix);

	Vector vLeftHeadPosAALocal = { 0.f, -15.f, 0.f };
	Vector vRightHeadPosAALocal = { 0.f, 15.f, 0.f };

	Vector vLeftLocal = { 0.f, -25.f, 0.f };
	Vector vLeftFrontLocal = { 50.f, -25.f, 0.f };

	Vector vRightLocal = { 0.f, 25.f, 0.f };
	Vector vRightFrontLocal = { 50.f, 25.f, 0.f };

	Vector vLeftWorld, vRightWorld, vLeftFrontWorld, vRightFrontWorld, vLeftHeadPosAAWorld, vRightHeadPosAAWorld;
	Math::VectorRotate(vLeftLocal, mViewMatrix, vLeftWorld);
	Math::VectorRotate(vRightLocal, mViewMatrix, vRightWorld);
	Math::VectorRotate(vLeftFrontLocal, mViewMatrix, vLeftFrontWorld);
	Math::VectorRotate(vRightFrontLocal, mViewMatrix, vRightFrontWorld);
	Math::VectorRotate(vLeftHeadPosAALocal, mViewMatrix, vLeftHeadPosAAWorld);
	Math::VectorRotate(vRightHeadPosAALocal, mViewMatrix, vRightHeadPosAAWorld);

	vLeftWorld += vEyePos;
	vRightWorld += vEyePos;
	vLeftFrontWorld += vEyePos;
	vRightFrontWorld += vEyePos;

	if (Config::b(g_Variables.m_Misc.m_bDebug))
	{
		RED_DEBUG_BOX(vEyePos);
		BLUE_DEBUG_BOX(vLeftWorld);
		GREEN_DEBUG_BOX(vRightWorld);
		WHITE_DEBUG_BOX(vLeftFrontWorld);
		BLACK_DEBUG_BOX(vRightFrontWorld);
	}

	GameTrace_t objLeftGameTrace = TraceShapeWrapper(vLeftWorld, vLeftFrontWorld);
	GameTrace_t objRightGameTrace = TraceShapeWrapper(vRightWorld, vRightFrontWorld);

	int iReturnYaw = NO_ANGLE;

	if (objLeftGameTrace.m_flFraction != objRightGameTrace.m_flFraction)
	{
		if (objLeftGameTrace.m_flFraction > objRightGameTrace.m_flFraction)
		{
			if (bAtTarget && bEnemyValid)
			{
				GameTrace_t objGameTrace = TraceShapeWrapper(vEnemyEyePos, vLeftHeadPosAAWorld);
				if (!objGameTrace.IsVisible())
					iReturnYaw = LEFT_ANGLE;
			}
			else
			{
				iReturnYaw = LEFT_ANGLE;
			}
		}
		else
		{
			if (bAtTarget && bEnemyValid)
			{
				GameTrace_t objGameTrace = TraceShapeWrapper(vEnemyEyePos, vRightHeadPosAAWorld);
				if (!objGameTrace.IsVisible())
					iReturnYaw = RIGHT_ANGLE;
			}
			else
			{
				iReturnYaw = RIGHT_ANGLE;
			}
		}
	}

	return iReturnYaw;
}

//float GetMouseOverrideYaw()
//{
//	HWND hwnd = Input::m_hWindow;
//	if (!hwnd)
//		return CAntiAim::m_flLastMouseOverrideYaw;
//
//	POINT mousePos;
//	GetCursorPos(&mousePos);
//	ScreenToClient(hwnd, &mousePos);
//
//	RECT clientRect;
//	GetClientRect(hwnd, &clientRect);
//
//	Vector2D screenCenter = {
//		static_cast<float>(clientRect.right) / 2.f,
//		static_cast<float>(clientRect.bottom) / 2.f
//	};
//
//	Vector2D mouseDelta = {
//		static_cast<float>(mousePos.x) - screenCenter.x,
//		-(static_cast<float>(mousePos.y) - screenCenter.y)
//	};
//
//	float distanceFromCenter = mouseDelta.Length();
//	if (distanceFromCenter < 50.f)
//		return CAntiAim::m_flLastMouseOverrideYaw;
//
//	float mouseAngle = atan2f(mouseDelta.y, mouseDelta.x);
//
//	float yaw = mouseAngle * (180.f / M_PI);
//	yaw -= 90.f;
//
//	while (yaw > 180.f) yaw -= 360.f;
//	while (yaw < -180.f) yaw += 360.f;
//
//	CAntiAim::m_flLastMouseOverrideYaw = yaw;
//
//	return yaw;
//}

void CAntiAim::DrawMouseOverrideIndicator()
{
	if (!Input::m_bMouseOverrideActive)
		return;

	HWND hwnd = Input::m_hWindow;
	if (!hwnd)
		return;

	RECT clientRect;
	GetClientRect(hwnd, &clientRect);

	Vector2D screenCenter = {
		static_cast<float>(clientRect.right) / 2.f,
		static_cast<float>(clientRect.bottom) / 2.f
	};

	float yaw = 0.f;

	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(hwnd, &mousePos);

	Vector2D mouseDelta = {
		static_cast<float>(mousePos.x) - screenCenter.x,
		-(static_cast<float>(mousePos.y) - screenCenter.y) 
	};

	float angle = atan2f(mouseDelta.y, mouseDelta.x) * (180.f / M_PI);
	yaw = angle;

	while (yaw > 180.f) yaw -= 360.f;
	while (yaw < -180.f) yaw += 360.f;

	float radius = 125.f;
	float indicatorAngle = yaw * (M_PI / 180.f);

	Vector2D circlePos = {
		screenCenter.x + radius * cosf(indicatorAngle),
		screenCenter.y + radius * sinf(indicatorAngle)
	};

	auto draw_list = ImGui::GetBackgroundDrawList();

	Color center_color = Config::c(g_Variables.m_Ragebot.m_AutoPeekColor);
	Color edge_color = center_color;
	edge_color[3] = 0;

	constexpr int segments = 16;
	const float step = 2.0f * M_PI / segments;

	std::vector<ImVec2> points;
	points.reserve(segments + 1);

	ImVec2 center(circlePos.x, circlePos.y);
	points.push_back(center);

	float circle_radius = 20.f;

	for (int i = 0; i <= segments; i++)
	{
		float segment_angle = step * i;
		ImVec2 point(
			circlePos.x + circle_radius * cosf(segment_angle),
			circlePos.y + circle_radius * sinf(segment_angle)
		);
		points.push_back(point);
	}

	if (points.size() < 3)
		return;

	const ImU32 center_col = center_color.GetU32();
	const ImU32 edge_col = edge_color.GetU32();

	for (size_t i = 1; i < points.size() - 1; ++i)
		draw_list->AddTriangleFilledMulticolor(points[0], points[i], points[i + 1], center_col, edge_col, edge_col);
}

float CAntiAim::GetYaw()
{
#ifdef DEBUG_OR_ALPHA
	static bool bWasActive = false;
	static float flStoredYaw = 0.f;

	bool bIsActive = Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_bMouseOverride));

	if (bIsActive && !bWasActive)
	{
		int iBaseYaw = 0;
		switch (Config::i(g_Variables.m_Ragebot.m_iYawValue))
		{
		case 1:
			iBaseYaw = 180.f;
			break;
		case 0:
		default:
			iBaseYaw = 0.f;
			break;
		}

		if (Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iLeftAntiaimKeybind)))
		{
			iBaseYaw -= 90.f;
		}
		else if (Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iRightAntiaimKeybind)))
		{
			iBaseYaw += 90.f;
		}

		flStoredYaw = static_cast<float>(iBaseYaw);
	}

	if (bIsActive)
	{
		HWND hwnd = Input::m_hWindow;
		if (hwnd)
		{
			POINT mousePos;
			GetCursorPos(&mousePos);
			ScreenToClient(hwnd, &mousePos);

			RECT clientRect;
			GetClientRect(hwnd, &clientRect);

			Vector2D screenCenter = {
				static_cast<float>(clientRect.right) / 2.f,
				static_cast<float>(clientRect.bottom) / 2.f
			};

			Vector2D mouseDelta = {
				screenCenter.x - static_cast<float>(mousePos.x),
				screenCenter.y - static_cast<float>(mousePos.y)
			};

			float distanceFromCenter = mouseDelta.Length();
			if (distanceFromCenter >= 10.f)
			{
				float mouseAngle = atan2f(mouseDelta.y, mouseDelta.x);
				float yaw = mouseAngle * (180.f / M_PI);

				yaw = -yaw + 90.f;

				while (yaw > 180.f) yaw -= 360.f;
				while (yaw < -180.f) yaw += 360.f;

				flStoredYaw = yaw;
			}
		}

		bWasActive = true;
		return flStoredYaw;
	}

	bWasActive = bIsActive;

	return flStoredYaw;
#endif

	int iYaw = 0;

	switch (Config::i(g_Variables.m_Ragebot.m_iYawValue))
	{
	case 1:
		iYaw = 180.f;
		break;
	case 0:
	default:
		break;
	}

	//if (Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iLeftAntiaimKeybind)))
	//{
	//	iYaw -= 90.f;
	//}
	//else if (Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iRightAntiaimKeybind)))
	//{
	//	iYaw += 90.f;
	//}

	return static_cast<float>(iYaw);
}

float CAntiAim::GetPitch()
{
	float m_iPitch = 0.f;

	switch (Config::i(g_Variables.m_Ragebot.m_iPitchValue))
	{
	case 1:
		m_iPitch = -89.f;
		break;
	case 2:
		m_iPitch = 89.f;
		break;
	case 3:
		m_iPitch = 0.f;
		break;
	case 0:
	default:
		break;
	}

	if (Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iFakePitch)))
		m_iPitch = -2182423346297399750336966557899.f;

	return m_iPitch;
}

float CAntiAim::GetRoll()
{
	return Config::i(g_Variables.m_Ragebot.m_iRoll);
}

static int iSpin = 0;
static float g_flGrenadeThrowEndTime = 0.f;

bool ShouldDisableAntiAim() {
	if (!Config::b(g_Variables.m_Ragebot.m_bAntiaim))
		return true;

	if (!Interfaces::m_pGameRules)
		return false;

	if (!Globals::m_pLocalPlayerPawn || Globals::m_pLocalPlayerPawn->m_iHealth() <= 0)
		return true;

	if (!Interfaces::m_pGameRules->m_bHasMatchStarted())
		return false;

	if (Interfaces::m_pGameRules->m_bFreezePeriod())
		return true;

	if (Interfaces::m_pGameRules->m_fRoundStartTime() > Interfaces::m_pGlobalVariables->m_flCurrentTime)
		return true;

	if (!Globals::m_pCmd)
		return true;

	if (Globals::m_pCmd->m_nButtons.m_nValue & IN_USE)
		return true;

	C_BaseCSGrenade* pNade = (C_BaseCSGrenade*)LocalPlayerData::m_pWeapon;

	if (pNade->m_fThrowTime() > 0.f) {
		g_flGrenadeThrowEndTime = Interfaces::m_pGlobalVariables->m_flCurrentTime + 0.1f;
		return true;
	}

	if (Interfaces::m_pGlobalVariables->m_flCurrentTime < g_flGrenadeThrowEndTime)
		return true;

	if (Globals::m_pLocalPlayerPawn->m_nActualMoveType() == MOVETYPE_LADDER || Globals::m_pLocalPlayerPawn->m_nActualMoveType() == MOVETYPE_NOCLIP)
		return true;

	return false;
}

void CAntiAim::OnCreateMove()
{
	if (ShouldDisableAntiAim())
		return;

	Vector pModifiedViewAngles = Vector(Globals::m_pBaseCmd->viewangles().x(), Globals::m_pBaseCmd->viewangles().y(), Globals::m_pBaseCmd->viewangles().z());

	if (Config::b(g_Variables.m_Ragebot.m_bAtTarget) && !Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iRightAntiaimKeybind)) && !Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_iLeftAntiaimKeybind)))
		pModifiedViewAngles.y = GetHighestThreat().m_aAngle.y;
	
	pModifiedViewAngles.x = this->GetPitch();
	pModifiedViewAngles.y += this->GetYaw();
	pModifiedViewAngles.z = this->GetRoll();

	//if (Config::b(g_Variables.m_Ragebot.m_bRandomizeYaw))
	//{
	//	float flRange = Config::f(g_Variables.m_Ragebot.m_flRandomizeYawRange);
	//	float flRandomYaw = Math::fnRandomFloat(-flRange, flRange);
	//	pModifiedViewAngles.y += flRandomYaw;
	//}

	//if (Config::b(g_Variables.m_Ragebot.m_bUnhittableYaw))
	//{
	//	auto nTickBase = Globals::m_pLocalPlayerController->m_nTickBase();
	//	if (nTickBase % 4)
	//	{
	//		pModifiedViewAngles.y -= Math::fnRandomFloat(60, 80);
	//	}
	//	else
	//	{
	//		pModifiedViewAngles.y += Math::fnRandomFloat(60, 80);
	//	}
	//}

	//if (Config::b(g_Variables.m_Ragebot.m_bYawJitter))
	//{
	//	static float flYawJitterMult = 0.f;
	//	if (flYawJitterMult == 0.f)
	//		flYawJitterMult = 1.f;

	//	flYawJitterMult *= -1.f;

	//	pModifiedViewAngles.y += Config::i(g_Variables.m_Ragebot.m_iYawJitter) * flYawJitterMult;
	//}

	//if (Input::HandleInput(Config::kb(g_Variables.m_Ragebot.m_kbEnableSpinAA)))
	//{
	//	iSpin += Config::i(g_Variables.m_Ragebot.m_iSpinAASpeed);
	//	pModifiedViewAngles.y += iSpin;
	//}
	//else if (abs(iSpin) > 0)
	//{
	//	iSpin = 0;
	//}

	pModifiedViewAngles.y += iSpin;
	pModifiedViewAngles.Normalize();

	Globals::SetBaseViewAngles(pModifiedViewAngles);
}
