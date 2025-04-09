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
#include <stdint.h>
#include <stdio.h>

typedef struct ezld_merged_sec ezld_merged_sec_t;
typedef struct ezld_obj        ezld_obj_t;

typedef struct ezld_obj_sec {
    ezld_obj_t        *o_file;
    Elf32_Shdr         shdr;
    size_t             sec_elems;
    size_t             transl_off;
    ezld_merged_sec_t *merged_sec;
    size_t             merged_idx;
    uint8_t           *buf;
} ezld_obj_sec_t;

typedef struct ezld_merged_sec {
    size_t name_idx;
    size_t index;
    size_t virt_addr;
    ezld_array(ezld_obj_sec_t *) sub;
} ezld_merged_sec_t;

typedef struct ezld_obj_sym_t {
    Elf32_Sym sym;
    size_t    glob_idx;
} ezld_obj_sym_t;

typedef struct ezld_obj_symtab {
    ezld_obj_sec_t *sec;
    ezld_array(ezld_obj_sym_t) syms;
} ezld_obj_symtab_t;

typedef struct ezld_obj {
    const char *path;
    FILE       *file;
    size_t      index;
    ezld_array(ezld_obj_sec_t) sections;
    ezld_obj_symtab_t symtab;
    Elf32_Ehdr        ehdr;
} ezld_obj_t;

typedef struct ezld_global_str {
    const char *str;
    size_t      len;
} ezld_global_str_t;

typedef struct ezld_sec_cfg {
    const char *name;
    size_t      virt_addr;
} ezld_sec_cfg_t;

typedef struct ezld_config {
    ezld_array(ezld_sec_cfg_t) sections;
} ezld_config_t;

typedef struct ezld_instance {
    ezld_array(ezld_merged_sec_t) mrg_sec;
    ezld_array(ezld_obj_t) o_files;
    ezld_array(Elf32_Sym) glob_symtab;
    ezld_array(ezld_global_str_t) glob_strtab;
    ezld_array(ezld_global_str_t) sh_strtab;
    ezld_config_t   config;
    ezld_obj_sym_t *entry_sym;
    const char     *entry_label;
} ezld_instance_t;

void ezld_link(ezld_instance_t *instance, FILE *output_file);
