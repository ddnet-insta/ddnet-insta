#ifndef GAME_SERVER_INSTAGIB_GAMECONTROLLER_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_IGAMECONTROLLER

#include <base/vmath.h>
#include <engine/map.h>
#include <engine/shared/protocol.h>
#include <game/server/teams.h>

#include <engine/shared/http.h> // ddnet-insta
#include <game/generated/protocol.h>
#include <game/generated/protocol7.h>

#include <game/server/gamecontext.h>
#include <game/server/instagib/enums.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/instagib/sql_stats_player.h>
#include <game/server/instagib/structs.h>

struct CScoreLoadBestTimeResult;

class IGameController
{
#endif // IN_CLASS_IGAMECONTROLLER

public:
	//      _     _            _        _           _
	//   __| | __| |_ __   ___| |_     (_)_ __  ___| |_ __ _
	//  / _` |/ _` | '_ \ / _ \ __|____| | '_ \/ __| __/ _` |
	// | (_| | (_| | | | |  __/ ||_____| | | | \__ \ || (_| |
	//  \__,_|\__,_|_| |_|\___|\__|    |_|_| |_|___/\__\__,_|
	//
	//

	// virtual bool OnLaserHitCharacter(vec2 From, vec2 To, class CLaser &Laser) {};
	/*
		Function: OnCharacterTakeDamage
			this function was added in ddnet-insta and is a non standard controller method.
			neither ddnet nor teeworlds have this

		Arguments:
			Force - Reference to force. Set this vector and it will be applied to the target characters velocity
			Dmg - Input and outoput damage that was applied. You can read and write it.
			From - Client Id of the player who delt the damage
			Weapon - Weapon id that was causing the damage see the WEAPON_* enums
			Character - Character that was damaged

		Returns:
			return true to skip ddnet CCharacter::TakeDamage() behavior
			which is applying the force and moving the damaged tee
			it also sets the happy eyes if the Dmg is not zero
	*/
	virtual bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) { return false; };

	/*
		Function: OnKill
			This method is called when one player kills another (no selfkills).
			It should be called before the victims character is marked as dead.
			It is similar to OnCharacterTakeDamage() and OnCharacterDeath()
			and is here to stadardize the concept of what a kill is across game types.

			Some modes have kills triggered by death and some triggered by damage.
			Some by both and some by only one of these.

			For example in fng the OnCharacterTakeDamage() is called on laser hit which is never a
			kill only a freeze.
			The real kill comes from a death triggered by the world.

			Modes like block and fly work similar to fng.

			And then there is modes like color catch where shots do not kill a tee in the world
			but conceptually a hit counts as a kill.

			So kill here conceptually stands for whatever the current gametype interprets as
			one player killing another player.

			The base implementation only sets the killers eye emote to happy.
			You can add additional things in here that you want to happen on kill.

		Arguments:
			pVictim - Player that died. Its character should be still alive but might die in this tick (depending on the gametype)
			pKiller - Player that causes the kill. Will never be nullptr. We need a killer for it to count as a kill.
			          Which means this method is not called if the killer left the game and his projectile hits after that.
			Weapon - Weapon id that was causing the damage see the WEAPON_* enums
	*/
	virtual void OnKill(class CPlayer *pVictim, class CPlayer *pKiller, int Weapon){};

	/*
		Function: SkipDamage
			Pure function without side effects to check if a damage will be applied.
			Used to implement all kinds of damage blockers like spawn protection,
			no self or team damage or custom configs such as hook only kills.

			Will be called from OnCharacterTakeDamage()

			You should never set any variables in this method.
			It might be called multiple times per damage.
			If you need a method with side effects that is ensured to
			only be called checkout.

			OnAnyDamage() and OnAppliedDamage()

		Arguments:
			Dmg - Input and outoput damage that was applied. You can read and write it.
			From - Client Id of the player who delt the damage
			Weapon - Weapon id that was causing the damage see the WEAPON_* enums
			Character - Character that was damaged
			ApplyForce - Output boolean if set to false will not apply force to the damaged target

		Returns:
			true - if the damage is skipped
			false - if the damage counts and will be applied
	*/
	virtual bool SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce) { return false; };

	/*
		Function: OnAnyDamage
			Side effect only function. That will be called for any damage caused.
			It is only called once per caused damage.
			It is also called for damage that will not be applied.
			So it is also called for team damage even if team damage is off.

			Implement your freeze/unfreeze effect here that would also apply to team mates.
			Implement your custom knock back here.

			But do not actually deal any damage here to the health and armor.
			Also do not kill here!

			If you need only the applied damage checkout OnAppliedDamage()

		Arguments:
			Force - Reference to force. Set this vector and it will be applied to the target characters velocity
			Dmg - Input and outoput damage that was applied. You can read and write it.
			From - Client Id of the player who delt the damage
			Weapon - Weapon id that was causing the damage see the WEAPON_* enums
			Character - Character that was damaged
	*/
	virtual void OnAnyDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter *pCharacter){};

	/*
		Function: OnAppliedDamage
			Side effect only function. That will be called for all actually applied damage.
			It is only called once per caused damage.
			Any blocked damage is excluded such as hitting team mates if firendly
			fire is off.

			If you also need hits that do not cause actual damage checkout OnAnyDamage()

		Arguments:
			Dmg - Input and outoput damage that was applied. You can read and write it.
			From - Client Id of the player who delt the damage
			Weapon - Weapon id that was causing the damage see the WEAPON_* enums
			Character - Character that was damaged
	*/
	virtual void OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter){};

	/*
		Function: ApplyVanillaDamage
			Creates the damage indicator effect.
			Plays the pain and hit sounds.
			Decreases the armor.
			But DOES NOT DECREASE HEALTH OR KILL.
			You have to applay the remaining Dmg to the characters health.
			It is recommended to use DecreaseHealthAndKill() for that

		Arguments:
			Dmg - Input and outoput damage. It might be decreased if it is self damage or hits armor.
			From - Client Id of the player who delt the damage
			Weapon - Weapon id that was causing the damage see the WEAPON_* enums
			Character - Character that was damaged
	*/
	virtual void ApplyVanillaDamage(int &Dmg, int From, int Weapon, CCharacter *pCharacter){};

	/*
		Function: DecreaseHealthAndKill
			Responsible for applying the damage.
			Decreases the health based on the damage.
			Does not decrease armor if you want to also decrease armor
			you need to call ApplyVanillaDamage() first.

			Also triggers the death of the victim character if the health goes below 1.

		Arguments:
			Dmg - Damage to be applied
			From - Client Id of the player who delt the damage
			Weapon - Weapon id that was causing the damage see the WEAPON_* enums
			Character - Character that was damaged

		Returns:
			true - if the character was killed
			false - if the character is still alive
	*/
	virtual bool DecreaseHealthAndKill(int Dmg, int From, int Weapon, CCharacter *pCharacter) { return false; };

	/*
		Function: OnInit
			Will be called at the end of CGameContext::OnInit
			and can be used in addition to the controllers constructor

			Its main use case is running code in a base controller after its child constructor
	*/
	virtual void OnInit(){};

	/*
		Function: ForceNetworkClipping
			Will be called on snap. Can be used to force remove entities from the snapshot.
			But can not be used to force add entities to the snapshot.

		Arguments:
			pEntity - entity that will or will not be included in the snapshot
			SnappingClient - ClientId of the player receiving the snapshot
			CheckPos - position of the entity

		Returns:
			true - to not include this entity in the snapshot for SnappingClient
			false - to let the ddnet code decide if clipping happens or not
	*/
	virtual bool ForceNetworkClipping(const CEntity *pEntity, int SnappingClient, vec2 CheckPos) { return false; }

	/*
		Function: ForceNetworkClippingLine
			Will be called on snap. Can be used to force remove entities from the snapshot.
			But can not be used to force add entities to the snapshot.

		Arguments:
			pEntity - entity that will or will not be included in the snapshot
			SnappingClient - ClientId of the player receiving the snapshot
			StartPos - start position of the line the entity is located at
			EndPos - end position of the line the enitity is located at

		Returns:
			true - to not include this entity in the snapshot for SnappingClient
			false - to let the ddnet code decide if clipping happens or not
	*/
	virtual bool ForceNetworkClippingLine(const CEntity *pEntity, int SnappingClient, vec2 StartPos, vec2 EndPos) { return false; }

	/*
		Function: OnClientDataPersist
			Will be called before map changes.
			Store your data here that you want to keep across map changes.
			To extended the data struct have a look at the file

			src/game/server/instagib/persistent_client_data.h

			And to load the data again you have to also implement
			OnClientDataRestore()

		Arguments:
			pPlayer - the player that is about to be destroyed (read from here)
			pData - the struct that can store the values across map changes (write to this)
	*/
	virtual void OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData){};

	/*
		Function: OnClientDataRestore
			Will be called on map load. But only for players
			that have persisted data from the last map change.

			Will be called before map changes.
			Store your data here that you want to keep across map changes.
			To extended the data struct have a look at the file

			src/game/server/instagib/persistent_client_data.h

			And to load the data again you have to also implement
			OnClientDataRestore()

		Arguments:
			pPlayer - the player that is about to be destroyed (write to this)
			pData - the struct that can store the values across map changes (read from here)
	*/
	virtual void OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData){};

	/*
		Function: OnRoundStart
			Will be called after OnInit when the server first launches
			Will also be called on the beginning of every round

			Beginning of a round is defined as after the warmup but before the countdown.

			For example if "sv_countdown_round_start" is set to "10"
			and someone runs the bang command "!restart 5" in chat
			this will happen:
				- 5 seconds warmup everyone can move around and warmup
				- OnRoundStart() is called
				- 10 seconds world is paused and there is a final countdown
				- all tees will be respawned and the game starts
	*/
	virtual void OnRoundStart(){};

	/*
		Function: OnRoundEnd
			Will be called at the beginning of the end of every round.
			If you need to run code after the waiting time in the death screen
			consider using `OnRoundStart()`
	*/
	virtual void OnRoundEnd(){};

	/*
		Function: OnLaserHit
			Will be called before Character::TakeDamage() and CGameController::OnCharacterTakeDamage()

			this function was added in ddnet-insta and is a non standard controller method.
			neither ddnet nor teeworlds have this

		Arguments:
			Bounces - 1 and more is a wallshot
			From - client id of the player who shot the laser
			Weapon - probably either WEAPON_LASER or WEAPON_SHOTGUN
			pVictim - character that was hit

		Returns:
			true - to call TakeDamage
			false - to skip TakeDamage
	*/
	virtual bool OnLaserHit(int Bounces, int From, int Weapon, CCharacter *pVictim) { return true; };

	/*
		Function: OnHammerHit
			Similar to CAntibot::OnHammerHit() called from the same spot.
			With same argument order with the aditional argument Force which
			can inform you about the Force that would be applied on hit.
			You can also overwrite that value to change the hammer knockback.

			Unlike OnLaserHit() it is called from within CGameController::OnCharacterTakeDamage()

			Be careful the pPlayer that hit the hammer might not have a character anymore!
			`pPlayer->GetCharacter()` can be `nullptr` because tees can land a hammer in the tick
			they die.

		Arguments:
			pPlayer - The player that landed the hammer hit. Can be dead already. `pPlayer->GetCharacter()` can be `nullptr`.
			          If you still need information about the character that landed the hit.
				  Use `pPlayer->GetCharacterDeadOrAlive()` but be careful!
			pTarget - The player that got hit with the hammer
			Force - The force that will be applied to the hit character.
			        It is already set to the default once this method is called.
				And you can overwrite the value if you want to apply
				a different velocity to the hit character.
	*/
	virtual void OnHammerHit(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force){};

	/*
		Function: OnExplosionHits
			Will be called after every explosion.
			When all hit targets are known.
			At this point damage has already been delt
			And players might have already been killed.
			All of that happens in CGameController::OnCharacterTakeDamage().

			Use this method if you need to know all targets of the explosion.

			Try to avoid dealing damage in this method otherwise it is duplicated
			with CGameController::OnCharacterTakeDamage().
			If you need to know all targets to deal damage you have to skip the damage dealing
			in CGameController::OnCharacterTakeDamage()
			Ideally by overwriting OnAppliedDamage()
			Do not use this method to deal damage that should happen in CGameController::OnCharacterTakeDamage().

		Arguments:
			OwnerId - Client Id of the player that triggered the explosion. Might be -1 if its not coming from a player but from the world.
			ExplosionHits - Characters that got hit by the explosion. They are not filtered yet by SkipDamage() you have to do that!
	*/
	virtual void OnExplosionHits(int OwnerId, CExplosionTarget *pTargets, int NumTargets){};

	/*
		Function: ApplyFngHammerForce
			Hammers in fng have differnt tuning.
			If sv_fng_hammer is set the hammer is a bit stronger.
			This method applies this custom knock back.

		Arguments:
			pPlayer - The player that landed the hammer hit. Can be dead already. `pPlayer->GetCharacter()` can be `nullptr`.
			          If you still need information about the character that landed the hit.
				  Use `pPlayer->GetCharacterDeadOrAlive()` but be careful!
			pTarget - The player that got hit with the hammer
			Force - The force that will be applied to the hit character.
			        It is already set to the default once this method is called.
				And you can overwrite the value if you want to apply
				a different velocity to the hit character.
	*/
	virtual void ApplyFngHammerForce(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force){};

	/*
		Function: ApplyFngHammerForce
			Hammers in fng can unfreeze team mates.
			But not like in ddrace with one hit.
			It actually takes a few hits.
			And every hit decreases the freeze time a bit.
			This method is implementing this freeze time decrease.

		Arguments:
			pPlayer - The player that landed the hammer hit. Can be dead already. `pPlayer->GetCharacter()` can be `nullptr`.
			          If you still need information about the character that landed the hit.
				  Use `pPlayer->GetCharacterDeadOrAlive()` but be careful!
			pTarget - The player that got hit with the hammer
			Force - The force that will be applied to the hit character.
			        It is already set to the default once this method is called.
				And you can overwrite the value if you want to apply
				a different velocity to the hit character.

				Actually not used in ddnet-insta
				can be used to implement your own freeze hammer that depends on how
				how far the hammer would throw for example
	*/
	virtual void FngUnmeltHammerHit(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force){};

	/*
		Function: OnFireWeapon
			this function was added in ddnet-insta and is a non standard controller method.
			neither ddnet nor teeworlds have this

		Returns:
			return true to skip ddnet CCharacter::FireWeapon() behavior
			which is doing standard ddnet fire weapon things
	*/
	virtual bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) { return false; };

	/*
		Function: AmmoRegen
			Called directly after FireWeapon().
			Implements the vanilla weapon ammo reloading for the gun in ctf gametypes.
			And also handles the ammo regeneration for grenades in instagib modes such as
			gdm, gctf and zCatch if ammo limits are configured for these modes

		Arguments:
			pChr - Character that might gain new ammo
	*/
	virtual void AmmoRegen(CCharacter *pChr);

	/*
		Function: OnClientPacket
			hooks early into CServer::ProcessClientPacket
			similar to CGameContext::OnMessage but convers both system and game messages
			and it can also drop the message before the server processes it

		Returns:
			return true to consume the message and drop it before it gets passed to the server code
			return false to let regular server code process the message
	*/
	virtual bool OnClientPacket(int ClientId, bool Sys, int MsgId, struct CNetChunk *pPacket, class CUnpacker *pUnpacker) { return false; }

	/*
		Function: OnChatMessage
			hooks into CGameContext::OnSayNetMessage()
			after unicode check and teehistorian already happend

		Returns:
			return true to not run the rest of CGameContext::OnSayNetMessage()
			which would print it to the chat or run it as a ddrace chat command
	*/
	virtual bool OnChatMessage(const CNetMsg_Cl_Say *pMsg, int Length, int &Team, CPlayer *pPlayer) { return false; };

	/*
		Function: OnTeamChatCmd
			Called when a player runs the /team ddnet chat command
			Called before the ddnet code runs

		Returns:
			return true to not run the ddnet code
	*/
	virtual bool OnTeamChatCmd(IConsole::IResult *pResult) { return false; }

	/*
		Function: OnSetDDRaceTeam
			Called every time a player changes team
			Either by explicitly using the /teams command sucessfully
			or implicitly by dieing or similar

		Returns:
			return true to not run the ddnet code
	*/
	virtual bool OnSetDDRaceTeam(int ClientId, int Team) { return false; }

	/*
		Function: OnChangeInfoNetMessage
			hooks into CGameContext::OnChangeInfoNetMessage()
			after spam protection check

		Returns:
			return true to not run the rest of CGameContext::OnChangeInfoNetMessage()
	*/
	virtual bool OnChangeInfoNetMessage(const CNetMsg_Cl_ChangeInfo *pMsg, int ClientId) { return false; }

	/*
		Function: OnSkinChange7
			gets run if a 0.7 client requested a skin change
			after spam protection check

		Returns:
			return true to skip the default behavior and consume the event
	*/
	virtual bool OnSkinChange7(protocol7::CNetMsg_Cl_SkinChange *pMsg, int ClientId) { return false; }

	/*
		Function: OnSetTeamNetMessage
			hooks into CGameContext::OnSetTeamNetMessage()
			before any spam protection check

			See also CanJoinTeam() which is called after the validation

		Returns:
			return true to not run the rest of CGameContext::OnSetTeamNetMessage()
	*/
	virtual bool OnSetTeamNetMessage(const CNetMsg_Cl_SetTeam *pMsg, int ClientId) { return false; };

	/*
		Function: OnCallVoteNetMessage
			hooks into CGameContext::OnCallVoteNetMessage()
			before any spam protection check

			This is being called when a player creates a new vote

			See also `OnVoteNetMessage()`

		Returns:
			return true to not run the rest of CGameContext::OnCallVoteNetMessage()
	*/
	virtual bool OnCallVoteNetMessage(const CNetMsg_Cl_CallVote *pMsg, int ClientId)
	{
		return false;
	}

	/*
		Function: OnVoteNetMessage
			hooks into CGameContext::OnVoteNetMessage()
			before any spam protection check

			This is being called when a player votes yes or no.

			See also `OnCallVoteNetMessage()`

		Returns:
			return true to not run the rest of CGameContext::OnVoteNetMessage()
	*/
	virtual bool OnVoteNetMessage(const CNetMsg_Cl_Vote *pMsg, int ClientId) { return false; }

	/*
		Function: GetPlayerTeam
			wraps CPlayer::GetTeam()
			to spoof fake teams for different versions
			this can be used to place players into spec for 0.6 and dead spec for 0.7

		Arguments:
			Sixup - will be true if that team value is sent to a 0.7 connection and false otherwise

		Returns:
			as integer TEAM_RED, TEAM_BLUE or TEAM_SPECTATORS
	*/
	virtual int GetPlayerTeam(class CPlayer *pPlayer, bool Sixup);

	/*
		Function: HasVanillaShotgun
			Check if a given player currently has a vanilla
			or ddrace shotgun.
			Where vanilla shotgun is defined as a shotgun that
			shoots multiple bullets which damage on hit.
			And a ddrace shotgun is a shotgun that shoots a single laser which pulls on hit.

			Even instagib shotguns where every bullet kills instantly
			are considered vanilla shotguns in this case.

		Arguments:
			pPlayer - player that will be checked

		Returns:
			true - if that pPlayer currently has a vanilla shotgun equipped
			false - if that pPlayer has a ddrace shotgun
	*/
	virtual bool HasVanillaShotgun(class CPlayer *pPlayer) { return m_IsVanillaGameType; };

	/*
		Function: GetDefaultWeapon
			Returns the weapon the tee should spawn with.
			Is not a complete list of all weapons the tee gets on spawn.

			The complete list of weapons depends on the active controller and what it sets in
			its OnCharacterSpawn() if it is a gamemode without fixed weapons
			it depends on sv_spawn_weapons then it will call IGameController:SetSpawnWeapons()
	*/
	virtual int GetDefaultWeapon(class CPlayer *pPlayer) { return WEAPON_GUN; }

	/*
		Function: SetSpawnWeapons
			Is empty by default because ddnet and many ddnet-insta modes cover that in
			IGameController::OnCharacterSpawn()
			This method was added to set spawn weapons independently of the gamecontroller
			so we can use the same zCatch controller for laser and grenade zCatch

			It is also different from GetDefaultWeapon() because it could set more then one weapon.

			All gamemodes that allow different type of spawn weapons should call SetSpawnWeapons()
			in their OnCharacterSpawn() hook and also set the default weapon to GetDefaultWeaponBasedOnSpawnWeapons()
	*/
	virtual void SetSpawnWeapons(class CCharacter *pChr){};

	/*
		Function: UpdateSpawnWeapons
			called when the config sv_spawn_weapons is updated
			to update the internal enum

		Arguments:
			Silent - if false it might print warnings to the admin console
			Apply - if false it has no effect. Used to make sure spawn weapons only get changed on reload.
	*/
	virtual void UpdateSpawnWeapons(bool Silent = false, bool Apply = false){};

	/*
		Function: IsWinner
			called on disconnect and round end
			used to track stats

		Arguments:
			pPlayer - the player to check
			pMessage - should be sent to pPlayer in chat contains messages such as "you gained one win", "this win did not count because xyz"
			SizeOfMessage - size of the message buffer
	*/
	virtual bool IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage) { return false; }

	/*
		Function: IsLoser
			called on disconnect and round end
			used to track stats

		Arguments:
			pPlayer - the player to check
	*/
	virtual bool IsLoser(const CPlayer *pPlayer) { return false; }

	/*
		Function: WinPointsForWin
			Computes the amount of win points for winning a round.
			"win points" are points you can only get by winning.
			The purpose of differentiating between amount of wins and
			win points is to have some kind of value of the win.
			Some wins are harder to obtain than others depending on
			the amount of enemies and scorelimit for example.

			By default the reward will be amount of enemies plus score on round end.
			In zCatch it only depends on the amount of kills in the winning streak.
			These are WinPoints are not to be confused with regular round Points.

		Arguments:
			pPlayer - the player that won
	*/
	virtual int WinPointsForWin(const CPlayer *pPlayer);

	/*
		Function: IsPlaying
			Should return true if the player is playing. But the player does not have to
			be alive. And might be currently in team spectators (for example LMS/zCatch).

			Should return false if the player is intentionally spectating
			and not participating in the game at all.

			This replaces the pPlayer->m_Team == TEAM_SPECTATORS check because it supports
			also dead players and any other situtations where players that are technically not
			just watching the game end up in the spectator team for a short period of time.

		Arguments:
			pPlayer - the player to check
	*/
	virtual bool IsPlaying(const CPlayer *pPlayer);

	/*
		Function: OnShowStatsAll
			called from the main thread when a SQL worker finished querying stats from the database

		Arguments:
			pStats - stats struct to display
			pRequestingPlayer - player who initiated the stats request (might differ from the requested player)
			pRequestedName - player name the stats belong to
	*/
	virtual void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName){};

	/*
		Function: OnShowRoundStats
			called when /stats command is executed
			print your gamemode specific round stats here

		Arguments:
			pStats - stats struct to display
			pRequestingPlayer - player who initiated the stats request (might differ from the requested player)
			pRequestedName - player name the stats belong to
	*/


	/*
		Function: CanClientDrop
			this function was added in ddnet-insta and is a non standard controller method.
			neither ddnet nor teeworlds have this

		Arguments:
			ClientId - id of the player to load the stats for
			pReason - Reason for shutdown

		Returns:
			bool - `true` to allow the client to disconnect, `false` to block the
	*/
	virtual bool CanClientDrop(int ClientId, const char *pReason){ return true; };

	virtual void OnShowRoundStats(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName){};

	/*
		Function: OnShowMultis
			called from the main thread when a SQL worker finished querying stats from the database
			called when someone uses the /multis chat command

		Arguments:
			pStats - stats struct to display
			pRequestingPlayer - player who initiated the stats request (might differ from the requested player)
			pRequestedName - player name the stats belong to
	*/
	virtual void OnShowMultis(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName){};

	/*
		Function: OnShowSteals
			called from the main thread when a SQL worker finished querying stats from the database
			called when someone uses the /steals chat command

		Arguments:
			pStats - stats struct to display
			pRequestingPlayer - player who initiated the stats request (might differ from the requested player)
			pRequestedName - player name the stats belong to
	*/
	virtual void OnShowSteals(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName){};

	/*
		Function: OnLoadedNameStats
			Called when the stats request finished that fetches the
			stats for players that just connected or changed their name

			This can be used for save servers to display the players
			all time stats in the scoreboard

		Arguments:
			pStats - stats struct that was loaded
			pPlayer - player the stats are from
	*/
	virtual void OnLoadedNameStats(const CSqlStatsPlayer *pStats, class CPlayer *pPlayer){};

	/*
		Function: OnShowRank
			called from the main thread when a SQL worker finished querying a rank from the database

		Arguments:
			Rank - is the rank the player got with its score compared to all other players (lower is better)
			RankedScore - is the score that was used to obtain the rank if its ranking kills this will be the amount of kills
			pRankType - is the displayable string that shows the type of ranks (for example "Kills")
			pRequestingPlayer - player who initiated the stats request (might differ from the requested player)
			pRequestedName - player name the stats belong to
	*/
	virtual void OnShowRank(
		int Rank,
		int RankedScore,
		const char *pRankType,
		class CPlayer *pRequestingPlayer,
		const char *pRequestedName){};

	/*
		Function: IsStatTrack
			Called before stats changed.
			If this returns false the stats will not be updated.
			This is used to protect against farming. Define for example a minium amount of in game players
			required to count the stats.

		Arguments:
			pReason - reason buffer for stat track being off
			SizeOfReason - reason buffer size

		Returns:
			true - count stats
			false - do not count stats
	*/
	virtual bool IsStatTrack(char *pReason = nullptr, int SizeOfReason = 0)
	{
		if(pReason)
			pReason[0] = '\0';
		return true;
	}

	/*
		Function: SaveStatsOnRoundEnd
			Called for every player on round end once
			the base_pvp controller implements stats saving
			you probably do not need to extend this.
			If a player leaves before round end the method
			SaveStatsOnDisconnect() will be called.

		Arguments:
			pPlayer - player to save stats for
	*/
	virtual void SaveStatsOnRoundEnd(CPlayer *pPlayer){};

	/*
		Function: SaveStatsOnDisconnect
			Called for every player that leaves the game
			unless the game state is in round end
			then SaveStatsOnRoundEnd() was already called

		Arguments:
			pPlayer - player to save stats for
	*/
	virtual void SaveStatsOnDisconnect(CPlayer *pPlayer){};
	/*
		Function: LoadNewPlayerNameData
			Similar to ddnets LoadPlayerData()
			Called on player connect and name change
			used to load stats for that name

		Arguments:
			ClientId - id of the player to load the stats for

		Returns:
			return true to not run any ddrace time loading code
	*/
	virtual bool LoadNewPlayerNameData(int ClientId) { return false; };
	virtual void OnPlayerReadyChange(class CPlayer *pPlayer); // 0.7 ready change
	virtual int SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags) { return DDRaceFlags; };
	virtual int SnapGameInfoExFlags2(int SnappingClient, int DDRaceFlags) { return DDRaceFlags; };

	/*
		Function: SnapPlayerFlags7
			Set custom player flags for 0.7 connections.

		Arguments:
			SnappingClient - Client Id of the player that will receive the snapshot
			pPlayer - CPlayer that is being snapped
			PlayerFlags7 - the flags that were already set for that player by ddnet

		Returns:
			return the new flags value that should be snapped to the SnappingClient
	*/
	virtual int SnapPlayerFlags7(int SnappingClient, CPlayer *pPlayer, int PlayerFlags7) { return PlayerFlags7; };

	/*
		Function: SnapPlayer6
			Alter snap values for 0.6 snapshots.
			For 0.7 use `SnapPlayerFlags7()` and `SnapPlayerScore()`

			Be careful with setting `pPlayerInfo->m_Score` to not overwrite
			what `SnapPlayerScore()` tries to set.

		Arguments:
			SnappingClient - Client Id of the player that will receive the snapshot
			pPlayer - CPlayer that is being snapped
			pClientInfo - (in and output) info that is being snappend which is already pre filled by ddnet and can be altered.
			pPlayerInfo - (in and output) info that is being snappend which is already pre filled by ddnet and can be altered.
	*/
	virtual void SnapPlayer6(int SnappingClient, CPlayer *pPlayer, CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo){};

	/*
		Function: SnapPlayerScore
			Warning its value could be overwritten by `SnapPlayer6()`

		Arguments:
			SnappingClient - Client Id of the player that will receive the snapshot
			pPlayer - CPlayer that is being snapped
			DDRaceScore - Current value of the score set by the ddnet code

		Returns:
			return the new score value that will be included in the snapshot
	*/
	virtual int SnapPlayerScore(int SnappingClient, CPlayer *pPlayer, int DDRaceScore);
	virtual void SnapDDNetCharacter(int SnappingClient, CCharacter *pChr, CNetObj_DDNetCharacter *pDDNetCharacter){};
	virtual void SnapDDNetPlayer(int SnappingClient, CPlayer *pPlayer, CNetObj_DDNetPlayer *pDDNetPlayer){};
	virtual int SnapRoundStartTick(int SnappingClient);
	virtual int SnapTimeLimit(int SnappingClient);

	/*
		Function: GetCarriedFlag
			Returns the type of flag the given player is currently carrying.
			Flag refers here to a CTF gametype flag which is either red, blue or none.

		Arguments:
			pPlayer - player to check

		Returns:
			FLAG_NONE -1
			FLAG_RED  0
			FLAG_BLUE 2
	*/
	virtual int GetCarriedFlag(class CPlayer *pPlayer);

	/*
		Function: InitPlayer
			Called once for every new CPlayer object that is being constructed
			is only called when a new player connects
			not on round end.
			See also `RoundInitPlayer()`

		Arguments:
			pPlayer - newly joined player
	*/
	virtual void InitPlayer(class CPlayer *pPlayer){};

	/*
		Function: RoundInitPlayer
			Called for all players when a new round starts
			And also for all players that join
			See also `InitPlayer()`

		Arguments:
			pPlayer - player that was connected on round start
	*/
	virtual void RoundInitPlayer(class CPlayer *pPlayer){};

	/*
		Function: DoTeamBalance
			Makes sure players are evenly distributed
			across team red and blue.

			Only affects team based modes.

			Will be called automatically if the user set
			the config variable.
			Or if an admin used the force_teambalance rcon command
	*/
	virtual void DoTeamBalance();

	/*
		Function: CanBeMovedOnBalance
			Check if a player can be moved during balance

		Arguments:
			ClientId - id of the player that is a candidate for balance move

		Returns:
			true - allow to move this player to another team
			false - do not allow to move this player to another team
	*/
	virtual bool CanBeMovedOnBalance(int ClientId) { return true; };

	/*
		Function: CheckTeamBalance
			Called on tick to check if teams should be balanced.
			Will then call DoteamBalance() to do the actual balancing.

			TODO: should this really be virtual?
	*/
	virtual void CheckTeamBalance();

	/*
		Function: FreeInGameSlots
			The amount of free in game slots.
			Used to block players from joining the game if
			for example a 1vs1 one is running and already
			2 players are playing.

			In ddnet-insta this value is more complex than
			looking at read red + team blue and the SvPlayerSlots / SvSpectatorSlots config
			because we also have game modes such as zCatch
			where players can be spectators during the time they are dead
			but they are still considered active players
			while there are also permanent spectators that do not
			occupy any slots.

			Call this method if you want to know how many players can still join the game.
			And overwrite it if you have a more custom demand to count these than
			looking at players that are not spectators.

			If this method returns 0 players will see this error in the broadcast
			"Only %d active players are allowed"
	*/
	virtual int FreeInGameSlots();
	virtual CClientMask FreezeDamageIndicatorMask(CCharacter *pChr);
	virtual void OnDDRaceTimeLoad(class CPlayer *pPlayer, float Time);

	// See also ddnet's SetArmorProgress() and ddnet-insta's SetArmorProgressEmpty()
	// used to keep armor progress bar in ddnet gametype
	// but remove it in favor of correct armor in vanilla based gametypes
	virtual void SetArmorProgressFull(CCharacter *pCharacter);

	// See also ddnet's SetArmorProgress() and ddnet-insta's SetArmorProgressFull()
	// used to keep armor progress bar in ddnet gametype
	// but remove it in favor of correct armor in vanilla based gametypes
	virtual void SetArmorProgressEmpty(CCharacter *pCharacter);

	// ddnet has grenade
	// but the actual implementation is in CGameControllerPvp::IsGrenadeGameType()
	virtual bool IsGrenadeGameType() const { return true; }
	virtual bool IsFngGameType() const { return false; }
	virtual bool IsZcatchGameType() const { return false; }
	bool IsVanillaGameType() const { return m_IsVanillaGameType; }
	virtual bool IsDDRaceGameType() const { return true; }
	bool m_IsVanillaGameType = false;
	// decides if own grenade explosions
	// or laser wallshots should harm the own tee
	// that includes vanilla damage, fng freeze and instagib kills
	bool m_SelfDamage = true;
	int m_DefaultWeapon = WEAPON_GUN;
	void CheckReadyStates(int WithoutId = -1);
	bool GetPlayersReadyState(int WithoutId = -1, int *pNumUnready = nullptr);
	void SetPlayersReadyState(bool ReadyState);
	bool IsPlayerReadyMode();
	void ToggleGamePause();
	void AbortWarmup()
	{
		if((m_GameState == IGS_WARMUP_GAME || m_GameState == IGS_WARMUP_USER) && m_GameStateTimer != TIMER_INFINITE)
		{
			SetGameState(IGS_GAME_RUNNING);
		}
	}
	void SwapTeamscore()
	{
		if(!IsTeamPlay())
			return;

		int Score = m_aTeamscore[TEAM_RED];
		m_aTeamscore[TEAM_RED] = m_aTeamscore[TEAM_BLUE];
		m_aTeamscore[TEAM_BLUE] = Score;
	};

	void AddTeamscore(int Team, int Score);

	// balancing
	enum
	{
		TBALANCE_CHECK = -2,
		TBALANCE_OK,
	};
	int m_aTeamSize[protocol7::NUM_TEAMS];
	// will be the first server tick where
	// teams started to be unbalanced
	// or the magic values TBALANCE_CHECK and TBALANCE_OK
	int m_UnbalancedTick = TBALANCE_OK;

	// game
	enum EGameState
	{
		// internal game states
		IGS_WARMUP_GAME, // warmup started by game because there're not enough players (infinite)
		IGS_WARMUP_USER, // warmup started by user action via rcon or new match (infinite or timer)

		IGS_START_COUNTDOWN_ROUND_START, // start countown to start match/round (tick timer)
		IGS_START_COUNTDOWN_UNPAUSE, // start countown to unpause the game

		IGS_GAME_PAUSED, // game paused (infinite or tick timer)
		IGS_GAME_RUNNING, // game running (infinite)

		IGS_END_ROUND, // round is over (tick timer)
	};
	EGameState m_GameState;
	EGameState GameState() const { return m_GameState; }
	bool IsWarmup() const { return m_GameState == IGS_WARMUP_GAME || m_GameState == IGS_WARMUP_USER; }
	bool IsInfiniteWarmup() const { return IsWarmup() && m_GameStateTimer == TIMER_INFINITE; }
	int IsGameRunning() const { return m_GameState == IGS_GAME_RUNNING; }
	int IsGameCountdown() const { return m_GameState == IGS_START_COUNTDOWN_ROUND_START || m_GameState == IGS_START_COUNTDOWN_UNPAUSE; }
	int m_GameStateTimer;

	enum EWinType
	{
		// First player or team to reach sv_scorelimit wins.
		WIN_BY_SCORE,

		// Last player or team to stay alive wins.
		WIN_BY_SURVIVAL,
	};

	EWinType m_WinType = WIN_BY_SCORE;

	// What is the determining factor to win the game.
	// Most game modes require reaching the sv_scorelimit.
	// But some also just look at who stays alive until the end.
	EWinType WinType() const { return m_WinType; }

	// custom ddnet-insta timers
	int m_UnpauseStartTick = 0;

	const char *GameStateToStr(EGameState GameState)
	{
		switch(GameState)
		{
		case IGS_WARMUP_GAME:
			return "IGS_WARMUP_GAME";
		case IGS_WARMUP_USER:
			return "IGS_WARMUP_USER";
		case IGS_START_COUNTDOWN_ROUND_START:
			return "IGS_START_COUNTDOWN_ROUND_START";
		case IGS_START_COUNTDOWN_UNPAUSE:
			return "IGS_START_COUNTDOWN_UNPAUSE";
		case IGS_GAME_PAUSED:
			return "IGS_GAME_PAUSED";
		case IGS_GAME_RUNNING:
			return "IGS_GAME_RUNNING";
		case IGS_END_ROUND:
			return "IGS_END_ROUND";
		}
		return "UNKNOWN";
	}

	bool HasEnoughPlayers() const { return (IsTeamPlay() && m_aTeamSize[TEAM_RED] > 0 && m_aTeamSize[TEAM_BLUE] > 0) || (!IsTeamPlay() && m_aTeamSize[TEAM_RED] > 1); }
	void SetGameState(EGameState GameState, int Timer = 0);

	bool m_AllowSkinColorChange = true;

	// protected:
