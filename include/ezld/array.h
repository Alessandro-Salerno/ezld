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

#include <stddef.h>
#include <stdlib.h>

#define ezld_array(type) \
    struct {             \
        type  *buf;      \
        size_t len;      \
        size_t cap;      \
    }

#define ezld_array_new()     {.buf = NULL, .len = 0, .cap = 0}
#define ezld_array_init(arr) arr.buf = NULL, arr.len = 0, arr.cap = 0
#define ezld_array_alloc(arr, size) \
    (arr).cap = (size), (arr).buf = malloc(sizeof(*(arr).buf) * size)
#define ezld_array_push(arr)                                               \
    (ezld_array_grow(                                                      \
         (void **)&(arr).buf, &(arr).len, &(arr).cap, sizeof(*(arr).buf)), \
     &((arr).buf[(arr).len - 1]))
#define ezld_array_free(arr) \
    free((arr).buf), (arr).buf = NULL, (arr).len = 0, (arr).cap = 0
#define ezld_container_push(cont) ezld_array_push((cont).arr)
#define ezld_array_first(arr)     ((arr).buf[0])
#define ezld_array_last(arr)      ((arr).buf[(arr).len - 1])
#define ezld_array_is_empty(arr)  (0 == (arr).len)

size_t ezld_array_grow(void **buf, size_t *len, size_t *cap, size_t elemsz);
