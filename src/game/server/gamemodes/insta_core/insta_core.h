#ifndef GAME_SERVER_GAMEMODES_INSTA_CORE_INSTA_CORE_H
#define GAME_SERVER_GAMEMODES_INSTA_CORE_INSTA_CORE_H

#include "../DDRace.h"

#include <generated/protocol7.h>

#include <game/server/player.h>

// base functionality of the ddnet-insta server
// should be inherited from in all gametypes
// it aims to be more generic than the base pvp controller
// and makes no assumption about your gameplay
// so it can also be used for ddrace based gamemodes
//
// the general rule of what should go into base pvp and what into insta core
// is the following:
// As much as possible should go into insta core so everyone can use it.
// Unless it is something that could be possibly unwanted by any gametype
// even ones not included in ddnet-insta. So everything that can not be turned off
// or ignored and could bother someone should go into the base pvp controller.
// Everything else into the insta core controller.
class CGameControllerInstaCore : public CGameControllerDDRace
{
public:
	CGameControllerInstaCore(class CGameContext *pGameServer);
	~CGameControllerInstaCore() override;

	// convenience accessors to copy code from gamecontext to the instagib controller
	class IServer *Server() const { return GameServer()->Server(); }
	class CConfig *Config() { return GameServer()->Config(); }
	class IConsole *Console() { return GameServer()->Console(); }
	class IStorage *Storage() { return GameServer()->Storage(); }

	void SendChatTarget(int To, const char *pText, int Flags = CGameContext::FLAG_SIX | CGameContext::FLAG_SIXUP) const;
	void SendChat(int ClientId, int Team, const char *pText, int SpamProtectionClientId = -1, int Flags = CGameContext::FLAG_SIX | CGameContext::FLAG_SIXUP);
	void SendChatSpectators(const char *pMessage, int Flags);

	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;

	void OnReset() override;
	void OnInit() override;
	void OnPlayerConnect(CPlayer *pPlayer) override;
	void OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason) override;

	// contains the base logic needed to make insta core work
	// such as ip storage tracking and frozen quitter tracking
	// you almost never want to not run this code
	virtual void InstaCoreDisconnect(CPlayer *pPlayer, const char *pReason);

	// logs the disconnect to console
	// and prints it to the chat
	// you can override this to silence the message or change it
	virtual void PrintDisconnect(CPlayer *pPlayer, const char *pReason);

	// prints the join message into public chat
	// you can override this to silence the message or change it
	virtual void PrintConnect(CPlayer *pPlayer, const char *pName);

	// prints the mod welcome and version message
	// called on join
	virtual void PrintModWelcome(CPlayer *pPlayer);

	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
	bool OnVoteNetMessage(const CNetMsg_Cl_Vote *pMsg, int ClientId) override;
	bool OnSetTeamNetMessage(const CNetMsg_Cl_SetTeam *pMsg, int ClientId) override;
	int GetPlayerTeam(class CPlayer *pPlayer, bool Sixup) override;
	int GetAutoTeam(int NotThisId) override;
	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	bool IsValidTeam(int Team) override;
	const char *GetTeamName(int Team) override;
	bool CanSpawn(int Team, vec2 *pOutPos, int DDTeam) override;
	bool OnSkinChange7(protocol7::CNetMsg_Cl_SkinChange *pMsg, int ClientId) override;
	void OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData) override;
	void OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData) override;
	void RoundInitPlayer(class CPlayer *pPlayer) override;
	void InitPlayer(class CPlayer *pPlayer) override;
	void Snap(int SnappingClient) override;
	int SnapFlagCarrierRed(int SnappingClient) override;
	int SnapFlagCarrierBlue(int SnappingClient) override;
	int SnapTeamscoreRed(int SnappingClient) override;
	int SnapTeamscoreBlue(int SnappingClient) override;
	int SnapPlayerFlags7(int SnappingClient, CPlayer *pPlayer, int PlayerFlags7) override;
	void SnapPlayer6(int SnappingClient, CPlayer *pPlayer, CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo) override;
	void SnapDDNetPlayer(int SnappingClient, CPlayer *pPlayer, CNetObj_DDNetPlayer *pDDNetPlayer) override;
	bool OnClientPacket(int ClientId, bool Sys, int MsgId, struct CNetChunk *pPacket, class CUnpacker *pUnpacker) override;
	bool UnfreezeOnHammerHit() const override;

	void OnPlayerTick(class CPlayer *pPlayer);
	void OnCharacterTick(class CCharacter *pChr);