public:
	struct CGameInfo
	{
		int m_MatchCurrent;
		int m_MatchNum;
		int m_ScoreLimit;
		int m_TimeLimit;
	} m_GameInfo;
	void SendGameInfo(int ClientId);
	/*
		Variable: m_GameStartTick
			Sent in snap to 0.7 clients for timer
	*/
	int m_GameStartTick;
	int m_aTeamscore[protocol7::NUM_TEAMS];

	void EndRound() { SetGameState(IGS_END_ROUND, TIMER_END); }

	float CalcKillDeathRatio(int Kills, int Deaths) const;

	void GetRoundEndStatsStrCsv(char *pBuf, size_t Size);
	void GetRoundEndStatsStrCsvTeamPlay(char *pBuf, size_t Size);
	void GetRoundEndStatsStrCsvNoTeamPlay(char *pBuf, size_t Size);
	void PsvRowPlayer(const CPlayer *pPlayer, char *pBuf, size_t Size);
	void GetRoundEndStatsStrJson(char *pBuf, size_t Size);
	void GetRoundEndStatsStrPsv(char *pBuf, size_t Size);
	void GetRoundEndStatsStrAsciiTable(char *pBuf, size_t SizeOfBuf);
	void GetRoundEndStatsStrHttp(char *pBuf, size_t Size);
	void GetRoundEndStatsStrDiscord(char *pBuf, size_t Size);
	void GetRoundEndStatsStrFile(char *pBuf, size_t Size);
	void PublishRoundEndStatsStrFile(const char *pStr);
	void PublishRoundEndStatsStrDiscord(const char *pStr);
	void PublishRoundEndStatsStrHttp(const char *pStr);
	void PublishRoundEndStats();
	void SendRoundTopMessage(int ClientId);

