#ifndef GAME_SERVER_INSTAGIB_ENTITIES_FOOT_FOOT_PROJECTILE_H
#define GAME_SERVER_INSTAGIB_ENTITIES_FOOT_FOOT_PROJECTILE_H

#include <game/server/entities/projectile.h>

class CFootProjectile : public CProjectile
{
public:
	CFootProjectile(
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

	int GetStartTick() const { return m_StartTick; }

	void Tick() override;
	void Snap(int SnappingClient) override;
	void DestroyBall(const vec2 &Pos, int Team, CClientMask TeamMask);
	virtual CCharacter *GetPlayerPickup(const vec2 &PrevPos, const vec2 &CurPos, CCharacter *pOwnerChar);

	int m_LastTouchedBy = -1;
	int m_LastTouchedTeam = -1;
	int m_LastTouchedTile = -1;

private:
	int m_Team = 0;
	int m_FootPickupDistance = 0;

	unsigned short m_CollisionByX = 0;
	unsigned short m_CollisionByY = 0;
};

#endif
