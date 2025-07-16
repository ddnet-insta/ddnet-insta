/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_DDNET_PVP_PVP_PROJECTILE_H
#define GAME_SERVER_ENTITIES_DDNET_PVP_PVP_PROJECTILE_H

#include <game/server/entities/projectile.h>

class CPvpProjectile : public CProjectile
{
public:
	CPvpProjectile(
		CGameWorld *pGameWorld,
		int Type,
		int Owner,
		vec2 Pos,
		vec2 Dir,
		int Span,
		bool Freeze,
		bool Explosive,
		int SoundImpact,
		vec2 InitDir,
		int Layer = 0,
		int Number = 0);

	~CPvpProjectile() override;

	void Tick() override;
	void Snap(int SnappingClient) override;

	// Kaizo-Insta projectile rollback
	// for rollback players the projectile will be forwarded
	// to match the tick they sent the fire input
	//
	// on DDNet Antiping clients it will send m_OrigStartTick on Snap so it is
	// intended to play with DDNet grenade antiping, but the best would be to find
	// a way to bypass client antiping and show what the server really has
	bool m_FirstTick;
	int m_OrigStartTick;
	bool m_FirstSnap;
	int m_aParticleIds[3]; // particles to let know others a projectile got fired, in case it was forwarded too much
};

#endif
