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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tarman/cli-lookup.h>

#define BASE_LINE_LEN        4
#define OPT_SEPARATOR_LEN    2
#define COLUMN_SEPARATOR_LEN 4

static void cli_out_spaces(size_t spaces) {
    for (size_t i = 0; i < spaces; i++) {
        putchar(' ');
    }
}

static void print_indent(void) {
    cli_out_spaces(BASE_LINE_LEN);
}

static size_t find_line_len(cli_drt_desc_t desc) {
    size_t cmd_len = BASE_LINE_LEN + COLUMN_SEPARATOR_LEN;

    if (NULL != desc.short_option) {
        cmd_len += strlen(desc.short_option);
    }

    if (NULL != desc.full_option) {
        cmd_len += strlen(desc.full_option);
    }

    return cmd_len;
}

static size_t find_max_line_len(cli_lkup_table_t table) {
    size_t max_cmd_len = BASE_LINE_LEN + COLUMN_SEPARATOR_LEN;
    for (size_t i = 0; i < table.num_entries; i++) {
        cli_drt_desc_t desc    = table.table[i];
        size_t         cmd_len = find_line_len(desc);

        if (cmd_len > max_cmd_len) {
            max_cmd_len = cmd_len;
        }
    }

    return max_cmd_len;
}

static void print_help_line(cli_drt_desc_t desc, size_t max_line_len) {
    size_t line_len = find_line_len(desc);
    size_t rem      = max_line_len - line_len;

    print_indent();

    if (NULL != desc.short_option && NULL != desc.full_option) {
        printf("%s, %s", desc.short_option, desc.full_option);
    } else if (NULL != desc.short_option) {
        printf("%s", desc.short_option);
    } else if (NULL != desc.full_option) {
        printf("%s", desc.full_option);
    }

    cli_out_spaces(COLUMN_SEPARATOR_LEN);
    cli_out_spaces(rem);
    puts(desc.description);
}

static void print_help_list(const char *title, cli_lkup_table_t table) {
    printf("%s:\n", title);

    size_t max_cmd_len = find_max_line_len(table);

    for (size_t i = 0; i < table.num_entries; i++) {
        cli_drt_desc_t desc = table.table[i];
        print_help_line(desc, max_cmd_len);
    }

    puts("");
}

void cli_cmd_help(ezld_config_t info) {
    (void)info;

    cli_lkup_table_t cmd_table = cli_lkup_cmdtable();
    cli_lkup_table_t opt_table = cli_lkup_opttable();

    puts("ezld by Alessandro Salerno");
    puts("The EZ ELF Linker");

    puts("");
    print_help_list("COMMANDS", cmd_table);
    print_help_list("OPTIONS", opt_table);
}
