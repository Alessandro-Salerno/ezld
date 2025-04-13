#include <ezld/linker.h>
#include <ezld/runtime.h>
#include <stdio.h>

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    const char *argv[] = {"FUZZEZLD", NULL};
    ezld_runtime_init(1, argv);

    FILE *fuzz = fopen("./bin/fuzzinfo", "wb");
    if (NULL == fuzz) {
        printf("could not fuzz\n");
        return 0;
    }
    fwrite(Data, 1, Size, fuzz);
    fclose(fuzz);

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

    *ezld_array_push(cfg.o_files) = "./bin/fuzzinfo";
    ezld_link(cfg);
    ezld_array_free(cfg.o_files);
    ezld_array_free(cfg.sections);
    return 0;
}
