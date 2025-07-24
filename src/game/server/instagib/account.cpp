#include "account.h"
#include "strhelpers.h"

bool IsValidUsernameAndPassword(const char *pUsername, const char *pPassword, char *pErrorMsg, size_t ErrorMsgLen)
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
	if(str_length(pPassword) < MIN_PASSWORD_LENGTH)
	{
		str_format(pErrorMsg, ErrorMsgLen, "Password too short (min %d)", MIN_PASSWORD_LENGTH);
		return false;
	}
	if(str_length(pPassword) > MAX_PASSWORD_LENGTH)
	{
		str_format(pErrorMsg, ErrorMsgLen, "Password too long (max %d)", MAX_PASSWORD_LENGTH);
		return false;
	}
	return true;
}
