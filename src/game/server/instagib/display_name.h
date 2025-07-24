#ifndef GAME_SERVER_INSTAGIB_DISPLAY_NAME_H
#define GAME_SERVER_INSTAGIB_DISPLAY_NAME_H

#include <engine/shared/protocol.h>
#include <game/server/instagib/account.h>

class CDisplayName
{
	// display name the player wants to set
	char m_aWantedName[MAX_NAME_LENGTH];

	// the last name that was broadcasted in chat
	// either in join or name change message
	// used to differentiate between actual name changes
	// and transition from pending names to confirmed names
	char m_aLastBroadcastedName[MAX_NAME_LENGTH];

	// assume all names to be claimed by others until confirmed otherwise
	bool m_CanUseName = false;

	// assume all names to be claimed until confirmed otherwise
	bool m_IsClaimed = true;

	// if it is false the owner fetch is still pending
	bool m_GotNameOwnerResponse = false;

	// account username of the account that claimed
	// the displayname m_aWantedName
	// if it is an empty string and m_GotNameOwnerResponse is true
	// the name is not claimed
	char m_aNameOwner[MAX_USERNAME_LENGTH];

	// the own account username
	char m_aUsername[MAX_USERNAME_LENGTH];

	// the name that should be displayed in game
	char m_aCurrentDisplayName[MAX_NAME_LENGTH];

	// the amount of times the desired wanted name changed
	// this is used to hide the first name change
	// from pending to accepted name
	// joining with a name counts as the first change
	int m_NumChanges = 0;

	void CheckUpdateDisplayName();

public:
	CDisplayName();

	void SetAccountUsername(const char *pUsername);
	void SetWantedName(const char *pDisplayName);
	void SetNameOwner(const char *pUsername);
	void SetLastBroadcastedName(const char *pDisplayName);

	// this name should not be displayed!
	// use DisplayName() instead
	const char *WantedName();

	const char *LastBroadcastedName();

	// get current display name
	// might have a prefix if the name is
	// claimed or the claim check is pending
	// this should be used to obtain the players current name
	const char *DisplayName();

	bool CanUseName() const;
	int NumChanges() const { return m_NumChanges; }
};

#endif
