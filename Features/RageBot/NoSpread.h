#pragma once

struct CalcSpreadOutput_t
{
	float x, y;
};


struct NoSpreadResult {
	bool bFound;
	QAngle angAdjusted;
	int iFoundAfter;
	int iSeed;
};

namespace NoSpread
{
	uint32_t ComputeRandomSeed(QAngle* angViewAngles, std::uint32_t nPlayerTickCount);
	CalcSpreadOutput_t CalculateSpread(int nRandomSeed, float flInAccuracy, float flSpread);
	NoSpreadResult NoSpread(QAngle& angle);
	Vector2D CalculateSpreadBasic(int nSeed, float flInaccuracy, float flSpread, float flRecoilIndex);
}