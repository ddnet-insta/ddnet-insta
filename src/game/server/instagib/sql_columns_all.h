// This file can be included several times.

#ifndef MACRO_ADD_COLUMN
#error "The column macros must be defined"
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ;
#endif

#ifndef MACRO_RANK_COLUMN
#error "The rank macros must be defined"
#define MACRO_RANK_COLUMN(name, sql_name, display_name, order_by) ;
#endif

#ifndef MACRO_TOP_COLUMN
#error "The top macros must be defined"
#define MACRO_TOP_COLUMN(name, sql_name, display_name, order_by) ;
#endif

#include <game/server/gamemodes/ball/foot/sql_columns.h>
#include <game/server/gamemodes/instagib/bolofng/sql_columns.h>
#include <game/server/gamemodes/instagib/boomfng/sql_columns.h>
#include <game/server/gamemodes/instagib/fng/sql_columns.h>
#include <game/server/gamemodes/instagib/gctf/sql_columns.h>
#include <game/server/gamemodes/instagib/ictf/sql_columns.h>
#include <game/server/gamemodes/instagib/idm/sql_columns.h>
#include <game/server/gamemodes/instagib/solofng/sql_columns.h>
#include <game/server/gamemodes/instagib/zcatch/sql_columns.h>
#include <game/server/gamemodes/vanilla/bomb/sql_columns.h>
#include <game/server/gamemodes/vanilla/ctf/sql_columns.h>
#include <game/server/gamemodes/vanilla/dm/sql_columns.h>

MACRO_RANK_COLUMN(WinPoints, win_points, "Win points", "DESC")
MACRO_RANK_COLUMN(Wins, wins, "Wins", "DESC")
MACRO_RANK_COLUMN(BestSpree, spree, "killing spree", "DESC")

MACRO_TOP_COLUMN(WinPoints, win_points, "Win points", "DESC")
MACRO_TOP_COLUMN(Wins, wins, "Wins", "DESC")
MACRO_TOP_COLUMN(BestSpree, spree, "killing spree", "DESC")
MACRO_TOP_COLUMN(Points, points, "Points", "DESC")
