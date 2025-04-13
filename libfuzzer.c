#include <ezld/linker.h>
#include <ezld/runtime.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

static int createMemfdWithData(const uint8_t *Data, size_t Size) {
    int fd = syscall(SYS_memfd_create, "fuzz_input", 0);
    if (fd < 0)
        return -1;

    write(fd, Data, Size);
    lseek(fd, 0, SEEK_SET); // Rewind

    return fd;
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    const char *argv[] = {"FUZZEZLD", NULL};
    ezld_runtime_init(1, argv);

    int fd = createMemfdWithData(Data, Size);
    if (fd < 0) {
        return 0;
    }

    char path[64];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

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

    *ezld_array_push(cfg.o_files) = path;
    ezld_link(cfg);
    ezld_array_free(cfg.o_files);
    ezld_array_free(cfg.sections);
    return 0;
}
