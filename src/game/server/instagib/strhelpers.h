#ifndef GAME_SERVER_INSTAGIB_STRHELPERS_H
#define GAME_SERVER_INSTAGIB_STRHELPERS_H

#include <cctype>
#include <cstring>

#include <base/system.h>

const char *str_find_digit(const char *Haystack);
bool str_contains_ip(const char *pStr);

/**
 * Replaces '%t' with timestamps in a string
 *
 * @ingroup Strings
 *
 * @param pStr Source string with %t placeholders
 * @param pBuf Destination string that will be written to
 * @param SizeOfBuf Size of destination string
 *
 * @remark Guarantees that pBuf string will contain zero-termination.
 */
void str_expand_timestamps(const char *pStr, char *pBuf, size_t SizeOfBuf);

/**
 * Escapes one csv value. And returns the result.
 * If no escaping is required it returns the raw result.
 * If escaping is required it will put the value in quotes.
 *
 * It is following pythons excel standard like ddnet's CsvWrite()
 *
 * @ingroup Strings
 *
 * @param pBuffer buffer used to store the new escaped value (but look at the return value instead)
 * @param BufferSize size of temporary buffer
 * @param pString Input value to be escaped
 *
 * @remark Guarantees that the return value is zero-terminated
 */
char *str_escape_csv(char *pBuffer, int BufferSize, const char *pString);

bool str_isalpha(char c);
bool str_isalphanumeric(char c);

#define STR_ALLOW_LOWERALPHA "abcdefghijklmnopqrstuvwxyz"
#define STR_ALLOW_ALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define STR_ALLOW_NUMERIC "0123456789"
#define STR_ALLOW_ALPHANUMERIC STR_ALLOW_ALPHA STR_ALLOW_NUMERIC
#define STR_ALLOW_LOWERALPHANUMERIC STR_ALLOW_LOWERALPHA STR_ALLOW_NUMERIC

/**
 * Check if a given string only contains characters from a given list
 *
 * @param pAllowedCharacters string with all the allowed characters for example the constant STR_ALLOW_LOWERALPHANUMERIC
 * @param pTestedString string to be checked if it contains only allowed characters
 *
 * @return `true` if the string is valid and only contains allowed characters
 */
bool str_contains_only_allowed_chars(const char *pAllowedCharacters, const char *pTestedString);

#endif
