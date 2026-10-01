# fng

There are multiple different fng gametypes in ddnet-insta.
You can enable them using the following config options:

- ``sv_gametype bolofng``
- ``sv_gametype solofng``
- ``sv_gametype boomfng``
- ``sv_gametype fng``

## fng specific config variables

If you want to customize or explore your ddnet-insta server it makes sense to read through
[all existing config variables](../settings_and_commands.md). But here are a set of configs
that are recommended for fng. They are not set as a default because there are no per gametype
defaults and these configs are not recommended for non fng modes.

### Essentials

Enabling these two configs is something most users probably want to do.

```
# fng traditionally has a stronger hammer
# it is off by default in ddnet-insta so you can activate it like this
sv_fng_hammer 1

# fng traditionally allows players to switch the weapon faster
# after shooting than regular vanilla teeworlds or ddnet would
# to enable that set the following config
sv_per_weapon_reload 1
```

### Fng specific customization

The following config variables are fng specific, but their defaults are already good for fng.
These are only interesting for users interested in customizing.

+ `sv_team_score_normal` Points a team receives for grabbing into normal spikes
+ `sv_team_score_gold` Points a team receives for grabbing into golden spikes
+ `sv_team_score_green` Points a team receives for grabbing into green spikes
+ `sv_team_score_purple` Points a team receives for grabbing into purple spikes
+ `sv_team_score_team` Points a team receives for grabbing into team spikes
+ `sv_player_score_normal` Points a player receives for grabbing into normal spikes
+ `sv_player_score_gold` Points a player receives for grabbing into golden spikes
+ `sv_player_score_green` Points a player receives for grabbing into green spikes
+ `sv_player_score_purple` Points a player receives for grabbing into purple spikes
+ `sv_player_score_team` Points a player receives for grabbing into team spikes
+ `sv_wrong_spike_freeze` The time, in seconds, a player gets frozen, if he grabbed a frozen opponent into the opponents spikes (0=off, fng only)
+ `sv_hammer_scale_x` linearly scale up hammer x power, percentage, for hammering enemies and unfrozen teammates (needs sv_fng_hammer)
+ `sv_hammer_scale_y` linearly scale up hammer y power, percentage, for hammering enemies and unfrozen teammates (needs sv_fng_hammer)
+ `sv_melt_hammer_scale_x` linearly scale up hammer x power, percentage, for hammering frozen teammates (needs sv_fng_hammer)
+ `sv_melt_hammer_scale_y` linearly scale up hammer y power, percentage, for hammering frozen teammates (needs sv_fng_hammer)
+ `sv_hit_freeze_delay` How many seconds will players remain frozen after being hit with a weapon (only fng)
+ `sv_spike_sound` Play flag capture sound when sacrificing an enemy into the spikes !0.6 only! (0=off/1=only the killer and the victim/2=everyone near the victim)
+ `sv_text_points` display text in the world on scoring (only fng for now. 1: laser, 2: projectile)
+ `sv_text_points_delay` Timer until text disappears in seconds (only fng for now)
+ `sv_announce_steals` show in chat when someone stole a kill (only fng for now)
+ `sv_kill_indicator` Shows the killer that he froze the player(only fng for now)
+ `sv_punish_freeze_disconnect` freeze player for 20 seconds on rejoin when leaving server while being frozen

## Generic pvp configs that also make sense in fng

+ `sv_scorelimit` Score limit (0 disables)
+ `sv_timelimit` Time limit in minutes (0 disables)
+ `sv_teambalance_time` How many minutes to wait before autobalancing teams (0=off)
+ `sv_teamdamage` Team damage
+ `sv_grenade_ammo_regen` Activate or deactivate grenade ammo regeneration in general
+ `sv_grenade_ammo_regen_time` Grenade ammo regeneration time in milliseconds
+ `sv_grenade_ammo_regen_num` Maximum number of grenades if ammo regeneration on
+ `sv_grenade_ammo_regen_speed` Give grenades back that push own player
+ `sv_grenade_ammo_regen_on_kill` Refill nades on kill (0=off, 1=1, 2=all)
+ `sv_grenade_ammo_regen_reset_on_fire` Reset regen time if shot is fired
+ `sv_sprayprotection` Spray protection
+ `sv_killingspree_kills` How many kills are needed to be on a killing-spree (0=off)
+ `sv_killingspree_reset_on_round_end` 0=allow spreeing across games 1=end spree on round end
+ `sv_damage_needed_for_kill` Grenade damage needed to kill in instagib modes
+ `sv_laser_reload_time_on_hit` 0=default/off ticks it takes to shoot again after a shot was hit (see also sv_fast_hit_full_auto)
+ `sv_fast_hit_full_auto` require fire button repress when sv_laser_reload_time_on_hit is set
