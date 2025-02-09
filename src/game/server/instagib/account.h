#ifndef GAME_SERVER_INSTAGIB_ACCOUNT_H
#define GAME_SERVER_INSTAGIB_ACCOUNT_H

#include <base/system.h>
#include <engine/shared/protocol.h>

#define MIN_USERNAME_LENGTH 1
#define MAX_USERNAME_LENGTH 24
#define MAX_PASSWORD_LENGTH 128
#define MIN_PASSWORD_LENGTH 3
#define MAX_CONTACT_LENGTH 64

bool IsValidUsernameAndPassword(const char *pUsername, const char *pPassword, char *pErrorMsg, size_t ErrorMsgLen);

class CAccount
{
public:
	char m_aUsername[MAX_USERNAME_LENGTH];
	char m_aHashWithSalt[MAX_PASSWORD_LENGTH]; // should be plaintext password for /changepassword command
	bool m_IsLoggedIn = false;

	// should never be written to
	// this value is not saved to avoid unlocking accounts
	// by doing /logout after getting locked
	bool m_IsLocked = false;

	char m_aServerIp[64];
	int m_ServerPort = 0;
	char m_aDisplayName[MAX_NAME_LENGTH];
	char m_aContact[MAX_CONTACT_LENGTH];
	int m_Pin = 0;
	char m_aRegiserIp[64];
	// TODO:
	// created_at
	// updated_at

	const char *Username() const
	{
		return m_aUsername;
	}

	bool IsLoggedIn() const
	{
		return m_IsLoggedIn;
	}

	bool IsLocked() const
	{
		return m_IsLocked;
	}

	const char *ServerIp() const
	{
		return m_aServerIp;
	}

	int ServerPort() const
	{
		return m_ServerPort;
	}

	void Reset()
	{
		m_aUsername[0] = '\0';
		m_aHashWithSalt[0] = '\0';
		m_IsLoggedIn = false;
		m_aServerIp[0] = '\0';
		m_ServerPort = 0;
	}

	CAccount()
	{
		Reset();
	}
};

#endif
