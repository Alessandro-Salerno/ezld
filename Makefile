# MIT License
#
# Copyright (c) 2025 Alessandro Salerno
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

CC?=gcc
CFLAGS=-Wall -Wpedantic -Wextra -std=gnu11 -Iinclude/ -DEXT_EZLD_BUILD="\"$(shell date +%y.%m.%d)\""
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
