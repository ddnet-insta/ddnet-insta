#ifndef GAME_SERVER_INSTAGIB_GAMECONTEXT_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_IGAMECONTEXT

#include <engine/console.h>
#include <engine/http.h>
#include <engine/server.h>

#include <game/server/instagib/enums.h>

class CGameContext : public IGameServer
{
#endif // IN_CLASS_IGAMECONTEXT

public:
	const char *ServerInfoClientScoreKind() override { return "points"; }

	CRollback m_Rollback; //ddnet-insta rollback;

	// instagib/gamecontext.cpp
	void OnInitInstagib();
	void AlertOnSpecialInstagibConfigs(int ClientId = -1) const;
	void ShowCurrentInstagibConfigsMotd(int ClientId = -1, bool Force = false) const;
	void UpdateVoteCheckboxes() const;
	void RefreshVotes();
	void SendBroadcastSix(const char *pText, bool Important = true);
	void PlayerReadyStateBroadcast();
	void SendGameMsg(int GameMsgId, int ClientId) const;
	void SendGameMsg(int GameMsgId, int ParaI1, int ClientId) const;
	void SendGameMsg(int GameMsgId, int ParaI1, int ParaI2, int ParaI3, int ClientId) const;
	void InstagibUnstackChatMessage(char *pUnstacked, const char *pMessage, int Size);
	void SwapTeams();
	bool OnClientPacket(int ClientId, bool Sys, int MsgId, struct CNetChunk *pPacket, class CUnpacker *pUnpacker) override;
	virtual void SetPlayerLastAckedSnapshot(int ClientId, int Tick) override; //ddnet-insta rollback

	enum
	{
		MAX_LINES = 25,
		MAX_LINE_LENGTH = 256
	};
	char m_aaLastChatMessages[MAX_LINES][MAX_LINE_LENGTH];
	int m_UnstackHackCharacterOffset;
	IHttp *m_pHttp;

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
	static void ConchainRollback(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

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
	static void ConAddMapToPool(IConsole::IResult *pResult, void *pUserData);
	static void ConClearMapPool(IConsole::IResult *pResult, void *pUserData);
	static void ConRandomMapFromPool(IConsole::IResult *pResult, void *pUserData);
	static void ConGctfAntibot(IConsole::IResult *pResult, void *pUserData);
	static void ConKnownAntibot(IConsole::IResult *pResult, void *pUserData);

	// chat_commands.cpp
	static void ConCreditsGctf(IConsole::IResult *pResult, void *pUserData);
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
	static void ConScore(IConsole::IResult *pResult, void *pUserData);
	static void ConRankKills(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaRankPoints(IConsole::IResult *pResult, void *pUserData);
	static void ConTopKills(IConsole::IResult *pResult, void *pUserData);
	static void ConRankFastcaps(IConsole::IResult *pResult, void *pUserData);
	static void ConTopFastcaps(IConsole::IResult *pResult, void *pUserData);
	static void ConTopNumCaps(IConsole::IResult *pResult, void *pUserData);
	static void ConRankFlagCaptures(IConsole::IResult *pResult, void *pUserData);
	static void ConRollback(IConsole::IResult *pResult, void *pUserData);
	static void ConTopSpikeColors(IConsole::IResult *pResult, void *pUserData);

#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ;
#define MACRO_RANK_COLUMN(name, sql_name, display_name, order_by) \
	static void ConInstaRank##name(IConsole::IResult *pResult, void *pUserData);
#define MACRO_TOP_COLUMN(name, sql_name, display_name, order_by) \
	static void ConInstaTop##name(IConsole::IResult *pResult, void *pUserData);
#include <game/server/instagib/sql_colums_all.h>
#undef MACRO_ADD_COLUMN
#undef MACRO_RANK_COLUMN
#undef MACRO_TOP_COLUMN

public:
#ifndef IN_CLASS_IGAMECONTEXT
};
#endif
