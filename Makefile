#  usertalk - a chat room application
#  Copyright (C) 2022 Davi Antônio da Silva Santos <antoniossdavi at gmail.com>
#  This file is part of usertalk.

#  Usertalk is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.

#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.

#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

SRC_DIR = src
OBJ_DIR = obj

MODULE := $(SRC_DIR)/server.c $(SRC_DIR)/client.c
EXE := $(notdir $(MODULE:%.c=%))

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CC = gcc

CPPFLAGS = -Iinclude

DEBUG := -g3
OPTIMIZATION := -O2
WARNINGS := -Wall -Wextra -pedantic -D_FORTIFY_SOURCE=1 -Wformat-overflow=2 \
-Wformat-security -Wformat-truncation=2 -fanalyzer
OTHER := $(OPTIMIZATION) $(WARNINGS)
CFLAGS := $(DEBUG) $(OTHER)

LDFLAGS =
LDLIBS =

HEADERS = $(wildcard include/*.h)

all : $(EXE)

$(EXE) : $(filter-out $(OBJ_DIR)/$@.c, $(OBJ)) $(HEADERS)
	@echo "Building target: $@"
	@echo "Invoking: GCC C Linker"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ "$(SRC_DIR)/$@.c" $(FOBJ) $(LDLIBS)
	@echo "Finished building target: $@"
	@echo " "

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@echo "Building file: $<"
	@echo "Invoking GCC C Compiler"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -c -o"$@" "$<"
	@echo "Finished building> $<"
	@echo " "

.PHONY: clean

clean:
	$(RM) $(EXE) $(OBJ)
