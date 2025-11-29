#include "../../Precompiled.h"

void GrenadePredictionObject_t::Draw( Color cColor, bool bOverlay, bool bEffects )
{
	if ( bOverlay )
	{
		Vector vPrev = m_GrenadePathPoint[ 0 ].m_vPos;
		ImVec2 vNadeStart_2D, vNadeEnd_2D;

		for ( auto& vNadePos : m_GrenadePathPoint )
		{
			if ( Draw::WorldToScreen( vPrev, vNadeStart_2D ) && Draw::WorldToScreen( vNadePos.m_vPos, vNadeEnd_2D ) )
			{
				Draw::AddLine( vNadeStart_2D, vNadeEnd_2D, cColor );
			}
			vPrev = vNadePos.m_vPos;
		}
	}
	
	if ( bEffects && !m_bDrewParticle )
	{
		std::vector<Vector> vecPoints;
		for ( auto& GrenadePathPoint : m_GrenadePathPoint )
		{
			vecPoints.emplace_back( GrenadePathPoint.m_vPos );
		}

		g_ParticleMgr->AddParticlePoints( "particles/entity/spectator_utility_trail.vpcf", vecPoints, Color( 255, 255, 255 ), TICKS_TO_TIME(m_nTickDetonation) );

		m_bDrewParticle = true;
	}
}

void CGrenadePrediction::SetGrenadeAct( int nButtons )
{
	bool bInAttack = nButtons & IN_ATTACK;
	bool bInAttack2 = nButtons & IN_ATTACK2;

	m_iGrenadeAct = ( bInAttack && bInAttack2 ) ? ACT_LOB :
		( bInAttack2 ) ? ACT_DROP :
		( bInAttack ) ? ACT_THROW :
		ACT_NONE;
}

void CGrenadePrediction::TraceHull( Vector& vSrc, Vector& end, GameTrace_t* pGameTrace ) const
{
	Ray_t ray{};
	TraceFilter_t filter{ 0x1C3003, Globals::m_pLocalPlayerPawn, NULL, 4 };

	Interfaces::m_pVPhys2World->TraceShape( &ray, vSrc, end, &filter, pGameTrace );
}

void CGrenadePrediction::Setup( Vector& vSrc, Vector& vThrow, QAngle aViewangles ) const noexcept
{
	QAngle aThrow = aViewangles;
	float flPitch = aThrow.x;

	if ( flPitch <= 90.0f )
	{
		if ( flPitch < -90.0f )
		{
			flPitch += 360.0f;
		}
	}
	else
	{
		flPitch -= 360.0f;
	}

	float a = flPitch - ( 90.0f - fabs( flPitch ) ) * 10.0f / 90.0f;
	aThrow.x = a;

	float flVel = 750.0f * 0.9f;

	static const float arrPower[ ] = { 1.0f, 1.0f, 0.5f, 0.0f };
	float b = arrPower[ m_iGrenadeAct ];
	b = b * 0.7f;
	b = b + 0.3f;
	flVel *= b;

	Vector vForward, vRight, vUp;
	aThrow.ToDirections( &vForward, &vRight, &vUp );

	float off = ( arrPower[ m_iGrenadeAct ] * 12.0f ) - 12.0f;
	vSrc.z += off;

	GameTrace_t GameTrace;
	Vector vecDest = vSrc;
	vecDest += vForward * 22.0f;

	TraceHull( vSrc, vecDest, &GameTrace );

	Vector vBack = vForward; 
	vBack *= 6.0f;
	vSrc = GameTrace.m_vecEndPos;
	vSrc -= vBack;

	vThrow = Globals::m_pLocalPlayerPawn->m_vecAbsVelocity( );
	vThrow *= 1.25f;
	vThrow += vForward * flVel;
}

int CGrenadePrediction::PhysicsClipVelocity( const Vector& vIn, const Vector& vNormal, Vector& vOut, float flOverBounce ) const noexcept
{
	static const float STOP_EPSILON = 0.1f;

	float    backoff;
	float    change;
	float    angle;
	int        i, blocked;

	blocked = 0;

	angle = vNormal[ 2 ];

	if ( angle > 0 )
	{
		blocked |= 1;
	}
	if ( !angle )
	{
		blocked |= 2;  
	}

	backoff = vIn.DotProduct( vNormal ) * flOverBounce;

	for ( i = 0; i < 3; i++ )
	{
		change = vNormal[ i ] * backoff;
		vOut[ i ] = vIn[ i ] - change;
		if ( vOut[ i ] > -STOP_EPSILON && vOut[ i ] < STOP_EPSILON )
		{
			vOut[ i ] = 0;
		}
	}

	return blocked;
}

void CGrenadePrediction::PushEntity( Vector& vSrc, const Vector& vMove, GameTrace_t* pGameTrace ) const noexcept
{
	Vector vecAbsEnd = vSrc;
	vecAbsEnd += vMove;
	this->TraceHull( vSrc, vecAbsEnd, pGameTrace );
}

