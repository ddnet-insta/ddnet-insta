#ifndef GAME_SERVER_INSTAGIB_GAMECONTEXT_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_IGAMECONTEXT

#include <string>
#include <vector>

#include <engine/console.h>
#include <engine/http.h>
#include <engine/server.h>

#include <game/server/instagib/enums.h>

class CGameContext : public IGameServer
{
#endif // IN_CLASS_IGAMECONTEXT

public:
	const char *ServerInfoPlayerScoreKind() override { return "points"; }

	// set by the config sv_display_score
	EDisplayScore m_DisplayScore = EDisplayScore::ROUND_POINTS;

	// bang commands
	void BangCommandVote(int ClientId, const char *pCommand, const char *pDesc);
	void ComCallShuffleVote(int ClientId);
	void ComCallSwapTeamsVote(int ClientId);
	void ComCallSwapTeamsRandomVote(int ClientId);
	void ComDropFlag(int ClientId);

	void SendGameMsg(int GameMsgId, int ClientId) const;
	void SendGameMsg(int GameMsgId, int ParaI1, int ClientId) const;
	void SendGameMsg(int GameMsgId, int ParaI1, int ParaI2, int ParaI3, int ClientId) const; // ddnet-insta
	void UpdateVoteCheckboxes() const;
	void RefreshVotes();
	void AlertOnSpecialInstagibConfigs(int ClientId = -1) const;
	void ShowCurrentInstagibConfigsMotd(int ClientId = -1, bool Force = false) const;
	void PlayerReadyStateBroadcast();
	void SendBroadcastSix(const char *pText, bool Important = true);
	enum
	{
		MAX_LINES = 25,
		MAX_LINE_LENGTH = 256
	};
	char m_aaLastChatMessages[MAX_LINES][MAX_LINE_LENGTH];
	int m_UnstackHackCharacterOffset;
	void InstagibUnstackChatMessage(char *pUnstacked, const char *pMessage, int Size);
	void RegisterInstagibCommands();
	void SwapTeams();
	IHttp *m_pHttp;
	void OnInitInstagib();
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

	// rcon
	static void ConHammer(IConsole::IResult *pResult, void *pUserData);
	static void ConGun(IConsole::IResult *pResult, void *pUserData);
	static void ConUnHammer(IConsole::IResult *pResult, void *pUserData);
	static void ConUnGun(IConsole::IResult *pResult, void *pUserData);
	static void ConGodmode(IConsole::IResult *pResult, void *pUserData);
	static void ConRainbow(IConsole::IResult *pResult, void *pUserData);
	static void ConForceReady(IConsole::IResult *pResult, void *pUserData);
	static void ConShuffleTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConSwapTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConSwapTeamsRandom(IConsole::IResult *pResult, void *pUserData);
	static void ConAddMapToPool(IConsole::IResult *pResult, void *pUserData);
	static void ConClearMapPool(IConsole::IResult *pResult, void *pUserData);
	static void ConRandomMapFromPool(IConsole::IResult *pResult, void *pUserData);

	// chat
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
	static void ConScore(IConsole::IResult *pResult, void *pUserData);
	static void ConRankKills(IConsole::IResult *pResult, void *pUserData);
	static void ConInstaRankPoints(IConsole::IResult *pResult, void *pUserData);
	static void ConTopKills(IConsole::IResult *pResult, void *pUserData);
	static void ConRankFastcaps(IConsole::IResult *pResult, void *pUserData);
	static void ConTopFastcaps(IConsole::IResult *pResult, void *pUserData);
	static void ConTopNumCaps(IConsole::IResult *pResult, void *pUserData);
	static void ConRankFlagCaptures(IConsole::IResult *pResult, void *pUserData);

#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ;
#define MACRO_RANK_COLUMN(name, sql_name, display_name, order_by) \
	static void ConInstaRank##name(IConsole::IResult *pResult, void *pUserData);
#define MACRO_TOP_COLUMN(name, sql_name, display_name, order_by) \
	static void ConInstaTop##name(IConsole::IResult *pResult, void *pUserData);
#include <game/server/instagib/sql_colums_all.h>
#undef MACRO_ADD_COLUMN
#undef MACRO_RANK_COLUMN
#undef MACRO_TOP_COLUMN

	static void ConCreditsGctf(IConsole::IResult *pResult, void *pUserData);

public:
#ifndef IN_CLASS_IGAMECONTEXT
};
#endif
