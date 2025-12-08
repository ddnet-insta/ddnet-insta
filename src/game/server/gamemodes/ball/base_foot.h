#ifndef GAME_SERVER_GAMEMODES_BALL_BASE_FOOT_H
#define GAME_SERVER_GAMEMODES_BALL_BASE_FOOT_H

#include "../instagib/base_instagib.h"

#include <game/server/instagib/entities/foot/foot_projectile.h>

#endif
class CGameControllerBaseFoot : public CGameControllerBasePvp
{
public:
	CGameControllerBaseFoot(class CGameContext *pGameServer);
	~CGameControllerBaseFoot() override;

	int m_aBallTickSpawning[NUM_DDRACE_TEAMS];

	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	void Tick() override;
	void OnReset() override;
	void OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason) override;
	void SnapDDNetCharacter(int SnappingClient, CCharacter *pChr, CNetObj_DDNetCharacter *pDDNetCharacter) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter) override;
	void BallReset(int DDrTeam, int Seconds) override;
	bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	bool SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce) override;
	bool DoWincheckRound() override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	int SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags) override;

	void OnAnyDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter *pCharacter) override {} // empty
	bool OnSelfkill(int ClientId) override { return true; }
	bool DecreaseHealthAndKill(int Dmg, int From, int Weapon, CCharacter *pCharacter) override { return false; }

	virtual void FireGrenade(CCharacter *Character, vec2 Direction, vec2 MouseTarget, vec2 ProjStartPos);
	virtual void OnAnyGoal(CPlayer *pPlayer, CFootProjectile *pProj);
	virtual void OnGoal(CPlayer *pPlayer, CFootProjectile *pProj);
	virtual void OnWrongGoal(CPlayer *pPlayer, CFootProjectile *pProj);
	virtual void OnTeamReset(int Team);
	virtual int TeamFromGoalTile(int Tile);

	bool IsFootGameType() const override { return true; }
};
