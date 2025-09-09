#ifndef GAME_SERVER_INSTAGIB_ROLLBACK_H
#define GAME_SERVER_INSTAGIB_ROLLBACK_H

#include <base/vmath.h>
#include <game/server/entities/character.h>

// Rollback from ICTFX and Kaizo-Insta, implemented as a class
// It have counterparts of collision related functions
// which will check collision with characters on a specific tick.
//
// The max past tick to check is defined by ROLLBACK_POSITION_HISTORY

class CGameContext;

class CRollback
{
	CGameContext *m_pGameServer;

	CGameWorld *GameWorld();
	CGameContext *GameServer();
	IServer *Server();
	CCollision *Collision();
	CTuningParams *Tuning();
	CTuningParams *TuningList();

public:
	void Init(CGameContext *pGameServer);

	inline int NormalizeTick(int Tick);

	//GameWorld
	CCharacter *IntersectCharacterOnTick(vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, const CCharacter *pNotThis, int CollideWith, const CCharacter *pThisOnly, int Tick = -1);
	int FindCharactersOnTick(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Tick = -1);

	//GameContext
	void CreateExplosionOnTick(vec2 Pos, int Owner, int Weapon, bool NoDamage, int ActivatedTeam, int Tick = -1, CClientMask Mask = CClientMask().set(), CClientMask SprayMask = CClientMask().set());
};

#endif
