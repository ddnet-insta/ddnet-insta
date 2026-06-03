#include "account.h"

#include "strhelpers.h"

#include <base/str.h>

#include <engine/shared/config.h>

static bool _IsValidUsernameAndPassword(bool IsStrict, const char *pUsername, const char *pPassword, char *pErrorMsg, size_t ErrorMsgLen)
{
	pErrorMsg[0] = '\0';

	if(!str_contains_only_allowed_chars(STR_ALLOW_ALPHANUMERIC, pUsername))
	{
		str_copy(pErrorMsg, "Username can only contain letters from a-z and numbers", ErrorMsgLen);
		return false;
	}
	if(str_length(pUsername) < MIN_USERNAME_LENGTH)
	{
		str_format(pErrorMsg, ErrorMsgLen, "Username too short (min %d)", MIN_USERNAME_LENGTH);
		return false;
	}
	if(str_length(pUsername) > MAX_USERNAME_LENGTH)
	{
		str_format(pErrorMsg, ErrorMsgLen, "Username too long (max %d)", MAX_USERNAME_LENGTH);
		return false;
	}
	if(pPassword)
	{
		if(IsStrict)
		{
			if(str_length(pPassword) < g_Config.m_SvMinPasswordLen)
			{
				str_format(pErrorMsg, ErrorMsgLen, "Password too short (min %d)", g_Config.m_SvMinPasswordLen);
				return false;
			}
		}
		// could use the config variable lower bound here
		// instead of hardcoding 3
		// https://github.com/ddnet/ddnet/issues/11695
		if(str_length(pPassword) < 3)
		{
			str_copy(pErrorMsg, "Password too short (min 3)", ErrorMsgLen);
			return false;
		}
		if(str_length(pPassword) > MAX_PASSWORD_LENGTH)
		{
			str_format(pErrorMsg, ErrorMsgLen, "Password too long (max %d)", MAX_PASSWORD_LENGTH);
			return false;
		}
	}
	return true;
}

bool IsValidUsernameAndPassword(const char *pUsername, const char *pPassword, char *pErrorMsg, size_t ErrorMsgLen)
{
	return _IsValidUsernameAndPassword(true, pUsername, pPassword, pErrorMsg, ErrorMsgLen);
}

bool IsValidUsernameAndPasswordRelaxed(const char *pUsername, const char *pPassword, char *pErrorMsg, size_t ErrorMsgLen)
{
	return _IsValidUsernameAndPassword(false, pUsername, pPassword, pErrorMsg, ErrorMsgLen);
}
