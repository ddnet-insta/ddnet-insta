#include <base/hash.h>
#include <base/system.h>
#include <game/server/instagib/account.h>
#include <game/server/instagib/strhelpers.h>

#include "password_hash.h"

void pass_gen_hash_with_salt(const char *pSalt, const char *pPlaintextPassword, char *pOut, size_t OutSize)
{
	dbg_assert(str_length(pSalt) > 1 && str_length(pSalt) <= MAX_SALT_LENGTH, "invalid salt length");
	dbg_assert(str_length(pPlaintextPassword) > 1 && str_length(pPlaintextPassword) <= MAX_PASSWORD_LENGTH, "invalid password length");
	dbg_assert(OutSize >= MAX_HASH_WITH_SALT_LENGTH, "output buffer too small");
	dbg_assert(str_contains_only_allowed_chars(STR_ALLOW_LOWERALPHANUMERIC, pSalt), "invalid characters in salt");

	char aSaltyPassword[MAX_SALT_LENGTH + MAX_PASSWORD_LENGTH + 8];
	str_format(aSaltyPassword, sizeof(aSaltyPassword), "%s%s", pSalt, pPlaintextPassword);
	SHA256_DIGEST Digest = sha256(aSaltyPassword, str_length(aSaltyPassword));
	char aHash[512];
	sha256_str(Digest, aHash, sizeof(aHash));
	str_format(pOut, OutSize, "%s$%s", pSalt, aHash);
}

void pass_get_salt(const char *pHashWithSalt, char *pSalt, size_t SaltSize)
{
	dbg_assert(SaltSize >= MAX_SALT_LENGTH, "buffer too small to store salt");
	bool FoundSep = false;
	int i;
	for(i = 0; pHashWithSalt[i]; i++)
	{
		dbg_assert(i < MAX_SALT_LENGTH, "salt is too long");
		if(pHashWithSalt[i] == '$')
		{
			FoundSep = true;
			break;
		}
		pSalt[i] = pHashWithSalt[i];
	}
	pSalt[i] = '\0';
	dbg_assert(FoundSep, "hash with salt is missing dollar separator");
}

bool pass_match(const char *pPlaintextPassword, const char *pHashWithSalt)
{
	char aSalt[MAX_SALT_LENGTH + 1];
	pass_get_salt(pHashWithSalt, aSalt, sizeof(aSalt));
	char aInputHashWithSalt[MAX_HASH_WITH_SALT_LENGTH];
	pass_gen_hash_with_salt(aSalt, pPlaintextPassword, aInputHashWithSalt, sizeof(aInputHashWithSalt));
	return str_comp(aInputHashWithSalt, pHashWithSalt) == 0;
}

static int random_range(int Min, int Max)
{
	return Min + (secure_rand() / (RAND_MAX / (Max - Min + 1) + 1));
}

char _pass_rand_salt_letter()
{
	return (char)random_range(97, 122);
}

char _pass_rand_salt_number()
{
	return (char)random_range(48, 56);
}

char _pass_rand_salt_char()
{
	return secure_rand() % 2 == 0 ? _pass_rand_salt_number() : _pass_rand_salt_letter();
}

void pass_gen_salt(char *pSalt, size_t SaltSize)
{
	dbg_assert(SaltSize >= MAX_SALT_LENGTH, "buffer too small to store salt");
	int i;
	for(i = 0; i < MAX_SALT_LENGTH - 1; i++)
		pSalt[i] = _pass_rand_salt_char();
	pSalt[i] = '\0';
}
