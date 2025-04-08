// ezld
// Copyright (C) 2025 Alessandro Salerno
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <ezld/runtime.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int          g_argc;
static const char **g_argv;

static void
vmsg(FILE *stream, const char *type, const char *fmt, va_list args) {
    fprintf(stream, "%s: %s: ", g_argv[0], type);
    vfprintf(stream, fmt, args);
    puts("");
}

void ezld_runtime_init(int argc, const char *argv[]) {
    g_argc = argc;
    g_argv = argv;
}

void ezld_runtime_message(const char *type, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vmsg(stdout, type, fmt, args);
    va_end(args);
}

__attribute__((noreturn)) void
ezld_runtime_exit(int code, const char *msgfmt, ...) {
    va_list args;
    va_start(args, msgfmt);
    vmsg(stderr, "fatal", msgfmt, args);
    va_end(args);
    exit(code);

    __builtin_unreachable();
}
