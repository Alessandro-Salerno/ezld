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

#include <ezld/array.h>
#include <ezld/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <tarman/cli-lookup.h>
#include <tarman/cli-parser.h>

void cli_parse(int            argc,
               const char    *argv[],
               ezld_config_t *cli_info,
               cli_exec_t    *handler) {
    if (2 > argc) {
        cli_drt_desc_t d;
        cli_lkup_command("--help", &d);
        *handler = d.exec_handler;
        return;
    }

    // Descriptor of the command
    cli_drt_desc_t cmd_desc;
    int            base = 1;

    // If no matching command is found, an
    // error is thrown
    if (cli_lkup_command(argv[1], &cmd_desc)) {
        *handler = cmd_desc.exec_handler;
        base++;
    }

    for (int i = base; i < argc; i++) {
        const char *argument = argv[i];
        const char *next     = NULL;
        if (argc - 1 != i) {
            next = argv[i + 1];
        }

        cli_drt_desc_t opt_desc;

        // If no mathcing option was found
        // This argument is treated as an input file
        if (!cli_lkup_option(argument, &opt_desc)) {
            if ('-' == argument[0]) {
                ezld_runtime_exit(
                    EZLD_ECODE_BADPARAM,
                    "unrecognized option '%s'. Try '%s help' for help",
                    argument,
                    argv[0]);
            }

            *ezld_array_push(cli_info->o_files) = argument;
            continue;
        }

        // Skip next CLI argument if the option required an arguments
        // of its own
        if (opt_desc.has_argument) {
            i++;
        }

        // Apply options
        if (NULL != opt_desc.handler) {
            opt_desc.handler(cli_info, next);
        }
    }
}
