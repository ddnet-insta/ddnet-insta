// ddnet-insta config variables
#ifndef ENGINE_SHARED_VARIABLES_INSTA_H
#define ENGINE_SHARED_VARIABLES_INSTA_H
#undef ENGINE_SHARED_VARIABLES_INSTA_H // this file will be included several times

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#endif

MACRO_CONFIG_INT(SvSpectatorVotes, sv_spectator_votes, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "Allow spectators to vote")
MACRO_CONFIG_INT(SvSpectatorVotesSixup, sv_spectator_votes_sixup, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "Allow 0.7 players to vote as spec if sv_spectator_vote is 1 (hacky dead spec)")
MACRO_CONFIG_INT(SvBangCommands, sv_bang_commands, 2, 0, 2, CFGFLAG_SAVE | CFGFLAG_SERVER, "chat cmds like !1vs1 0=off 1=read only no votes 2=all commands")
MACRO_CONFIG_INT(SvRedirectAndShutdownOnRoundEnd, sv_redirect_and_shutdown_on_round_end, 0, 0, 65535, CFGFLAG_SAVE | CFGFLAG_SERVER, "0=off otherwise it is the port all players will be redirected to on round end")

MACRO_CONFIG_INT(SvCountdownUnpause, sv_countdown_unpause, 0, -1, 1000, CFGFLAG_SAVE | CFGFLAG_SERVER, "Number of seconds to freeze the game in a countdown before match continues after pause")
MACRO_CONFIG_INT(SvCountdownRoundStart, sv_countdown_round_start, 0, -1, 1000, CFGFLAG_SAVE | CFGFLAG_SERVER, "Number of seconds to freeze the game in a countdown before match starts (0 enables only for survival gamemodes, -1 disables)")
MACRO_CONFIG_INT(SvScorelimit, sv_scorelimit, 600, 0, 1000, CFGFLAG_SAVE | CFGFLAG_SERVER, "Score limit (0 disables)")
MACRO_CONFIG_INT(SvTimelimit, sv_timelimit, 0, 0, 1000, CFGFLAG_SAVE | CFGFLAG_SERVER, "Time limit in minutes (0 disables)")
MACRO_CONFIG_INT(SvPlayerReadyMode, sv_player_ready_mode, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "When enabled, players can pause/unpause the game and start the game on warmup via their ready state")
MACRO_CONFIG_INT(SvForceReadyAll, sv_force_ready_all, 0, 0, 60, CFGFLAG_SAVE | CFGFLAG_SERVER, "minutes after which a game will be force unpaused (0=off) related to sv_player_ready_mode")
MACRO_CONFIG_INT(SvStopAndGoChat, sv_stop_and_go_chat, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "pause then game when someone writes 'pause' or 'stop' and start with 'go' or 'start'")
MACRO_CONFIG_INT(SvPowerups, sv_powerups, 1, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "Allow powerups like ninja")

MACRO_CONFIG_INT(SvTeamScoreSpikeNormal, sv_team_score_normal, 5, 0, 100, CFGFLAG_SERVER, "Points a team receives for grabbing into normal spikes")
MACRO_CONFIG_INT(SvTeamScoreSpikeGold, sv_team_score_gold, 12, 0, 100, CFGFLAG_SERVER, "Points a team receives for grabbing into golden spikes")
MACRO_CONFIG_INT(SvTeamScoreSpikeGreen, sv_team_score_green, 15, 0, 100, CFGFLAG_SERVER, "Points a team receives for grabbing into green spikes(non 4-teams fng only)")
MACRO_CONFIG_INT(SvTeamScoreSpikePurple, sv_team_score_purple, 18, 0, 100, CFGFLAG_SERVER, "Points a team receives for grabbing into purple spikes(non 4-teams fng only)")
MACRO_CONFIG_INT(SvTeamScoreSpikeTeam, sv_team_score_team, 10, 0, 100, CFGFLAG_SERVER, "Points a team receives for grabbing into team spikes")

