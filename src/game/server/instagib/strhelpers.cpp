#include <base/system.h>

#include "strhelpers.h"

const char *str_find_digit(const char *Haystack)
{
	char aaDigits[][2] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

	const char *pFirst = NULL;
	for(const char *aDigit : aaDigits)
	{
		const char *s = str_find(Haystack, aDigit);
		if(s && (!pFirst || pFirst > s))
			pFirst = s;
	}
	return pFirst;
}

bool str_contains_ip(const char *pStr)
{
	const char *s = str_find_digit(pStr);
	if(!s)
		return false;
	// dbg_msg("str_contains_ip", "s=%s", s);
	for(int Byte = 0; Byte < 4; Byte++)
	{
		int i;
		for(i = 0; i < 3; i++)
		{
			// dbg_msg("str_contains_ip", "b=%d s=%c", byte, s[0]);
			if(!s && !s[0])
			{
				// dbg_msg("str_contains_ip", "EOL");
				return false;
			}
			if(i > 0 && s[0] == '.')
			{
				// dbg_msg("str_contains_ip", "break in byte %d at i=%d because got dot", byte, i);
				break;
			}
			if(!isdigit(s[0]))
			{
				if(i > 0)
				{
					// dbg_msg("str_contains_ip", "we got ip with less than 3 digits in the end");
					return true;
				}
				// dbg_msg("str_contains_ip", "non digit char");
				return false;
			}
			s++;
		}
		if(Byte == 3 && i > 1)
		{
			// dbg_msg("str_contains_ip", "we got ip with 3 digits in the end");
			return true;
		}
		if(s[0] == '.')
			s++;
		else
		{
			// dbg_msg("str_contains_ip", "expected dot got no dot!!");
			return false;
		}
	}
	return false;
}

void str_expand_timestamps(const char *pStr, char *pBuf, size_t SizeOfBuf)
{
	char aDate[64];
	str_timestamp(aDate, sizeof(aDate));
	int WriteIndex = 0;
	for(int ReadIndex = 0; pStr[ReadIndex] && WriteIndex < (int)(SizeOfBuf - 1); ReadIndex++)
	{
		if(pStr[ReadIndex] == '%' && pStr[ReadIndex + 1] == 't')
		{
			ReadIndex++;
			pBuf[WriteIndex] = '\0';
			str_append(pBuf, aDate, SizeOfBuf - str_length(pBuf));
			WriteIndex += str_length(aDate);
			continue;
		}
		pBuf[WriteIndex++] = pStr[ReadIndex];
	}
	// the timestamp can expand out of the buffer
	if(WriteIndex >= (int)SizeOfBuf)
		WriteIndex = (int)(SizeOfBuf - 1);
	pBuf[WriteIndex] = '\0';
}

char *str_escape_csv(char *pBuffer, int BufferSize, const char *pString)
{
	if(!str_find(pString, "\"") && !str_find(pString, ","))
	{
		str_copy(pBuffer, pString, BufferSize);
		return pBuffer;
	}

	int WriteIndex = 0;
	pBuffer[WriteIndex++] = '"';
	for(int ReadIndex = 0; pString[ReadIndex] && WriteIndex < (BufferSize - 1); ReadIndex++)
	{
		if(pString[ReadIndex] == '"')
		{
			pBuffer[WriteIndex++] = '"';
			pBuffer[WriteIndex++] = '"';
			continue;
		}
		pBuffer[WriteIndex++] = pString[ReadIndex];
	}
	if(WriteIndex >= BufferSize)
		WriteIndex = BufferSize - 1;
	pBuffer[WriteIndex++] = '"';
	if(WriteIndex >= BufferSize)
		WriteIndex = BufferSize - 1;
	pBuffer[WriteIndex] = '\0';
	return pBuffer;
}

bool str_isalpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool str_isalphanumeric(char c)
{
	return str_isalpha(c) || str_isnum(c);
}

bool str_contains_only_allowed_chars(const char *pAllowedCharacters, const char *pTestedString)
{
	const char *pTestChar = pTestedString;
	while(*pTestChar)
	{
		const char *pAllowedChar = pAllowedCharacters;
		bool CharOk = false;
		while(*pAllowedChar)
		{
			if(*pAllowedChar == *pTestChar)
			{
				CharOk = true;
				break;
			}
			pAllowedChar++;
		}
		if(!CharOk)
			return false;
		pTestChar++;
	}
	return true;
}

// int test_thing()
// {
// 	char aMsg[512];
// 	strncpy(aMsg, "join  my server 127.0.0.1 omg it best", sizeof(aMsg));

//     dbg_msg("test", "contains %d", str_contains_ip(aMsg));

// 	return 0;
// }
