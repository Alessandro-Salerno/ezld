// MIT License
//
// Copyright (c) 2025 Alessandro Salerno
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
#ifndef EXT_EZLD_FUZZER
    exit(code);
#else
    exit(0);
#endif
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
