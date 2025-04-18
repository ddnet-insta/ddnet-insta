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

#endif
