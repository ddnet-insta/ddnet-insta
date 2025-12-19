#ifndef GAME_SERVER_INSTAGIB_ENTITIES_TEXT_LASER_H
#define GAME_SERVER_INSTAGIB_ENTITIES_TEXT_LASER_H

#include "text.h"

class CLaserText : public CText
{
	const float m_CellSize = 16.0f;

public:
	CLaserText(CGameWorld *pGameWorld, CClientMask Mask, vec2 Pos, int AliveTicks, const char *pText);

	void Snap(int SnappingClient) override;
};

#endif
