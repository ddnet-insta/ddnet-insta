#ifndef GAME_SERVER_GAMEMODES_BASE_PVP_BASE_PVP_H
#define GAME_SERVER_GAMEMODES_BASE_PVP_BASE_PVP_H

#include <game/server/gamemodes/insta_core/insta_core.h>
#include <game/server/instagib/extra_columns.h>

// base pvp functionality such as damage
// should be inherited from in all pvp modes such as ctf, dm, zcatch and so on
// if you need a more raw controller that is not focused on pvp
// you can also inherit from CGameControllerInstaCore directly
// but you might be missing out on some things
// technically you can also build a simple ddrace block mode based on the pvp controller
//
// things base pvp handles that insta core does not handle:
// - team balance
// - spawn protection
// - killing sprees
// - database stats for kills/deaths and more
// - fastcap flag capture time ranks in database
// - rounds (timelimit, scorelimit, score)
// - pause the game and ready mode
// - vanilla weapon tuning and damage
// - network clipping against zoom cheats
class CGameControllerBasePvp : public CGameControllerInstaCore
{
public:
	CGameControllerBasePvp(class CGameContext *pGameServer);
	~CGameControllerBasePvp() override;

	bool BlockFirstShotOnSpawn(class CCharacter *pChr, int Weapon) const;
	bool BlockFullAutoUntilRepress(class CCharacter *pChr, int Weapon) const;
	void OnInit() override;
	void OnPlayerConnect(CPlayer *pPlayer) override;
	void OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason) override;
	void DoTeamChange(CPlayer *pPlayer, int Team, bool DoChatMsg) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
	int SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags) override;
	int SnapGameInfoExFlags2(int SnappingClient, int DDRaceFlags) override;
	int SnapPlayerScore(int SnappingClient, CPlayer *pPlayer, int DDRaceScore) override;
	int GetDefaultWeapon(class CPlayer *pPlayer) override { return m_DefaultWeapon; }
	void OnDDRaceTimeLoad(class CPlayer *pPlayer, float Time) override {}
	bool LoadNewPlayerNameData(int ClientId) override;
	void OnLoadedNameStats(const CSqlStatsPlayer *pStats, class CPlayer *pPlayer) override;
	bool OnTeamChatCmd(IConsole::IResult *pResult) override;
	bool OnSetDDRaceTeam(int ClientId, int Team) override;

	// chat.cpp
	bool AllowPublicChat(const CPlayer *pPlayer);
	bool ParseChatCmd(char Prefix, int ClientId, const char *pCmdWithArgs);
	bool IsChatBlocked(const CNetMsg_Cl_Say *pMsg, int Length, int Team, CPlayer *pPlayer) const;
	bool OnBangCommand(int ClientId, const char *pCmd, int NumArgs, const char **ppArgs);
	void SmartChatTick();
	bool DetectedCasualRound();
	void DoWarmup(int Seconds) override;

	void AddSpree(CPlayer *pPlayer);
	void EndSpree(CPlayer *pPlayer, CPlayer *pKiller);
	void CheckForceUnpauseGame();

	// ddnet-insta only
	// return false to not cause any damage
	bool OnLaserHit(int Bounces, int From, int Weapon, CCharacter *pVictim) override;
	void OnExplosionHits(int OwnerId, CExplosionTarget *pTargets, int NumTargets) override;
	void OnHammerHit(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force) override;
	void ApplyFngHammerForce(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force) override;
	void FngUnmeltHammerHit(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force) override;
	void OnKill(class CPlayer *pVictim, class CPlayer *pKiller, int Weapon) override;
	bool DecreaseHealthAndKill(int Dmg, int From, int Weapon, CCharacter *pCharacter) override;
	bool SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce) override;
	void OnAnyDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter *pCharacter) override;
	void OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter) override;
	bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
	bool OnChatMessage(const CNetMsg_Cl_Say *pMsg, int Length, int &Team, CPlayer *pPlayer) override;
	bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) override;
	void SetArmorProgress(CCharacter *pCharacter, int Progress) override {}
	void SetArmorProgressFull(CCharacter *pCharacter) override {}
	void SetArmorProgressEmpty(CCharacter *pCharacter) override {}
	void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnShowRank(int Rank, int RankedScore, const char *pRankType, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnRoundStart() override;
	void OnRoundEnd() override;
	bool IsGrenadeGameType() const override;
	bool IsDDRaceGameType() const override { return false; }
	void OnFlagCapture(class CFlag *pFlag, float Time, int TimeTicks) override;
	bool ForceNetworkClipping(const CEntity *pEntity, int SnappingClient, vec2 CheckPos) override;
	bool ForceNetworkClippingLine(const CEntity *pEntity, int SnappingClient, vec2 StartPos, vec2 EndPos) override;

	// pPlayer is the player that just hit
	// an enemy with the grenade
	//
	// can be called multiple times for one bullet
	// of the explosion has multiple hits
	void RefillGrenadesOnHit(CPlayer *pPlayer);

	bool IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage) override;
	bool IsLoser(const CPlayer *pPlayer) override;
	bool IsStatTrack(char *pReason = nullptr, int SizeOfReason = 0) override;
	void SaveStatsOnRoundEnd(CPlayer *pPlayer) override;
	void SaveStatsOnDisconnect(CPlayer *pPlayer) override;

	// generic helpers

	bool IsSpawnProtected(const CPlayer *pVictim, const CPlayer *pKiller) const;

	// returns the amount of tee's that are not spectators
	int NumActivePlayers();

	// returns the amount of players that currently have a tee in the world
	int NumAlivePlayers();

	// different than NumAlivePlayers()
	// it does check m_IsDead which is set in OnCharacterDeath
	// instead of checking the character which only gets destroyed
	// after OnCharacterDeath
	//
	// needed for the wincheck in zcatch to get triggered on kill
	int NumNonDeadActivePlayers();

	// returns the client id of the player with the highest
	// killing spree (active spree not high score)
	// returns -1 if nobody made a kill since spawning
	int GetHighestSpreeClientId();

	/*
		m_pExtraColumns

		Should be allocated in the gamemmodes constructor and will be freed by the base constructor.
		It holds a few methods that describe the extension of the base database layout.
		If a gamemode needs more columns it can implement one. Otherwise it will be a nullptr which is fine.

		Checkout gctf/gctf.h gctf/gctf.cpp and gctf/sql_columns.h for an example
	*/
	CExtraColumns *m_pExtraColumns = nullptr;
};
#endif // GAME_SERVER_GAMEMODES_BASE_PVP_BASE_PVP_H
