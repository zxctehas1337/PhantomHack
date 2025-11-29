#pragma once

class CAntiAim
{
private:
	float GetYaw();
	float GetPitch();
	float GetRoll();
	float GetAtTarget(float flOldYaw);
public:
	void DrawMouseOverrideIndicator();
	float GetMouseOverrideYaw();
	QAngle m_angAtTarget;
	static float m_flStoredYaw;
	static bool m_bHasStoredYaw;
	void OnCreateMove();
};
inline auto g_AntiAim = std::make_unique<CAntiAim>();