private:
	bool m_InvalidateConnectedIpsCache = true;
	int m_NumConnectedIpsCached = 0;

	// pPlayer is the player whose skin changed
	// not the receiver of the message
	// the message is sent to all 0.7 players that are in clip region
	void SendSkinChangeToAllSixup(protocol7::CNetMsg_Sv_SkinChange *pMsg, CPlayer *pPlayer, bool ApplyNetworkClipping);

public:
	/* UpdateSpawnWeapons
	 *
	 * called when sv_spawn_weapons is updated
	 */
	void UpdateSpawnWeapons(bool Silent, bool Apply) override;
	enum ESpawnWeapons
	{
		SPAWN_WEAPON_LASER,
		SPAWN_WEAPON_GRENADE,
		NUM_SPAWN_WEAPONS
	};
	ESpawnWeapons m_SpawnWeapons;
	ESpawnWeapons GetSpawnWeapons(int ClientId) const { return m_SpawnWeapons; }
	int GetDefaultWeaponBasedOnSpawnWeapons() const;
	void SetSpawnWeapons(class CCharacter *pChr) override;

	void OnUpdateSpectatorVotesConfig() override;

	// Used for sv_punish_freeze_disconnect
	// restore freeze state on reconnect
	// this is used for players trying to bypass
	// getting frozen in fng or by anticamper
	std::vector<NETADDR> m_vFrozenQuitters;
	int64_t m_ReleaseAllFrozenQuittersTick = 0;
	void RestoreFreezeStateOnRejoin(CPlayer *pPlayer);

	// ***************
	// generic helpers
	// ***************

	void Anticamper();
	void ApplyVanillaDamage(int &Dmg, int From, int Weapon, CCharacter *pCharacter) override;

	// displays fng styled laser text points in the world
	void MakeLaserTextPoints(vec2 Pos, int Points, int Seconds, CClientMask Mask);

	// plays the satisfying hit sound
	// that is used in teeworlds when a projectile causes damage
	// in ddnet-insta it is used the same way in CTF/DM
	// but also for instagib weapons
	//
	// the sound name is SOUND_HIT
	// and it is only audible to the player who caused the damage
	// and to the spectators of that player
	void DoDamageHitSound(int KillerId);

	// play the flag capture sound for spike kills
	// call this on kill
	//
	// the sound name is SOUND_CTF_CAPTURE
	// when sv_spike_sound is 1 play for the killer and victim
	// when sv_spike_sound is 2 play for everyone nearby
	void DoSpikeKillSound(int VictimId, int KillerId);

	// cached amount of unique ips
	int NumConnectedIps();

	// get the lowest client id that has a tee in the world
	// returns -1 if no player is alive
	int GetFirstAlivePlayerId();

	// kills the tee of all connected players
	//
	// WARNING: this instantly deletes the character pointer!
	//          if this is called during a character tick it will cause use after free!
	//          Within a character tick you have to use CCharacter::Die instead of CPlayer::KillCharacter
	//          rule of thumb is that all controller methods that get passed a character pointer
	//          are in a character tick. In those you should never call this method!
	void KillAllPlayers();

	// returns player pointer or nullptr if none is found
	// UniqueId unlike regular ClientIds are not reused
	// they start at one and get incremented for every new player that gets created
	CPlayer *GetPlayerByUniqueId(uint32_t UniqueId);
};
#endif // GAME_SERVER_GAMEMODES_INSTA_CORE_INSTA_CORE_H
