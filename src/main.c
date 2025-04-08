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

#include <ezld/linker.h>
#include <ezld/runtime.h>
#include <stdio.h>

int main(int argc, const char *argv[]) {
    ezld_runtime_init(argc, argv);
    ezld_instance_t instance = {.merged_sections = ezld_array_new(),
                                .object_files    = ezld_array_new()};

    if (1 == argc) {
        ezld_runtime_exit(EZLD_ECODE_NOPARAM,
                          "insufficient commandline arguments: need to specify "
                          "files to link");
    }

    for (int i = 1; i < argc; i++) {
        FILE *file = fopen(argv[i], "rb");

        if (NULL == file) {
            ezld_runtime_exit(
                EZLD_ECODE_NOFILE, "could not open input file '%s'", argv[i]);
        }

        ezld_obj_t *obj = ezld_array_push(instance.object_files);
        obj->file       = file;
        obj->path       = argv[i];
        obj->index      = i - 1;
        ezld_array_init(obj->sections);
    }

    FILE *out = fopen("a.out", "wb");

    if (NULL == out) {
        ezld_runtime_exit(EZLD_ECODE_NOFILE, "could not open output file");
    }

    ezld_link(&instance, NULL);
    return EXIT_SUCCESS;
}
