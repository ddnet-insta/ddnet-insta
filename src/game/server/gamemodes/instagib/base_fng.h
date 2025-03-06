#ifndef GAME_SERVER_GAMEMODES_INSTAGIB_BASE_FNG_H
#define GAME_SERVER_GAMEMODES_INSTAGIB_BASE_FNG_H

#include <base/types.h>

#include <vector>

#include "base_instagib.h"

class CGameControllerBaseFng : public CGameControllerInstagib
{
public:
	CGameControllerBaseFng(class CGameContext *pGameServer);
	~CGameControllerBaseFng() override;

	void SetDefaultConfigValues() override;
	void Tick() override;
	void Snap(int SnappingClient) override;
	void OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason) override;
	void OnPlayerConnect(CPlayer *pPlayer) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	bool SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce) override;
	bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
	bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) override;
	int SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags) override;
	void SnapDDNetCharacter(int SnappingClient, CCharacter *pChr, CNetObj_DDNetCharacter *pDDNetCharacter) override;
	CClientMask FreezeDamageIndicatorMask(class CCharacter *pChr) override;
	bool OnSelfkill(int ClientId) override;
	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	bool OnLaserHit(int Bounces, int From, int Weapon, CCharacter *pVictim) override;
	void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnShowMultis(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;

	void UpdateScoresAndDisplayPoints(CPlayer *pKiller, int PlayerScore, int TeamScore);
	void OnSpike(class CCharacter *pChr, int SpikeTile);
	void OnWrongSpike(class CPlayer *pPlayer);

	bool IsFngGameType() const override { return true; }
};
#endif // GAME_SERVER_GAMEMODES_INSTAGIB_BASE_FNG_H
