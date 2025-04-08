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

#pragma once

#include <stdio.h>

#define EZLD_ECODE_NOPARAM -1
#define EZLD_ECODE_NOFILE  -2
#define EZLD_ECODE_BADFILE -3
#define EZLD_ECODE_NOMEM   -4
#define EZLD_ECODE_BADSEC  -5

#define EZLD_EMSG_WARN "warning"
#define EZLD_EMSG_ERR  "error"
#define EZLD_EMSG_INFO "info"

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
