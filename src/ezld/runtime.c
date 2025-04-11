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

void ezld_runtime_read_exact(void       *buf,
                             size_t      size,
                             const char *filename,
                             FILE       *file) {
    if (size != fread(buf, 1, size, file)) {
        ezld_runtime_exit(EZLD_ECODE_BADFILE,
                          "could not read %zu byte(s) from file '%s'",
                          size,
                          (filename) ? filename : "");
    }
}

void ezld_runtime_seek(size_t off, const char *filename, FILE *file) {
    if (0 != fseek(file, off, SEEK_SET)) {
        ezld_runtime_exit(EZLD_ECODE_BADFILE,
                          "offset %zu exceeds size of file '%s'",
                          off,
                          (filename) ? filename : "");
    }
}

void ezld_runtime_read_exact_at(void       *buf,
                                size_t      size,
                                size_t      off,
                                const char *filename,
                                FILE       *file) {
    long start = ftell(file);
    ezld_runtime_seek(off, filename, file);
    ezld_runtime_read_exact(buf, size, filename, file);
    (void)fseek(file, start, SEEK_SET);
}

void *ezld_runtime_alloc(size_t elemsz, size_t numelems) {
    void *buf = malloc(elemsz * numelems);

    if (NULL == buf) {
        ezld_runtime_exit(EZLD_ECODE_NOMEM, "out of memory");
    }

    return buf;
}

void ezld_runtime_write_exact(void       *buf,
                              size_t      size,
                              const char *filename,
                              FILE       *file) {
    if (size != fwrite(buf, 1, size, file)) {
        ezld_runtime_exit(EZLD_ECODE_BADFILE,
                          "could not write %zu byte(s) to file '%s'",
                          size,
                          (filename) ? filename : "");
    }
}

void ezld_runtime_write_exact_at(void       *buf,
                                 size_t      size,
                                 size_t      off,
                                 const char *filename,
                                 FILE       *file) {
    long start = ftell(file);
    ezld_runtime_seek(off, filename, file);
    ezld_runtime_write_exact(buf, size, filename, file);
    (void)fseek(file, start, SEEK_SET);
}

void ezld_runtime_seek_end(const char *filename, FILE *file) {
    if (0 != fseek(file, 0, SEEK_END)) {
        ezld_runtime_exit(
            EZLD_ECODE_BADFILE, "unable to find end of '%s'", filename);
    }
}

void *ezld_runtime_realloc(void *buf, size_t size) {
    void *new_buf = realloc(buf, size);

    if (NULL == new_buf) {
        ezld_runtime_exit(EZLD_ECODE_NOMEM, "out of memory");
    }

    return new_buf;
}

bool ezld_runtime_is_big_endian(void) {
    int i = 1;
    return !*((char *)&i);
}
