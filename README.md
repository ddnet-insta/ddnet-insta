[![DDraceNetwork](other/ddnet-insta.png)](https://ddnet.tw) [![](https://github.com/ddnet-insta/ddnet-insta/workflows/Build/badge.svg)](https://github.com/ddnet-insta/ddnet-insta/actions?query=workflow%3ABuild+event%3Apush+branch%3Amaster)

DDNet-insta based on DDRaceNetwork, a Teeworlds mod. See the [website](https://ddnet.tw) for more information.

For build instructions visit the [ddnet repo](https://github.com/ddnet/ddnet).

---

A ddnet based pvp mod. With the focus on correct 0.6 and 0.7 support and staying close to and up to date with ddnet.
While being highly configurable and feature rich.

Implementing most of the relevant pvp gametypes: ctf, dm, gctf, ictf, gdm, idm, gtdm, itdm, zCatch, bolofng, solofng, boomfng, fng, tsmash, foot, bomb, block

# Project name and scope

The name **ddnet-insta** is short for DummyDragRaceNetwork Instagib. Because it started out as a ddnet based modification with the
focus on gCTF with the possible future scope for also other instagib (one shot kills) gametypes such as iCTF, zCatch and fng.
But it now has expanded to support also damage based gameplay such as teeworlds vanilla CTF and DM. So currently it aims to support
any kind of round based pvp gametype that can be implemented without causing too much maintenance cost.


NOTE FOR DEVELOPERS:


It also tries to be extendable and fork friendly. If you want to build a pvp gamemode with score points and rounds.
This code base should get you started quite easily. Just create a new folder for your gamemode. After wiring it up in
[your_mod.cpp](https://github.com/ddnet-insta/ddnet-insta/blob/ada1612bf585e836a6e72893c8f93011154affbc/src/game/server/gamemodes/mod.cpp#L28)
and [cmakelist](https://github.com/ddnet-insta/ddnet-insta/blob/ba38c11ccf46e888786c90c7dc0f09503be19a49/CMakeLists.txt#L2751-L2752)
you should be able to build almost any gametype without ever having to edit code outside
of your gamemode directory. Just look at the existing gamemodes for examples. A simple mode to look at would be
[vanilla deathmatch](https://github.com/ddnet-insta/ddnet-insta/tree/ba38c11ccf46e888786c90c7dc0f09503be19a49/src/game/server/gamemodes/vanilla/dm).
Adding your own stats columns and rank/top commands can be done in a few lines of code without writing any SQL.

There also is a command line tool that will generate the boilerplate code of a new controller for you.
To create a new gametype called zombie ball run the following command (needs git submodules and only works on linux/macOS):

```
./scripts/cli zombie_ball:base_pvp
```

More information about the cli tool can be found [here](https://github.com/ddnet-insta/cli)

# Project goals

- Stay close and up to date with upstream ddnet. Keep the git diff in files edited by ddnet as minimal as possible to keep merging cheap.
- Be generic, consistent and configurable. If possible the same config variables and concepts should be applied to all gametypes.
  For example configs such as anticamper should not be ``sv_zcatch_anticamper`` but ``sv_anticamper`` and work in all gametypes.
- Support latest ddnet and teeworlds clients fully and correctly.
- Be friendly to downstream projects. Commits and releases should warn about breaking changes.
  Downstream projects should have an easy time to add new gametypes that can be updated to new ddnet-insta versions
  without a lot of effort.

# Docker

There are official builds and docker files. If you have docker installed you can start a ddnet-insta server with only one
command like this:

```
docker run -p 8303:8303/udp ghcr.io/ddnet-insta/ddnet-insta:latest "sv_gametype dm;sv_map dm1;sv_rcon_password mypass"
```

For more information see [./ddnet-insta/docker/README.md](./ddnet-insta/docker/README.md).

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

By default ddnet-insta tries to be ready to be used in competitive games. Being as close to prior implementations
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

### TSmash

``sv_gametype tsmash``

A hammer deathmatch where you only die from spikes.
Every time you are hit you loose health which increases the knockback you take.
Pickup hearts and shields to reduce knockback.

### TTSmash

``sv_gametype ttsmash``

Like TSmash but with teams.

### foot

``sv_gametype foot``

A football (soccer) teamplay mode. Where the grenade is your ball.
And you have to score it into the enemy goal.

### bomb

``sv_gametype bomb``

One or more players get randomly selected as bomb which they can pass on
by hitting others. The bomb has a timer. The player that holds the bomb when it explodes or dies otherwise gets eliminated.
Last to survive wins.

### block

``sv_gametype block``

A ddrace based gametype where the goal is to kill others by throwing them into freeze tiles.
It is a free for all mode where one kill gives one point and first to reach the scorelimit wins.

### tblock

``sv_gametype block``

Like block but with teams.

### ddrace

``sv_gametype ddrace``

The core idea of ddnet-insta is basically everything but ddrace. But ddrace still works.
It is regular ddnet plus all the ddnet-insta extensions such as the rcon command `rainbow`,
anticamper and all the other features that can be disabled or do not directly interrupt the ddrace gameplay.

### ddnet

``sv_gametype ddnet``

This is as pure ddnet as possible. Some ddnet-insta specific extensions will still be visible such as rcon commands and chat commands.
But most of these will not work. This gametype is not really recommended. Either use actual official [ddnet](https://github.com/ddnet/ddnet)
for pure ddnet without bugs. Or use `sv_gametype ddrace` for ddnet plus all the ddnet-insta features.

# Configs

ddnet-insta added a bunch of configs, rcon commands and chat commands and even a own custom bang command system.
Have a look at the [settings and commands documentation](./ddnet-insta/docs/settings_and_commands.md) before starting your server.
Especially the section about the reset.cfg file is important to get a proper pvp server.

# Publish round stats

At the end of every round the stats about every players score can be published to discord (and other destinations).

The following configs determine which format the stats will be represented in.

+ `sv_round_stats_format_discord` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_round_stats_format_http` 0=csv 1=psv 2=ascii table 3=markdown table 4=json
+ `sv_round_stats_format_file` 0=csv 1=psv 2=ascii table 3=markdown table 4=json

And these configs determine where the stats will be sent to.

+ `sv_round_stats_discord_webhooks` Will do a discord webhook POST request to that url. The url has to look like this: `https://discord.com/api/webhooks/1232962289217568799/8i_a89XXXXXXXXXXXXXXXXXXXXXXX`
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

There also is an [example for a simple python server consuming that data here](./ddnet-insta/round_stats/)

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
                        "hooks": 0,
                        "hooks_missed": 0,
                        "hooks_hit_player": 0,
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
                        "hooks": 4,
                        "hooks_missed": 1,
                        "hooks_hit_player": 2,
                        "flag_grabs": 0,
                        "flag_captures": 0
                }
        ]
}
```
