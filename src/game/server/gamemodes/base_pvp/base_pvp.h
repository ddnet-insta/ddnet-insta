#ifndef GAME_SERVER_GAMEMODES_BASE_PVP_BASE_PVP_H
#define GAME_SERVER_GAMEMODES_BASE_PVP_BASE_PVP_H

#include <game/server/instagib/extra_columns.h>
#include <game/server/instagib/sql_stats.h>

#include "../DDRace.h"

class CGameControllerPvp : public CGameControllerDDRace
{
public:
	CGameControllerPvp(class CGameContext *pGameServer);
	~CGameControllerPvp() override;

	// convience accessors to copy code from gamecontext to the instagib controller
	class IServer *Server() const { return GameServer()->Server(); }
	class CConfig *Config() { return GameServer()->Config(); }
	class IConsole *Console() { return GameServer()->Console(); }
	class IStorage *Storage() { return GameServer()->Storage(); }

	void SendChatTarget(int To, const char *pText, int Flags = CGameContext::FLAG_SIX | CGameContext::FLAG_SIXUP) const;
	void SendChat(int ClientId, int Team, const char *pText, int SpamProtectionClientId = -1, int Flags = CGameContext::FLAG_SIX | CGameContext::FLAG_SIXUP);

	void OnCharacterConstruct(class CCharacter *pChr);
	void OnPlayerTick(class CPlayer *pPlayer);
	void OnCharacterTick(class CCharacter *pChr);

