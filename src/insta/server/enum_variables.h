// This file can be included several times.

#ifndef LINK_CONFIG
#error "The config macros must be defined"
// This helps IDEs properly syntax highlight the uses of the macro below.
#define LINK_CONFIG(ConfigName, ConfigScriptName, EnumName)
#endif

LINK_CONFIG(SvBombtagBombWeapon, sv_bombtag_bomb_weapon, EBombWeapon)

#define BOMB_WEAPON_ENUM \
	X(GUN) \
	X(GRENADE) \
	X(LASER)