public:
	enum
	{
		TIMER_INFINITE = -1,
		TIMER_END = 10,
	};

	virtual bool DoWincheckRound(); // returns true when the match is over
	virtual void OnFlagReturn(class CFlag *pFlag); // ddnet-insta
	virtual void OnFlagGrab(class CFlag *pFlag); // ddnet-insta
	virtual void OnFlagCapture(class CFlag *pFlag, float Time, int TimeTicks); // ddnet-insta
	// return true to consume the event
	// and supress default ddnet selfkill behavior
	virtual bool OnSelfkill(int ClientId) { return false; };
	virtual void OnUpdateZcatchColorConfig(){};
	virtual void OnUpdateSpectatorVotesConfig(){};
	virtual bool DropFlag(class CCharacter *pChr) { return false; };
	virtual bool HasWinningScore(const CPlayer *pPlayer) const;

	/*
		Variable: m_GamePauseStartTime

		gets set to time_get() when a player pauses the game
		using the ready change if sv_player_ready_mode is active

		it can then be used to track how long a game has been paused already

		it is set to -1 if the game is currently not paused
	*/
	int64_t m_GamePauseStartTime;

	// if it is greater than 0 it means in that many
	// ticks the server will be shutdown
	//
	// depends on the base pvp controller to tick
	int m_TicksUntilShutdown = 0;

	bool IsSkinColorChangeAllowed() const { return m_AllowSkinColorChange; }
	int GameFlags() const { return m_GameFlags; }
	void CheckGameInfo();
	bool IsFriendlyFire(int ClientId1, int ClientId2) const;

	// get client id by in game name
	int GetCidByName(const char *pName);

	// it is safe to pass in any ClientId
	// returned value might be null
	CPlayer *GetPlayerOrNullptr(int ClientId) const;

	// only used in ctf gametypes
	class CFlag *m_apFlags[NUM_FLAGS];

	CSqlStats *m_pSqlStats = nullptr;
	const char *m_pStatsTable = "";
	const char *StatsTable() const { return m_pStatsTable; }

private:
#ifndef IN_CLASS_IGAMECONTROLLER
};
#endif
