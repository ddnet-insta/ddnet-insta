#ifndef INSTA_SERVER_GAMEMODES_VANILLA_CTF_CTF_H
#define INSTA_SERVER_GAMEMODES_VANILLA_CTF_CTF_H

#include "../base_ctf.h"

#define SQL_COLUMN_FILE <insta/server/gamemodes/vanilla/ctf/sql_columns.h>
#define SQL_COLUMN_CLASS CCtfColumns
#include <insta/server/column_template.h>

class CGameControllerCTF : public CGameControllerBaseCTF
{
public:
	CGameControllerCTF(class CGameContext *pGameServer);
	~CGameControllerCTF() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
	int SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags) override;
	bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
};
#endif
