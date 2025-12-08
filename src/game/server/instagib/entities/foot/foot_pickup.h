#ifndef GAME_SERVER_INSTAGIB_ENTITIES_FOOT_FOOT_PICKUP_H
#define GAME_SERVER_INSTAGIB_ENTITIES_FOOT_FOOT_PICKUP_H

#include <game/server/entity.h>

class CFootPickup : public CEntity
{
public:
	static constexpr int PICKUP_PHYS_SIZE = 14;

	CFootPickup(CGameWorld *pGameWorld, int Layer = 0, int Number = 0);

	void Reset() override;
	void Tick() override;
	void TickPaused() override;
	void Snap(int SnappingClient) override;

private:
	int m_Type;
	int m_Subtype;
	int m_aSpawnTickTeam[NUM_DDRACE_TEAMS];

	// DDRace
	void Move();
	vec2 m_Core;
};

#endif
