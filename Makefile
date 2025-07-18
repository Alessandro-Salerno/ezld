# ezld
# Copyright (C) 2025 Alessandro Salerno
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

CC?=gcc
CFLAGS=-Wall -Wpedantic -Wextra -std=gnu11 -Iinclude/ -DEXT_EZLD_BUILD="\"$(shell date +%y.%m.%d)\"" -lm
LDFLAGS=

DEBUG_CFLAGS=-O0 -fsanitize=undefined -fsanitize=address -g
DEBUG_LDFLAGS=
RELEASE_CLFAGS=-O2 -Werror
RELEASE_LDFLAGS=-flto
FUZZER_CFLAGS=$(DEBUG_CFLAGS) -fsanitize=fuzzer -DEXT_EZLD_NOMAIN
LIBRARY_CFLAGS=$(RELEASE_CFLAGS) -DEXT_EZLD_NOMAIN

CUSTOM_CFLAGS?=
CUSTOM_LDFLAGS?=
CUSTOM_SRC?=

CFLAGS+=$(CUSTOM_CFLAGS)
LDFLAGS+=$(CUSTOM_LDFLAGS)

BIN=bin
EXEC?=$(BIN)/ezld
LIB=$(BIN)/libezld.a

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard ,$d, $2) $(filter $(subst *, %, $2),$d))
SRC=$(call rwildcard, src, *.c)
SRC+=$(CUSTOM_SRC)

OBJ=$(patsubst src/%.c,obj/%.o, $(SRC))

debug:
	@echo =========== COMPILING IN DEBUG MODE ===========
	@make ezld CUSTOM_CFLAGS="$(DEBUG_CFLAGS)" "CUSTOM_LDFLAGS=$(DEBUG_LDFLAGS)"

release:
	@echo =========== COMPILING IN RELEASE MODE ===========
	@make CUSTOM_CFLAGS=$(RELEASE_CLFAGS) CUSTOM_LDFLAGS=$(RELEASE_LDFLAGS) ezld

library:
	@echo =========== COMPILING AS A LIBRARY ===========
	@make CUSTOM_CFLAGS=$(RELEASE_CLFAGS) CUSTOM_LDFLAGS=$(RELEASE_LDFLAGS) libezld

compile: dirs $(OBJ)

ezld: compile $(EXEC)
	@echo
	@echo All done!

libezld: compile $(LIB)
	@echo
	@echo All done!

$(EXEC): obj $(OBJ)
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJ) -o $(EXEC)
	@echo

$(LIB): obj $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@
	@echo

force: ;

fuzz:
	@make CUSTOM_SRC=libfuzzer.c EXEC=$(BIN)/fuzz CC=clang CUSTOM_CFLAGS="$(FUZZER_CFLAGS)" CUSTOM_LDFLAGS=$(DEBUG_LDFLAGS) ezld && ./bin/fuzz -ignore_crashes=1 ./fuzztest

dirs:
	@mkdir -p obj/
	@mkdir -p $(BIN)

clean:
	rm -rf obj/; \
	rm -rf $(BIN); \
	rm -rf lib/
