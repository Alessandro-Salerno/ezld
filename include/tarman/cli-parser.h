/*************************************************************************
| tarman                                                                 |
| Copyright (C) 2024 - 2025 Alessandro Salerno                           |
|                                                                        |
| This program is free software: you can redistribute it and/or modify   |
| it under the terms of the GNU General Public License as published by   |
| the Free Software Foundation, either version 3 of the License, or      |
| (at your option) any later version.                                    |
|                                                                        |
| This program is distributed in the hope that it will be useful,        |
| but WITHOUT ANY WARRANTY; without even the implied warranty of         |
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          |
| GNU General Public License for more details.                           |
|                                                                        |
| You should have received a copy of the GNU General Public License      |
| along with this program.  If not, see <https://www.gnu.org/licenses/>. |
*************************************************************************/

#pragma once

#include <ezld/linker.h>
#include <stdbool.h>

typedef void (*cli_fcn_t)(ezld_config_t *info, const char *next);
typedef void (*cli_exec_t)(ezld_config_t info);

typedef struct {
    const char *short_option;
    const char *full_option;
    cli_fcn_t   handler;
    bool        has_argument;
    cli_exec_t  exec_handler;
    const char *description;
} cli_drt_desc_t;

void cli_parse(int            argc,
               const char    *argv[],
               ezld_config_t *cli_info,
               cli_exec_t    *handler);
