/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_DDNET_PVP_PVP_LASER_H
#define GAME_SERVER_ENTITIES_DDNET_PVP_PVP_LASER_H

#include <game/server/entities/laser.h>

class CPvpLaser : public CLaser
{
public:
	CPvpLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Type);

	virtual void Tick() override;

protected:
	bool HitCharacterPvp(vec2 From, vec2 To);
	void DoBouncePvp();

	int m_FireAckedTick = -1; //rollback (ddnet-insta specific)
};

#endif
