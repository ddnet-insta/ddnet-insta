// https://github.com/FoxNet-DDNet/FoxNet/blob/master/src/game/server/foxnet/entities/text/laser.cpp

#include "laser.h"

#include <base/vmath.h>

#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/server/entity.h>
#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>

CLaserText::CLaserText(CGameWorld *pGameWorld, CClientMask Mask, vec2 Pos, int AliveTicks, const char *pText) :
	CText(pGameWorld, Mask, Pos, AliveTicks, pText, CGameWorld::ENTTYPE_LASER)
{
	m_CurTicks = Server()->Tick();
	m_Pos = Pos;

	SetData(m_CellSize);

	GameWorld()->InsertEntity(this);
}

void CLaserText::Snap(int SnappingClient)
{
	if(!m_Mask.test(SnappingClient))
		return;

	for(const auto *pData : m_pData)
	{
		const vec2 Pos = pData->m_Pos - vec2(m_CenterX, 0);
		if(NetworkClipped(SnappingClient, Pos))
			continue;

		CNetObj_DDNetLaser Obj = {};
		Obj.m_ToX = (float)Pos.x;
		Obj.m_ToY = (float)Pos.y;
		Obj.m_FromX = (float)Pos.x;
		Obj.m_FromY = (float)Pos.y;
		Obj.m_StartTick = Server()->Tick();
		Obj.m_Owner = -1;
		Obj.m_Type = LASERTYPE_RIFLE;
		Obj.m_Flags = LASERFLAG_NO_PREDICT;
		Server()->SnapNewItem(pData->m_Id, Obj);
	}
}
