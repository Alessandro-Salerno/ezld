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
#include <ezld/linker.h>
#include <ezld/runtime.h>
#include <tarman/cli-parser.h>

#ifndef EXT_EZLD_NOMAIN
int main(int argc, const char *argv[]) {
    ezld_runtime_init(argc, argv);
    ezld_config_t cfg = {0};

    cfg.cfg_entrysym = "_start";
    cfg.cfg_outpath  = "a.out";
    cfg.cfg_segalign = 0x1000;
    ezld_array_init(cfg.cfg_objpaths);
    ezld_array_init(cfg.cfg_sections);
    *ezld_array_push(cfg.cfg_sections) =
        (ezld_sec_cfg_t){.sc_name = ".text", .sc_vaddr = 0x00400000};
    *ezld_array_push(cfg.cfg_sections) =
        (ezld_sec_cfg_t){.sc_name = ".data", .sc_vaddr = 0x10000000};

    cli_exec_t command = ezld_link;
    cli_parse(argc, argv, &cfg, &command);

    command(cfg);

    ezld_array_free(cfg.cfg_objpaths);
    ezld_array_free(cfg.cfg_sections);
    return EXIT_SUCCESS;
}
#endif
