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

#include <ezld/array.h>
#include <musl/elf.h>
#include <stddef.h>
#include <stdio.h>

typedef struct ezld_merged_sec ezld_merged_sec_t;

typedef struct ezld_obj_sec {
    size_t             file_idx;
    Elf32_Shdr         shdr;
    size_t             sec_elems;
    size_t             transl_off;
    ezld_merged_sec_t *merged_sec;
} ezld_obj_sec_t;

typedef struct ezld_merged_sec {
    const char *name;
    size_t      index;
    ezld_array(ezld_obj_sec_t *) obj_sections;
} ezld_merged_sec_t;

typedef struct ezld_obj {
    const char *path;
    FILE       *file;
    size_t      index;
    ezld_array(ezld_obj_sec_t) sections;
} ezld_obj_t;

typedef struct ezld_instance {
    ezld_array(ezld_merged_sec_t) merged_sections;
    ezld_array(ezld_obj_t) object_files;
} ezld_instance_t;

void ezld_link(ezld_instance_t *instance, FILE *output_file);
