#ifdef CONF_SSH

#include <base/str.h>

#include <engine/console.h>
#include <engine/external/unicode-width/unicode_width.h>

#include <gtest/gtest.h>
#include <insta/engine/shared/ssh_server.h>

TEST(Ssh, GetCommand)
{
	CSshClient Client(0, nullptr);

	char aCmd[IConsole::CMDLINE_LENGTH];
	char aCmdName[IConsole::CMDLINE_LENGTH];

	Client.m_InputIdx = 0;
	Client.GetCommand("kick", aCmd);
	EXPECT_STREQ(aCmd, "kick");

	Client.m_InputIdx = 3;
	Client.GetCommand("kick", aCmd);
	EXPECT_STREQ(aCmd, "kick");

	Client.m_InputIdx = 4;
	Client.GetCommand("kick   ", aCmd);
	EXPECT_STREQ(aCmd, "kick   ");

	Client.m_InputIdx = 5;
	Client.GetCommand("kick   ", aCmd);
	EXPECT_STREQ(aCmd, "kick   ");

	Client.m_InputIdx = 5;
	Client.GetCommand("kick;status", aCmd);
	EXPECT_STREQ(aCmd, "status");

	Client.m_InputIdx = 5;
	Client.GetCommand("kick   ", aCmd);
	EXPECT_STREQ(aCmd, "kick   ");
	CSshServer::StrCopyUntilSpaceOrEol(aCmdName, sizeof(aCmdName), aCmd);
	EXPECT_STREQ(aCmdName, "kick");

	Client.m_InputIdx = 2;
	Client.GetCommand("kick", aCmd);
	EXPECT_STREQ(aCmd, "kick");
	CSshServer::StrCopyUntilSpaceOrEol(aCmdName, sizeof(aCmdName), aCmd);
	EXPECT_STREQ(aCmdName, "kick");
}

TEST(Ssh, History)
{
	CSshClient Client(0, nullptr);
	EXPECT_STREQ(Client.PrevInputFromHistory(), "");
	EXPECT_STREQ(Client.NextInputFromHistory(), "");

	Client.AddToInputHistory("hello");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "hello");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "hello");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "hello");
	EXPECT_STREQ(Client.NextInputFromHistory(), "");

	Client.AddToInputHistory("world");

	EXPECT_STREQ(Client.PrevInputFromHistory(), "world");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "hello");
	EXPECT_STREQ(Client.NextInputFromHistory(), "world");
	EXPECT_STREQ(Client.NextInputFromHistory(), "");

	Client.AddToInputHistory("foo");
	Client.AddToInputHistory("bar");
	EXPECT_STREQ(Client.NextInputFromHistory(), "");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "bar");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "foo");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "world");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "hello");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "hello");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "hello");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "hello");
	EXPECT_STREQ(Client.NextInputFromHistory(), "world");
	EXPECT_STREQ(Client.NextInputFromHistory(), "foo");
	EXPECT_STREQ(Client.NextInputFromHistory(), "bar");
	EXPECT_STREQ(Client.NextInputFromHistory(), "");
	EXPECT_STREQ(Client.NextInputFromHistory(), "");
	EXPECT_STREQ(Client.NextInputFromHistory(), "");
	EXPECT_STREQ(Client.NextInputFromHistory(), "");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "bar");
}

TEST(Ssh, HistoryPrevSpam)
{
	// here we simulate a common real world
	// use case of me personally
	//
	// join ssh server then run "status" command
	// then run "dump_antibot" then press arrow key
	// up to get "dump_antibot" again and then press enter
	// to send it again. And then keep spamming "dump_antibot"
	//
	// Currently there is a bug where one arrow key up would
	// sometimes fetch "status"

	CSshClient Client(0, nullptr);
	Client.AddToInputHistory("status");
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// now that we ran "dump_antibot" a bunch of times we would like to
	// run "status" again
	// and it should work with two arrow key up presses
	// having to scroll through all duplicated "dump_antibot"s would be annoying asf

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "status");
}

