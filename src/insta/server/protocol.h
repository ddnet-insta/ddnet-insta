#ifndef INSTA_SERVER_PROTOCOL_H
#define INSTA_SERVER_PROTOCOL_H

enum
{
	// https://github.com/ddnet/ddnet/pull/8892/commits/74bb32779921c5efa38187d7b1760932c02763d5
	VERSION_DDNET_NO_SCOREBOARD_DURING_PAUSE = 18051,

	// https://github.com/ddnet/ddnet/pull/10098/commits/165712dd86b328531a23b1b5d2ae2d454d0066a6
	VERSION_DDNET_PICKUPFLAG_NO_PREDICT = 19040,
};

#endif
