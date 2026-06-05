// This file can be included several times.
// doc gen ignore: ready, pause, pausevoted, specvoted, shuffle, swap, drop, spec, team, lock, unlock, invite, join, team0mode, kill

#ifndef CHAT_COMMAND
#error "The config macros must be defined"
// This helps IDEs properly syntax highlight the uses of the macro below.
#define CHAT_COMMAND(name, params, flags, callback, userdata, help) ;
#endif

// some commands ddnet-insta defined already existed in ddnet
// these are marked as "shadows"
// if a command is registered twice in the ddnet console system
// the latter one overwrites the former
// ideally the ddnet-insta commands call the original command
// if sv_gametype is "ddnet"

// "credits" shadows a ddnet command
CHAT_COMMAND("credits", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaModeCredits, this, "Shows the credits of the current ddnet-insta mode");
CHAT_COMMAND("credits_insta", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaCredits, this, "Shows the credits of the entire ddnet-insta project");
CHAT_COMMAND("credits_ddnet", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConCredits, this, "Shows the credits of the DDNet mod");

// all these team related commands shadow ddnet commands
CHAT_COMMAND("team", "?i[id]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaTeam, this, "Lets you join team i (shows your team if left blank)");
CHAT_COMMAND("lock", "?i['0'|'1']", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaLock, this, "Toggle team lock so no one else can join and so the team restarts when a player dies. /lock 0 to unlock, /lock 1 to lock");
CHAT_COMMAND("unlock", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaUnlock, this, "Unlock a team");
CHAT_COMMAND("invite", "r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaInvite, this, "Invite a person to a locked team");
CHAT_COMMAND("join", "r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaJoin, this, "Join the team of the specified player");
CHAT_COMMAND("team0mode", "?i['0'|'1']", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaTeam0Mode, this, "Toggle team between team 0 and team mode. This mode will make your team behave like team 0.");

// "rank" shadows a ddnet command
CHAT_COMMAND("rank", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankCmdlist, this, "Lists available rank commands")
// "top5" shadows a ddnet command
CHAT_COMMAND("top5", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopCmdlist, this, "Lists available top commands")
CHAT_COMMAND("top", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopCmdlist, this, "Lists available top commands")

// "pause" shadows a ddnet command, alias for "ready" in pvp modes, Has pvp description. This is wrong in ddrace gametypes.
CHAT_COMMAND("pause", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaTogglePause, this, "Pause or resume the game")
// "spec" shadows a ddnet command
CHAT_COMMAND("spec", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaToggleSpec, this, "Toggles spec (if not available behaves as /pause)");
// "pausevoted" shadows a ddnet command
CHAT_COMMAND("pausevoted", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaTogglePauseVoted, this, "Toggles pause on the currently voted player");
// "specvoted" shadows a ddnet command
CHAT_COMMAND("specvoted", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaToggleSpecVoted, this, "Toggles spec on the currently voted player");
// "kill" shadows a ddnet command
CHAT_COMMAND("kill", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaKill, this, "Kill yourself when kill-protected during a long game (use f1, kill for regular kill)");

// alias for "pause" in pvp modes
CHAT_COMMAND("ready", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConReadyChange, this, "Pause or resume the game")
// "swap" shadows a ddnet command
CHAT_COMMAND("swap", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaSwap, this, "Call a vote to swap teams")
CHAT_COMMAND("shuffle", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaShuffle, this, "Call a vote to shuffle teams")
CHAT_COMMAND("swap_random", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaSwapRandom, this, "Call vote to swap teams to a random side")
CHAT_COMMAND("drop", "?s[flag]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaDrop, this, "Drop the flag")

CHAT_COMMAND("stats", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConStatsRound, this, "Shows the current round stats of player name (your stats by default)")

CHAT_COMMAND("statsall", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConStatsAllTime, this, "Shows the all time stats of player name (your stats by default)")
CHAT_COMMAND("stats_all", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConStatsAllTime, this, "Shows the all time stats of player name (your stats by default)")
CHAT_COMMAND("multis", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConMultis, this, "Shows the all time fng multi kill stats")
CHAT_COMMAND("steals", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConSteals, this, "Shows all time and round fng kill steal stats")
CHAT_COMMAND("round_top", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRoundTop, this, "Shows the top players of the current round")

// which points to display in scoreboard
// all time stats are implicit and round stats are specific
// so "points" is all time stats of players points
// and "round_points" is only for the current round
//
// the default value can be set by the config sv_display_score
CHAT_COMMAND("score", "?s['points'|'round_points'|'spree'|'current_spree'|'win_points'|'wins'|'kills'|'round_kills']", CFGFLAG_CHAT | CFGFLAG_SERVER, ConScore, this, "change which type of score is displayed in scoreboard")

// "points" shadows a ddnet command
CHAT_COMMAND("points", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaRankPoints, this, "Shows the all time points rank of player name (your stats by default)")
CHAT_COMMAND("rank_points", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaRankPoints, this, "Shows the all time points rank of player name (your stats by default)")

CHAT_COMMAND("rank_kills", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankKills, this, "Shows the all time kills rank of player name (your stats by default)")
CHAT_COMMAND("top5kills", "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopKills, this, "Shows the all time best ranks by kills")

// TODO: what about rank flag times with stat track on vs off? how does the user choose which to show
//       i think the best is to not let the user choose but show all at once
//       like ddnet does show regional and global rankings together
//       the flag ranks could show ranks for the current gametype and for all gametypes and for stat track off/on
CHAT_COMMAND("rank_flags", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankFastcaps, this, "Shows the all time flag time rank of player name (your stats by default)")
CHAT_COMMAND("top5flags", "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopFastcaps, this, "Shows the all time best ranks by flag time")
CHAT_COMMAND("top5caps", "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopNumCaps, this, "Shows the all time best ranks by amount of flag captures")
CHAT_COMMAND("rank_caps", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankFlagCaptures, this, "Shows the all time flag capture rank of player name (your stats by default)")
CHAT_COMMAND("top5spikes", "?s['gold'|'green'|'purple'] ?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopSpikeColors, this, "Shows the all time best ranks by spike kills")

CHAT_COMMAND("setspawn", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConSetSpawn, this, "Set the spawn point")
CHAT_COMMAND("spawn_reset", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConSpawnReset, this, "Resets the spawn point")
