//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game rules for Half-Life 2.
//
//=============================================================================//

#ifndef HL2_GAMERULES_H
#define HL2_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "gamerules.h"
#include "singleplay_gamerules.h"
#include "hl2_shareddefs.h"
#include "discharge/cs_urlretrieveprices.h"

#ifdef CLIENT_DLL
#include "networkstringtable_clientdll.h"
#else
#endif

#ifdef CLIENT_DLL
	#define CHalfLife2 C_HalfLife2
	#define CHalfLife2Proxy C_HalfLife2Proxy
#endif


class CHalfLife2Proxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CHalfLife2Proxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};


class CHalfLife2 : public CSingleplayRules
{
public:
	DECLARE_CLASS( CHalfLife2, CSingleplayRules );

	// Damage Query Overrides.
	virtual bool			Damage_IsTimeBased( int iDmgType );
	// TEMP:
	virtual int				Damage_GetTimeBased( void );
	
	virtual bool			ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool			ShouldUseRobustRadiusDamage(CBaseEntity *pEntity);
#ifndef CLIENT_DLL
	virtual bool			ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target );
	virtual float			GetAutoAimScale( CBasePlayer *pPlayer );
	virtual float			GetAmmoQuantityScale( int iAmmoIndex );
	virtual void			LevelInitPreEntity();
#endif
	virtual bool			ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void			Think( void );
	virtual void			CreateStandardEntities( void );	

public:
	bool IsBlackMarket(void) { return m_bBlackMarket; }
	const weeklyprice_t *GetBlackMarketPriceList(void);

	int GetBlackMarketPriceForWeapon(int iWeaponID);
	int GetBlackMarketPreviousPriceForWeapon(int iWeaponID);

	void SetBlackMarketPrices(bool bSetDefaults);

	// Black market
	INetworkStringTable *m_StringTableBlackMarket;
	const weeklyprice_t *m_pPrices;

	virtual const unsigned char *GetEncryptionKey(void) { return (unsigned char *)"d7NSuLq2"; } // both the client and server need this key

private:
	// Rules change for the mega physgun
	CNetworkVar(bool, m_bMegaPhysgun);
	CNetworkVar(bool, m_bBlackMarket);

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.
public:
	CHalfLife2();
	virtual ~CHalfLife2();
#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.

	CHalfLife2();
	virtual ~CHalfLife2();



	virtual void			PlayerSpawn( CBasePlayer *pPlayer );

	virtual void			InitDefaultAIRelationships( void );
	virtual const char*		AIClassText(int classType);
	virtual const char *GetGameDescription( void ) { return "Half-Life 2"; }

	// Ammo
	virtual void			PlayerThink( CBasePlayer *pPlayer );
	virtual float			GetAmmoDamage( CBaseEntity *pAttacker, CBaseEntity *pVictim, int nAmmoType );

	virtual bool			ShouldBurningPropsEmitLight();
public:

	bool AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	bool	NPC_ShouldDropGrenade( CBasePlayer *pRecipient );
	bool	NPC_ShouldDropHealth( CBasePlayer *pRecipient );
	void	NPC_DroppedHealth( void );
	void	NPC_DroppedGrenade( void );
	bool	MegaPhyscannonActive( void ) { return m_bMegaPhysgun;	}
	
	virtual bool IsAlyxInDarknessMode();

private:

	float	m_flLastHealthDropTime;
	float	m_flLastGrenadeDropTime;

	void AdjustPlayerDamageTaken( CTakeDamageInfo *pInfo );
	float AdjustPlayerDamageInflicted( float damage );

	int						DefaultFOV( void ) { return 75; }
#endif
};


//-----------------------------------------------------------------------------
// Gets us at the Half-Life 2 game rules
//-----------------------------------------------------------------------------
inline CHalfLife2* HL2GameRules()
{
	return static_cast<CHalfLife2*>(g_pGameRules);
}

#if defined(DISCHARGE_DLL)
// These go in CCSPlayer::m_iAddonBits and get sent to the client so it can create
// grenade models hanging off players.
#define ADDON_FLASHBANG_1		0x001
#define ADDON_FLASHBANG_2		0x002
#define ADDON_HE_GRENADE		0x004
#define ADDON_SMOKE_GRENADE		0x008
#define ADDON_C4				0x010
#define ADDON_DEFUSEKIT			0x020
#define ADDON_PRIMARY			0x040
#define ADDON_PISTOL			0x080
#define ADDON_PISTOL2			0x100
#define NUM_ADDON_BITS			9

#define CS_MUZZLEFLASH_NONE -1
#define CS_MUZZLEFLASH_NORM	0
#define CS_MUZZLEFLASH_X	1
#endif



#endif // HL2_GAMERULES_H
