// This file can be included several times.

#ifndef CONSOLE_COMMAND
#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help)
#endif

CONSOLE_COMMAND("hammer", "", CFGFLAG_SERVER | CMDFLAG_TEST, ConHammer, this, "Gives a hammer to you")
CONSOLE_COMMAND("gun", "", CFGFLAG_SERVER | CMDFLAG_TEST, ConGun, this, "Gives a gun to you")
CONSOLE_COMMAND("unhammer", "", CFGFLAG_SERVER | CMDFLAG_TEST, ConUnHammer, this, "Removes a hammer from you")
CONSOLE_COMMAND("ungun", "", CFGFLAG_SERVER | CMDFLAG_TEST, ConUnGun, this, "Removes a gun from you")
CONSOLE_COMMAND("godmode", "?v[id]", CFGFLAG_SERVER | CMDFLAG_TEST, ConGodmode, this, "Removes damage")
CONSOLE_COMMAND("rainbow", "?v[id]", CFGFLAG_SERVER | CMDFLAG_TEST, ConRainbow, this, "Toggle rainbow skin on or off")

CONSOLE_COMMAND("force_ready", "?v[id]", CFGFLAG_SERVER | CMDFLAG_TEST, ConForceReady, this, "Sets a player to ready (when the game is paused)")
CONSOLE_COMMAND("chat", "r[message]", CFGFLAG_SERVER | CMDFLAG_TEST, ConChat, this, "Send a message in chat bypassing all mute features")

CONSOLE_COMMAND("shuffle_teams", "", CFGFLAG_SERVER, ConShuffleTeams, this, "Shuffle the current teams")
CONSOLE_COMMAND("swap_teams", "", CFGFLAG_SERVER, ConSwapTeams, this, "Swap the current teams")
CONSOLE_COMMAND("swap_teams_random", "", CFGFLAG_SERVER, ConSwapTeamsRandom, this, "Swap the current teams or not (random chance)")
CONSOLE_COMMAND("force_teambalance", "", CFGFLAG_SERVER, ConForceTeamBalance, this, "Force team balance")

CONSOLE_COMMAND("add_map_to_pool", "s[map name]", CFGFLAG_SERVER, ConAddMapToPool, this, "Can be picked by random_map_from_pool command (entries can be duplicated to increase chance)")
CONSOLE_COMMAND("clear_map_pool", "", CFGFLAG_SERVER, ConClearMapPool, this, "Clears pool used by random_map_from_pool command")
CONSOLE_COMMAND("random_map_from_pool", "", CFGFLAG_SERVER, ConRandomMapFromPool, this, "Changes to random map from pool (see add_map_to_pool)")

CONSOLE_COMMAND("redirect", "v[victim] i[port]", CFGFLAG_SERVER, CServer::ConRedirect, Server(), "Redirect client to given port use victim \"all\" to redirect all but your self")

#undef CONSOLE_COMMAND
