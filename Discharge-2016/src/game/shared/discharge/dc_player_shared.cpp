#include "cbase.h"
#include "weapon_csbase.h"
#include "hl2_gamerules.h"
#include "decals.h"
#include "in_buttons.h"
#include "datacache/imdlcache.h"
#include "cs_playeranimstate.h"
#include "basecombatweapon_shared.h"
#include "util_shared.h"
#include "effect_dispatch_data.h"
#include "props_shared.h"
#include "engine/ivdebugoverlay.h"

#ifdef CLIENT_DLL
	#include "discharge/c_dc_player.h"
#else
	#include "discharge/dc_player.h"
	#include "soundent.h"
	#include "KeyValues.h"
	#include "triggers.h"
#endif

ConVar sv_showimpacts("sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point (1=both, 2=client-only, 3=server-only)");
ConVar sv_showplayerhitboxes("sv_showplayerhitboxes", "0", FCVAR_REPLICATED, "Show lag compensated hitboxes for the specified player index whenever a player fires.");

#define	CS_MASK_SHOOT (MASK_SOLID|CONTENTS_DEBRIS)

void DispatchEffect(const char *pName, const CEffectData &data);

//Pistols
ConVar mp_weapon_glock_price("mp_weapon_glock_price", "400", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_usp_price("mp_weapon_usp_price", "500", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_p228_price("mp_weapon_p228_price", "600", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_deagle_price("mp_weapon_deagle_price", "650", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_fiveseven_price("mp_weapon_fiveseven_price", "750", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_elite_price("mp_weapon_elite_price", "800", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);

//Shotguns
ConVar mp_weapon_m3_price("mp_weapon_m3_price", "1700", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_xm1014_price("mp_weapon_xm1014_price", "3000", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);

//Sub-Machineguns
ConVar mp_weapon_tmp_price("mp_weapon_tmp_price", "1250", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_mac10_price("mp_weapon_mac10_price", "1400", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_mp5navy_price("mp_weapon_mp5navy_price", "1500", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_ump45_price("mp_weapon_ump45_price", "1700", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_p90_price("mp_weapon_p90_price", "2350", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);

//Rifles
ConVar mp_weapon_famas_price("mp_weapon_famas_price", "2250", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_galil_price("mp_weapon_galil_price", "2000", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_scout_price("mp_weapon_scout_price", "2750", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_ak47_price("mp_weapon_ak47_price", "2500", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_m4a1_price("mp_weapon_m4a1_price", "3100", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_aug_price("mp_weapon_aug_price", "3500", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_sg552_price("mp_weapon_sg552_price", "3500", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_sg550_price("mp_weapon_sg550_price", "4200", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_awp_price("mp_weapon_awp_price", "4750", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);
ConVar mp_weapon_g3sg1_price("mp_weapon_g3sg1_price", "5000", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);

//Machineguns
ConVar mp_weapon_m249_price("mp_weapon_m249_price", "5750", FCVAR_REPLICATED | FCVAR_UNLOGGED | FCVAR_PRINTABLEONLY);

#ifdef _DEBUG

// This is some extra code to collect weapon accuracy stats:

struct bulletdata_s
{
	float	timedelta;	// time delta since first shot of this round
	float	derivation;	// derivation for first shoot view angle
	int		count;
};

#define STATS_MAX_BULLETS	50

static bulletdata_s s_bullet_stats[STATS_MAX_BULLETS];

Vector	s_firstImpact = Vector(0, 0, 0);
float	s_firstTime = 0;
float	s_LastTime = 0;
int		s_bulletCount = 0;

void ResetBulletStats()
{
	s_firstTime = 0;
	s_LastTime = 0;
	s_bulletCount = 0;
	s_firstImpact = Vector(0, 0, 0);
	Q_memset(s_bullet_stats, 0, sizeof(s_bullet_stats));
}

void PrintBulletStats()
{
	for (int i = 0; i<STATS_MAX_BULLETS; i++)
	{
		if (s_bullet_stats[i].count == 0)
			break;

		Msg("%3i;%3i;%.4f;%.4f\n", i, s_bullet_stats[i].count,
			s_bullet_stats[i].timedelta, s_bullet_stats[i].derivation);
	}
}

void AddBulletStat(float time, float dist, Vector &impact)
{
	if (time > s_LastTime + 2.0f)
	{
		// time delta since last shoot is bigger than 2 seconds, start new row
		s_LastTime = s_firstTime = time;
		s_bulletCount = 0;
		s_firstImpact = impact;

	}
	else
	{
		s_LastTime = time;
		s_bulletCount++;
	}

	if (s_bulletCount >= STATS_MAX_BULLETS)
		s_bulletCount = STATS_MAX_BULLETS - 1;

	if (dist < 1)
		dist = 1;

	int i = s_bulletCount;

	float offset = VectorLength(s_firstImpact - impact);

	float timedelta = time - s_firstTime;
	float derivation = offset / dist;

	float weight = (float)s_bullet_stats[i].count / (float)(s_bullet_stats[i].count + 1);

	s_bullet_stats[i].timedelta *= weight;
	s_bullet_stats[i].timedelta += (1.0f - weight) * timedelta;

	s_bullet_stats[i].derivation *= weight;
	s_bullet_stats[i].derivation += (1.0f - weight) * derivation;

	s_bullet_stats[i].count++;
}

CON_COMMAND(stats_bullets_reset, "Reset bullet stats")
{
	ResetBulletStats();
}

CON_COMMAND(stats_bullets_print, "Print bullet stats")
{
	PrintBulletStats();
}

#endif


void CDC_Player::GetBulletTypeParameters(
	int iBulletType,
	float &fPenetrationPower,
	float &flPenetrationDistance)
{
	//MIKETODO: make ammo types come from a script file.
	if (IsAmmoType(iBulletType, BULLET_PLAYER_50AE))
	{
		fPenetrationPower = 30;
		flPenetrationDistance = 1000.0;
	}
	else if (IsAmmoType(iBulletType, BULLET_PLAYER_762MM))
	{
		fPenetrationPower = 39;
		flPenetrationDistance = 5000.0;
	}
	else if (IsAmmoType(iBulletType, BULLET_PLAYER_556MM) ||
		IsAmmoType(iBulletType, BULLET_PLAYER_556MM_BOX))
	{
		fPenetrationPower = 35;
		flPenetrationDistance = 4000.0;
	}
	else if (IsAmmoType(iBulletType, BULLET_PLAYER_338MAG))
	{
		fPenetrationPower = 45;
		flPenetrationDistance = 8000.0;
	}
	else if (IsAmmoType(iBulletType, BULLET_PLAYER_9MM))
	{
		fPenetrationPower = 21;
		flPenetrationDistance = 800.0;
	}
	else if (IsAmmoType(iBulletType, BULLET_PLAYER_BUCKSHOT))
	{
		fPenetrationPower = 0;
		flPenetrationDistance = 0.0;
	}
	else if (IsAmmoType(iBulletType, BULLET_PLAYER_45ACP))
	{
		fPenetrationPower = 15;
		flPenetrationDistance = 500.0;
	}
	else if (IsAmmoType(iBulletType, BULLET_PLAYER_357SIG))
	{
		fPenetrationPower = 25;
		flPenetrationDistance = 800.0;
	}
	else if (IsAmmoType(iBulletType, BULLET_PLAYER_57MM))
	{
		fPenetrationPower = 30;
		flPenetrationDistance = 2000.0;
	}
	else
	{
		// What kind of ammo is this?
		Assert(false);
		fPenetrationPower = 0;
		flPenetrationDistance = 0.0;
	}
}

static void GetMaterialParameters(int iMaterial, float &flPenetrationModifier, float &flDamageModifier)
{
	switch (iMaterial)
	{
	case CHAR_TEX_METAL:
		flPenetrationModifier = 0.5;  // If we hit metal, reduce the thickness of the brush we can't penetrate
		flDamageModifier = 0.3;
		break;
	case CHAR_TEX_DIRT:
		flPenetrationModifier = 0.5;
		flDamageModifier = 0.3;
		break;
	case CHAR_TEX_CONCRETE:
		flPenetrationModifier = 0.4;
		flDamageModifier = 0.25;
		break;
	case CHAR_TEX_GRATE:
		flPenetrationModifier = 1.0;
		flDamageModifier = 0.99;
		break;
	case CHAR_TEX_VENT:
		flPenetrationModifier = 0.5;
		flDamageModifier = 0.45;
		break;
	case CHAR_TEX_TILE:
		flPenetrationModifier = 0.65;
		flDamageModifier = 0.3;
		break;
	case CHAR_TEX_COMPUTER:
		flPenetrationModifier = 0.4;
		flDamageModifier = 0.45;
		break;
	case CHAR_TEX_WOOD:
		flPenetrationModifier = 1.0;
		flDamageModifier = 0.6;
		break;
	default:
		flPenetrationModifier = 1.0;
		flDamageModifier = 0.5;
		break;
	}

	Assert(flPenetrationModifier > 0);
	Assert(flDamageModifier < 1.0f); // Less than 1.0f for avoiding infinite loops
}


static bool TraceToExit(Vector &start, Vector &dir, Vector &end, float flStepSize, float flMaxDistance)
{
	float flDistance = 0;
	Vector last = start;

	while (flDistance <= flMaxDistance)
	{
		flDistance += flStepSize;

		end = start + flDistance *dir;

		if ((UTIL_PointContents(end, MASK_SOLID)) == 0)
		{
			// found first free point
			return true;
		}
	}

	return false;
}

inline void UTIL_TraceLineIgnoreTwoEntities(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask,
	const IHandleEntity *ignore, const IHandleEntity *ignore2, int collisionGroup, trace_t *ptr)
{
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);
	CTraceFilterSkipTwoEntities traceFilter(ignore, ignore2, collisionGroup);
	enginetrace->TraceRay(ray, mask, &traceFilter, ptr);
	if (r_visualizetraces.GetBool())
	{
		DebugDrawLine(ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f);
	}
}

void CDC_Player::FireBullet(
	Vector vecSrc,	// shooting postion
	const QAngle &shootAngles,  //shooting angle
	float vecSpread, // spread vector
	float flDistance, // max distance 
	int iPenetration, // how many obstacles can be penetrated
	int iBulletType, // ammo type
	int iDamage, // base damage
	float flRangeModifier, // damage range modifier
	CBaseEntity *pevAttacker, // shooter
	bool bDoEffects,
	float x,
	float y
	)
{
	float fCurrentDamage = iDamage;   // damage of the bullet at it's current trajectory
	float flCurrentDistance = 0.0;  //distance that the bullet has traveled so far

	Vector vecDirShooting, vecRight, vecUp;
	AngleVectors(shootAngles, &vecDirShooting, &vecRight, &vecUp);

	// MIKETODO: put all the ammo parameters into a script file and allow for CS-specific params.
	float flPenetrationPower = 0;		// thickness of a wall that this bullet can penetrate
	float flPenetrationDistance = 0;	// distance at which the bullet is capable of penetrating a wall
	float flDamageModifier = 0.5;		// default modification of bullets power after they go through a wall.
	float flPenetrationModifier = 1.f;

	GetBulletTypeParameters(iBulletType, flPenetrationPower, flPenetrationDistance);


	if (!pevAttacker)
		pevAttacker = this;  // the default attacker is ourselves

	// add the spray 
	Vector vecDir = vecDirShooting +
		x * vecSpread * vecRight +
		y * vecSpread * vecUp;

	VectorNormalize(vecDir);

	//Adrian: visualize server/client player positions
	//This is used to show where the lag compesator thinks the player should be at.
#if 0 
	for (int k = 1; k <= gpGlobals->maxClients; k++)
	{
		CBasePlayer *clientClass = (CBasePlayer *)CBaseEntity::Instance(k);

		if (clientClass == NULL)
			continue;

		if (k == entindex())
			continue;

#ifdef CLIENT_DLL
		debugoverlay->AddBoxOverlay(clientClass->GetAbsOrigin(), clientClass->WorldAlignMins(), clientClass->WorldAlignMaxs(), QAngle(0, 0, 0), 255, 0, 0, 127, 4);
#else
		NDebugOverlay::Box(clientClass->GetAbsOrigin(), clientClass->WorldAlignMins(), clientClass->WorldAlignMaxs(), 0, 0, 255, 127, 4);
#endif

	}

#endif

	bool bFirstHit = true;

	CBasePlayer *lastPlayerHit = NULL;

	if (sv_showplayerhitboxes.GetInt() > 0)
	{
		CBasePlayer *lagPlayer = UTIL_PlayerByIndex(sv_showplayerhitboxes.GetInt());
		if (lagPlayer)
		{
#ifdef CLIENT_DLL
			lagPlayer->DrawClientHitboxes(4, true);
#else
			lagPlayer->DrawServerHitboxes(4, true);
#endif
		}
	}

	MDLCACHE_CRITICAL_SECTION();
	while (fCurrentDamage > 0)
	{
		Vector vecEnd = vecSrc + vecDir * flDistance;

		trace_t tr; // main enter bullet trace

		UTIL_TraceLineIgnoreTwoEntities(vecSrc, vecEnd, CS_MASK_SHOOT | CONTENTS_HITBOX, this, lastPlayerHit, COLLISION_GROUP_NONE, &tr);
		{
			CTraceFilterSkipTwoEntities filter(this, lastPlayerHit, COLLISION_GROUP_NONE);

			// Check for player hitboxes extending outside their collision bounds
			const float rayExtension = 40.0f;
			UTIL_ClipTraceToPlayers(vecSrc, vecEnd + vecDir * rayExtension, CS_MASK_SHOOT | CONTENTS_HITBOX, &filter, &tr);
		}

		lastPlayerHit = ToBasePlayer(tr.m_pEnt);

		if (tr.fraction == 1.0f)
			break; // we didn't hit anything, stop tracing shoot

#ifdef _DEBUG		
		if (bFirstHit)
			AddBulletStat(gpGlobals->realtime, VectorLength(vecSrc - tr.endpos), tr.endpos);
#endif

		bFirstHit = false;

#ifndef CLIENT_DLL
		//
		// Propogate a bullet impact event
		// @todo Add this for shotgun pellets (which dont go thru here)
		//
		IGameEvent * event = gameeventmanager->CreateEvent("bullet_impact");
		if (event)
		{
			event->SetInt("userid", GetUserID());
			event->SetFloat("x", tr.endpos.x);
			event->SetFloat("y", tr.endpos.y);
			event->SetFloat("z", tr.endpos.z);
			gameeventmanager->FireEvent(event);
		}
#endif

		/************* MATERIAL DETECTION ***********/
		surfacedata_t *pSurfaceData = physprops->GetSurfaceData(tr.surface.surfaceProps);
		int iEnterMaterial = pSurfaceData->game.material;

		GetMaterialParameters(iEnterMaterial, flPenetrationModifier, flDamageModifier);

		bool hitGrate = tr.contents & CONTENTS_GRATE;

		// since some railings in de_inferno are CONTENTS_GRATE but CHAR_TEX_CONCRETE, we'll trust the
		// CONTENTS_GRATE and use a high damage modifier.
		if (hitGrate)
		{
			// If we're a concrete grate (TOOLS/TOOLSINVISIBLE texture) allow more penetrating power.
			flPenetrationModifier = 1.0f;
			flDamageModifier = 0.99f;
		}

#ifdef CLIENT_DLL
		if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 2)
		{
			// draw red client impact markers
			debugoverlay->AddBoxOverlay(tr.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 255, 0, 0, 127, 4);

			if (tr.m_pEnt && tr.m_pEnt->IsPlayer())
			{
				C_BasePlayer *player = ToBasePlayer(tr.m_pEnt);
				player->DrawClientHitboxes(4, true);
			}
		}
#else
		if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 3)
		{
			// draw blue server impact markers
			NDebugOverlay::Box(tr.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), 0, 0, 255, 127, 4);

			if (tr.m_pEnt && tr.m_pEnt->IsPlayer())
			{
				CBasePlayer *player = ToBasePlayer(tr.m_pEnt);
				player->DrawServerHitboxes(4, true);
			}
		}
#endif

		//calculate the damage based on the distance the bullet travelled.
		flCurrentDistance += tr.fraction * flDistance;
		fCurrentDamage *= pow(flRangeModifier, (flCurrentDistance / 500));

		// check if we reach penetration distance, no more penetrations after that
		if (flCurrentDistance > flPenetrationDistance && iPenetration > 0)
			iPenetration = 0;

#ifndef CLIENT_DLL
		// This just keeps track of sounds for AIs (it doesn't play anything).
		CSoundEnt::InsertSound(SOUND_BULLET_IMPACT, tr.endpos, 400, 0.2f, this);
#endif

		int iDamageType = DMG_BULLET | DMG_NEVERGIB;

		if (bDoEffects)
		{
			// See if the bullet ended up underwater + started out of the water
			if (enginetrace->GetPointContents(tr.endpos) & (CONTENTS_WATER | CONTENTS_SLIME))
			{
				trace_t waterTrace;
				UTIL_TraceLine(vecSrc, tr.endpos, (MASK_SHOT | CONTENTS_WATER | CONTENTS_SLIME), this, COLLISION_GROUP_NONE, &waterTrace);

				if (waterTrace.allsolid != 1)
				{
					CEffectData	data;
					data.m_vOrigin = waterTrace.endpos;
					data.m_vNormal = waterTrace.plane.normal;
					data.m_flScale = random->RandomFloat(8, 12);

					if (waterTrace.contents & CONTENTS_SLIME)
					{
						data.m_fFlags |= FX_WATER_IN_SLIME;
					}

					DispatchEffect("gunshotsplash", data);
				}
			}
			else
			{
				//Do Regular hit effects

				// Don't decal nodraw surfaces
				if (!(tr.surface.flags & (SURF_SKY | SURF_NODRAW | SURF_HINT | SURF_SKIP)))
				{
					CBaseEntity *pEntity = tr.m_pEnt;
					UTIL_ImpactTrace(&tr, iDamageType);
				}
			}
		} // bDoEffects

		// add damage to entity that we hit

#ifndef CLIENT_DLL
		ClearMultiDamage();

		CTakeDamageInfo info(pevAttacker, pevAttacker, fCurrentDamage, iDamageType);
		CalculateBulletDamageForce(&info, iBulletType, vecDir, tr.endpos);
		tr.m_pEnt->DispatchTraceAttack(info, vecDir, &tr);

		TraceAttackToTriggers(info, tr.startpos, tr.endpos, vecDir);

		ApplyMultiDamage();
#endif

		// check if bullet can penetarte another entity
		if (iPenetration == 0 && !hitGrate)
			break; // no, stop

		// If we hit a grate with iPenetration == 0, stop on the next thing we hit
		if (iPenetration < 0)
			break;

		Vector penetrationEnd;

		// try to penetrate object, maximum penetration is 128 inch
		if (!TraceToExit(tr.endpos, vecDir, penetrationEnd, 24, 128))
			break;

		// find exact penetration exit
		trace_t exitTr;
		UTIL_TraceLine(penetrationEnd, tr.endpos, CS_MASK_SHOOT | CONTENTS_HITBOX, NULL, &exitTr);

		if (exitTr.m_pEnt != tr.m_pEnt && exitTr.m_pEnt != NULL)
		{
			// something was blocking, trace again
			UTIL_TraceLine(penetrationEnd, tr.endpos, CS_MASK_SHOOT | CONTENTS_HITBOX, exitTr.m_pEnt, COLLISION_GROUP_NONE, &exitTr);
		}

		// get material at exit point
		pSurfaceData = physprops->GetSurfaceData(exitTr.surface.surfaceProps);
		int iExitMaterial = pSurfaceData->game.material;

		hitGrate = hitGrate && (exitTr.contents & CONTENTS_GRATE);

		// if enter & exit point is wood or metal we assume this is 
		// a hollow crate or barrel and give a penetration bonus
		if (iEnterMaterial == iExitMaterial)
		{
			if (iExitMaterial == CHAR_TEX_WOOD ||
				iExitMaterial == CHAR_TEX_METAL)
			{
				flPenetrationModifier *= 2;
			}
		}

		float flTraceDistance = VectorLength(exitTr.endpos - tr.endpos);

		// check if bullet has enough power to penetrate this distance for this material
		if (flTraceDistance > (flPenetrationPower * flPenetrationModifier))
			break; // bullet hasn't enough power to penetrate this distance

		// penetration was successful

		// bullet did penetrate object, exit Decal
		if (bDoEffects)
		{
			UTIL_ImpactTrace(&exitTr, iDamageType);
		}

		//setup new start end parameters for successive trace

		flPenetrationPower -= flTraceDistance / flPenetrationModifier;
		flCurrentDistance += flTraceDistance;

		// NDebugOverlay::Box( exitTr.endpos, Vector(-2,-2,-2), Vector(2,2,2), 0,255,0,127, 8 );

		vecSrc = exitTr.endpos;
		flDistance = (flDistance - flCurrentDistance) * 0.5;

		// reduce damage power each time we hit something other than a grate
		fCurrentDamage *= flDamageModifier;

		// reduce penetration counter
		iPenetration--;
	}

}

void CDC_Player::SetAnimation(PLAYER_ANIM playerAnim)
{
	// In CS, its CPlayerAnimState object manages ALL the animation state.
	return;
}

bool CDC_Player::CanMove() const
{
	// When we're in intro camera mode, it's important to return false here 
	// so our physics object doesn't fall out of the world.
	if (GetMoveType() == MOVETYPE_NONE)
		return false;

	if (IsObserver())
		return true; // observers can move all the time

	return true;
}



CWeaponCSBase* CDC_Player::CSAnim_GetActiveWeapon()
{
	return GetActiveCSWeapon();
}


bool CDC_Player::CSAnim_CanMove()
{
	return CanMove();
}
