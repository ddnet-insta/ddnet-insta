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

		auto *pObj = Server()->SnapNewItem<CNetObj_DDNetLaser>(pData->m_Id);
		if(!pObj)
			return;

		pObj->m_ToX = (float)Pos.x;
		pObj->m_ToY = (float)Pos.y;
		pObj->m_FromX = (float)Pos.x;
		pObj->m_FromY = (float)Pos.y;
		pObj->m_StartTick = Server()->Tick();
		pObj->m_Owner = -1;
		pObj->m_Type = LASERTYPE_RIFLE;
		pObj->m_Flags = LASERFLAG_NO_PREDICT;
	}
}
