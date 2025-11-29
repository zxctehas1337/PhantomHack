#pragma once

enum EParticleSetting : unsigned int
{
	PARTICLE_SETTING_POSITION = 0,
	PARTICLE_SETTING_DENSITY = 2,
	PARTICLE_SETTING_INFO = 3,
	PARTICLE_SETTING_COLOR = 16
};

class CParticleMgr
{
private:
	struct PointParticle_t
	{
		unsigned int m_uiEffectIndex;
		float m_flCurrentTimeGlobal;
		float m_flParticleTimeBeforeDelete;
	};
private:
	std::vector<unsigned int> m_vecEffectIndexes;
	int8_t m_iLastEffectCached;
	float m_flLastReloadTime;
	std::deque<PointParticle_t> m_deqPointParticlesAdded;
public:
	void OnEvent(CGameEvent* pEvent);
	void AddParticlePoints(const char* szParticlePath, std::vector<Vector> vecPoints, Color colColor, float flTime, bool bCustom = false);
	void AddParticle2Point(const char* szParticlePath, Vector vStart, Vector vEnd, Color colColor, float flTime, bool bCustom = false);
	void AddParticle1Point(const char* szParticlePath, Vector vPosition, Color colColor, float flTime, bool bCustom = false);

	void CreateParticle(unsigned int& nEffectIndex, const char* szParticlePath);
	void ReleaseParticles();
	void UpdateParticles();
};
inline auto g_ParticleMgr = std::make_unique<CParticleMgr>();