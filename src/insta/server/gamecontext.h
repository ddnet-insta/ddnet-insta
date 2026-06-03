#ifndef INSTA_SERVER_GAMECONTEXT_H
#define INSTA_SERVER_GAMECONTEXT_H
#undef INSTA_SERVER_GAMECONTEXT_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_IGAMECONTEXT

#include <engine/console.h>
#include <engine/http.h>
#include <engine/server.h>
#include <engine/shared/protocol.h>

#include <insta/server/enums.h>
#include <insta/server/ip_storage.h>
#include <insta/server/strhelpers.h>

#include <string>
#include <unordered_set>
#include <vector>

class CGameContext : public IGameServer
{
#endif // IN_CLASS_IGAMECONTEXT
	friend class IGameController;
	friend class CGameControllerTrainFng;

	std::unordered_map<const char *, int *, CStrHash, CStrEq> m_IntConfigs;

	// When reading /etc/hostname we cache the value
	// so changes to the filesystem do not get the gameserver
	// into inconsistent state.
	// This value is used to match logged in accounts
	// to game server instances.
	// So force logout can only be done by owning server.
	char m_aHostnameCached[512] = "";

public:
	// instagib/gamecontext.cpp
	void OnInitInstagib(bool ServerStart);
	void PrintInstaCredits();
	const char *ServerInfoClientScoreKind() override;
	void AlertOnSpecialInstagibConfigs(int ClientId = -1) const;
	void ShowCurrentInstagibConfigsMotd(int ClientId = -1, bool Force = false) const;
	void RefreshVotes();
	void SendBroadcastSix(const char *pText, bool Important = true);
	void PlayerReadyStateBroadcast();
	void SendGameMsg(int GameMsgId, int ClientId) const;
	void SendGameMsg(int GameMsgId, int ParaI1, int ClientId) const;
	void SendGameMsg(int GameMsgId, int ParaI1, int ParaI2, int ParaI3, int ClientId) const;
	void InstagibUnstackChatMessage(char *pUnstacked, const char *pMessage, int Size);
	void SwapTeams();
	bool OnClientPacket(int ClientId, bool Sys, int MsgId, struct CNetChunk *pPacket, class CUnpacker *pUnpacker) override;
	void DeepJailId(int AdminId, int ClientId, int Minutes);
	void DeepJailIp(int AdminId, const char *pAddrStr, int Minutes);
	void UndeepJail(CIpStorage *pEntry);
	void ListDeepJails(int RequesterId) const;
	void ShuffleTeams() const;
	void RegisterIntConfigs();
	void UpdateVoteCheckboxes() const;

	// If passed "sv_port" it might return 8303
	std::optional<int> GetIntConfigValue(const char *pConfigName) const;

	/**
	 * reads the `sv_hostname` config
	 * if the config is not set it tries the following fallbacks:
	 *
	 * - /etc/hostname on linux (it will read that value only once and then cache it)
	 * - the hardcodet string "windows" on windows
	 *   because windows locally should just work
	 *   and is rarely used for a multi node production cluster
	 *
	 * this is meant to be a convenience for users to not have
	 * to set `sv_hostname` because some kind of host identifier
	 * is a hard requirement for `sv_accounts`
	 *
	 * @param pHostname buffer where the hostname will be written to if found (can be NULL)
	 * @param HostnameSize the size of the output buffer in bytes
	 *
	 * @return `true` if it found a hostname
	 */
	bool GetHostname(char *pHostname, int HostnameSize);

	// prints not allowed message in chat for ClientId and returns false
	// if calling votes with chat commands such as !shuffle or /shuffle
	// are not allowed
	// returns true and prints nothing otherwise
	bool IsChatCmdAllowed(int ClientId) const;

	/*
		Function: ChangeName
			Perform a full name change for the given player
			Sets the new name on the server side
			sends the new name to all clients in a name change message
			loads new stats for that player based on the name

			does not do ratelimiting or name claim checks

			this function is not called for ALL name changes
			do not use this as a change name event
			but as a change name action

		Arguments:
			ClientId - the client who will receive the new name
			pName - the new name that will be set
			Silent - if false it sends a chat message that the name was changed
			UpdateSixup - send a client reconnect to sixup connections so they see the new name
	*/
	void ChangeName(int ClientId, const char *pName, bool Silent, bool UpdateSixup);

	// list of names that can not be claimed
	// with the /claimname chat command
	// this is used to block names such as "nameless tee"
	std::unordered_set<std::string> m_UnclaimableNames;

	// results of the sql worker thread
	// for rcon commands operating on accounts
	std::vector<std::shared_ptr<CAccountRconCmdResult>> m_vAccountRconCmdQueryResults;

