#include <gtest/gtest.h>

#include <base/log.h>
#include <base/system.h>
#include <game/server/instagib/password_hash.h>
#include <game/server/instagib/strhelpers.h>

TEST(InstaPasswordHash, GenSalt)
{
	char aSalt[MAX_SALT_LENGTH];
	pass_gen_salt(aSalt, sizeof(aSalt));
	if(!str_contains_only_allowed_chars(STR_ALLOW_LOWERALPHANUMERIC, aSalt))
	{
		log_error("test", "invalid characters in salt '%s'", aSalt);
		EXPECT_EQ(true, false);
	}
}

TEST(InstaPasswordHash, GetSalt)
{
	char aSalt[MAX_SALT_LENGTH];
	pass_get_salt("an7mwl3z2y7128j$06e9682522b505e42da44abec31499348cf9c1ce08775eeca6c3f99ed6746147", aSalt, sizeof(aSalt));
	EXPECT_STREQ(aSalt, "an7mwl3z2y7128j");
}

TEST(InstaPasswordHash, GenSaltyHash)
{
	const char *pPassword = "foo";
	const char *pSalt = "abc123";
	char aSaltedHash[512];
	pass_gen_hash_with_salt(pSalt, pPassword, aSaltedHash, sizeof(aSaltedHash));
	EXPECT_STREQ(aSaltedHash, "abc123$b0a790f418837363ec38a0c75443665b7ff36a7ce7fc6cd024bab5df8424cbd4");
}

TEST(InstaPasswordHash, MatchPasswords)
{
	EXPECT_EQ(pass_match("foo", "abc123$b0a790f418837363ec38a0c75443665b7ff36a7ce7fc6cd024bab5df8424cbd4"), true);
	EXPECT_EQ(pass_match("fooo", "abc123$b0a790f418837363ec38a0c75443665b7ff36a7ce7fc6cd024bab5df8424cbd4"), false);
	EXPECT_EQ(pass_match("foo", "abc666$b0a790f418837363ec38a0c75443665b7ff36a7ce7fc6cd024bab5df8424cbd4"), false);

	EXPECT_EQ(pass_match("🫣", "123$12b3b6053731986a71ed0bb9b8ceeecb0e9456f1bd4fdb4f5efecdee74bb6ed0"), true);
	EXPECT_EQ(pass_match("🫣x", "123$12b3b6053731986a71ed0bb9b8ceeecb0e9456f1bd4fdb4f5efecdee74bb6ed0"), false);
}
