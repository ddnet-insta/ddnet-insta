#ifndef GAME_SERVER_INSTAGIB_STRUCTS_H
#define GAME_SERVER_INSTAGIB_STRUCTS_H

#include <base/vmath.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>

// ddnet-insta specific "structs"
// classes that only hold data no behavior
// and are shared across multiple parts of the code
//
// in my head these things are called structs
// and not classes.
//
// but to follow the ddnet style guide they technically
// will be classes also with the C prefix in the name

// Holds information about a character
// that got hit by an explosion
class CExplosionTarget
{
public:
	// the character that got hit
	class CCharacter *m_pCharacter = nullptr;

	// push back from the explosion
	vec2 m_Force;

	// amount of damage the explosion caused
	int m_Dmg;

	// the weapon that created the explosion
	// could be for example WEAPON_GRENADE
	int m_Weapon = WEAPON_GRENADE;

	bool m_NoDamage = false;

	// Can be -1
	// The ddrace team that the explosion was triggered from
	int m_ActivatedTeam = -1;

	// Mask with the client ids that will receive the explosion event
	// this is used to hide the explosion from players that
	// are in a different ddrace teams
	CClientMask m_NetworkMask;

	// Mask with the client ids that can be damaged by the explosion
	// Should be ignored if sv_sprayprotection is unset
	// If spray protection is on only players for which
	// this evaluates to true should be affected by the explosion
	//
	// SprayMask.test(pPlayer->GetCid()
	//
	CClientMask m_SprayMask;

	// the position of the explosion
	vec2 m_ExplosionCenter;

	void Init(
		class CCharacter *pCharacter,
		vec2 Force,
		int Dmg,
		int Weapon,
		bool NoDamage,
		int ActivatedTeam,
		CClientMask NetworkMask,
		CClientMask SprayMask,
		vec2 ExplosionCenter)
	{
		m_pCharacter = pCharacter;
		m_Force = Force;
		m_Dmg = Dmg;
		m_Weapon = Weapon;
		m_NoDamage = NoDamage;
		m_ActivatedTeam = ActivatedTeam;
		m_NetworkMask = NetworkMask;
		m_SprayMask = SprayMask;
		m_ExplosionCenter = ExplosionCenter;
	}
};

#endif