	// is set to time_get() when sv_accounts was attempted to be set to 1
	// but it failed because sv_hostname or sv_port were not set yet
	// if sv_hostname or sv_port are set later with a few seconds delay
	// we will then activate sv_accounts
	//
	// this allows any kind of ordering in semicolon separated rcon commands
	// and especially any kind of order in autoexec config files
	//
	// but it will not turn on sv_accounts if some admin manually
	// sets sv_hostname or sv_port minutes after the sv_accounts attempt
	// because that would be weird
	int64_t m_LastAccountTurnOnAttempt = 0;

	// stores the initial sv_port value
	// unless it is 0 it should be the real port
	// the server is currently using
	// because changing sv_port does not have any effects
	//
	// can probably be removed if this gets merged
	// https://github.com/ddnet/ddnet/pull/9667
	int m_ServerPortOnLaunch = 0;

	enum
	{
		MAX_LINES = 25,
		MAX_LINE_LENGTH = 256
	};
	char m_aaLastChatMessages[MAX_LINES][MAX_LINE_LENGTH];
	int m_UnstackHackCharacterOffset;
	IHttp *m_pHttp;
	CIpStorageController m_IpStorageController;

	// A copy of the m_pGameType string the controller holds
	// this is used to detect gametype changes.
	char m_aGameType[512] = "";

	// returns mutable pointer into either the offline ip storage
	// vector or into the still connected player
	// or nullptr if entry id is not found
	//
	// see also m_IpStorageController.FindEntry() to search only
	// in offline entries
	CIpStorage *FindIpStorageEntryOfflineAndOnline(int EntryId);

	// https://github.com/ddnet-insta/ddnet-insta/issues/638
	// passwords that can be used in addition to the standard "password"
	// config variable
	std::vector<std::string> m_vPasswords;

	// set by the config sv_display_score
	EDisplayScore m_DisplayScore = EDisplayScore::ROUND_POINTS;

	// bangcommands.cpp
	void BangCommandVote(int ClientId, const char *pCommand, const char *pDesc);
	void ComCallShuffleVote(int ClientId);
	void ComCallSwapTeamsVote(int ClientId);
	void ComCallSwapTeamsRandomVote(int ClientId);
	void ComDropFlag(int ClientId);

