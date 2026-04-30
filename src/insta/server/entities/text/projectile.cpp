// https://github.com/FoxNet-DDNet/FoxNet/blob/master/src/game/server/foxnet/entities/text/projectile.cpp

#include "projectile.h"

#include <base/math.h>
#include <base/vmath.h>

#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/server/entity.h>
#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>

CProjectileText::CProjectileText(CGameWorld *pGameWorld, CClientMask Mask, vec2 Pos, int AliveTicks, const char *pText, int Type) :
	CText(pGameWorld, Mask, Pos, AliveTicks, pText, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_CurTicks = Server()->Tick();
	m_Pos = Pos;
	m_Type = Type;

	SetData(m_CellSize);

	GameWorld()->InsertEntity(this);
}

void CProjectileText::Snap(int SnappingClient)
{
	if(!m_Mask.test(SnappingClient))
		return;

	int Idx = 0;
	const size_t NumIds = m_pData.size();
	const int TickParity = Server()->Tick() & 1;
	for(const auto *pData : m_pData)
	{
		const vec2 Pos = pData->m_Pos - vec2(m_CenterX, 0);
		if(NetworkClipped(SnappingClient, Pos))
			continue;
		if(NumIds >= 135 && ((Idx + TickParity) & 1) != 0)
		{
			Idx++;
			continue;
		}

		CNetObj_DDNetProjectile Proj = {};
		Proj.m_X = round_to_int(Pos.x * 100.0f);
		Proj.m_Y = round_to_int(Pos.y * 100.0f);
		Proj.m_Type = m_Type;
		Proj.m_Owner = -1;
		Proj.m_StartTick = 0;
		Proj.m_VelX = 0;
		Proj.m_VelY = 0;
		Server()->SnapNewItem(pData->m_Id, Proj);
		Idx++;
	}
}
