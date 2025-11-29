#pragma once

struct GrenadePathPoint_t;
struct GrenadePredictionObject_t
{
	Vector m_vInitialPosition;
	Vector m_vInitialVelocity;

	C_BaseEntity* m_pGrenadeEntity;

	int m_nTickDetonation;

	std::vector<GrenadePathPoint_t> m_GrenadePathPoint;

	bool m_bDrewParticle = false;
	
	void Draw( Color cColor, bool bOverlay, bool bEffects );
};
inline std::vector < GrenadePredictionObject_t > s_vecPredictedGrenades;

struct GrenadePathPoint_t
{
	Vector m_vPos;
	int m_nTick;

	GrenadePathPoint_t( Vector vPos, int nTick ) : m_vPos( vPos ), m_nTick( nTick ) { }

	bool HasPassed( int currentTick ) const
	{
		return currentTick > m_nTick;
	}
};

class CGrenadePrediction
{
private:
	std::vector<std::pair<ImVec2, ImVec2>> m_vecScreenPoints;
	std::vector<std::pair<ImVec2, ImVec2>> m_vecEndPoints;
	std::vector<std::pair<ImVec2, std::string>> m_vecDmgPoints;

	int m_iGrenadeAct = 1;
private:
	enum EAct
	{
		ACT_NONE,
		ACT_THROW,
		ACT_LOB,
		ACT_DROP,
	};
public:
	void SetGrenadeAct( int nButtons );
	void TraceHull( Vector& vSrc, Vector& end, GameTrace_t* pGameTrace ) const;
	void Setup( Vector& vSrc, Vector& vThrow, QAngle aViewangles ) const noexcept;
	int PhysicsClipVelocity( const Vector& vIn, const Vector& vNormal, Vector& vOut, float flOverBounce ) const noexcept;
	void PushEntity( Vector& vSrc, const Vector& vMove, GameTrace_t* pGameTrace ) const noexcept;
	void ResolveFlyCollisionCustom( GameTrace_t* pGameTrace, Vector& vVelocity, float flInterval ) const noexcept;
	bool CheckDetonate( const Vector& vThrow, GameTrace_t* pGameTrace, int iTick, float flInterval, C_CSWeaponBase* pActiveWeapon ) noexcept;
	void AddGravityMove( Vector& vMove, Vector& vVel, float flFrameTime, bool bOnGround ) noexcept;
	float CalculateArmor( float flDamage, int iArmorValue ) noexcept;
	std::vector<GrenadePathPoint_t> GetGrenadePathFromEntity( Vector vSrc, Vector vVelocity, C_BaseCSGrenade* pGrenade, float flSpawnTime );
	void Run( );
};
inline static const auto g_GrenadePrediction = std::make_unique<CGrenadePrediction>( );