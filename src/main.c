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

#ifndef EXT_EZLD_FUZZER
int main(int argc, const char *argv[]) {
    ezld_runtime_init(argc, argv);
    ezld_config_t cfg = {0};

    cfg.entry_label = "_start";
    cfg.out_path    = "a.out";
    cfg.seg_align   = 0x1000;
    ezld_array_init(cfg.o_files);
    ezld_array_init(cfg.sections);
    *ezld_array_push(cfg.sections) =
        (ezld_sec_cfg_t){.name = ".text", .virt_addr = 0x00400000};
    *ezld_array_push(cfg.sections) =
        (ezld_sec_cfg_t){.name = ".data", .virt_addr = 0x10000000};

    cli_exec_t command = ezld_link;
    cli_parse(argc, argv, &cfg, &command);

    command(cfg);
    ezld_array_free(cfg.o_files);
    ezld_array_free(cfg.sections);
    return EXIT_SUCCESS;
}
#endif
