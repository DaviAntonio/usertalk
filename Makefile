SRC_DIR = src
OBJ_DIR = obj

MODULE := $(SRC_DIR)/server.c $(SRC_DIR)/client.c
EXE := $(notdir $(MODULE:%.c=%))

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CC = gcc

CPPFLAGS = -Iinclude

DEBUG := -g3
OPTIMIZATION := -flto -O2
WARNINGS := -Wall -Wextra -pedantic -D_FORTIFY_SOURCE=1 -Wformat-overflow=2 \
-Wformat-security -Wformat-truncation=2 -fanalyzer
OTHER := $(OPTIMIZATION) $(WARNINGS)
CFLAGS := $(DEBUG) $(OTHER)

LDFLAGS =
LDLIBS =

all : $(EXE)

$(EXE) : $(filter-out $(OBJ_DIR)/$@.c, $(OBJ))
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
	$(RM) $(EXE) $(OBJ) $(MAPFILE)
