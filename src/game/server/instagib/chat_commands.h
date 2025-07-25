// This file can be included several times.

#ifndef CONSOLE_COMMAND
#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help)
#endif

// some commands ddnet-insta defined already existed in ddnet
// these are marked as "shadows"
// if a command is registered twice in the ddnet console system
// the latter one overwrites the former
// ideally the ddnet-insta commands call the original command
// if sv_gametype is "ddnet"

CONSOLE_COMMAND("credits_ddnet", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConCredits, this, "Shows the credits of the DDNet mod");

// "rank" shadows a ddnet command
CONSOLE_COMMAND("rank", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankCmdlist, this, "Lists available rank commands")
// "top5" shadows a ddnet command
CONSOLE_COMMAND("top5", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopCmdlist, this, "Lists available top commands")
CONSOLE_COMMAND("top", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopCmdlist, this, "Lists available top commands")

// "pause" shadows a ddnet command
CONSOLE_COMMAND("pause", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConReadyChange, this, "Pause or resume the game")
CONSOLE_COMMAND("ready", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConReadyChange, this, "Pause or resume the game")
// "swap" shadows a ddnet command
CONSOLE_COMMAND("swap", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaSwap, this, "Call a vote to swap teams")
CONSOLE_COMMAND("shuffle", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaShuffle, this, "Call a vote to shuffle teams")
CONSOLE_COMMAND("swap_random", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaSwapRandom, this, "Call vote to swap teams to a random side")
CONSOLE_COMMAND("drop", "?s[flag]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaDrop, this, "Drop the flag")

CONSOLE_COMMAND("stats", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConStatsRound, this, "Shows the current round stats of player name (your stats by default)")

CONSOLE_COMMAND("statsall", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConStatsAllTime, this, "Shows the all time stats of player name (your stats by default)")
CONSOLE_COMMAND("stats_all", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConStatsAllTime, this, "Shows the all time stats of player name (your stats by default)")
CONSOLE_COMMAND("multis", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConMultis, this, "Shows the all time fng multi kill stats")
CONSOLE_COMMAND("steals", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConSteals, this, "Shows all time and round fng kill steal stats")
CONSOLE_COMMAND("round_top", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRoundTop, this, "Shows the top players of the current round")

// which points to display in scoreboard
// all time stats are implicit and round stats are specific
// so "points" is all time stats of players points
// and "round_points" is only for the current round
//
// the default value can be set by the config sv_display_score
CONSOLE_COMMAND("score", "?s['points'|'round_points'|'spree'|'current_spree'|'win_points'|'wins'|'kills'|'round_kills']", CFGFLAG_CHAT | CFGFLAG_SERVER, ConScore, this, "change which type of score is displayed in scoreboard")

// "points" shadows a ddnet command
CONSOLE_COMMAND("points", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaRankPoints, this, "Shows the all time points rank of player name (your stats by default)")
CONSOLE_COMMAND("rank_points", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaRankPoints, this, "Shows the all time points rank of player name (your stats by default)")

CONSOLE_COMMAND("rank_kills", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankKills, this, "Shows the all time kills rank of player name (your stats by default)")
CONSOLE_COMMAND("top5kills", "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopKills, this, "Shows the all time best ranks by kills")

// TODO: what about rank flag times with stat track on vs off? how does the user choose which to show
//       i think the best is to not let the user choose but show all at once
//       like ddnet does show regional and global rankings together
//       the flag ranks could show ranks for the current gametype and for all gametypes and for stat track off/on
CONSOLE_COMMAND("rank_flags", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankFastcaps, this, "Shows the all time flag time rank of player name (your stats by default)")
CONSOLE_COMMAND("top5flags", "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopFastcaps, this, "Shows the all time best ranks by flag time")
CONSOLE_COMMAND("top5caps", "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopNumCaps, this, "Shows the all time best ranks by amount of flag captures")
CONSOLE_COMMAND("rank_caps", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankFlagCaptures, this, "Shows the all time flag capture rank of player name (your stats by default)")
CONSOLE_COMMAND("top5spikes", "?s['gold'|'green'|'purple'] ?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopSpikeColors, this, "Shows the all time best ranks by spike kills")

#undef CONSOLE_COMMAND
