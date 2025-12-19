#ifndef GAME_SERVER_INSTAGIB_ENTITIES_TEXT_PROJECTILE_H
#define GAME_SERVER_INSTAGIB_ENTITIES_TEXT_PROJECTILE_H

#include "text.h"

class CProjectileText : public CText
{
	const float m_CellSize = 8.0f;
	int m_Type;

public:
	CProjectileText(CGameWorld *pGameWorld, CClientMask Mask, vec2 Pos, int AliveTicks, const char *pText, int Type = WEAPON_HAMMER);

	void Snap(int SnappingClient) override;
};

#endif
