# Configs

ddnet-insta inherited all configs from ddnet. So make sure to also check ddnet's documentation.
The following ddnet configs are highly recommended to set in ddnet-insta to get the best pvp
experience.

```
# autoexec_server.cfg

sv_tune_reset 0
sv_destroy_bullets_on_death 0
sv_destroy_lasers_on_death 0
sv_no_weak_hook 1
sv_vote_veto_time 0
conn_timeout 10
conn_timeout_protection 5
```

ddnet also has something called a reset file. Which is a special config.
By default it is loaded from reset.cfg and it can be set to a custom location
using the config ``sv_reset_file``


In this **reset.cfg** it is recommended to set the following configs
to get a more classic pvp experience.

```
# reset.cfg

sv_team 0
sv_old_laser 1 # wallshot should not collide with own tee
tune laser_bounce_num 1
```

Below is a list of all the settings that were added in ddnet-insta.

## ddnet-insta configs

+ `sv_gametype` Game type (gctf, ictf, gdm, idm, gtdm, itdm, ctf, dm, tdm, zcatch, bolofng, solofng, boomfng, fng)
+ `sv_spectator_votes` Allow spectators to vote
+ `sv_spectator_votes_sixup` Allow 0.7 players to vote as spec if sv_spectator_vote is 1 (hacky dead spec)
+ `sv_bang_commands` chat cmds like !1vs1 -1=fully gone 0=off with error 1=read only no votes 2=all commands
+ `sv_redirect_and_shutdown_on_round_end` 0=off otherwise it is the port all players will be redirected to on round end
+ `sv_countdown_unpause` Number of seconds to freeze the game in a countdown before match continues after pause
+ `sv_countdown_round_start` Number of seconds to freeze the game in a countdown before match starts (0 enables only for survival gamemodes, -1 disables)
+ `sv_scorelimit` Score limit (0 disables)
+ `sv_timelimit` Time limit in minutes (0 disables)
+ `sv_player_ready_mode` When enabled, players can pause/unpause the game and start the game on warmup via their ready state
+ `sv_force_ready_all` minutes after which a game will be force unpaused (0=off) related to sv_player_ready_mode
+ `sv_stop_and_go_chat` pause then game when someone writes 'pause' or 'stop' and start with 'go' or 'start'
+ `sv_powerups` Allow powerups like ninja
+ `sv_teambalance_time` How many minutes to wait before autobalancing teams (0=off)
+ `sv_teamdamage` Team damage
+ `sv_team_score_normal` Points a team receives for grabbing into normal spikes
+ `sv_team_score_gold` Points a team receives for grabbing into golden spikes
+ `sv_team_score_green` Points a team receives for grabbing into green spikes(non 4-teams fng only)
+ `sv_team_score_purple` Points a team receives for grabbing into purple spikes(non 4-teams fng only)
+ `sv_team_score_team` Points a team receives for grabbing into team spikes
+ `sv_player_score_normal` Points a player receives for grabbing into normal spikes
+ `sv_player_score_gold` Points a player receives for grabbing into golden spikes
+ `sv_player_score_green` Points a player receives for grabbing into green spikes(non 4-teams fng only)
+ `sv_player_score_purple` Points a player receives for grabbing into purple spikes(non 4-teams fng only)
+ `sv_player_score_team` Points a player receives for grabbing into team spikes
+ `sv_wrong_spike_freeze` The time, in seconds, a player gets frozen, if he grabbed a frozen opponent into the opponents spikes (0=off, fng only)
+ `sv_hammer_scale_x` linearly scale up hammer x power, percentage, for hammering enemies and unfrozen teammates (needs sv_fng_hammer)
+ `sv_hammer_scale_y` linearly scale up hammer y power, percentage, for hammering enemies and unfrozen teammates (needs sv_fng_hammer)
+ `sv_melt_hammer_scale_x` linearly scale up hammer x power, percentage, for hammering frozen teammates (needs sv_fng_hammer)
+ `sv_melt_hammer_scale_y` linearly scale up hammer y power, percentage, for hammering frozen teammates (needs sv_fng_hammer)
+ `sv_fng_hammer` use sv_hammer_scale_x/y and sv_melt_hammer_scale_x/y tuning for hammer
+ `sv_hit_freeze_delay` How many seconds will players remain frozen after being hit with a weapon (only fng)
+ `sv_spike_sound` Play flag capture sound when sacrificing an enemy into the spikes !0.6 only! (0=off/1=only the killer and the victim/2=everyone near the victim)
+ `sv_text_points` display text in the world on scoring (only fng for now. 1: laser, 2: projectile)
+ `sv_text_points_delay` Timer until text disappears in seconds (only fng for now)
+ `sv_announce_steals` show in chat when someone stole a kill (only fng for now)
+ `sv_grenade_ammo_regen` Activate or deactivate grenade ammo regeneration in general
+ `sv_grenade_ammo_regen_time` Grenade ammo regeneration time in milliseconds
+ `sv_grenade_ammo_regen_num` Maximum number of grenades if ammo regeneration on
+ `sv_grenade_ammo_regen_speed` Give grenades back that push own player
+ `sv_grenade_ammo_regen_on_kill` Refill nades on kill (0=off, 1=1, 2=all)
+ `sv_grenade_ammo_regen_reset_on_fire` Reset regen time if shot is fired
+ `sv_sprayprotection` Spray protection
+ `sv_only_hook_kills` Only count kills when enemy is hooked
+ `sv_only_wallshot_kills` Only count kills when enemy is wallshotted (needs laser)
+ `sv_kill_hook` Hook kills
+ `sv_killingspree_kills` How many kills are needed to be on a killing-spree (0=off)
+ `sv_killingspree_reset_on_round_end` 0=allow spreeing across games 1=end spree on round end
+ `sv_damage_needed_for_kill` Grenade damage needed to kill in instagib modes
+ `sv_swap_flags` swap blue and red flag spawns in ctf modes
+ `sv_allow_zoom` allow ddnet clients to use the client side zoom feature
+ `sv_strict_snap_distance` only send players close by (helps against zoom cheats)
+ `sv_anticamper` Toggle to enable/disable Anticamper
+ `sv_anticamper_freeze` If a player should freeze on camping (and how long) or die
+ `sv_anticamper_time` How long to wait till the player dies/freezes
+ `sv_anticamper_range` Distance how far away the player must move to escape anticamper
+ `sv_release_game` auto release on kill (only affects sv_gametype zCatch)
+ `sv_zcatch_require_multiple_ips_to_start` only start games if 5 or more different ips are connected
+ `sv_respawn_protection_ms` Delay in milliseconds a tee can not damage or get damaged after spawning
+ `sv_drop_flag_on_selfkill` drop flag on selfkill (activates chat cmd '/drop flag')
+ `sv_drop_flag_on_vote` drop flag on vote yes (activates chat cmd '/drop flag')
+ `sv_laser_reload_time_on_hit` 0=default/off ticks it takes to shoot again after a shot was hit (see also sv_fast_hit_full_auto)
+ `sv_fast_hit_full_auto` require fire button repress when sv_reload_time_on_hit is set
+ `sv_punish_freeze_disconnect` freeze player for 20 seconds on rejoin when leaving server while being frozen
+ `sv_freeze_on_spawn` freeze all players for x seconds after every respawn
+ `sv_self_damage_respawn_delay_ms` time in milliseconds it takes to respawn after dying by self damage
+ `sv_self_kill_respawn_delay_ms` time in milliseconds it takes to respawn after sending kill bind
+ `sv_enemy_kill_respawn_delay_ms` time in milliseconds it takes to respawn after getting killed by enemies
+ `sv_world_kill_respawn_delay_ms` time in milliseconds it takes to respawn after touching a deathtile
+ `sv_game_kill_respawn_delay_ms` time in milliseconds it takes to respawn after team change, round start and so on
+ `sv_chat_ratelimit_long_messages` Needs sv_spamprotection 0 (0=off, 1=only messages longer than 12 chars are limited)
+ `sv_chat_ratelimit_spectators` Needs sv_spamprotection 0 (0=off, 1=specs have slow chat)
+ `sv_chat_ratelimit_public_chat` Needs sv_spamprotection 0 (0=off, 1=non team chat is slow)
+ `sv_chat_ratelimit_non_calls` Needs sv_spamprotection 0 (0=off, 1=ratelimit all but call binds such as 'help')
+ `sv_chat_ratelimit_spam` Needs sv_spamprotection 0 (0=off, 1=ratelimit chat detected as spam)
+ `sv_chat_ratelimit_debug` Logs which of the ratelimits kicked in
+ `sv_require_chat_flag_to_chat` clients have to send playerflag chat to use public chat (commands are unrelated)
+ `sv_always_track_stats` Track stats no matter how many players are online
+ `sv_ignore_kills_before_race_start` Hide kills from feed and reward no points if the involved players did not touch the start line yet
+ `sv_accounts` See /register and /login chat commands
+ `sv_claimable_names` 0=off 1=lock claimed names but no /claimname cmd 2=fully on
+ `sv_points_needed_to_register` Amount of round points needed to be able to /register an account (anti spam)
+ `sv_join_register_delay` Delay in seconds after join before one can /register an account (anti spam)
+ `sv_debug_catch` Debug zCatch ticks caught and in game
+ `sv_debug_stats` Verbose logging for the SQL player stats
+ `sv_debug_accounts` Verbose logging for the account system
+ `sv_vote_checkboxes` Fill [ ] checkbox in vote name if the config is already set
+ `sv_hide_admins` Only send admin status to other authed players
+ `sv_show_settings_motd` Show insta game settings in motd on join
+ `sv_unstack_chat` Revert ddnet clients duplicated chat message stacking
+ `sv_casual_rounds` 1=start rounds automatically, 0=require restart vote to properly start game
+ `sv_allow_ddr_team_change` enable or disable /team chat command but does not affect set_team_ddr rcon command
+ `sv_allow_team_change` allow players to switch teams for example from red to blue or to spectators
+ `sv_allow_team_change_during_pause` allow players to join the game or spectators during pause
+ `sv_allow_selfkill` if set to 0 selfkills and the /kill command will be blocked
+ `sv_tournament` Print messages saying tournament is running. No other effects.
+ `sv_tournament_chat` 0=off, 1=Spectators can not public chat, 2=Nobody can public chat
+ `sv_tournament_chat_smart` Turns sv_tournament_chat on on restart and off on round end
+ `sv_tournament_join_msgs` Hide join/leave of spectators in chat !0.6 only for now! (0=off,1=hidden,2=shown for specs)
+ `sv_round_stats_format_discord` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_round_stats_format_http` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_round_stats_format_file` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_print_round_stats` print top players in chat on round end
+ `sv_publish_live_stats` publish the round stats before the round ends as soon as the score changes
+ `sv_live_stats_interval` minimum delay in seconds between stats republish if sv_publish_live_stats is on
+ `sv_clear_stats_on_race_start` Delete players round stats without saving them when touching the start line
+ `sv_kill_tile_destroys_ball` Destroy the ball when it touches death tile (only foot)
+ `sv_ball_bounce_friction` The ball looses that much speed after a bounce (only foot)
+ `sv_ball_explode` Should the grenades explode (only foot)
+ `sv_ball_respawn_delay` Respawn time of the ball (only foot)
+ `sv_score_diff` Difference between the team-scores before a team can win (only foot)
+ `sv_sudden_death_score_diff` Difference between the team-scores before a team can win in sudden death (only foot)
+ `sv_suicide_penalty` Indicates if score is decremented on suicide
+ `sv_freeze_hammer` The amount of ticks of freeze inflicted when hitting a player. (0 to disable)
+ `sv_bombtag_bombs_per_player` The amount of bombs that should spawn per X alive players, 1 for everyone except one.
+ `sv_bombtag_seconds_to_explosion` The amount of seconds till the bomb explodes.
+ `sv_bombtag_minimum_seconds_to_explosion` The minimum amount of seconds a tee's bomb timer will have after getting bomb.
+ `sv_bombtag_bomb_damage` The amount of seconds removed from a bombs timer when hit by someone.
+ `sv_bombtag_bomb_weapon` Which weapon should the bomb be given? 0 - Hammer, 1 - Gun, 2 - Shotgun, 3 - Grenade, 4 - Laser, 5 - Ninja
+ `sv_bombtag_collateral_damage` Enable collateral damage, exploding bombs will eliminate any tees in a 3 tile radius.
+ `sv_mystery_rounds_chance` The percentage of a mystery round happening! A random line from sv_mysteryrounds_filename will be executed.
+ `sv_shuffle_on_round_start` Should teams of players be re-assigned each round
+ `sv_kill_indicator` Shows the killer that he froze the player(only fng for now)
+ `sv_deathrate_threshold` Death frequency threshold per second. If the player dies more often, their saved training checkpoint is removed (0 = disabled)
+ `sv_spawn_weapons` possible values: grenade, laser
+ `sv_zcatch_colors` Color scheme for zCatch options: teetime, savander
+ `sv_display_score` values: points, round_points, spree, current_spree, win_points, wins, kills, round_kills
+ `sv_tournament_welcome_chat` Chat message shown in chat on join when sv_tournament is 1
+ `sv_round_stats_discord_webhooks` If set will post score stats there on round end. Can be a comma separated list.
+ `sv_round_stats_http_endpoints` If set will post score stats there on round end. Can be a comma separated list.
+ `sv_round_stats_output_file` If set will write score stats there on round end
+ `sv_race_stats_http_endpoints` publish stats on finish for each player individually
+ `sv_mystery_rounds_filename` File which contains mystery round commands, one round per line.
+ `sv_mystery_rounds_reset_filename` File which contains the commands to execute after a mystery round.

# Rcon commands

+ `hammer` Gives a hammer to you
+ `gun` Gives a gun to you
+ `unhammer` Removes a hammer from you
+ `ungun` Removes a gun from you
+ `godmode` Removes damage
+ `rainbow` Toggle rainbow skin on or off
+ `force_ready` Sets a player to ready (when the game is paused)
+ `chat` Send a message in chat bypassing all mute features
+ `shuffle_teams` Shuffle the current teams
+ `swap_teams` Swap the current teams
+ `swap_teams_random` Swap the current teams or not (random chance)
+ `force_teambalance` Force team balance
+ `add_password` add another password in addition to the \"password\" config
+ `remove_password` remove password added with \"add_password\"
+ `clear_passwords` remove all passwords added with \"add_password\"
+ `list_passwords` list passwords added with \"add_password\"
+ `add_map_to_pool` Can be picked by random_map_from_pool command (entries can be duplicated to increase chance)
+ `clear_map_pool` Clears pool used by random_map_from_pool command
+ `random_map_from_pool` Changes to random map from pool (see add_map_to_pool)
+ `post_stats` Publish round stats before round end without clearing them (see *round_stats* configs for more info
+ `delete_round_stats` Delete all players current round stats WITHOUT saving them to the database!
+ `delete_session_stats` Delete all players current session stats
+ `gctf_antibot` runs the antibot command gctf (depends on closed source module)
+ `known_antibot` runs the antibot command known (depends on antibob antibot module)
+ `kick_events_antibot` runs the antibot command kick_events (depends on antibob antibot module)
+ `redirect` Redirect client to given port use victim \"all\" to redirect all but your self
+ `deep_jailid` deep freeze (undeep tile works) will be restored on respawn and reconnect
+ `deep_jailip` deep freeze (undeep tile works) will be restored on respawn and reconnect
+ `deep_jails` list all perma deeped players deeped by deep_jailid and deep_jailip commands
+ `undeep_jail` list all perma deeped players deeped by deep_jailid and deep_jailip commands
+ `acc_list` List account info of online players
+ `acc_set_password` Set new password for given account. Useful if players forget theirs. Keeps player logged in.
+ `acc_logout` Force logout player on current server
+ `acc_lock` Lock account and logout players logged in to it
+ `acc_unlock` Unlock account
+ `acc_info` Get information about an account
+ `acc_status` Get general account system status information
+ `add_unclaimable_name` Exclude given name from /claimname chat command
+ `remove_unclaimable_name` Make given name available to /claimname chat command

# Chat commands

Most ddnet slash chat commands were inherited and are still functional.
But /pause and /spec got removed since it is conflicting with pausing games and usually not wanted for pvp games.

ddnet-insta then added a bunch of own slash chat commands and also bang (!) chat commands

+ `!ready` `!pause` `/pause` `/ready` to pause the game. Needs `sv_player_ready_mode 1` and 0.7 clients can also send the 0.7 ready change message
+ `!shuffle` `/shuffle` call vote to shuffle teams
+ `!swap` `/swap` call vote to swap teams
+ `!swap_random` `/swap_random` call vote to swap teams to random sides
+ `!settings` show current game settings in the message of the day. It will show if spray protection is on or off and similar game relevant settings.
+ `!1v1` `!2v2` `!v1` `!v2` `!1on1` ... call vote to change in game slots
+ `!restart ?(seconds)` call vote to restart game with optional parameter of warmup seconds (default: 10)
+ `/drop flag` if it is a CTF gametype the flagger can drop the flag without dying if either `sv_drop_flag_on_selfkill` or `sv_drop_flag_on_vote` is set
+ `/credits` Shows the credits of the current ddnet-insta mode"
+ `/credits_insta` Shows the credits of the entire ddnet-insta project"
+ `/credits_ddnet` Shows the credits of the DDNet mod"
+ `/rank` Lists available rank commands
+ `/top5` Lists available top commands
+ `/top` Lists available top commands
+ `/stats` Shows the current round stats of player name (your stats by default)
+ `/statsall` Shows the all time stats of player name (your stats by default)
+ `/stats_all` Shows the all time stats of player name (your stats by default)
+ `/multis` Shows the all time fng multi kill stats
+ `/steals` Shows all time and round fng kill steal stats
+ `/round_top` Shows the top players of the current round
+ `/register` register account
+ `/login` login to account
+ `/logout` logout account
+ `/changepassword` change the password of your account
+ `/claimname` claim the current nick name so nobody else can use it
+ `/profile` show a users profile
+ `/slow_account_operation` used to simulate high load for debugging account system stability
+ `/score` change which type of score is displayed in scoreboard
+ `/points` Shows the all time points rank of player name (your stats by default)
+ `/rank_points` Shows the all time points rank of player name (your stats by default)
+ `/rank_kills` Shows the all time kills rank of player name (your stats by default)
+ `/top5kills` Shows the all time best ranks by kills
+ `/rank_flags` Shows the all time flag time rank of player name (your stats by default)
+ `/top5flags` Shows the all time best ranks by flag time
+ `/top5caps` Shows the all time best ranks by amount of flag captures
+ `/rank_caps` Shows the all time flag capture rank of player name (your stats by default)
+ `/top5spikes` Shows the all time best ranks by spike kills
+ `/setspawn` Set the spawn point
+ `/spawn_reset` Resets the spawn point

# More

ddnet-insta is based on ddnet so it also inherits all settings and commands from ddnet.
Make sure to check the [ddnet documentation](https://ddnet.org/settingscommands/) for those.
