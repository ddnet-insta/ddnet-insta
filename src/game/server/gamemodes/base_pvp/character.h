#ifndef GAME_SERVER_GAMEMODES_BASE_PVP_CHARACTER_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_CHARACTER

#include <game/server/entity.h>
#include <game/server/save.h>

class CCharacter : public CEntity
{
#endif // IN_CLASS_CHARACTER

	friend class CGameControllerVanilla;
	friend class CGameControllerPvp;
	friend class CGameControllerCTF;
	friend class CGameControllerBaseFng;

public:
	// ddnet-insta
	/*
		Reset instagib tee without resetting ddnet or teeworlds tee
		update grenade ammo state without selfkill
		useful for votes
	*/
	void ResetInstaSettings();

	// player can not be damaged with weapons
	bool m_IsGodmode = false;

	int Health() { return m_Health; };
	int Armor() { return m_Armor; };

	void SetHealth(int Amount) { m_Health = Amount; };
	// void SetArmor(int Amount) { m_Armor = Amount; }; // defined by ddnet

	void AddHealth(int Amount) { m_Health += Amount; };
	void AddArmor(int Amount) { m_Armor += Amount; };
	int GetAimDir() { return m_Input.m_TargetX < 0 ? -1 : 1; };

	void AmmoRegen();
	/*
		Function: IsTouchingTile

		High sensitive tile collision checker. Used for fng spikes.
		Same radius as vanilla death tiles.
	*/
	bool IsTouchingTile(int Tile);

	// High sensitive tile collision checker. Used for fng spikes.
	// Same radius as vanilla death tiles.
	// If we touch multiple tiles at the same time it returns the closest distance.
	//
	// Basically the same as IsTouchingTile()
	// but it returns 1024.0f if not close enough
	// and otherwise the distance to the tile.
	float DistToTouchingTile(int Tile);

	void Rainbow(bool Activate);
	bool HasRainbow() const { return m_Rainbow; }

	const class CPlayer *GetPlayer() const { return m_pPlayer; }
	int HookedPlayer() const { return m_Core.HookedPlayer(); }

private:
	// players skin changes colors
	bool m_Rainbow = false;

#ifndef IN_CLASS_CHARACTER
};
#endif