TEST(Ssh, HistoryPrevSpamReverse)
{
	CSshClient Client(0, nullptr);
	Client.AddToInputHistory("status");
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");
	// enter to send fetched command
	Client.AddToInputHistory("dump_antibot");

	// run new different command
	Client.AddToInputHistory("kick");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "kick");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "dump_antibot");

	// arrow key up
	EXPECT_STREQ(Client.PrevInputFromHistory(), "status");

	// now that we are the beginning of the history go back
	// in the other direction

	// arrow key down
	EXPECT_STREQ(Client.NextInputFromHistory(), "dump_antibot");

	// arrow key down (expect to skip all the duplicated dump_antibot entries)
	EXPECT_STREQ(Client.NextInputFromHistory(), "kick");

	// arrow key down (end of history)
	EXPECT_STREQ(Client.NextInputFromHistory(), "");
}

TEST(Ssh, HistoryDuplicates)
{
	CSshClient Client(0, nullptr);
	EXPECT_STREQ(Client.PrevInputFromHistory(), "");
	EXPECT_STREQ(Client.NextInputFromHistory(), "");

	Client.AddToInputHistory("hello");
	Client.AddToInputHistory("world");
	Client.AddToInputHistory("world");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "world");
	EXPECT_STREQ(Client.PrevInputFromHistory(), "hello");
}

TEST(Ssh, LineWrap)
{
	unicode_width_state_t State;
	unicode_width_init(&State);

	char aSshLine[512];
	int Linebreaks;

	Linebreaks = CSshLogger::LineWrapForSsh("hello world", aSshLine, sizeof(aSshLine), 256, &State);
	EXPECT_EQ(Linebreaks, 1);
	EXPECT_STREQ(aSshLine, "hello world");

	Linebreaks = CSshLogger::LineWrapForSsh("hello\nworld", aSshLine, sizeof(aSshLine), 256, &State);
	EXPECT_EQ(Linebreaks, 2);
	EXPECT_STREQ(aSshLine, "hello\r\nworld");

	// this is from a real terminal with width 10
	//
	// +----------+
	// |> test_cmd|
	// |hello worl|
	// |>         |
	// +----------+
	//
	// +----------+
	// |> test_cmd|
	// |hello worl|
	// |d         |
	// |>         |
	// +----------+
	//
	// +----------+
	// |> test_cmd|
	// |hello worl|
	// |d!        |
	// |>         |
	// +----------+

	Linebreaks = CSshLogger::LineWrapForSsh("hello worl", aSshLine, sizeof(aSshLine), 10, &State);
	EXPECT_EQ(Linebreaks, 1);
	EXPECT_STREQ(aSshLine, "hello worl");

	Linebreaks = CSshLogger::LineWrapForSsh("hello world", aSshLine, sizeof(aSshLine), 10, &State);
	EXPECT_EQ(Linebreaks, 2);
	EXPECT_STREQ(aSshLine, "hello world");

	Linebreaks = CSshLogger::LineWrapForSsh("hello world!", aSshLine, sizeof(aSshLine), 10, &State);
	EXPECT_EQ(Linebreaks, 2);
	EXPECT_STREQ(aSshLine, "hello world!");

	Linebreaks = CSshLogger::LineWrapForSsh("✅✅✅✅✅✅✅✅✅✅✅✅✅✅✅", aSshLine, sizeof(aSshLine), 30, &State);
	EXPECT_EQ(Linebreaks, 1);
	EXPECT_STREQ(aSshLine, "✅✅✅✅✅✅✅✅✅✅✅✅✅✅✅");

	Linebreaks = CSshLogger::LineWrapForSsh("✅✅✅✅✅✅✅✅✅✅✅✅✅✅✅", aSshLine, sizeof(aSshLine), 29, &State);
	EXPECT_EQ(Linebreaks, 2);
	EXPECT_STREQ(aSshLine, "✅✅✅✅✅✅✅✅✅✅✅✅✅✅✅");

	Linebreaks = CSshLogger::LineWrapForSsh("abc✅✅✅", aSshLine, sizeof(aSshLine), 36, &State);
	EXPECT_EQ(Linebreaks, 1);
	EXPECT_STREQ(aSshLine, "abc✅✅✅");
}

#endif
