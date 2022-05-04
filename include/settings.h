/*
 *  usertalk - a chat room application
 *  Copyright (C) 2022 Davi Antônio da Silva Santos <antoniossdavi at gmail.com>
 *  This file is part of usertalk.

 *  Usertalk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.

 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#define STR(x) #x
#define XSTR(x) STR(x)

// messages
#define _MSG_LEN 400
#define MSG_LEN (_MSG_LEN)
#define MSG_LEN_STR XSTR(_MSG_LEN)

#define _MAX_MSG_LEN 399
#define MAX_MSG_LEN (_MAX_MSG_LEN)
#define MAX_MSG_LEN_STR XSTR(_MAX_MSG_LEN)

// nicknames
#define _NICK_LEN 20
#define NICK_LEN (_NICK_LEN)
#define NICK_LEN_STR XSTR(_NICK_LEN)

#define _MAX_NICK_LEN 19
#define MAX_NICK_LEN (_MAX_NICK_LEN)
#define MAX_NICK_LEN_STR XSTR(_MAX_NICK_LEN)

// room
#define _ROOM_NAME_LEN 20
#define ROOM_NAME_LEN (_ROOM_NAME_LEN)
#define ROOM_NAME_LEN_STR XSTR(_ROOM_NAME_LEN)

#define _MAX_ROOM_NAME_LEN 19
#define MAX_ROOM_NAME_LEN (_MAX_ROOM_NAME_LEN)
#define MAX_ROOM_NAME_LEN_STR XSTR(_MAX_ROOM_NAME_LEN)

// maximum number of epoll events
#define MAX_EVENTS (100)

#endif
