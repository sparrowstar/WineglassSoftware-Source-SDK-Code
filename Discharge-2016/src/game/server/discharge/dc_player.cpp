// Purpose: Player for Discharge.

#include "cbase.h"
#include "dc_player.h"
#include "discharge/weapon_csbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define PLAYER_MODEL ("models/player/ct_urban.mdl")


// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS(CTEPlayerAnimEvent, CBaseTempEntity);
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent(const char *name) : CBaseTempEntity(name)
	{
	}

	CNetworkHandle(CBasePlayer, m_hPlayer);
	CNetworkVar(int, m_iEvent);
	CNetworkVar(int, m_nData);
};

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEPlayerAnimEvent, DT_TEPlayerAnimEvent)
SendPropEHandle(SENDINFO(m_hPlayer)),
SendPropInt(SENDINFO(m_iEvent), Q_log2(PLAYERANIMEVENT_COUNT) + 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_nData), 32)
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent("PlayerAnimEvent");

void TE_PlayerAnimEvent(CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData)
{
	CPVSFilter filter((const Vector&)pPlayer->EyePosition());

	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create(filter, 0);
}

CDC_Player::CDC_Player()
{
	m_PlayerAnimState = CreatePlayerAnimState(this, this, LEGANIM_9WAY, true);

	UseClientSideAnimation();

	m_iThrowGrenadeCounter = 0;
}

#define THROWGRENADE_COUNTER_BITS 3

LINK_ENTITY_TO_CLASS(player, CDC_Player);

PRECACHE_REGISTER(player);


IMPLEMENT_SERVERCLASS_ST(CDC_Player, DT_DC_Player)
SendPropInt(SENDINFO(m_iShotsFired), 8, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iThrowGrenadeCounter), THROWGRENADE_COUNTER_BITS, SPROP_UNSIGNED),
END_SEND_TABLE()

void CDC_Player::Precache(void)
{
	BaseClass::Precache();
	PrecacheModel(PLAYER_MODEL);
}

void CDC_Player::PreThink(void)
{
	BaseClass::PreThink();
}

void CDC_Player::PostThink(void)
{
	BaseClass::PostThink();
	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

	m_PlayerAnimState->Update(m_angEyeAngles[YAW], m_angEyeAngles[PITCH]);
}

void CDC_Player::Spawn(void)
{
	SetModel(PLAYER_MODEL);

	BaseClass::Spawn();
}

void CDC_Player::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	if (event == PLAYERANIMEVENT_THROW_GRENADE)
	{
		// Grenade throwing has to synchronize exactly with the player's grenade weapon going away,
		// and events get delayed a bit, so we let CCSPlayerAnimState pickup the change to this
		// variable.
		m_iThrowGrenadeCounter = (m_iThrowGrenadeCounter + 1) % (1 << THROWGRENADE_COUNTER_BITS);
	}
	else
	{
		m_PlayerAnimState->DoAnimationEvent(event, nData);
		TE_PlayerAnimEvent(this, event, nData);	// Send to any clients who can see this guy.
	}
}

static unsigned int s_BulletGroupCounter = 0;

void CDC_Player::StartNewBulletGroup()
{
	s_BulletGroupCounter++;
}

void CDC_Player::FireBullets(const FireBulletsInfo_t &info)
{
	NoteWeaponFired();

	BaseClass::FireBullets(info);
}

void CDC_Player::NoteWeaponFired()
{
	Assert(m_pCurrentCommand);
	if (m_pCurrentCommand)
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

CWeaponCSBase* CDC_Player::GetActiveCSWeapon() const
{
	return dynamic_cast< CWeaponCSBase* >(GetActiveWeapon());
}

void CDC_Player::ImpulseCommands()
{
	BaseClass::ImpulseCommands();
}

bool CDC_Player::ClientCommand(const CCommand &args)
{
	return BaseClass::ClientCommand(args);
}

void CDC_Player::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	BaseClass::PlayerRunCommand(ucmd, moveHelper);
}

CDC_Player::~CDC_Player(void)
{

}