MACRO_CONFIG_INT(SvPlayerScoreSpikeNormal, sv_player_score_normal, 3, 0, 100, CFGFLAG_SERVER, "Points a player receives for grabbing into normal spikes")
MACRO_CONFIG_INT(SvPlayerScoreSpikeGold, sv_player_score_gold, 8, 0, 100, CFGFLAG_SERVER, "Points a player receives for grabbing into golden spikes")
MACRO_CONFIG_INT(SvPlayerScoreSpikeGreen, sv_player_score_green, 6, 0, 100, CFGFLAG_SERVER, "Points a player receives for grabbing into green spikes(non 4-teams fng only)")
MACRO_CONFIG_INT(SvPlayerScoreSpikePurple, sv_player_score_purple, 10, 0, 100, CFGFLAG_SERVER, "Points a player receives for grabbing into purple spikes(non 4-teams fng only)")
MACRO_CONFIG_INT(SvPlayerScoreSpikeTeam, sv_player_score_team, 5, 0, 100, CFGFLAG_SERVER, "Points a player receives for grabbing into team spikes")

MACRO_CONFIG_INT(SvGrenadeAmmoRegen, sv_grenade_ammo_regen, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "Activate or deactivate grenade ammo regeneration in general")
MACRO_CONFIG_INT(SvGrenadeAmmoRegenTime, sv_grenade_ammo_regen_time, 128, 1, 9000, CFGFLAG_SAVE | CFGFLAG_SERVER, "Grenade ammo regeneration time in miliseconds")
MACRO_CONFIG_INT(SvGrenadeAmmoRegenNum, sv_grenade_ammo_regen_num, 6, 1, 10, CFGFLAG_SAVE | CFGFLAG_SERVER, "Maximum number of grenades if ammo regeneration on")
MACRO_CONFIG_INT(SvGrenadeAmmoRegenSpeedNade, sv_grenade_ammo_regen_speed, 1, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "Give grenades back that push own player")
MACRO_CONFIG_INT(SvGrenadeAmmoRegenOnKill, sv_grenade_ammo_regen_on_kill, 2, 0, 2, CFGFLAG_SAVE | CFGFLAG_SERVER, "Refill nades on kill (0=off, 1=1, 2=all)")
MACRO_CONFIG_INT(SvGrenadeAmmoRegenResetOnFire, sv_grenade_ammo_regen_reset_on_fire, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "Reset regen time if shot is fired")
MACRO_CONFIG_INT(SvSprayprotection, sv_sprayprotection, 0, 0, 1, CFGFLAG_SERVER, "Spray protection")
MACRO_CONFIG_INT(SvOnlyHookKills, sv_only_hook_kills, 0, 0, 1, CFGFLAG_SERVER, "Only count kills when enemy is hooked")
MACRO_CONFIG_INT(SvOnlyWallshotKills, sv_only_wallshot_kills, 0, 0, 1, CFGFLAG_SERVER, "Only count kills when enemy is wallshotted (needs laser)")
MACRO_CONFIG_INT(SvKillHook, sv_kill_hook, 0, 0, 1, CFGFLAG_SERVER, "Hook kills")
MACRO_CONFIG_INT(SvKillingspreeKills, sv_killingspree_kills, 0, 0, 20, CFGFLAG_SERVER, "How many kills are needed to be on a killing-spree (0=off)")
MACRO_CONFIG_INT(SvKillingspreeResetOnRoundEnd, sv_killingspree_reset_on_round_end, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "0=allow spreeing across games 1=end spree on round end")
MACRO_CONFIG_INT(SvDamageNeededForKill, sv_damage_needed_for_kill, 4, 0, 5, CFGFLAG_SERVER, "Damage needed to kill")
MACRO_CONFIG_INT(SvAllowZoom, sv_allow_zoom, 0, 0, 1, CFGFLAG_SERVER, "allow ddnet clients to use the client side zoom feature")
MACRO_CONFIG_INT(SvStrictSnapDistance, sv_strict_snap_distance, 1, 0, 1, CFGFLAG_SERVER, "only send players close by (helps against zoom cheats)")
MACRO_CONFIG_STR(SvSpawnWeapons, sv_spawn_weapons, 900, "grenade", CFGFLAG_SERVER, "possible values: grenade, laser")
MACRO_CONFIG_INT(SvAnticamper, sv_anticamper, 0, 0, 1, CFGFLAG_SERVER, "Toggle to enable/disable Anticamper")
MACRO_CONFIG_INT(SvAnticamperFreeze, sv_anticamper_freeze, 7, 0, 15, CFGFLAG_SERVER, "If a player should freeze on camping (and how long) or die")
MACRO_CONFIG_INT(SvAnticamperTime, sv_anticamper_time, 10, 5, 120, CFGFLAG_SERVER, "How long to wait till the player dies/freezes")
MACRO_CONFIG_INT(SvAnticamperRange, sv_anticamper_range, 200, 0, 1000, CFGFLAG_SERVER, "Distance how far away the player must move to escape anticamper")
MACRO_CONFIG_INT(SvReleaseGame, sv_release_game, 0, 0, 1, CFGFLAG_SERVER, "auto release on kill (only affects sv_gametype zCatch)")
MACRO_CONFIG_STR(SvZcatchColors, sv_zcatch_colors, 512, "teetime", CFGFLAG_SERVER, "Color scheme for zCatch options: teetime, savander")
MACRO_CONFIG_INT(SvZcatchRequireMultipleIpsToStart, sv_zcatch_require_multiple_ips_to_start, 0, 0, 1, CFGFLAG_SERVER, "only start games if 5 or more different ips are connected")
MACRO_CONFIG_INT(SvRespawnProtectionMs, sv_respawn_protection_ms, 0, 0, 999999999, CFGFLAG_SERVER, "Delay in milliseconds a tee can not damage or get damaged after spawning")
MACRO_CONFIG_INT(SvDropFlagOnSelfkill, sv_drop_flag_on_selfkill, 0, 0, 1, CFGFLAG_SERVER, "drop flag on selfkill (activates chat cmd '/drop flag')")
MACRO_CONFIG_INT(SvDropFlagOnVote, sv_drop_flag_on_vote, 0, 0, 1, CFGFLAG_SERVER, "drop flag on vote yes (activates chat cmd '/drop flag')")
MACRO_CONFIG_INT(SvOnFireMode, sv_on_fire_mode, 0, 0, 1, CFGFLAG_SERVER, "no reload delay after hitting an enemy with rifle")
// matches ddnet clients prediction code by default
// https://github.com/ddnet/ddnet/blob/f9df4a85be4ca94ca91057cd447707bcce16fd94/src/game/client/prediction/entities/character.cpp#L334-L346
MACRO_CONFIG_INT(SvHammerScaleX, sv_hammer_scale_x, 320, 1, 1000, CFGFLAG_SERVER, "(fng) linearly scale up hammer x power, percentage, for hammering enemies and unfrozen teammates")
MACRO_CONFIG_INT(SvHammerScaleY, sv_hammer_scale_y, 120, 1, 1000, CFGFLAG_SERVER, "(fng) linearly scale up hammer y power, percentage, for hammering enemies and unfrozen teammates")
MACRO_CONFIG_INT(SvMeltHammerScaleX, sv_melt_hammer_scale_x, 50, 1, 1000, CFGFLAG_SERVER, "(fng) linearly scale up hammer x power, percentage, for hammering frozen teammates")
MACRO_CONFIG_INT(SvMeltHammerScaleY, sv_melt_hammer_scale_y, 50, 1, 1000, CFGFLAG_SERVER, "(fng) linearly scale up hammer y power, percentage, for hammering frozen teammates")
MACRO_CONFIG_INT(SvFngHammer, sv_fng_hammer, 1, 0, 1, CFGFLAG_SERVER, "(fng only) use sv_hammer_scale_x/y and sv_melt_hammer_scale_x/y tuning for hammer")
MACRO_CONFIG_INT(SvFngSpikeSound, sv_fng_spike_sound, 1, 0, 1, CFGFLAG_SERVER, "(fng only) play flag capture sound when sacrificing an enemy into the spikes")
MACRO_CONFIG_INT(SvPunishFreezeDisconnect, sv_punish_freeze_disconnect, 1, 0, 1, CFGFLAG_SERVER, "freeze player for 20 seconds on rejoin when leaving server while being frozen")
MACRO_CONFIG_STR(SvDisplayScore, sv_display_score, 512, "round_points", CFGFLAG_SERVER, "values: points, round_points, spree, current_spree, wins, kills, round_kills")
MACRO_CONFIG_INT(SvLaserTextPoints, sv_laser_text_points, 1, 0, 1, CFGFLAG_SERVER, "display laser text in the world on scoring (only fng for now)")
MACRO_CONFIG_INT(SvSelfDamageRespawnDelayMs, sv_self_damage_respawn_delay_ms, 500, 0, 10000, CFGFLAG_SERVER, "time in miliseconds it takes to respawn after dieing by self damage")
MACRO_CONFIG_INT(SvSelfKillRespawnDelayMs, sv_self_kill_respawn_delay_ms, 3000, 0, 10000, CFGFLAG_SERVER, "time in miliseconds it takes to respawn after sending kill bind")
MACRO_CONFIG_INT(SvEnemyKillRespawnDelayMs, sv_enemy_kill_respawn_delay_ms, 500, 0, 10000, CFGFLAG_SERVER, "time in miliseconds it takes to respawn after getting killed by enemies")
MACRO_CONFIG_INT(SvWorldKillRespawnDelayMs, sv_world_kill_respawn_delay_ms, 500, 0, 10000, CFGFLAG_SERVER, "time in miliseconds it takes to respawn after touching a deathtile")
MACRO_CONFIG_INT(SvGameKillRespawnDelayMs, sv_game_kill_respawn_delay_ms, 500, 0, 10000, CFGFLAG_SERVER, "time in miliseconds it takes to respawn after team change, round start and so on")

