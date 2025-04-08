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

#include <stddef.h>
#include <stdlib.h>

#define ezld_array(type) \
    struct {             \
        type  *buf;      \
        size_t len;      \
        size_t cap;      \
        type   _dummy;   \
    }

#define ezld_array_new()     {.buf = NULL, .len = 0, .cap = 0}
#define ezld_array_init(arr) arr.buf = NULL, arr.len = 0, arr.cap = 0
#define ezld_array_alloc(arr, size) \
    (arr).cap = (size), (arr).buf = malloc(sizeof((arr)._dummy) * size)
#define ezld_array_push(arr)                                                 \
    (ezld_array_grow(                                                        \
         (void **)&(arr).buf, &(arr).len, &(arr).cap, sizeof((arr)._dummy)), \
     &((arr).buf[(arr).len - 1]))
#define ezld_array_free(arr) \
    free((arr).buf), (arr).buf = NULL, (arr).len = 0, (arr).cap = 0
#define ezld_container_push(cont) ezld_array_push((cont).arr)

size_t ezld_array_grow(void **buf, size_t *len, size_t *cap, size_t elemsz);
