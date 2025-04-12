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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <tarman/cli-lookup.h>

static cli_drt_desc_t commands[] = {
    {"-h", "--help", NULL, false, cli_cmd_help, "show this menu"},
    {"-v", "--version", NULL, false, NULL, "show version information"},
};

static cli_drt_desc_t options[] = {
    {"-e",
     "--entry-sym",
     NULL,
     false,
     NULL,
     "set the entry point symbol (default: '_start')"},
    {"-s",
     "--section",
     NULL,
     true,
     NULL,
     "set the base virtual address for a given section (example: -s "
     ".text=0x4000)"},
    {"-a",
     "--align",
     NULL,
     true,
     NULL,
     "set the alignent of PT_LOAD segments (example: -a 0x1000)"}};

static bool find_desc(cli_drt_desc_t  descriptors[],
                      size_t          num_desc,
                      const char     *arg,
                      cli_drt_desc_t *dst) {
    for (size_t i = 0; i < num_desc; i++) {
        cli_drt_desc_t opt_desc = descriptors[i];

        if ((NULL == opt_desc.short_option ||
             0 != strcmp(opt_desc.short_option, arg)) &&
            (NULL == opt_desc.full_option ||
             0 != strcmp(opt_desc.full_option, arg))) {
            continue;
        }

        *dst = opt_desc;
        return true;
    }

    return false;
}

bool cli_lkup_command(const char *command, cli_drt_desc_t *dst) {
    return find_desc(
        commands, sizeof commands / sizeof(cli_drt_desc_t), command, dst);
}

bool cli_lkup_option(const char *option, cli_drt_desc_t *dst) {
    return find_desc(
        options, sizeof options / sizeof(cli_drt_desc_t), option, dst);
}

cli_lkup_table_t cli_lkup_cmdtable(void) {
    return (cli_lkup_table_t){.table = commands,
                              .num_entries =
                                  sizeof commands / sizeof(cli_drt_desc_t)};
}

cli_lkup_table_t cli_lkup_opttable(void) {
    return (cli_lkup_table_t){.table = options,
                              .num_entries =
                                  sizeof options / sizeof(cli_drt_desc_t)};
}
