#include <gtest/gtest.h>
#include <insta/server/strhelpers.h>

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

TEST(InstaStrhelpers, Utf8ToSkeletonStr)
{
	char aBuf[512];
	bool Ok;

	Ok = str_utf8_to_skeleton_str("foo", aBuf, sizeof(aBuf));
	EXPECT_EQ(Ok, true);
	EXPECT_STREQ(aBuf, "foo");
	Ok = str_utf8_to_skeleton_str("foo bar", aBuf, 10);
	EXPECT_EQ(Ok, false);
	EXPECT_STREQ(aBuf, "foo");
	Ok = str_utf8_to_skeleton_str(
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		aBuf,
		sizeof(aBuf));
	EXPECT_EQ(Ok, false);
	EXPECT_STREQ(aBuf, "(internal buffer too small)");

	// uppercase i vs lowercase L
	str_utf8_to_skeleton_str("ChillerDragon", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "ChiiierDragon");
	str_utf8_to_skeleton_str("ChiIIerDragon", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "ChiiierDragon");

	// edge cases
	str_utf8_to_skeleton_str("💩", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "💩");
	str_utf8_to_skeleton_str("♿♿♿", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "♿♿♿");
	str_utf8_to_skeleton_str("", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "");
	str_utf8_to_skeleton_str(" ", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "");
	str_utf8_to_skeleton_str("        foo -  bar   ", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "foo-bar");
	str_utf8_to_skeleton_str("        foo    bar   ", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "foobar");
	str_utf8_to_skeleton_str("        foo bar   ", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "foobar");
	str_utf8_to_skeleton_str("²2", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "²2");
	str_utf8_to_skeleton_str("𝕗𝕠𝕠𝕓𝕒𝕣", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "foobar");

	// m gets translated into two characters
	str_utf8_to_skeleton_str("mango", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "rnango");

	// cyrillic a
	str_utf8_to_skeleton_str("pаypal", aBuf, sizeof(aBuf));
	EXPECT_STREQ(aBuf, "paypai");
}