void CGrenadePrediction::ResolveFlyCollisionCustom( GameTrace_t* pGameTrace, Vector& vVelocity, float flInterval ) const noexcept
{
	const float surfaceElasticity = 1.0;
	const float grenadeElasticity = 0.45f;
	float totalElasticity = grenadeElasticity * surfaceElasticity;
	if ( totalElasticity > 0.9f ) totalElasticity = 0.9f;
	if ( totalElasticity < 0.0f ) totalElasticity = 0.0f;

	Vector vAbsVelocity;
	PhysicsClipVelocity( vVelocity, pGameTrace->m_vecNormal, vAbsVelocity, 2.0f );
	vAbsVelocity *= totalElasticity;

	float speedSqr = vAbsVelocity.LengthSqr( );
	static const float minSpeedSqr = 20.0f * 20.0f;

	if ( speedSqr < minSpeedSqr )
	{
		vAbsVelocity.x = 0.0f;
		vAbsVelocity.y = 0.0f;
		vAbsVelocity.z = 0.0f;
	}

	if ( pGameTrace->m_vecNormal.z > 0.7f )
	{
		vVelocity = vAbsVelocity;
		vAbsVelocity *= ( ( 1.0f - pGameTrace->m_flFraction ) * flInterval );
		PushEntity( pGameTrace->m_vecEndPos, vAbsVelocity, pGameTrace );
	}
	else
	{
		vVelocity = vAbsVelocity;
	}
}
bool CGrenadePrediction::CheckDetonate( const Vector& vThrow, GameTrace_t* pGameTrace, int iTick, float flInterval, C_CSWeaponBase* pActiveWeapon ) noexcept
{
	SchemaClassInfoData_t* pGrenadeSchema;
	pActiveWeapon->GetSchemaClassInfo( &pGrenadeSchema );
	if ( !pGrenadeSchema )
		return false;

	FNV1A_t uHashedName = FNV1A::Hash( pGrenadeSchema->szName );

	static float flTpLen = 0.1f;
	switch ( uHashedName )
	{
	case FNV1A::HashConst( "C_SmokeGrenadeProjectile" ):
	case FNV1A::HashConst( "C_SmokeGrenade" ):
	case FNV1A::HashConst( "C_DecoyGrenade" ):
	case FNV1A::HashConst( "C_DecoyProjectile" ):
		if ( vThrow.Length2D( ) < flTpLen )
		{
			int iDetonateTickMod = (int)( 0.2f / flInterval );
			return !( iTick % iDetonateTickMod );
		}
		return false;
	case FNV1A::HashConst( "C_MolotovProjectile" ):
	case FNV1A::HashConst( "C_IncendiaryGrenade" ):
	case FNV1A::HashConst( "C_MolotovGrenade" ):
		if ( pGameTrace->m_flFraction != 1.0f && pGameTrace->m_vecNormal.z > 0.7f )
			return true;
	case FNV1A::HashConst( "C_Flashbang" ):
	case FNV1A::HashConst( "C_FlashbangProjectile" ):
	case FNV1A::HashConst( "C_HEGrenade" ):
	case FNV1A::HashConst( "C_HEGrenadeProjectile" ):
		return (float)iTick * flInterval > 1.5f && !( iTick % (int)( 0.2f / flInterval ) );
	default:
		return false;
	}
}

void CGrenadePrediction::AddGravityMove( Vector& vMove, Vector& vVel, float flFrameTime, bool bOnGround ) noexcept
{
	Vector vBaseVelocity = { 0.0f, 0.0f, 0.0f };

	vMove.x = ( vVel.x + vBaseVelocity.x ) * flFrameTime;
	vMove.y = ( vVel.y + vBaseVelocity.y ) * flFrameTime;

	if ( bOnGround )
	{
		vMove.z = ( vVel.z + vBaseVelocity.z ) * flFrameTime;
	}
	else
	{
		float flGravity = 800.0f * 0.4f;
		float flNewZ = vVel.z - ( flGravity * flFrameTime );
		vMove.z = ( ( vVel.z + flNewZ ) / 2.0f + vBaseVelocity.z ) * flFrameTime;
		vVel.z = flNewZ;
	}
}

float CGrenadePrediction::CalculateArmor( float flDamage, int iArmorValue ) noexcept
{
	if ( iArmorValue > 0 ) 
	{
		float newDamage = flDamage * 0.5f;
		float armor = ( flDamage - newDamage ) * 0.5f;

		if ( armor > static_cast<float>( iArmorValue ) ) {
			armor = static_cast<float>( iArmorValue ) * ( 1.f / 0.5f );
			newDamage = flDamage - armor;
		}

		flDamage = newDamage;
	}
	return flDamage;
}

