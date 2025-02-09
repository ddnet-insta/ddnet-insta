#ifndef GAME_SERVER_INSTAGIB_PASSWORD_HASH_H
#define GAME_SERVER_INSTAGIB_PASSWORD_HASH_H

#include <cstddef>

#define MAX_SALT_LENGTH 16
#define MAX_HASH_WITH_SALT_LENGTH MAX_SALT_LENGTH + 4 + SHA256_MAXSTRSIZE

// returns false on error
// generates the string that will be saved in the database
// it has the following format
// the salt can only be [a-z0-9]
//
// "salt$sha256password"
void pass_gen_hash_with_salt(const char *pSalt, const char *pPlaintextPassword, char *pOut, size_t OutSize);

// returns true if the plaintext password matches the hash
// that is stored in the database in the following format
// the salt can only be [a-z0-9]
//
// "salt$sha256password"
bool pass_match(const char *pPlaintextPassword, const char *pHashWithSalt);

// extracts salt from pHashWithSalt into pSalt
// extract only the salt from a full hashed string as it is stored
// in the db in the following format
//
// "salt$sha256password"
void pass_get_salt(const char *pHashWithSalt, char *pSalt, size_t SaltSize);

// writes a random salt into pSalt
void pass_gen_salt(char *pSalt, size_t SaltSize);

// internals
char _pass_rand_salt_letter();
char _pass_rand_salt_number();
char _pass_rand_salt_char();

#endif
