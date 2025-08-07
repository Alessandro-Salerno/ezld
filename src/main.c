// MIT License
//
// Copyright (c) 2025 Alessandro Salerno
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
