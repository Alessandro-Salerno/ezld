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

#pragma once

#include <stdbool.h>
#include <stdio.h>

#define EZLD_ECODE_NOPARAM  -1
#define EZLD_ECODE_NOFILE   -2
#define EZLD_ECODE_BADFILE  -3
#define EZLD_ECODE_NOMEM    -4
#define EZLD_ECODE_BADSEC   -5
#define EZLD_ECODE_BADPARAM -6

#define EZLD_EMSG_WARN "warning"
#define EZLD_EMSG_ERR  "error"
#define EZLD_EMSG_INFO "info"

#if defined(_MSC_VER)
#define EZLD_IS_BIG_ENDIAN() false
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define EZLD_IS_BIG_ENDIAN() false
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define EZLD_IS_BIG_ENDIAN() true
#else
#warning unable to determine byte order at compile time, falling back to runtime check
#define EZLD_IS_BIG_ENDIAN() ezld_runtime_is_big_endian()
#endif

void ezld_runtime_init(int argc, const char *argv[]);
void ezld_runtime_message(const char *type, const char *fmt, ...);
__attribute__((noreturn)) void
      ezld_runtime_exit(int code, const char *msgfmt, ...);
void  ezld_runtime_read_exact(void       *buf,
                              size_t      size,
                              const char *filename,
                              FILE       *file);
void  ezld_runtime_seek(size_t off, const char *filename, FILE *file);
void  ezld_runtime_read_exact_at(void       *buf,
                                 size_t      size,
                                 size_t      off,
                                 const char *filename,
                                 FILE       *file);
void *ezld_runtime_alloc(size_t elemsz, size_t numelems);
void  ezld_runtime_write_exact(void       *buf,
                               size_t      size,
                               const char *filename,
                               FILE       *file);
void  ezld_runtime_write_exact_at(void       *buf,
                                  size_t      size,
                                  size_t      off,
                                  const char *filename,
                                  FILE       *file);
void  ezld_runtime_seek_end(const char *filename, FILE *file);
void *ezld_runtime_realloc(void *buf, size_t size);
bool  ezld_runtime_is_big_endian(void);