	// rcon_configs.cpp
	void RegisterInstagibCommands();
	static void ConchainInstaSettingsUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainGameinfoUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainResetInstasettingTees(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainSpawnWeapons(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainSmartChat(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainTournamentChat(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainZcatchColors(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainSpectatorVotes(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainDisplayScore(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainOnlyWallshotKills(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainAllowZoom(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainFngHammerScale(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainGrenadeAmmoRegenSetting(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainAccounts(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainClaimableNames(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	// rcon_commands.cpp
	static void ConHammer(IConsole::IResult *pResult, void *pUserData);
	static void ConGun(IConsole::IResult *pResult, void *pUserData);
	static void ConUnHammer(IConsole::IResult *pResult, void *pUserData);
	static void ConUnGun(IConsole::IResult *pResult, void *pUserData);
	static void ConGodmode(IConsole::IResult *pResult, void *pUserData);
	static void ConRainbow(IConsole::IResult *pResult, void *pUserData);
	static void ConForceReady(IConsole::IResult *pResult, void *pUserData);
	static void ConChat(IConsole::IResult *pResult, void *pUserData);
	static void ConShuffleTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConSwapTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConSwapTeamsRandom(IConsole::IResult *pResult, void *pUserData);
	static void ConForceTeamBalance(IConsole::IResult *pResult, void *pUserData);
	static void ConAddPassword(IConsole::IResult *pResult, void *pUserData);
	static void ConRemovePassword(IConsole::IResult *pResult, void *pUserData);
	static void ConClearPasswords(IConsole::IResult *pResult, void *pUserData);
	static void ConListPasswords(IConsole::IResult *pResult, void *pUserData);
	static void ConAddMapToPool(IConsole::IResult *pResult, void *pUserData);
	static void ConClearMapPool(IConsole::IResult *pResult, void *pUserData);
	static void ConRandomMapFromPool(IConsole::IResult *pResult, void *pUserData);
	static void ConPostStats(IConsole::IResult *pResult, void *pUserData);
	static void ConDeleteRoundStats(IConsole::IResult *pResult, void *pUserData);
	static void ConDeleteSessionStats(IConsole::IResult *pResult, void *pUserData);
	static void ConGctfAntibot(IConsole::IResult *pResult, void *pUserData);
	static void ConKnownAntibot(IConsole::IResult *pResult, void *pUserData);
	static void ConKickEventsAntibot(IConsole::IResult *pResult, void *pUserData);
	static void ConDeepJailId(IConsole::IResult *pResult, void *pUserData);
	static void ConDeepJailIp(IConsole::IResult *pResult, void *pUserData);
	static void ConDeepJails(IConsole::IResult *pResult, void *pUserData);
	static void ConUndeepJail(IConsole::IResult *pResult, void *pUserData);
	static void ConDumpCoords(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaPause(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaRestart(IConsole::IResult *pResult, void *pUserData);
	static void ConAccountList(IConsole::IResult *pResult, void *pUserData);
	static void ConAccountForceSetPassword(IConsole::IResult *pResult, void *pUserData);
	static void ConAccountForceLogout(IConsole::IResult *pResult, void *pUserData);
	static void ConLockAccount(IConsole::IResult *pResult, void *pUserData);
	static void ConUnlockAccount(IConsole::IResult *pResult, void *pUserData);
	static void ConAccountInfo(IConsole::IResult *pResult, void *pUserData);
	static void ConAccountStatus(IConsole::IResult *pResult, void *pUserData);
	static void ConAccountRatelimits(IConsole::IResult *pResult, void *pUserData);
	static void ConAddUnclaimableName(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveUnclaimableName(IConsole::IResult *pResult, void *pUserData);

	// chat_commands.cpp
	static bool BlockAccountOperation(CGameContext *pSelf, int ClientId, const char *pOperation);
	static void ConInstaInfo(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaModeCredits(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaCredits(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaLock(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaUnlock(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaInvite(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaJoin(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaTeam0Mode(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaTogglePause(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaToggleSpec(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaTogglePauseVoted(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaToggleSpecVoted(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaKill(IConsole::IResult *pResult, void *pUserData);
	static void ConReadyChange(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaSwap(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaSwapRandom(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaShuffle(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaDrop(IConsole::IResult *pResult, void *pUserData);
	static void ConRankCmdlist(IConsole::IResult *pResult, void *pUserData);
	static void ConTopCmdlist(IConsole::IResult *pResult, void *pUserData);
	static void ConStatsRound(IConsole::IResult *pResult, void *pUserData);
	static void ConStatsAllTime(IConsole::IResult *pResult, void *pUserData);
	static void ConMultis(IConsole::IResult *pResult, void *pUserData);
	static void ConSteals(IConsole::IResult *pResult, void *pUserData);
	static void ConRoundTop(IConsole::IResult *pResult, void *pUserData);
	static void ConRegister(IConsole::IResult *pResult, void *pUserData);
	static void ConLogin(IConsole::IResult *pResult, void *pUserData);
	static void ConLogoutAccount(IConsole::IResult *pResult, void *pUserData);
	static void ConChangePassword(IConsole::IResult *pResult, void *pUserData);
	static void ConDisplayName(IConsole::IResult *pResult, void *pUserData);
	static void ConLockName(IConsole::IResult *pResult, void *pUserData);
	static void ConSlowAccountOperation(IConsole::IResult *pResult, void *pUserData);
	static void ConScore(IConsole::IResult *pResult, void *pUserData);
	static void ConRankKills(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaRankPoints(IConsole::IResult *pResult, void *pUserData);
	static void ConTopKills(IConsole::IResult *pResult, void *pUserData);
	static void ConRankFastcaps(IConsole::IResult *pResult, void *pUserData);
	static void ConTopFastcaps(IConsole::IResult *pResult, void *pUserData);
	static void ConTopNumCaps(IConsole::IResult *pResult, void *pUserData);
	static void ConRankFlagCaptures(IConsole::IResult *pResult, void *pUserData);
	static void ConTopSpikeColors(IConsole::IResult *pResult, void *pUserData);
	static void ConSetSpawn(IConsole::IResult *pResult, void *pUserData);
	static void ConSpawnReset(IConsole::IResult *pResult, void *pUserData);

#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ;
#define MACRO_RANK_COLUMN(name, sql_name, display_name, order_by) \
	static void ConInstaRank##name(IConsole::IResult *pResult, void *pUserData);
#define MACRO_TOP_COLUMN(name, sql_name, display_name, order_by) \
	static void ConInstaTop##name(IConsole::IResult *pResult, void *pUserData);
#include <insta/server/sql_columns_all.h>
#undef MACRO_ADD_COLUMN
#undef MACRO_RANK_COLUMN
#undef MACRO_TOP_COLUMN

private:
public:
#ifndef IN_CLASS_IGAMECONTEXT
};
#endif