/*

sv_chat_ratelimit_long_messages

12 is the magic max size of team call binds
TODO: count characters not size because 🅷🅴🅻🅿 is longer than 12 chars

         0123456789
1234567891111111111
           |
top        |
bottom     |
help!      |
come base  |
need help  |

*/
MACRO_CONFIG_INT(SvChatRatelimitLongMessages, sv_chat_ratelimit_long_messages, 1, 0, 1, CFGFLAG_SERVER, "Needs sv_spamprotection 0 (0=off, 1=only messages longer than 12 chars are limited)")
MACRO_CONFIG_INT(SvChatRatelimitSpectators, sv_chat_ratelimit_spectators, 1, 0, 1, CFGFLAG_SERVER, "Needs sv_spamprotection 0 (0=off, 1=specs have slow chat)")
MACRO_CONFIG_INT(SvChatRatelimitPublicChat, sv_chat_ratelimit_public_chat, 1, 0, 1, CFGFLAG_SERVER, "Needs sv_spamprotection 0 (0=off, 1=non team chat is slow)")
MACRO_CONFIG_INT(SvChatRatelimitNonCalls, sv_chat_ratelimit_non_calls, 1, 0, 1, CFGFLAG_SERVER, "Needs sv_spamprotection 0 (0=off, 1=ratelimit all but call binds such as 'help')")
MACRO_CONFIG_INT(SvChatRatelimitSpam, sv_chat_ratelimit_spam, 1, 0, 1, CFGFLAG_SERVER, "Needs sv_spamprotection 0 (0=off, 1=ratelimit chat detected as spam)")
MACRO_CONFIG_INT(SvChatRatelimitDebug, sv_chat_ratelimit_debug, 0, 0, 1, CFGFLAG_SERVER, "Logs which of the ratelimits kicked in")
MACRO_CONFIG_INT(SvRequireChatFlagToChat, sv_require_chat_flag_to_chat, 0, 0, 1, CFGFLAG_SERVER, "clients have to send playerflag chat to use public chat (commands are unrelated)")

