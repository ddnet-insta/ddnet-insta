#ifndef INSTA_SERVER_ACCOUNT_H
#define INSTA_SERVER_ACCOUNT_H

#include <engine/shared/protocol.h>

#include <generated/insta/mode_account.h>

#include <ctime>
#include <optional>

#define MIN_USERNAME_LENGTH 1
#define MAX_USERNAME_LENGTH 24
#define MAX_PASSWORD_LENGTH 128
#define MIN_PASSWORD_LENGTH 3
#define MAX_CONTACT_LENGTH 64

bool IsValidUsernameAndPassword(const char *pUsername, const char *pPassword, char *pErrorMsg, size_t ErrorMsgLen);

class CAccount
{
public:
	int m_Id;
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
	bool m_IsNameProtected = false;
	char m_aContact[MAX_CONTACT_LENGTH];
	std::optional<int> m_Pin = std::nullopt;
	char m_aRegisterIp[64];
	std::optional<time_t> m_LastLogin = std::nullopt;
	time_t m_RegisterDate;

	CModeAccount m_Mode;

	int Id() const
	{
		return m_Id;
	}

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
		m_aDisplayName[0] = '\0';
		m_IsNameProtected = false;
		m_LastLogin = std::nullopt;
		m_RegisterDate = 0;

		m_Mode.Reset();
	}

	CAccount()
	{
		Reset();
	}
};

#endif
