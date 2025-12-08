/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_INSTAGIB_ENTITIES_DDNET_PVP_VANILLA_PROJECTILE_H
#define GAME_SERVER_INSTAGIB_ENTITIES_DDNET_PVP_VANILLA_PROJECTILE_H

#include <game/server/entities/projectile.h>

class CVanillaProjectile : public CProjectile
{
public:
	CVanillaProjectile(
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

	void Tick() override;
	void Snap(int SnappingClient) override;
};

#endif