MACRO_CONFIG_INT(SvDebugCatch, sv_debug_catch, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "Debug zCatch ticks caught and in game")
MACRO_CONFIG_INT(SvDebugStats, sv_debug_stats, 0, 0, 2, CFGFLAG_SAVE | CFGFLAG_SERVER, "Verbose logging for the SQL player stats")
MACRO_CONFIG_INT(SvVoteCheckboxes, sv_vote_checkboxes, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "Fill [ ] checkbox in vote name if the config is already set")
MACRO_CONFIG_INT(SvHideAdmins, sv_hide_admins, 0, 0, 1, CFGFLAG_SAVE | CFGFLAG_SERVER, "Only send admin status to other authed players")
MACRO_CONFIG_INT(SvShowSettingsMotd, sv_show_settings_motd, 1, 0, 1, CFGFLAG_SERVER, "Show insta game settings in motd on join")
MACRO_CONFIG_INT(SvUnstackChat, sv_unstack_chat, 1, 0, 1, CFGFLAG_SERVER, "Revert ddnet clients duplicated chat message stacking")
MACRO_CONFIG_INT(SvCasualRounds, sv_casual_rounds, 1, 0, 1, CFGFLAG_SERVER, "1=start rounds automatically, 0=require restart vote to properly start game")
MACRO_CONFIG_INT(SvAllowTeamChangeDuringPause, sv_allow_team_change_during_pause, 1, 0, 1, CFGFLAG_SERVER, "allow players to join the game or spectators during pause")
MACRO_CONFIG_INT(SvTournament, sv_tournament, 0, 0, 1, CFGFLAG_SERVER, "Print messages saying tournament is running. No other effects.")
MACRO_CONFIG_STR(SvTournamentWelcomeChat, sv_tournament_welcome_chat, 900, "", CFGFLAG_SERVER, "Chat message shown in chat on join when sv_tournament is 1")
MACRO_CONFIG_INT(SvTournamentChat, sv_tournament_chat, 0, 0, 2, CFGFLAG_SERVER, "0=off, 1=Spectators can not public chat, 2=Nobody can public chat")
MACRO_CONFIG_INT(SvTournamentChatSmart, sv_tournament_chat_smart, 0, 0, 1, CFGFLAG_SERVER, "Turns sv_tournament_chat on on restart and off on round end")
MACRO_CONFIG_INT(SvTournamentJoinMsgs, sv_tournament_join_msgs, 0, 0, 2, CFGFLAG_SERVER, "Hide join/leave of spectators in chat !0.6 only for now! (0=off,1=hidden,2=shown for specs)")
MACRO_CONFIG_STR(SvRoundStatsDiscordWebhooks, sv_round_stats_discord_webhooks, 512, "", CFGFLAG_SERVER, "If set will post score stats there on round end. Can be a comma separated list.")
MACRO_CONFIG_STR(SvRoundStatsHttpEndpoints, sv_round_stats_http_endpoints, 512, "", CFGFLAG_SERVER, "If set will post score stats there on round end. Can be a comma separated list.")
MACRO_CONFIG_STR(SvRoundStatsOutputFile, sv_round_stats_output_file, 512, "", CFGFLAG_SERVER, "If set will write score stats there on round end")
MACRO_CONFIG_INT(SvRoundStatsFormatDiscord, sv_round_stats_format_discord, 1, 0, 4, CFGFLAG_SERVER, "0=csv 1=psv 2=ascii table 3=markdown table 4=json")
MACRO_CONFIG_INT(SvRoundStatsFormatHttp, sv_round_stats_format_http, 4, 0, 4, CFGFLAG_SERVER, "0=csv 1=psv 2=ascii table 3=markdown table 4=json")
MACRO_CONFIG_INT(SvRoundStatsFormatFile, sv_round_stats_format_file, 1, 0, 4, CFGFLAG_SERVER, "0=csv 1=psv 2=ascii table 3=markdown table 4=json")

#endif
