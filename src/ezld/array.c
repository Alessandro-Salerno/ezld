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

#include <ezld/array.h>
#include <ezld/runtime.h>

size_t ezld_array_grow(void **buf, size_t *len, size_t *cap, size_t elemsz) {
    if (0 == *cap) {
        *cap = 8;
        *buf = ezld_runtime_alloc(elemsz, *cap);
    } else if (*len == *cap) {
        *cap *= 2;
        *buf = ezld_runtime_realloc(*buf, elemsz * *cap);
    }
    (*len)++;
    return (*len) - 1;
}
