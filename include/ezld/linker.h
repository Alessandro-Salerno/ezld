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
    const char *sc_name;
    size_t      sc_vaddr;
} ezld_sec_cfg_t;

typedef struct ezld_config {
    ezld_array(ezld_sec_cfg_t) cfg_sections;
    ezld_array(const char *) cfg_objpaths;
    size_t      cfg_segalign;
    const char *cfg_entrysym;
    const char *cfg_outpath;
} ezld_config_t;

void ezld_link(ezld_config_t params);
