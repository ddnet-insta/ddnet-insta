[![DDraceNetwork](other/ddnet-insta.png)](https://ddnet.tw) [![](https://github.com/ddnet-insta/ddnet-insta/workflows/Build/badge.svg)](https://github.com/ddnet-insta/ddnet-insta/actions?query=workflow%3ABuild+event%3Apush+branch%3Amaster)

DDNet-insta based on DDRaceNetwork, a Teeworlds mod. See the [website](https://ddnet.tw) for more information.

For build instructions visit the [ddnet repo](https://github.com/ddnet/ddnet).

---

A ddnet based pvp mod. With the focus on correct 0.6 and 0.7 support and staying close to and up to date with ddnet.
While being highly configurable and feature rich.

Implementing most of the relevant pvp gametypes: ctf, dm, gctf, ictf, gdm, idm, gtdm, itdm, zCatch, bolofng, solofng, boomfng, fng

# Project name and scope

The name **ddnet-insta** is short for DummyDragRaceNetwork Instagib. Because it started out as a ddnet based modification with the
focus on gCTF with the possible future scope for also other instagib (one shot kills) gametypes such as iCTF, zCatch and fng.
But it now has expanded to support also damage based gameplay such as teeworlds vanilla CTF and DM. So currently it aims to support
any kind of round based pvp gametype that can be implemented without causing too much maintenance cost.


NOTE FOR DEVELOPERS:


It also tries to be extendable and fork friendly. If you want to build a pvp gamemode with score points and rounds.
This code base should get you started quite easily. Just create a new folder for your gamemode. After wiring it up in
[gamecontext](https://github.com/ddnet-insta/ddnet-insta/blob/ba38c11ccf46e888786c90c7dc0f09503be19a49/src/game/server/gamecontext.cpp#L4018-L4019)
and [cmakelist](https://github.com/ddnet-insta/ddnet-insta/blob/ba38c11ccf46e888786c90c7dc0f09503be19a49/CMakeLists.txt#L2751-L2752)
you should be able to build almost any gametype without ever having to edit code outside
of your gamemode directory. Just look at the existing gamemodes for examples. A simple mode to look at would be
[vanilla deathmatch](https://github.com/ddnet-insta/ddnet-insta/tree/ba38c11ccf46e888786c90c7dc0f09503be19a49/src/game/server/gamemodes/vanilla/dm).
Adding your own stats columns and rank/top commands can be done in a few lines of code without writing any SQL.

# Project goals

- Stay close and up to date with upstream ddnet. Keep the git diff in files edited by ddnet as minimal as possible to keep merging cheap.
- Be generic, consistent and configurable. If possible the same config variables and concepts should be applied to all gametypes.
  For example configs such as anticamper should not be ``sv_zcatch_anticamper`` but ``sv_anticamper`` and work in all gametypes.
- Support latest ddnet and teeworlds clients fully and correctly.
- Be friendly to downstream projects. Commits and releases should warn about breaking changes.
  Downstream projects should have an easy time to add new gametypes that can be updated to new ddnet-insta versions
  without a lot of effort.

# Features

## Stats tracked in sql database

Every players kills, deaths, wins and more statistics are persisted in a database.
There are no accounts. The stats are tacked on the players names. What exactly is tracked
depends on the ``sv_gametype``. But here are some chat commands that work in any gamemode:

- ``/stats`` Shows the current round stats. Takes a player name as optional argument.
- ``/statsall`` Shows the all time stats. Takes a player name as optional argument.
- ``/top5kills`` Shows the all time top 5 players by amount of kills. Takes an offset as optional argument ``/top5kills 5`` to see rank 5 till 10 for example.
- ``/rank_kills`` Show the all time rank of a players kills compared to others. Takes a player name as optional argument.
- ``/rank`` to list all rank commands for the current gametype
- ``/top`` to list all top commands for the current gametype

## Checkbox votes

If a vote is added starting with a ``[ ]`` in the display name. It will be used as a checkbox.
If the underlying config is currently set that checkbox will be ticked and users see ``[x]`` in the vote menu.
This feature is optional and if you do not put any  ``[ ]`` in your config it will not be using any checkboxes.
It is only applied for ddnet-insta settings not for all ddnet configs.
It is recommended to set ``sv_vote_checkboxes 0`` at the start of your autoexec and ``sv_vote_checkboxes 1``
at the end so it does not update all votes for every setting it loads.

![checkbox votes](https://raw.githubusercontent.com/ddnet-insta/images/c6c3e871a844fa06b460b8be61ba0ff01d0a82f6/checkbox_votes.png)

## Unstack chat for ddnet clients

Newer DDNet clients do not show duplicated messages multiple times. This is not always wanted when using call binds for team communication during pvp games. So there is ``sv_unstack_chat`` to revert that ddnet feature and ensure every message is sent properly in chat.

![unstack_chat](https://raw.githubusercontent.com/ddnet-insta/images/3c437acdea599788fb245518e9c25de7c0e63795/unstack_chat.png)

## 0.6 and 0.7 support including ready change

ddnet-insta uses the 0.6/0.7 server side version bridge from ddnet. So all gametypes are playable by latest teeworlds clients and ddnet clients at the same time.

In 0.7 there was a ready change added which allows users to pause the game. It only continues when everyone presses the ready bind.
This feature is now also possible for 0.6 clients using the /pause chat command. This feature should be turned off on public servers ``sv_player_ready_mode 0`` because it will be used by trolls.

![pause game](https://raw.githubusercontent.com/ddnet-insta/images/1a2d10c893605d704aeea8320cf0e65f8e0c2aa3/ready_change.png)

## 0.7 dead players in zCatch

In 0.6 dead players join the spectators team in the zCatch gamemode.

![zCatch 0.6](https://raw.githubusercontent.com/ddnet-insta/images/master/zCatch_teetime_06.png)

In 0.7 they are marked as dead players and are separate from spectators.

![zCatch 0.7](https://raw.githubusercontent.com/ddnet-insta/images/master/zCatch_teetime_07.png)

## Allow spectator votes for 0.7

The official teeworlds 0.7 client does block voting on the client side for spectators.
To make `sv_spectator_votes` portable and fair for both 0.6 and 0.7 players there is an option to allow
0.7 clients to vote as spectators. It is a bit hacky so it is hidden behind then config `sv_spectator_votes_sixup 1`
when that is set it will make the 0.7 clients believe they are in game and unlock the call vote menu.
But this also means that to join the game the users have to press the "spectate" button.

## DDNet rcon user support for 0.7

The vanilla teeworlds client does not send a rcon username but the ddnet client does.
DDNet servers and thus also ddnet-insta have regular rcon passwords but also users with username and password.
These can be added using the rcon command ``auth_add`` or ``auth_add_p``.
On regular ddnet servers it is impossible for teeworlds clients to login using these credentials.
In ddnet-insta it is possible for 0.7 players to send ``username:password`` as the password.
And it will log them in if those are valid credentials.

## Lots of little fun opt in features

By default ddnet-insta tries to be ready to be used in competetive games. Being as close to prior implementations
of the gametypes as possible. With that being said there are lots of opt in configurations to customize the gameplay.


Check the [Configs](#Configs) section for a complete list. One of the highlights would be dropping the flag in CTF
gametypes.

![drop flags](https://raw.githubusercontent.com/ddnet-insta/images/master/drop_flag.gif)

Most of the settings that affect the gameplay can be shown to the user with `sv_show_settings_motd`
so they know what is going on:

![settings motd](https://raw.githubusercontent.com/ddnet-insta/images/master/settings_motd.png)

## Gametype support

Make sure to also `reload` or switch the map when changing the gametype.

### CTF

``sv_gametype ctf``

Vanilla teeworlds capture the flag. Is a team based mode where players can collect shields/health and weapons.
Capturing the enemy flag scores your team 100 points.


ddnet-insta is based on [ddnet](https://github.com/ddnet/ddnet) but it aims to fully implement correct [teeworlds](https://github.com/teeworlds/teeworlds) gameplay for the vanilla modes.
But there are vanilla gameplay features that require you to set some [configs](https://github.com/ddnet-insta/ddnet-insta?tab=readme-ov-file#configs).
And the 0.5 wallhammer bug is intentionally not fixed like it is in the official teeworlds 0.6 and 0.7 versions.

### DM

``sv_gametype dm``

Vanilla teeworlds deathmatch. Is a free for all mode where players can collect shields/health and weapons.
First player to reach the scorelimit wins.


ddnet-insta is based on [ddnet](https://github.com/ddnet/ddnet) but it aims to fully implement correct [teeworlds](https://github.com/teeworlds/teeworlds) gameplay for the vanilla modes.
But there are vanilla gameplay features that require you to set some [configs](https://github.com/ddnet-insta/ddnet-insta?tab=readme-ov-file#configs).
And the 0.5 wallhammer bug is intentionally not fixed like it is in the official teeworlds 0.6 and 0.7 versions.

### iCTF

``sv_gametype iCTF``

Instagib capture the flag. Is a team based mode where every player only has a laser rifle.
It kills with one shot and capturing the enemy flag scores your team 100 points.

### gCTF

``sv_gametype gCTF``

Grenade capture the flag. Is a team based mode where every player only has a rocket launcher.
It kills with one shot and capturing the enemy flag scores your team 100 points.

### iDM

``sv_gametype iDM``

Laser death match. One shot kills. First to reach the scorelimit wins.

### gDM

``sv_gametype gDM``

Grenade death match. One shot kills. First to reach the scorelimit wins.

### iTDM

``sv_gametype iTDM``

Laser team death match. One shot kills. First team to reach the scorelimit wins.

### gTDM

``sv_gametype gTDM``

Grenade team death match. One shot kills. First team to reach the scorelimit wins.

### zCatch

``sv_gametype zCatch``

If you get killed you stay dead until your killer dies. Last man standing wins.
It is an instagib gametype so one shot kills. You can choose the weapon with
`sv_spawn_weapons` the options are `grenade` or `laser`.

### bolofng

``sv_gametype bolofng``

Freeze next generation mode with grenade. One grenade hit freezes enemies.
Frozen enemies can be sacrificed to the gods by killing them in special spikes.
First player to reach the scorelimit wins.

### solofng

``sv_gametype solofng``

Like bolofng but with laser.

### boomfng

``sv_gametype boomfng``

Like bolofng but with teams.

### fng

``sv_gametype fng``

Like boomfng but with laser.

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


In this **reset.cfg** it is recommeded to set the following configs
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
+ `sv_bang_commands` chat cmds like !1vs1 0=off 1=read only no votes 2=all commands
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
+ `sv_hit_freeze_delay` How many seconds will players remain frozen after being hit with a weapon (only fng)
+ `sv_melt_hammer_scale_x` linearly scale up hammer x power, percentage, for hammering frozen teammates (needs sv_fng_hammer)
+ `sv_melt_hammer_scale_y` linearly scale up hammer y power, percentage, for hammering frozen teammates (needs sv_fng_hammer)
+ `sv_fng_hammer` use sv_hammer_scale_x/y and sv_melt_hammer_scale_x/y tuning for hammer
+ `sv_fng_spike_sound` Play flag capture sound when sacrificing an enemy into the spikes !0.6 only! (0=off/1=only the killer and the victim/2=everyone near the victim)
+ `sv_laser_text_points` display laser text in the world on scoring (only fng for now)
+ `sv_announce_steals` show in chat when someone stole a kill (only fng for now)
+ `sv_grenade_ammo_regen` Activate or deactivate grenade ammo regeneration in general
+ `sv_grenade_ammo_regen_time` Grenade ammo regeneration time in miliseconds
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
+ `sv_reload_time_on_hit` 0=default/off ticks it takes to shoot again after a shot was hit
+ `sv_punish_freeze_disconnect` freeze player for 20 seconds on rejoin when leaving server while being frozen
+ `sv_self_damage_respawn_delay_ms` time in miliseconds it takes to respawn after dieing by self damage
+ `sv_self_kill_respawn_delay_ms` time in miliseconds it takes to respawn after sending kill bind
+ `sv_enemy_kill_respawn_delay_ms` time in miliseconds it takes to respawn after getting killed by enemies
+ `sv_world_kill_respawn_delay_ms` time in miliseconds it takes to respawn after touching a deathtile
+ `sv_game_kill_respawn_delay_ms` time in miliseconds it takes to respawn after team change, round start and so on
+ `sv_chat_ratelimit_long_messages` Needs sv_spamprotection 0 (0=off, 1=only messages longer than 12 chars are limited)
+ `sv_chat_ratelimit_spectators` Needs sv_spamprotection 0 (0=off, 1=specs have slow chat)
+ `sv_chat_ratelimit_public_chat` Needs sv_spamprotection 0 (0=off, 1=non team chat is slow)
+ `sv_chat_ratelimit_non_calls` Needs sv_spamprotection 0 (0=off, 1=ratelimit all but call binds such as 'help')
+ `sv_chat_ratelimit_spam` Needs sv_spamprotection 0 (0=off, 1=ratelimit chat detected as spam)
+ `sv_chat_ratelimit_debug` Logs which of the ratelimits kicked in
+ `sv_require_chat_flag_to_chat` clients have to send playerflag chat to use public chat (commands are unrelated)
+ `sv_always_track_stats` Track stats no matter how many players are online
+ `sv_debug_catch` Debug zCatch ticks caught and in game
+ `sv_debug_stats` Verbose logging for the SQL player stats
+ `sv_vote_checkboxes` Fill [ ] checkbox in vote name if the config is already set
+ `sv_hide_admins` Only send admin status to other authed players
+ `sv_show_settings_motd` Show insta game settings in motd on join
+ `sv_unstack_chat` Revert ddnet clients duplicated chat message stacking
+ `sv_casual_rounds` 1=start rounds automatically, 0=require restart vote to properly start game
+ `sv_allow_team_change_during_pause` allow players to join the game or spectators during pause
+ `sv_tournament` Print messages saying tournament is running. No other effects.
+ `sv_tournament_chat` 0=off, 1=Spectators can not public chat, 2=Nobody can public chat
+ `sv_tournament_chat_smart` Turns sv_tournament_chat on on restart and off on round end
+ `sv_tournament_join_msgs` Hide join/leave of spectators in chat !0.6 only for now! (0=off,1=hidden,2=shown for specs)
+ `sv_round_stats_format_discord` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_round_stats_format_http` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_round_stats_format_file` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_print_round_stats` print top players in chat on round end
+ `sv_spawn_weapons` possible values: grenade, laser
+ `sv_zcatch_colors` Color scheme for zCatch options: teetime, savander
+ `sv_display_score` values: points, round_points, spree, current_spree, win_points, wins, kills, round_kills
+ `sv_tournament_welcome_chat` Chat message shown in chat on join when sv_tournament is 1
+ `sv_round_stats_discord_webhooks` If set will post score stats there on round end. Can be a comma separated list.
+ `sv_round_stats_http_endpoints` If set will post score stats there on round end. Can be a comma separated list.
+ `sv_round_stats_output_file` If set will write score stats there on round end

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
+ `add_map_to_pool` Can be picked by random_map_from_pool command (entries can be duplicated to increase chance)
+ `clear_map_pool` Clears pool used by random_map_from_pool command
+ `random_map_from_pool` Changes to random map from pool (see add_map_to_pool)
+ `gctf_antibot` runs the antibot command gctf (depends on closed source module)
+ `known_antibot` runs the antibot command known (depends on antibob antibot module)
+ `redirect` Redirect client to given port use victim \"all\" to redirect all but your self
+ `deep_jailid` deep freeze (undeep tile works) will be restored on respawn and reconnect
+ `deep_jailip` deep freeze (undeep tile works) will be restored on respawn and reconnect
+ `deep_jails` list all perma deeped players deeped by deep_jailid and deep_jailip commands
+ `undeep_jail` list all perma deeped players deeped by deep_jailid and deep_jailip commands

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
+ `/drop flag` if it is a CTF gametype the flagger can drop the flag without dieing if either `sv_drop_flag_on_selfkill` or `sv_drop_flag_on_vote` is set
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

# Publish round stats

At the end of every round the stats about every players score can be published to discord (and other destinations).

The following configs determine which format the stats will be represented in.

+ `sv_round_stats_format_discord` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_round_stats_format_http` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_round_stats_format_file` 0=csv 1=psv 2=ascii table 3=markdown table 4=json

And these configs determin where the stats will be sent to.

+ `sv_round_stats_discord_webhook` Will do a discord webhook POST request to that url. The url has to look like this: `https://discord.com/api/webhooks/1232962289217568799/8i_a89XXXXXXXXXXXXXXXXXXXXXXX`
  If you don't know how to setup a discord webhook, don't worry its quite simple. You need to have admin access to a discord server and then you can follow this [1 minute youtube tutorial](https://www.youtube.com/watch?v=fKksxz2Gdnc).
+ `sv_round_stats_http_endpoint` It will do a http POST request to that url with the round stats as payload. You can set this to your custom api endpoint that collect stats. Example: `https://api.zillyhuhn.com/insta/round_stats`
+ `sv_round_stats_output_file` It will write the round stats to a file located at that path. You could then read that file with another tool or expose it with an http server.
  It can be a relaltive path then it uses your storage.cfg location. Or a absolute path if it starts with a slash. The file will be overwritten on every round end.
  To avoid that you can use the `%t` placeholder in the filename and it will expand to a timestamp to avoid file name collisions.
  Example values: `stats.json`, `/tmp/round_stats_%t.csv`

## csv - comma separated values (format 0)

The first two rows are special. The first is a csv header.
The second one is the red and blue team score.
After that each row is two players each. Red player first and blue player second.
If player names include commas their name will be quoted (see the example player `foo,bar`).
So this is how the result of a 2x2 could look like:

```
red_name, red_score, blue_name, blue_score
red, 24, blue, 3
"foo,bar", 15, (1)ChillerDrago, 2
ChillerDragon, 0, ChillerDragon.*, 1
```

## psv - pipe separated values (format 1)

```
---> Server: unnamed server, Map: tmp/maps-07/ctf5_spikes, Gametype: gctf.
(Length: 0 min 17 sec, Scorelimit: 1, Timelimit: 0)

**Red Team:**                                       
Clan: **|*KoG*|**                                   
Id: 0 | Name: ChillerDragon | Score: 2 | Kills: 1 | Deaths: 0 | Ratio: 1.00
**Blue Team:**                                      
Clan: **|*KoG*|**                                   
Id: 1 | Name: ChillerDragon.* | Score: 0 | Kills: 0 | Deaths: 1 | Ratio: 0.00
---------------------                               
**Red: 1 | Blue 0**  
```

Here is how it would display when posted on discord:

![psv on discord](https://raw.githubusercontent.com/ddnet-insta/images/5fafe03ed60153096facf4cc5d56c5df9ff20a5c/psv_discord.png)

## Ascii table (format 2)

```
+-----------------+-----------+------------+
| map             | red_score | blue_score |
+-----------------+-----------+------------+
| ctf5            | 203       | 1          |
+-----------------+-----------+------------+

+-----+------+-----------------+--------+--------+--------+--------+------------+---------------+
| id  | team | name            | score  | kills  | deaths | ratio  | flag_grabs | flag_captures |
+-----+------+-----------------+--------+--------+--------+--------+------------+---------------+
| 0   | red  | foo             | 1      | 1      | 3      | 0.3%   | 0          | 0             |
| 1   | blue | bar             | 1      | 2      | 5      | 0.4%   | 0          | 0             |
| 2   | red  | ChillerDragon   | 19     | 7      | 2      | 3.5%   | 0          | 0             |
| 3   | blue | ChillerDragon.* | 2      | 2      | 5      | 0.4%   | 0          | 0             |
+-----+------+-----------------+--------+--------+--------+--------+------------+---------------+
```

## Markdown (format 3)

NOT IMPLEMENTED YET

## JSON - javascript object notation (format 4)

```json
{
        "server": "unnamed server",
        "map": "tmp/maps-07/ctf5_spikes",
        "game_type": "gctf",
        "game_duration_seconds": 67,
        "score_limit": 200,
        "time_limit": 0,
        "score_red": 203,
        "score_blue": 0,
        "players": [
                {
                        "id": 0,
                        "team": "red",
                        "name": "ChillerDragon",
                        "score": 15,
                        "kills": 3,
                        "deaths": 1,
                        "ratio": 3,
                        "flag_grabs": 3,
                        "flag_captures": 2
                },
                {
                        "id": 1,
                        "team": "blue",
                        "name": "ChillerDragon.*",
                        "score": 0,
                        "kills": 0,
                        "deaths": 3,
                        "ratio": 0,
                        "flag_grabs": 0,
                        "flag_captures": 0
                }
        ]
}
```