	bool CanSpawn(int Team, vec2 *pOutPos, int DDTeam) override;
	bool BlockFirstShotOnSpawn(class CCharacter *pChr, int Weapon) const;
	void SendChatSpectators(const char *pMessage, int Flags);
	void OnInit() override;
	void OnPlayerConnect(CPlayer *pPlayer) override;
	void OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason) override;
	void DoTeamChange(CPlayer *pPlayer, int Team, bool DoChatMsg) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	int GetAutoTeam(int NotThisId) override;
	int SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags) override;
	int SnapGameInfoExFlags2(int SnappingClient, int DDRaceFlags) override;
	int SnapPlayerFlags7(int SnappingClient, CPlayer *pPlayer, int PlayerFlags7) override;
	void SnapPlayer6(int SnappingClient, CPlayer *pPlayer, CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo) override;
	void SnapDDNetPlayer(int SnappingClient, CPlayer *pPlayer, CNetObj_DDNetPlayer *pDDNetPlayer) override;
	int SnapPlayerScore(int SnappingClient, CPlayer *pPlayer, int DDRaceScore) override;
	int GetDefaultWeapon(class CPlayer *pPlayer) override { return m_DefaultWeapon; }
	int GetPlayerTeam(class CPlayer *pPlayer, bool Sixup) override;
	void OnUpdateSpectatorVotesConfig() override;
	bool OnSetTeamNetMessage(const CNetMsg_Cl_SetTeam *pMsg, int ClientId) override;
	void OnDDRaceTimeLoad(class CPlayer *pPlayer, float Time) override{};
	void RoundInitPlayer(class CPlayer *pPlayer) override;
	void InitPlayer(class CPlayer *pPlayer) override;
	bool LoadNewPlayerNameData(int ClientId) override;
	void OnLoadedNameStats(const CSqlStatsPlayer *pStats, class CPlayer *pPlayer) override;
	void OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData) override;
	void OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData) override;
	bool OnSkinChange7(protocol7::CNetMsg_Cl_SkinChange *pMsg, int ClientId) override;

	void ModifyWeapons(IConsole::IResult *pResult, void *pUserData, int Weapon, bool Remove);

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

	/* UpdateSpawnWeapons
	 *
	 * called when sv_spawn_weapons is updated
	 */
	void UpdateSpawnWeapons(bool Silent, bool Apply) override;
	enum ESpawnWeapons
	{
		SPAWN_WEAPON_LASER,
		SPAWN_WEAPON_GRENADE,
		NUM_SPAWN_WEAPONS
	};
	ESpawnWeapons m_SpawnWeapons;
	ESpawnWeapons GetSpawnWeapons(int ClientId) const { return m_SpawnWeapons; }
	int GetDefaultWeaponBasedOnSpawnWeapons() const;
	void SetSpawnWeapons(class CCharacter *pChr) override;

	// ddnet-insta only
	// return false to not cause any damage
	bool OnLaserHit(int Bounces, int From, int Weapon, CCharacter *pVictim) override;
	bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
	bool OnChatMessage(const CNetMsg_Cl_Say *pMsg, int Length, int &Team, CPlayer *pPlayer) override;
	bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) override;
	void SetArmorProgress(CCharacter *pCharacer, int Progress) override{};
	void SetArmorProgressFull(CCharacter *pCharacer) override{};
	void SetArmorProgressEmpty(CCharacter *pCharacer) override{};
	bool OnVoteNetMessage(const CNetMsg_Cl_Vote *pMsg, int ClientId) override;
	void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnShowRank(int Rank, int RankedScore, const char *pRankType, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnRoundStart() override;
	void OnRoundEnd() override;
	bool IsGrenadeGameType() const override;
	bool IsDDRaceGameType() const override { return false; }
	void OnFlagCapture(class CFlag *pFlag, float Time, int TimeTicks) override;
	bool ForceNetworkClipping(const CEntity *pEntity, int SnappingClient, vec2 CheckPos) override;
	bool ForceNetworkClippingLine(const CEntity *pEntity, int SnappingClient, vec2 StartPos, vec2 EndPos) override;

	bool IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage) override;
	bool IsLoser(const CPlayer *pPlayer) override;
	bool IsStatTrack(char *pReason = nullptr, int SizeOfReason = 0) override;
	void SaveStatsOnRoundEnd(CPlayer *pPlayer) override;
	void SaveStatsOnDisconnect(CPlayer *pPlayer) override;

	bool m_InvalidateConnectedIpsCache = true;
	int m_NumConnectedIpsCached = 0;

	// Anticamper
	void Anticamper();

	// generic helpers

	// displays fng styled laser text points in the world
	void MakeLaserTextPoints(vec2 Pos, int Points, int Seconds);

	// plays the satisfying hit sound
	// that is used in teeworlds when a projectile causes damage
	// in ddnet-insta it is used the same way in CTF/DM
	// but also for instagib weapons
	//
	// the sound name is SOUND_HIT
	// and it is only audible to the player who caused the damage
	// and to the spectators of that player
	void DoDamageHitSound(int KillerId);

	bool IsSpawnProtected(CPlayer *pVictim, CPlayer *pKiller) const;

	// returns the amount of tee's that are not spectators
	int NumActivePlayers();

	// returns the amount of players that currently have a tee in the world
	int NumAlivePlayers();

	// cached amount of unique ips
	int NumConnectedIps();

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

	// get the lowest client id that has a tee in the world
	// returns -1 if no player is alive
	int GetFirstAlivePlayerId();

	// kills the tee of all connected players
	void KillAllPlayers();

	/*
		m_pExtraColums

		Should be allocated in the gamemmodes constructor and will be freed by the base constructor.
		It holds a few methods that describe the extension of the base database layout.
		If a gamemode needs more columns it can implement one. Otherwise it will be a nullptr which is fine.

		Checkout gctf/gctf.h gctf/gctf.cpp and gctf/sql_columns.h for an example
	*/
	CExtraColumns *m_pExtraColumns = nullptr;

	// Used for sv_punish_freeze_disconnect
	// restore freeze state on reconnect
	// this is used for players trying to bypass
	// getting frozen in fng or by anticamper
	std::vector<NETADDR> m_vFrozenQuitters;
	int64_t m_ReleaseAllFrozenQuittersTick = 0;
	void RestoreFreezeStateOnRejoin(CPlayer *pPlayer);
};
#endif // GAME_SERVER_GAMEMODES_BASE_PVP_BASE_PVP_H
