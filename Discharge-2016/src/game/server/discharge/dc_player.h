#ifndef DC_PLAYER_H
#define DC_PLAYER_H
#pragma once

#include "hl2_player.h"
#include "discharge/cs_playeranimstate.h"

class CWeaponCSBase;

class CDC_Player : public CHL2_Player, public ICSPlayerAnimStateHelpers
{
public:
	DECLARE_CLASS(CDC_Player, CHL2_Player);

	CDC_Player();
	~CDC_Player(void);

	static CDC_Player *CreatePlayer(const char *className, edict_t *ed)
	{
		CDC_Player::s_PlayerEdict = ed;
		return (CDC_Player*)CreateEntityByName(className);
	}

	DECLARE_SERVERCLASS();
	//DECLARE_DATADESC();

public:
	// ICSPlayerAnimState overrides.
	virtual CWeaponCSBase* CSAnim_GetActiveWeapon();
	virtual bool CSAnim_CanMove();


	void FireBullet(
		Vector vecSrc,
		const QAngle &shootAngles,
		float vecSpread,
		float flDistance,
		int iPenetration,
		int iBulletType,
		int iDamage,
		float flRangeModifier,
		CBaseEntity *pevAttacker,
		bool bDoEffects,
		float x,
		float y);

	void KickBack(
		float up_base,
		float lateral_base,
		float up_modifier,
		float lateral_modifier,
		float up_max,
		float lateral_max,
		int direction_change);

	void GetBulletTypeParameters(
		int iBulletType,
		float &fPenetrationPower,
		float &flPenetrationDistance);

	// Returns true if the player is allowed to move.
	bool CanMove() const;

	static void	StartNewBulletGroup();	// global function

	virtual void SetAnimation(PLAYER_ANIM playerAnim);
	ICSPlayerAnimState *GetPlayerAnimState() { return m_PlayerAnimState; }

	virtual void FireBullets(const FireBulletsInfo_t &info);

	// Called whenever this player fires a shot.
	void NoteWeaponFired();

	// This passes the event to the client's and server's CPlayerAnimState.
	void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);

	CWeaponCSBase* GetActiveCSWeapon() const;

	virtual void		Precache(void);
	virtual void		Spawn(void);

	CNetworkVar(int, m_iDirection);	// The current lateral kicking direction; 1 = right,  0 = left
	CNetworkVar(int, m_iShotsFired);	// number of shots fired recently
	CNetworkVar(int, m_iThrowGrenadeCounter);	// used to trigger grenade throw animations.

	// custom player functions
	virtual void			ImpulseCommands(void);
	virtual bool			ClientCommand(const CCommand &args);
	virtual void			PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper);




private:
	ICSPlayerAnimState *m_PlayerAnimState;

	// Last usercmd we shot a bullet on.
	int m_iLastWeaponFireUsercmd;

	// Copyed from EyeAngles() so we can send it to the client.
	CNetworkQAngle(m_angEyeAngles);

public:
	bool				m_bResumeZoom;
	int					m_iLastZoom;			// after firing a shot, set the FOV to 90, and after showing the animation, bring the FOV back to last zoom level.

protected:
	virtual void		PreThink(void);
	virtual	void		PostThink(void);


};

inline CDC_Player *ToDCPlayer(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

	return dynamic_cast<CDC_Player*>(pEntity);
}

#endif