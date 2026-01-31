#ifndef INSTA_SERVER_GAMEMODES_BALL_FOOT_FOOT_H
#define INSTA_SERVER_GAMEMODES_BALL_FOOT_FOOT_H

#include <insta/server/gamemodes/ball/base_foot.h>

#define SQL_COLUMN_FILE <insta/server/gamemodes/ball/foot/sql_columns.h>
#define SQL_COLUMN_CLASS CFootColumns
#include <insta/server/column_template.h>

class CGameControllerFoot : public CGameControllerBaseFoot
{
public:
	CGameControllerFoot(class CGameContext *pGameServer);
	~CGameControllerFoot() override;
};
#endif
