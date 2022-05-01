#ifndef SETTINGS_H
#define SETTINGS_H

#define STR(x) #x
#define XSTR(x) STR(x)

// messages
#define _MSG_LEN 200
#define MSG_LEN (_MSG_LEN)
#define MSG_LEN_STR XSTR(_MSG_LEN)

#define _MAX_MSG_LEN 199
#define MAX_MSG_LEN (_MAX_MSG_LEN)
#define MAX_MSG_LEN_STR XSTR(_MAX_MSG_LEN)

// nicknames
#define _NICK_LEN 20
#define NICK_LEN (_NICK_LEN)
#define NICK_LEN_STR XSTR(_NICK_LEN)

#define _MAX_NICK_LEN 19
#define MAX_NICK_LEN (_MAX_NICK_LEN)
#define MAX_NICK_LEN_STR XSTR(_MAX_NICK_LEN)

#endif
