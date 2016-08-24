#include "cbase.h"
#include "c_dc_player.h"
#include "c_basetempentity.h"

#if defined(CDC_Player)
#undef CDC_Player
#endif

class CAddonInfo
{
public:
	const char *m_pAttachmentName;
	const char *m_pWeaponClassName;	// The addon uses the w_ model from this weapon.
	const char *m_pModelName;		//If this is present, will use this model instead of looking up the weapon 
	const char *m_pHolsterName;
};



// These must follow the ADDON_ ordering.
CAddonInfo g_AddonInfo[] =
{
	{ "grenade0", "weapon_flashbang", 0, 0 },
	{ "grenade1", "weapon_flashbang", 0, 0 },
	{ "grenade2", "weapon_hegrenade", 0, 0 },
	{ "grenade3", "weapon_smokegrenade", 0, 0 },
	{ "c4", "weapon_c4", 0, 0 },
	{ "defusekit", 0, "models/weapons/w_defuser.mdl", 0 },
	{ "primary", 0, 0, 0 },	// Primary addon model is looked up based on m_iPrimaryAddon
	{ "pistol", 0, 0, 0 },	// Pistol addon model is looked up based on m_iSecondaryAddon
	{ "eholster", 0, "models/weapons/w_eq_eholster_elite.mdl", "models/weapons/w_eq_eholster.mdl" },
};

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS(C_TEPlayerAnimEvent, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate(DataUpdateType_t updateType)
	{
		// Create the effect.
		C_DC_Player *pPlayer = dynamic_cast< C_DC_Player* >(m_hPlayer.Get());
		if (pPlayer && !pPlayer->IsDormant())
		{
			pPlayer->DoAnimationEvent((PlayerAnimEvent_t)m_iEvent.Get(), m_nData);
		}
	}

public:
	CNetworkHandle(CBasePlayer, m_hPlayer);
	CNetworkVar(int, m_iEvent);
	CNetworkVar(int, m_nData);
};

IMPLEMENT_CLIENTCLASS_EVENT(C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent);

BEGIN_RECV_TABLE_NOBASE(C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent)
RecvPropEHandle(RECVINFO(m_hPlayer)),
RecvPropInt(RECVINFO(m_iEvent)),
RecvPropInt(RECVINFO(m_nData))
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS(player, C_DC_Player);

IMPLEMENT_CLIENTCLASS_DT(C_DC_Player, DT_DC_Player, CDC_Player)
RecvPropInt(RECVINFO(m_iShotsFired)),
RecvPropInt(RECVINFO(m_iThrowGrenadeCounter)),
RecvPropFloat(RECVINFO(m_angEyeAngles[0])),
RecvPropFloat(RECVINFO(m_angEyeAngles[1])),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA(C_DC_Player)
END_PREDICTION_DATA()


C_DC_Player::C_DC_Player() :
m_iv_angEyeAngles("C_DC_Player::m_iv_angEyeAngles")
{
	m_PlayerAnimState = CreatePlayerAnimState(this, this, LEGANIM_9WAY, true);

	m_angEyeAngles.Init();

	AddVar(&m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR);
}

void C_DC_Player::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	if (event == PLAYERANIMEVENT_THROW_GRENADE)
	{
		// Let the server handle this event. It will update m_iThrowGrenadeCounter and the client will
		// pick up the event in CCSPlayerAnimState.
	}
	else
	{
		m_PlayerAnimState->DoAnimationEvent(event, nData);
	}
}

void C_DC_Player::OnPreDataChanged(DataUpdateType_t type)
{
	BaseClass::OnPreDataChanged(type);
}

void C_DC_Player::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);
}

void C_DC_Player::PostDataUpdate(DataUpdateType_t updateType)
{
	BaseClass::PostDataUpdate(updateType);
}

void C_DC_Player::UpdateClientSideAnimation()
{
	// We do this in a different order than the base class.
	// We need our cycle to be valid for when we call the playeranimstate update code, 
	// or else it'll synchronize the upper body anims with the wrong cycle.
	if (GetSequence() != -1)
	{
		// move frame forward
		FrameAdvance(0.0f); // 0 means to use the time we last advanced instead of a constant
	}

	if (this == C_DC_Player::GetLocalDCPlayer())
		m_PlayerAnimState->Update(EyeAngles()[YAW], EyeAngles()[PITCH]);
	else
		m_PlayerAnimState->Update(m_angEyeAngles[YAW], m_angEyeAngles[PITCH]);
	;

	if (GetSequence() != -1)
	{
		// latch old values
		OnLatchInterpolatedVariables(LATCH_ANIMATION_VAR);
	}
}

const QAngle& C_DC_Player::GetRenderAngles()
{
	if (IsRagdoll())
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}

CWeaponCSBase* C_DC_Player::GetActiveCSWeapon() const
{
	return dynamic_cast< CWeaponCSBase* >(GetActiveWeapon());
}

void C_DC_Player::ClientThink()
{
	BaseClass::ClientThink();
}

C_DC_Player::~C_DC_Player()
{
	if (m_PlayerAnimState)
	{
		m_PlayerAnimState->Release();
	}
}