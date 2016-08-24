#ifndef C_DCPLAYER_H
#define C_DCPLAYER_H
#pragma once


#include "c_basehlplayer.h"
#include "discharge/cs_playeranimstate.h"
#include "discharge/weapon_csbase.h"

class C_DC_Player : public C_BaseHLPlayer, public ICSPlayerAnimStateHelpers
{
public:
	DECLARE_CLASS(C_DC_Player, C_BaseHLPlayer);

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_DC_Player();
	~C_DC_Player(void);

	static inline C_DC_Player* GetLocalDCPlayer()
	{
		return (C_DC_Player*)C_BasePlayer::GetLocalPlayer();
	}

	CWeaponCSBase* GetActiveCSWeapon() const;
	CWeaponCSBase* GetCSWeapon(CSWeaponID id) const;

	virtual const QAngle& GetRenderAngles();

	ICSPlayerAnimState *GetPlayerAnimState() { return m_PlayerAnimState; }
	virtual void			OnPreDataChanged(DataUpdateType_t type);
	virtual void			OnDataChanged(DataUpdateType_t type);
	virtual void			PostDataUpdate(DataUpdateType_t updateType);
	virtual void UpdateClientSideAnimation();

	virtual void		ClientThink();


public:

	ICSPlayerAnimState *m_PlayerAnimState;

	// Used to control animation state.
	Activity m_Activity;
	// Predicted variables.
	bool m_bResumeZoom;
	int m_iLastZoom;			// after firing a shot, set the FOV to 90, and after showing the animation, bring the FOV back to last zoom level.
	CNetworkVar(int, m_iThrowGrenadeCounter);	// used to trigger grenade throw animations.
	CNetworkVar(int, m_iShotsFired);	// number of shots fired recently
	CNetworkVar(bool, m_bNightVisionOn);
	CNetworkVar(bool, m_bHasNightVision);

	// Implemented in shared code.
public:

	void GetBulletTypeParameters(
		int iBulletType,
		float &fPenetrationPower,
		float &flPenetrationDistance);

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

	// Returns true if the player is allowed to move.
	bool CanMove() const;

	virtual void SetAnimation(PLAYER_ANIM playerAnim);

	// Called by shared code.
public:

	// ICSPlayerAnimState overrides.
	virtual CWeaponCSBase* CSAnim_GetActiveWeapon();
	virtual bool CSAnim_CanMove();


	void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);

private:
	C_DC_Player(const C_DC_Player &);

	QAngle				m_angEyeAngles;
	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;
};

inline C_DC_Player *ToDCPlayer(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

	return dynamic_cast<C_DC_Player*>(pEntity);
}

#endif