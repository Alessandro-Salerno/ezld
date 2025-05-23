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

typedef struct ezld_sec_cfg {
    const char *name;
    size_t      virt_addr;
} ezld_sec_cfg_t;

typedef struct ezld_config {
    ezld_array(ezld_sec_cfg_t) sections;
    ezld_array(const char *) o_files;
    size_t      seg_align;
    const char *entry_label;
    const char *out_path;
} ezld_config_t;

void ezld_link(ezld_config_t params);
