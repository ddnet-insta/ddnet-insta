#include <gtest/gtest.h>

#include <base/system.h>
#include <game/server/instagib/strhelpers.h>

TEST(InstaStrhelpers, AllowedChars)
{
	EXPECT_EQ(str_contains_only_allowed_chars("abc", "abcabc"), true);
	EXPECT_EQ(str_contains_only_allowed_chars("abc", "abc"), true);

	// weird one
	EXPECT_EQ(str_contains_only_allowed_chars("aaaaaa", "aaa"), true);
	EXPECT_EQ(str_contains_only_allowed_chars("", ""), true);

	EXPECT_EQ(str_contains_only_allowed_chars("", "abc"), false);
	EXPECT_EQ(str_contains_only_allowed_chars("123", "abc"), false);
	EXPECT_EQ(str_contains_only_allowed_chars("12", "123"), false);

	EXPECT_EQ(str_contains_only_allowed_chars(STR_ALLOW_NUMERIC, "123"), true);
	EXPECT_EQ(str_contains_only_allowed_chars(STR_ALLOW_NUMERIC, "123a"), false);
	EXPECT_EQ(str_contains_only_allowed_chars(STR_ALLOW_LOWERALPHANUMERIC, "123a"), true);
	EXPECT_EQ(str_contains_only_allowed_chars(STR_ALLOW_LOWERALPHA, "123a"), false);
	EXPECT_EQ(str_contains_only_allowed_chars(STR_ALLOW_LOWERALPHA, "abc"), true);
}
