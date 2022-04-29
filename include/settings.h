#ifndef SETTINGS_H
#define SETTINGS_H

#define STR(x) #x
#define XSTR(x) STR(x)

#define _MSG_LEN 100
#define MSG_LEN (_MSG_LEN)
#define MSG_LEN_STR XSTR(_MSG_LEN)

#define _MAX_MSG_LEN 99
#define MAX_MSG_LEN (_MAX_MSG_LEN)
#define MAX_MSG_LEN_STR XSTR(_MAX_MSG_LEN)

#endif