std::vector<GrenadePathPoint_t> CGrenadePrediction::GetGrenadePathFromEntity( Vector vSrc, Vector vVelocity, C_BaseCSGrenade* pGrenade, float flSpawnTime )
{
	std::vector<GrenadePathPoint_t> points{};
	int maxTicksBetweenPoints = static_cast<int>( 0.05f / INTERVAL_PER_TICK );
	int tickTimer = 0;

	int spawnTick = TIME_TO_TICKS( flSpawnTime );

	int nBounces = 0;
	Vector velocity = vVelocity;
	Vector position = vSrc;

	for ( unsigned int simTick = 0; simTick < 256; ++simTick ) {
		if ( !tickTimer ) points.emplace_back( GrenadePathPoint_t( position, spawnTick + simTick ) );
		int resultFlag = 0;
		Vector move;
		this->AddGravityMove( move, velocity, INTERVAL_PER_TICK, false );
		GameTrace_t tr;
		PushEntity( position, move, &tr );
		if ( CheckDetonate( velocity, &tr, simTick, INTERVAL_PER_TICK, pGrenade ) ) resultFlag = 1;
		else if ( nBounces > 20 || ( std::abs( velocity.x ) < 20.f && std::abs( velocity.y ) < 20.f ) ) resultFlag = 1;
		if ( tr.m_flFraction != 1.0f ) {
			resultFlag = 2;
			nBounces++;
			this->ResolveFlyCollisionCustom( &tr, velocity, INTERVAL_PER_TICK );
		}
		position = tr.m_vecEndPos;
		if ( resultFlag == 1 ) break;
		if ( resultFlag == 2 || tickTimer >= maxTicksBetweenPoints ) tickTimer = 0;
		else tickTimer++;
	}

	return points;
}

void CGrenadePrediction::Run( )
{
	m_vecScreenPoints.clear( );
	m_vecEndPoints.clear( );
	m_vecDmgPoints.clear( );

	if ( !Config::b( g_Variables.m_WorldVisuals.m_bGrenadePrediction ) )
		return;

	if (!Interfaces::m_pEngine->IsConnected() || !Interfaces::m_pEngine->IsInGame())
		return;

	C_CSPlayerPawn* pawn = Globals::m_pLocalPlayerPawn;
	if (!pawn)
		return;

	C_CSWeaponBase* actw = pawn->m_pWeaponServices()->m_hActiveWeapon().Get();
	if (!actw)
		return;

	C_AttributeContainer* att = actw->m_AttributeManager();
	if (!att)
		return;

	C_EconItemView* ec = att->m_Item();
	if (!ec)
		return;

	uint16_t def = ec->m_iItemDefinitionIndex();
	if (!def)
		return;

	if (def == WEAPON_C4_EXPLOSIVE)
		return;

	for ( int i = 0; i < s_vecPredictedGrenades.size( ); i++ )
	{
		GrenadePredictionObject_t& GrenadePredictionObject = s_vecPredictedGrenades[ i ];
		if ( TICKS_TO_TIME( GrenadePredictionObject.m_GrenadePathPoint.back( ).m_nTick ) <= Interfaces::m_pGlobalVariables->m_flCurrentTime )
		{
			s_vecPredictedGrenades.erase( s_vecPredictedGrenades.begin( ) + i );
			continue;
		}

		GrenadePredictionObject.Draw(Config::c(g_Variables.m_WorldVisuals.m_cGrenadePrediction), true, false);
	}

	C_CSPlayerPawn* pPawn = Globals::m_pLocalPlayerPawn;
	if ( pPawn->m_nActualMoveType( ) == MOVETYPE_NOCLIP )
		return;

	this->SetGrenadeAct( Globals::m_pCmd->m_nButtons.m_nValue );

	C_BaseCSGrenade* pGrenade = reinterpret_cast<C_BaseCSGrenade*>( LocalPlayerData::m_pWeapon );
	if ( !pGrenade || !LocalPlayerData::m_pWeaponBaseVData || !pGrenade->m_bPinPulled( ) )
		return;

	Vector vStartPos = NRagebot::m_vShootPos, vVelocity;
	this->Setup( vStartPos, vVelocity, Interfaces::m_pInput->GetViewAngles( ) );

	std::vector<GrenadePathPoint_t> vecGrenadePathPoint = this->GetGrenadePathFromEntity( vStartPos, vVelocity, pGrenade, Interfaces::m_pGlobalVariables->m_flCurrentTime );

	Vector vPrev = vecGrenadePathPoint[ 0 ].m_vPos;
	ImVec2 vNadeStart_2D, vNadeEnd_2D;
	Color cGrenadePredictionColor = Config::c( g_Variables.m_WorldVisuals.m_cGrenadePrediction );
	for ( auto& vNade : vecGrenadePathPoint )
	{
		if ( Draw::WorldToScreen( vPrev, vNadeStart_2D ) && Draw::WorldToScreen( vNade.m_vPos, vNadeEnd_2D ) )
		{
			Draw::AddLine( vNadeStart_2D, vNadeEnd_2D, cGrenadePredictionColor );
		}
		vPrev = vNade.m_vPos;
	}
}