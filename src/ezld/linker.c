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

#include <assert.h>
#include <ezld/linker.h>
#include <ezld/runtime.h>
#include <musl/elf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define EZLD_ENTRY_NAME         0
#define EZLD_GLOB_SYM_UNDEF     0
#define EZLD_ELF_OUT_FLAG_UNSET 0

#define EZLD_MAYBE_FUTURE __attribute__((used))

typedef struct ezld_mrg_sec ezld_mrg_sec_t;
typedef struct ezld_obj     ezld_obj_t;

/**
 * @brief An object file section
 *
 * This type represent a section in an ELF object file and is used for in-memory
 * linking
 */
typedef struct ezld_obj_sec {
    /** Pointer to the object file struct to which this section belongs */
    ezld_obj_t *os_obj;
    /** Copy of the ELF section header of this section */
    Elf32_Shdr os_shdr;
    /** Number of "elements"" in this section. Equivilent to ``os_shdr.sh_size /
     * os_shdr.sh_entsize` (or `os_shdr.sh_size` if `os_shdr.sh_entsize == 0`)
     */
    size_t os_elems;
    /** The translation applied to this section in the final merged section.
     * This field is not set upon loding, but rather on merge */
    size_t os_transl;
    /** Pointer to the merged section of which this object section is a part of.
     * This field is not set upon loading, but rather on merge */
    ezld_mrg_sec_t *os_mrg;
    /** Index into `os_mrg.ms_oss` where this section is located */
    size_t os_ndx;
    /** Buffer containing all data relative to this section present in the
     * relative segment in the object file. This field is not set upon loading,
     * but rather it is set by the reader of this field if it is `NULL` */
    uint8_t *os_data;
} ezld_obj_sec_t;

/**
 * @brief An in-memory representation of a section obtained by merging sections
 * of different object files
 */
typedef struct ezld_mrg_sec {
    /** Index into the global string table where the name of this merged section
     * lies */
    size_t ms_name;
    /** Index into g_self.i_mss where this merged section lies */
    size_t ms_ndx;
    /** Virtual address associated with this merged section */
    size_t ms_vaddr;
    /** Memory size of this merged section */
    size_t ms_memsz;
    /** Offset into the final executable file where the segment relative to this
     * merged section is found. This field is 0 upon allocation, and is set in
     * `write_exec` for use by that function and by relocation functions */
    size_t ms_fileoff;
    /** Array of object file sections from which this merged section was
     * obtained */
    ezld_array(ezld_obj_sec_t *) ms_oss;
} ezld_mrg_sec_t;

/**
 * @brief A symbol contained in an object file symbol table
 */
typedef struct ezld_obj_sym_t {
    /** Data read from the symbol table entry */
    Elf32_Sym osy_esym;
    /** Index into `g_self.i_globstrtab.gst_strs` where the name of this symbol
     * is found. If equal to `EZLD_GLOB_SYM_UNDEF` (`0`), this field is to be
     * considered unset and should not be used to read from the table */
    size_t osy_globndx;
    /** Name of this symbol obtained from the object file symbol table using
     * `osy_esym.st_name` */
    const char *osy_name;
} ezld_obj_sym_t;

/**
 * @brief An in-memory representation of an object file symbol table
 */
typedef struct ezld_obj_symtab {
    /** Pointer to  the object file section struct of this symbol table */
    ezld_obj_sec_t *ost_os;
    /** Array of symbols contained in this table */
    ezld_array(ezld_obj_sym_t) ost_syms;
} ezld_obj_symtab_t;

/**
 * @brief An in-memory representation of an object file
 */
typedef struct ezld_obj {
    /** Path to the object file */
    const char *obj_filepath;
    /** Read-mode file */
    FILE *obj_file;
    /** Index into `g_self.i_objs` where this object instance is found */
    size_t obj_ndx;
    /** Object file symbol table instance relative to this object file. NOTE:
     * Currently ezld supports only one symbol table per object file for
     * simplicity, though ELF allows files to contain multiple ones, and most
     * modern compilers and assemblers make use of this feature */
    ezld_obj_symtab_t obj_ost;
    /**
     * ELF header of this object file
     */
    Elf32_Ehdr obj_ehdr;
    /** Array of object file sections contained in this object file */
    ezld_array(ezld_obj_sec_t) obj_oss;
} ezld_obj_t;

/**
 * @brief An entry into a global string table
 */
typedef struct ezld_glob_str {
    /** Buffer where the string's characters are hel */
    const char *gs_data;
    /** Length of the string */
    size_t gs_len;
    /** Offset into the string table where the buffer is found */
    size_t gs_offset;
} ezld_glob_str_t;

/**
 * @brief An in-memory string table
 */
typedef struct ezld_glob_strtab {
    ezld_array(ezld_glob_str_t) gst_strs;
} ezld_glob_strtab_t;

/**
 * @brief A description of the final output of the linker
 */
typedef struct ezld_output {
    /** Write-mode file of the final executable */
    FILE *out_file;
    /** Endianness of the output ELF */
    uint8_t out_endian;
    /** Machine architecture of the output ELF */
    uint8_t out_mach;
    /** ABI of the output ELF */
    uint8_t out_abi;
    /** ABI version of the output ELF */
    uint8_t out_abi_ver;
    /** `true` if the values above are set */
    bool out_set;
} ezld_output_t;

typedef struct ezld_instance {
    /** Array of merged sections */
    ezld_array(ezld_mrg_sec_t) i_mss;
    /** Array of object files */
    ezld_array(ezld_obj_t) i_objs;
    /** Global symbol table  where all symbols are added */
    ezld_array(Elf32_Sym) i_globsymtab;
    /** Global string table */
    ezld_glob_strtab_t i_globstrtab;
    /** Global section header string table */
    ezld_glob_strtab_t i_shstrtab;
    /** User configuration */
    ezld_config_t i_cfg;
    /** Pointer to the entry symbol. This field is not set by the user or upon
     * loading (unlike `i_cfg.cfg_entrysym`), but is set during linking when the
     * symbol specified by `i_cfg.cfg_entrysym` is found. This field is `NULL`
     * if the symbol was not found or has not been found yet */
    ezld_obj_sym_t *i_osentry;
    /** Final output */
    ezld_output_t i_out;
} ezld_instance_t;

// TODO: NOTE: When using ezld as a CLI tool, there's only one instance, so
// keeping it global does not create issues. However, multithreaded applications
// using libezld currently have to syncronize calls to `ezld_link` to avoid
// overriding oneanother. This can be solved by turning this into a parameter
// passed to all functions, or by syncronizing in ezld directly (worse)
static ezld_instance_t *g_self = NULL;

/**
 * @return `true` if the endianness of the input/output files is different from
 * that of the host machine, `false` otherwise
 */
static inline bool endian_should_swap(void) {
    return (ELFDATA2LSB == g_self->i_out.out_endian && EZLD_IS_BIG_ENDIAN()) ||
           (ELFDATA2MSB == g_self->i_out.out_endian && !EZLD_IS_BIG_ENDIAN());
}

/**
 * @brief Given a 32-bit bitmask, this function ensures returns the same bitmask
 * but flipped if the host machine is big endian
 *
 * @param mask the mask
 */
static inline uint32_t mask32(uint32_t mask) {
    if (EZLD_IS_BIG_ENDIAN()) {
        return ((mask << 24) & 0xFF000000) | ((mask << 8) & 0x00FF0000) |
               ((mask >> 8) & 0x0000FF00) | ((mask >> 24) & 0x000000FF);
    }

    return mask;
}

/**
 * @brief This function ensures endianness compatibility for a 16-bit value
 *
 * @param val the value
 *
 * @return val but expresses in the correct endianness given the endianness of
 * ELF and that of the host machine
 */
static inline uint16_t endian16(uint16_t val) {
    if (endian_should_swap()) {
        return (val << 8) | (val >> 8);
    }

    return val;
}

/**
 * @brief This function ensures endianness compatibility for a 32-bit value
 *
 * @param val the value
 *
 * @return val but expresses in the correct endianness given the endianness of
 * ELF and that of the host machine
 */
static inline uint32_t endian32(uint32_t val) {
    if (endian_should_swap()) {
        return ((val << 24) & 0xFF000000) | ((val << 8) & 0x00FF0000) |
               ((val >> 8) & 0x0000FF00) | ((val >> 24) & 0x000000FF);
    }

    return val;
}

/**
 * @brief This function ensures endianness compatibility for a 64-bit value
 *
 * @param val the value
 *
 * @return val but expresses in the correct endianness given the endianness of
 * ELF and that of the host machine
 */
EZLD_MAYBE_FUTURE static inline uint64_t endian64(uint64_t val) {
    if (endian_should_swap()) {
        return ((val << 56) & 0xFF00000000000000ULL) |
               ((val << 40) & 0x00FF000000000000ULL) |
               ((val << 24) & 0x0000FF0000000000ULL) |
               ((val << 8) & 0x000000FF00000000ULL) |
               ((val >> 8) & 0x00000000FF000000ULL) |
               ((val >> 24) & 0x0000000000FF0000ULL) |
               ((val >> 40) & 0x000000000000FF00ULL) |
               ((val >> 56) & 0x00000000000000FFULL);
    }

    return val;
}

/**
 * @brief performs sign extension of a value to 32 bits
 *
 * @param bits number of bits of the input value
 *
 * @return the sign extended value
 */
EZLD_MAYBE_FUTURE static inline int32_t sext(uint32_t x, int bits) {
    int m = 32 - bits;
    return ((int32_t)(x << m)) >> m;
}

EZLD_MAYBE_FUTURE static ezld_mrg_sec_t *find_mrg_sec(size_t name_idx) {
    for (size_t i = 0; i < g_self->i_mss.len; i++) {
        ezld_mrg_sec_t *s = &g_self->i_mss.buf[i];

        if (s->ms_name == name_idx) {
            return s;
        }
    }

    return NULL;
}

static ezld_glob_str_t shstr_from_idx(size_t shstr_idx) {
    return g_self->i_shstrtab.gst_strs.buf[shstr_idx];
}

EZLD_MAYBE_FUTURE static ezld_glob_str_t globstr_from_idx(size_t glob_idx) {
    return g_self->i_globstrtab.gst_strs.buf[glob_idx];
}

/**
 * @brief Adds a string to a global string table
 *
 * @param str the string to be added (null-terminated)
 * @param strtab the global string table to which the string will be added
 *
 * @return the index in the string table where str can be found
 */
static size_t strtab_add(const char *str, ezld_glob_strtab_t *strtab) {
    size_t len = strlen(str);

    for (size_t i = 0; i < strtab->gst_strs.len; i++) {
        ezld_glob_str_t s = strtab->gst_strs.buf[i];

        if (len == s.gs_len && 0 == strcmp(str, s.gs_data)) {
            return i;
        }
    }

    ezld_glob_str_t s = {.gs_data = str, .gs_len = strlen(str), .gs_offset = 1};
    if (!ezld_array_is_empty(strtab->gst_strs)) {
        s.gs_offset = ezld_array_last(strtab->gst_strs).gs_offset +
                      ezld_array_last(strtab->gst_strs).gs_len + 1;
    }
    *ezld_array_push(strtab->gst_strs) = s;
    return strtab->gst_strs.len - 1;
}

/**
 * @brief Adds a string to the section header string table
 *
 * @param str the string to be added (null-terminated)
 *
 * @return the index in the string table where str can be found
 */
static size_t shstr_add(const char *str) {
    return strtab_add(str, &g_self->i_shstrtab);
}

/**
 * @brief Adds a string to the global string table
 *
 * @param str the string to be added (null-terminated)
 *
 * @return the index in the string table where str can be found
 */
static size_t globstr_add(const char *str) {
    return strtab_add(str, &g_self->i_globstrtab);
}

static size_t resolve_sym(Elf32_Sym      *ret,
                          ezld_obj_sym_t *objsym,
                          size_t          glob_stridx,
                          bool            use_sym_name) {
    if (NULL != objsym && EZLD_GLOB_SYM_UNDEF != objsym->osy_globndx) {
        *ret = g_self->i_globsymtab.buf[objsym->osy_globndx - 1];
        return objsym->osy_globndx;
    }

    if (use_sym_name && NULL != objsym && NULL != objsym->osy_name) {
        glob_stridx = globstr_add(objsym->osy_name);
    }

    for (size_t i = 0; i < g_self->i_globsymtab.len; i++) {
        Elf32_Sym s = g_self->i_globsymtab.buf[i];

        if (s.st_name == glob_stridx) {
            if (NULL != objsym) {
                objsym->osy_globndx = i + 1;
            }
            *ret = s;
            return i + 1;
        }
    }

    return EZLD_GLOB_SYM_UNDEF;
}

/**
 * @brief Loads the contents of an object file section into the os_data field if
 * it is not been populated yet
 *
 * @param sec the object file section pointer
 */
static void read_section_contents(ezld_obj_sec_t *sec) {
    if (NULL == sec->os_data) {
        sec->os_data = ezld_runtime_alloc(1, sec->os_shdr.sh_size);
        ezld_runtime_read_exact_at(sec->os_data,
                                   sec->os_shdr.sh_size,
                                   sec->os_shdr.sh_offset,
                                   sec->os_obj->obj_filepath,
                                   sec->os_obj->obj_file);
    }
}

/**
 * @brief merges an object file section with similar sections in one merged
 * section to be written to the final output file
 *
 * @param objsec the object file section pointer
 * @objsec_name index into the global string table where the name of this
 * section is found
 */
static void merge_section(ezld_obj_sec_t *objsec, size_t objsec_name) {
    for (size_t i = 0; i < g_self->i_mss.len; i++) {
        ezld_mrg_sec_t *mrg = &g_self->i_mss.buf[i];

        if (mrg->ms_name == objsec_name) {
            size_t next_idx = mrg->ms_oss.len;

            if (ezld_array_is_empty(mrg->ms_oss)) {
                *ezld_array_push(mrg->ms_oss) = objsec;
                objsec->os_ndx                = next_idx;
                objsec->os_mrg                = mrg;
                objsec->os_transl             = 0;
                mrg->ms_memsz                 = objsec->os_shdr.sh_size;
                return;
            }

            ezld_obj_sec_t *last = ezld_array_last(mrg->ms_oss);

            if (objsec->os_shdr.sh_type != last->os_shdr.sh_type) {
                ezld_runtime_exit(EZLD_ECODE_BADSEC,
                                  "section '%s' in '%s' has conflicting type "
                                  "with '%s' sections in other files",
                                  objsec_name,
                                  objsec->os_obj->obj_filepath,
                                  objsec_name);
            }

            if (objsec->os_shdr.sh_flags != last->os_shdr.sh_flags) {
                ezld_runtime_exit(EZLD_ECODE_BADSEC,
                                  "section '%s' in '%s' has conflicting flags "
                                  "with '%s' sections in other files",
                                  objsec_name,
                                  objsec->os_obj->obj_filepath,
                                  objsec_name);
            }

            if (objsec->os_shdr.sh_addralign != last->os_shdr.sh_addralign) {
                ezld_runtime_exit(
                    EZLD_ECODE_BADSEC,
                    "section '%s' in '%s' has conflicting alignment "
                    "with '%s' sections in other files",
                    shstr_from_idx(objsec_name).gs_data,
                    objsec->os_obj->obj_filepath,
                    shstr_from_idx(objsec_name).gs_data);
            }

            size_t transl_off = last->os_transl + last->os_shdr.sh_size;
            *ezld_array_push(mrg->ms_oss) = objsec;
            objsec->os_ndx                = next_idx;
            objsec->os_mrg                = mrg;
            objsec->os_transl             = transl_off;
            mrg->ms_memsz += objsec->os_shdr.sh_size;
            return;
        }
    }

    size_t          next_mrg_idx = g_self->i_mss.len;
    ezld_mrg_sec_t *new_mrg      = ezld_array_push(g_self->i_mss);
    new_mrg->ms_name             = objsec_name;
    new_mrg->ms_ndx              = next_mrg_idx;
    new_mrg->ms_memsz            = 0;
    new_mrg->ms_fileoff          = 0;
    ezld_array_init(new_mrg->ms_oss);
    objsec->os_ndx                    = 0;
    objsec->os_mrg                    = new_mrg;
    objsec->os_transl                 = 0;
    *ezld_array_push(new_mrg->ms_oss) = objsec;
}

static Elf32_Shdr endian_shdr(Elf32_Shdr shdr) {
    shdr.sh_flags     = endian32(shdr.sh_flags);
    shdr.sh_addr      = endian32(shdr.sh_addr);
    shdr.sh_addralign = endian32(shdr.sh_addralign);
    shdr.sh_entsize   = endian32(shdr.sh_entsize);
    shdr.sh_info      = endian32(shdr.sh_info);
    shdr.sh_link      = endian32(shdr.sh_link);
    shdr.sh_offset    = endian32(shdr.sh_offset);
    shdr.sh_name      = endian32(shdr.sh_name);
    shdr.sh_size      = endian32(shdr.sh_size);
    shdr.sh_type      = endian32(shdr.sh_type);
    return shdr;
}

static Elf32_Sym endian_sym(Elf32_Sym sym) {
    sym.st_name  = endian32(sym.st_name);
    sym.st_shndx = endian16(sym.st_shndx);
    sym.st_size  = endian32(sym.st_size);
    sym.st_value = endian32(sym.st_value);
    return sym;
}

static Elf32_Ehdr endian_ehdr(Elf32_Ehdr ehdr) {
    ehdr.e_type      = endian16(ehdr.e_type);
    ehdr.e_machine   = endian16(ehdr.e_machine);
    ehdr.e_version   = endian32(ehdr.e_version);
    ehdr.e_entry     = endian32(ehdr.e_entry);
    ehdr.e_phoff     = endian32(ehdr.e_phoff);
    ehdr.e_shoff     = endian32(ehdr.e_shoff);
    ehdr.e_flags     = endian32(ehdr.e_flags);
    ehdr.e_ehsize    = endian16(ehdr.e_ehsize);
    ehdr.e_phentsize = endian16(ehdr.e_phentsize);
    ehdr.e_phnum     = endian16(ehdr.e_phnum);
    ehdr.e_shentsize = endian16(ehdr.e_shentsize);
    ehdr.e_shnum     = endian16(ehdr.e_shnum);
    ehdr.e_shstrndx  = endian16(ehdr.e_shstrndx);
    return ehdr;
}

static Elf32_Phdr endian_phdr(Elf32_Phdr phdr) {
    phdr.p_type   = endian32(phdr.p_type);
    phdr.p_align  = endian32(phdr.p_align);
    phdr.p_filesz = endian32(phdr.p_filesz);
    phdr.p_flags  = endian32(phdr.p_flags);
    phdr.p_memsz  = endian32(phdr.p_memsz);
    phdr.p_offset = endian32(phdr.p_offset);
    phdr.p_paddr  = endian32(phdr.p_paddr);
    phdr.p_vaddr  = endian32(phdr.p_vaddr);
    return phdr;
}

/**
 * @brief Reads a section header from an object file
 *
 * @param shndx the section header index
 * @param randacc if true, use shndx, else read from the file normally
 * @param obj the object file
 *
 * @return the section header contents
 */
static Elf32_Shdr read_shdr(size_t shndx, bool randacc, ezld_obj_t *obj) {
    Elf32_Shdr shdr = {0};
    size_t     shsz = obj->obj_ehdr.e_shentsize;

    if (randacc) {
        ezld_runtime_read_exact_at(&shdr,
                                   shsz,
                                   shndx * shsz + obj->obj_ehdr.e_shoff,
                                   obj->obj_filepath,
                                   obj->obj_file);
    } else {
        ezld_runtime_read_exact(&shdr, shsz, obj->obj_filepath, obj->obj_file);
    }

    return endian_shdr(shdr);
}

/**
 * @brief Reads a symbol table entry from an object file
 *
 * @param stndx the symbol index
 * @param randacc if true, use stndx, else read from the file normally
 * @param obj the object file
 *
 * @return the symbol table entry contents
 */
static Elf32_Sym read_sym(size_t stndx, bool randacc, ezld_obj_t *obj) {
    ezld_obj_sec_t *obj_symtab = obj->obj_ost.ost_os;
    Elf32_Sym       entry      = {0};

    if (randacc) {
        (void)stndx;
        (void)obj_symtab;
        assert(false);
        // TODO: implement this? This function should technically be able to
        // read at `strdx`, but this feature is not used in the code, and might
        // never be used
    } else {
        ezld_runtime_read_exact(
            &entry, sizeof(Elf32_Sym), obj->obj_filepath, obj->obj_file);
    }

    return endian_sym(entry);
}

/**
 * @brief Adds all symbols from an object file symbol table to the in-memory
 * table and adds known symbols to the global symbol table
 *
 * @param obj the object file
 */
static void merge_symtabs(ezld_obj_t *obj) {
    ezld_obj_sec_t *obj_symtab = obj->obj_ost.ost_os;

    if (!(SHF_INFO_LINK & obj_symtab->os_shdr.sh_flags)) {
        ezld_runtime_message(
            EZLD_EMSG_WARN,
            "section '%s' in '%s' is of type SHT_SYMTAB "
            "but is missing flag SHF_LINK_INFO",
            shstr_from_idx(obj_symtab->os_mrg->ms_name).gs_data,
            obj->obj_filepath);
    }

    ezld_obj_sec_t *strtab_sec = &obj->obj_oss.buf[obj_symtab->os_shdr.sh_link];
    size_t          num_entires =
        obj_symtab->os_shdr.sh_size / obj_symtab->os_shdr.sh_entsize;
    read_section_contents(strtab_sec);
    ezld_runtime_seek(
        obj_symtab->os_shdr.sh_offset, obj->obj_filepath, obj->obj_file);

    ezld_array_alloc(obj->obj_ost.ost_syms, num_entires);

    for (size_t i = 0; i < num_entires; i++) {
        Elf32_Sym entry = read_sym(i, false, obj);

        ezld_obj_sym_t *obj_sym = ezld_array_push(obj->obj_ost.ost_syms);
        obj_sym->osy_esym       = entry;
        obj_sym->osy_name       = (char *)&strtab_sec->os_data[entry.st_name];

        if (SHN_UNDEF == entry.st_shndx) {
            obj_sym->osy_globndx = 0;
            continue;
        }

        ezld_obj_sec_t *sym_sec =
            &obj_symtab->os_obj->obj_oss.buf[entry.st_shndx];
        uint32_t glob_shidx   = sym_sec->os_mrg->ms_ndx;
        uint32_t glob_sym_off = entry.st_value + sym_sec->os_transl;

        Elf32_Sym *glob_sym = ezld_array_push(g_self->i_globsymtab);
        glob_sym->st_shndx  = glob_shidx;
        glob_sym->st_value  = glob_sym_off;
        glob_sym->st_name   = globstr_add(obj_sym->osy_name);
        glob_sym->st_size   = entry.st_size;
        glob_sym->st_info   = entry.st_info;

        // This starts at 1 to use 0 as NULL
        obj_sym->osy_globndx = g_self->i_globsymtab.len;

        if (NULL == g_self->i_osentry && EZLD_ENTRY_NAME == glob_sym->st_name) {
            g_self->i_osentry = obj_sym;
        }
    }
}

/**
 * @brief Runs the first stage of linking on a given object file
 *
 * @param obj the object file
 */
static void read_object(ezld_obj_t *obj) {
    ezld_array_init(obj->obj_oss);
    obj->obj_ost.ost_os = NULL;
    ezld_array_init(obj->obj_ost.ost_syms);

    Elf32_Ehdr ehdr = {0};
    ezld_runtime_read_exact(
        &ehdr, sizeof(Elf32_Ehdr), obj->obj_filepath, obj->obj_file);
    obj->obj_ehdr = ehdr;

    if (ELFMAG0 != ehdr.e_ident[EI_MAG0] || ELFMAG1 != ehdr.e_ident[EI_MAG1] ||
        ELFMAG2 != ehdr.e_ident[EI_MAG2] || ELFMAG3 != ehdr.e_ident[EI_MAG3]) {
        ezld_runtime_exit(
            EZLD_ECODE_BADFILE, "'%s' is not an ELF file", obj->obj_filepath);
    }

    if (ELFCLASS32 != ehdr.e_ident[EI_CLASS]) {
        ezld_runtime_exit(
            EZLD_ECODE_BADFILE, "'%s' is not a 32-bit ELF", obj->obj_filepath);
    }

    if (!g_self->i_out.out_set) {
        g_self->i_out.out_set     = true;
        g_self->i_out.out_endian  = ehdr.e_ident[EI_DATA];
        g_self->i_out.out_abi     = ehdr.e_ident[EI_OSABI];
        g_self->i_out.out_abi_ver = ehdr.e_ident[EI_ABIVERSION];
        g_self->i_out.out_mach    = endian16(ehdr.e_machine);
    }

    ehdr          = endian_ehdr(ehdr);
    obj->obj_ehdr = ehdr;

    if (ET_REL != ehdr.e_type) {
        ezld_runtime_exit(EZLD_ECODE_BADFILE,
                          "'%s' is not a relocatable object file",
                          obj->obj_filepath);
    }

    Elf32_Shdr shstrtab_sh = read_shdr(ehdr.e_shstrndx, true, obj);

    if (SHT_STRTAB != shstrtab_sh.sh_type) {
        ezld_runtime_message(
            EZLD_EMSG_WARN,
            "section header string section in '%s' is not of type SHT_STRTAB",
            obj->obj_filepath);
    }

    char *shstrtab = (char *)ezld_runtime_alloc(1, shstrtab_sh.sh_size);
    ezld_runtime_read_exact_at(shstrtab,
                               shstrtab_sh.sh_size,
                               shstrtab_sh.sh_offset,
                               obj->obj_filepath,
                               obj->obj_file);

    ezld_runtime_seek(ehdr.e_shoff, obj->obj_filepath, obj->obj_file);
    for (size_t i = 0; i < ehdr.e_shnum; i++) {
        Elf32_Shdr shdr = read_shdr(0, false, obj);

        ezld_obj_sec_t *objsec = ezld_array_push(obj->obj_oss);
        objsec->os_obj         = obj;
        objsec->os_shdr        = shdr;
        objsec->os_elems       = shdr.sh_size;
        objsec->os_data        = NULL;

        if (0 != shdr.sh_entsize) {
            objsec->os_elems /= shdr.sh_entsize;
        }

        if (ehdr.e_shstrndx == i) {
            objsec->os_data = (uint8_t *)shstrtab;
        }

        if (SHT_SYMTAB == shdr.sh_type) {
            if (NULL != obj->obj_ost.ost_os) {
                ezld_runtime_message(
                    EZLD_EMSG_WARN,
                    "object file '%s' has more than one "
                    "section of type SHT_SYMTAB, using first one",
                    obj->obj_filepath);
            } else {
                obj->obj_ost.ost_os = objsec;
                objsec->os_mrg      = NULL;
                objsec->os_ndx      = 0;
            }
        } else if (SHT_PROGBITS == shdr.sh_type || SHT_NOBITS == shdr.sh_type ||
                   SHT_SYMTAB == shdr.sh_type) {
            const char *objsec_name = &shstrtab[shdr.sh_name];
            merge_section(objsec, shstr_add(objsec_name));
        }
    }

    merge_symtabs(obj);
}

/**
 * @brief Runs the first stage of linking for all object file
 */
static void read_objects(void) {
    for (size_t i = 0; i < g_self->i_objs.len; i++) {
        read_object(&g_self->i_objs.buf[i]);
    }
}

/**
 * @brief Writes a segment to disk
 *
 * @param sec the merged section to which the segment is relative
 * @param off the offset in the file where to place the segment's contents
 * @param filename the name of the destination file
 * @param file the actual file
 *
 * @return the number of bytes written to the file
 */
static size_t write_segment(ezld_mrg_sec_t *sec,
                            size_t          off,
                            const char     *filename,
                            FILE           *file) {
    if (SHT_NOBITS == ezld_array_first(sec->ms_oss)->os_shdr.sh_type) {
        return 0;
    }

    size_t written = 0;
    for (size_t i = 0; i < sec->ms_oss.len; i++) {
        ezld_obj_sec_t *s = sec->ms_oss.buf[i];
        read_section_contents(s);
        ezld_runtime_write_exact_at(
            s->os_data, s->os_shdr.sh_size, off + written, filename, file);
        written += s->os_shdr.sh_size;
    }

    return written;
}

/**
 * @brief Converts an in-memory global string table into a real ELF strtab. It
 * writes the string table on the destination file starting from the file's
 * current cursor position
 *
 * @param sec_name the name of the string table section
 * @param strtab the global string table instance
 *
 * @return the section header describing the string table
 */
static Elf32_Shdr write_strtab(const char         *sec_name,
                               ezld_glob_strtab_t *strtab) {
    char zero = '\0';

    Elf32_Shdr strtab_shdr = {0};
    strtab_shdr.sh_type    = SHT_STRTAB;
    strtab_shdr.sh_name    = shstr_from_idx(shstr_add(sec_name)).gs_offset;
    strtab_shdr.sh_offset  = ftell(g_self->i_out.out_file);
    strtab_shdr.sh_size    = 1;

    ezld_runtime_write_exact(
        &zero, 1, g_self->i_cfg.cfg_outpath, g_self->i_out.out_file);
    for (size_t i = 0; i < strtab->gst_strs.len; i++) {
        ezld_glob_str_t *str = &strtab->gst_strs.buf[i];
        ezld_runtime_write_exact((void *)(str->gs_data),
                                 str->gs_len + 1,
                                 g_self->i_cfg.cfg_outpath,
                                 g_self->i_out.out_file);
        strtab_shdr.sh_size += str->gs_len + 1;
    }

    return endian_shdr(strtab_shdr);
}

/**
 * @brief Writes the output executable to disk (without relocations)
 */
static void write_exec(void) {
    Elf32_Ehdr ehdr             = {0};
    ehdr.e_ident[EI_MAG0]       = ELFMAG0;
    ehdr.e_ident[EI_MAG1]       = ELFMAG1;
    ehdr.e_ident[EI_MAG2]       = ELFMAG2;
    ehdr.e_ident[EI_MAG3]       = ELFMAG3;
    ehdr.e_ident[EI_CLASS]      = ELFCLASS32;
    ehdr.e_ident[EI_DATA]       = g_self->i_out.out_endian;
    ehdr.e_ident[EI_VERSION]    = 1;
    ehdr.e_ident[EI_OSABI]      = g_self->i_out.out_abi;
    ehdr.e_ident[EI_ABIVERSION] = g_self->i_out.out_abi_ver;

    ehdr.e_type    = ET_EXEC;
    ehdr.e_machine = EM_RISCV;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_ehsize  = sizeof(Elf32_Ehdr);

    if (NULL == g_self->i_osentry ||
        EZLD_GLOB_SYM_UNDEF == g_self->i_osentry->osy_globndx) {
        ezld_runtime_message(EZLD_EMSG_WARN,
                             "could not resolve entry point symbol '%s', "
                             "defaulting to base of '.text' section",
                             g_self->i_cfg.cfg_entrysym);
        // TODO: actually default to that
    } else {
        Elf32_Sym gsym;
        resolve_sym(&gsym, g_self->i_osentry, EZLD_ENTRY_NAME, false);
        ehdr.e_entry = gsym.st_value;
    }

    // Header will be added later
    ezld_runtime_seek(
        sizeof(Elf32_Ehdr), g_self->i_cfg.cfg_outpath, g_self->i_out.out_file);

    ehdr.e_phoff     = sizeof(Elf32_Ehdr);
    ehdr.e_phentsize = sizeof(Elf32_Phdr);
    ehdr.e_shnum     = 3; // NULL, ..., .strtab, .shstrtab
    ehdr.e_shentsize = sizeof(Elf32_Shdr);
    ezld_array(struct {
        Elf32_Phdr      phdr;
        ezld_mrg_sec_t *sec;
    }) tmp_phdrs     = ezld_array_new();
    size_t phdrs_end = ehdr.e_phoff;

    for (size_t i = 0; i < g_self->i_mss.len; i++) {
        ezld_mrg_sec_t *mrg = &g_self->i_mss.buf[i];

        if (!ezld_array_is_empty(mrg->ms_oss) &&
            (SHF_ALLOC & ezld_array_first(mrg->ms_oss)->os_shdr.sh_flags)) {
            Elf32_Shdr base = ezld_array_first(mrg->ms_oss)->os_shdr;
            ehdr.e_phnum++;
            Elf32_Phdr phdr = {0};
            phdr.p_type     = PT_LOAD;
            phdr.p_align    = g_self->i_cfg.cfg_segalign;
            phdr.p_vaddr    = mrg->ms_vaddr;
            phdr.p_paddr    = mrg->ms_vaddr;
            phdr.p_memsz    = mrg->ms_memsz;
            phdr.p_flags    = PF_R;
            size_t sh_flags = ezld_array_first(mrg->ms_oss)->os_shdr.sh_flags;

            if (SHT_NOBITS != base.sh_type) {
                phdr.p_filesz = mrg->ms_memsz;
            }

            if (SHF_WRITE & sh_flags) {
                phdr.p_flags |= PF_W;
            }

            if (SHF_EXECINSTR & sh_flags) {
                phdr.p_flags = PF_X;
            }

            (void)ezld_array_push(tmp_phdrs);
            ezld_array_last(tmp_phdrs).phdr = phdr;
            ezld_array_last(tmp_phdrs).sec  = mrg;
            phdrs_end += sizeof(Elf32_Phdr);
        }
    }

    size_t seg_off = phdrs_end;
    for (size_t i = 0; i < tmp_phdrs.len; i++) {
        Elf32_Phdr      phdr = tmp_phdrs.buf[i].phdr;
        ezld_mrg_sec_t *sec  = tmp_phdrs.buf[i].sec;
        seg_off += phdr.p_align - (seg_off % phdr.p_align);
        phdr.p_offset   = seg_off;
        sec->ms_fileoff = seg_off;
        phdr            = endian_phdr(phdr);
        ezld_runtime_write_exact(&phdr,
                                 sizeof(Elf32_Phdr),
                                 g_self->i_cfg.cfg_outpath,
                                 g_self->i_out.out_file);
        (void)write_segment(
            sec, seg_off, g_self->i_cfg.cfg_outpath, g_self->i_out.out_file);
        seg_off += phdr.p_filesz;
    }

    ezld_runtime_seek(
        seg_off, g_self->i_cfg.cfg_outpath, g_self->i_out.out_file);

    Elf32_Shdr strtab_shdr   = write_strtab(".strtab", &g_self->i_globstrtab);
    Elf32_Shdr shstrtab_shdr = write_strtab(".shstrtab", &g_self->i_shstrtab);

    ehdr.e_shoff = ftell(g_self->i_out.out_file);

    Elf32_Shdr null_shdr = {0};
    // TODO: set something?
    ezld_runtime_write_exact(&null_shdr,
                             sizeof(Elf32_Shdr),
                             g_self->i_cfg.cfg_outpath,
                             g_self->i_out.out_file);

    for (size_t i = 0; i < g_self->i_mss.len; i++) {
        ezld_mrg_sec_t *s = &g_self->i_mss.buf[i];

        if (!ezld_array_is_empty(s->ms_oss)) {
            Elf32_Shdr shdr = ezld_array_first(s->ms_oss)->os_shdr;
            shdr.sh_size    = s->ms_memsz;
            shdr.sh_name    = shstr_from_idx(s->ms_name).gs_offset;
            shdr.sh_addr    = s->ms_vaddr;
            shdr.sh_offset  = s->ms_fileoff;
            shdr            = endian_shdr(shdr);
            ezld_runtime_write_exact(&shdr,
                                     sizeof(Elf32_Shdr),
                                     g_self->i_cfg.cfg_outpath,
                                     g_self->i_out.out_file);
            ehdr.e_shnum++;
        }
    }

    ezld_runtime_write_exact(&strtab_shdr,
                             sizeof(Elf32_Shdr),
                             g_self->i_cfg.cfg_outpath,
                             g_self->i_out.out_file);
    ezld_runtime_write_exact(&shstrtab_shdr,
                             sizeof(Elf32_Shdr),
                             g_self->i_cfg.cfg_outpath,
                             g_self->i_out.out_file);
    ehdr.e_shstrndx = ehdr.e_shnum - 1;
    ehdr            = endian_ehdr(ehdr);
    ezld_runtime_seek(0, g_self->i_cfg.cfg_outpath, g_self->i_out.out_file);
    ezld_runtime_write_exact(&ehdr,
                             sizeof(Elf32_Ehdr),
                             g_self->i_cfg.cfg_outpath,
                             g_self->i_out.out_file);

    ezld_array_free(tmp_phdrs);
}

/**
 * @brief Aligns all merged sections to the alignment specified by the
 * configuration
 */
static void align_sections(void) {
    for (size_t i = 0; i < g_self->i_mss.len; i++) {
        ezld_mrg_sec_t *mrg      = &g_self->i_mss.buf[i];
        const char     *sec_name = shstr_from_idx(mrg->ms_name).gs_data;

        if (ezld_array_is_empty(mrg->ms_oss)) {
            ezld_runtime_message(EZLD_EMSG_WARN,
                                 "section '%s' is empty",
                                 shstr_from_idx(mrg->ms_name).gs_data);
            continue;
        }

        size_t sh_align  = ezld_array_first(mrg->ms_oss)->os_shdr.sh_addralign;
        size_t seg_align = g_self->i_cfg.cfg_segalign;
        size_t align     = sh_align;
        if (SHF_ALLOC & ezld_array_first(mrg->ms_oss)->os_shdr.sh_flags &&
            seg_align > sh_align) {
            align = seg_align;
        }
        mrg->ms_memsz = mrg->ms_memsz + (align - (mrg->ms_memsz % align));

        if (!(SHF_ALLOC & ezld_array_first(mrg->ms_oss)->os_shdr.sh_flags)) {
            continue;
        }

        if (i > 0) {
            ezld_mrg_sec_t *prev_mrg = &g_self->i_mss.buf[i - 1];

            if (mrg->ms_vaddr < prev_mrg->ms_vaddr + prev_mrg->ms_memsz) {
                size_t diff =
                    prev_mrg->ms_vaddr + prev_mrg->ms_memsz - mrg->ms_vaddr;
                size_t old_virt = mrg->ms_vaddr;
                mrg->ms_vaddr += diff;

                ezld_runtime_message(EZLD_EMSG_WARN,
                                     "section '%s' has overlapping virtual "
                                     "address 0x%08x, changing to 0x%08x",
                                     sec_name,
                                     old_virt,
                                     mrg->ms_vaddr);
            }
        }

        if (0 == mrg->ms_vaddr) {
            ezld_runtime_message(EZLD_EMSG_WARN,
                                 "section '%s' has virtual address 0x%08x",
                                 sec_name,
                                 0);
        } else if (0 != mrg->ms_vaddr % align) {
            size_t aligned_virt = mrg->ms_vaddr + (mrg->ms_vaddr % align);
            ezld_runtime_message(
                EZLD_EMSG_WARN,
                "section '%s' has misaligned virtual address 0x%08x (requires "
                "alignment of %u byte(s)), changing to 0x%08x",
                sec_name,
                mrg->ms_vaddr,
                align,
                aligned_virt);
            mrg->ms_vaddr = aligned_virt;
        }
    }
}

/**
 * @brief Translates all symbol virtual addresses by adding the relative
 * section's virtual address. This is used because object files store offsets in
 * symbol table, whereas final executables include the real virtual address
 */
void virtualize_syms(void) {
    for (size_t i = 0; i < g_self->i_globsymtab.len; i++) {
        Elf32_Sym      *sym = &g_self->i_globsymtab.buf[i];
        ezld_mrg_sec_t *sec = &g_self->i_mss.buf[sym->st_shndx];
        sym->st_value += sec->ms_vaddr;
    }
}

/**
 * @brief Creates initial merged sections from configuration
 */
static void setup_sections(void) {
    for (size_t i = 0; i < g_self->i_cfg.cfg_sections.len; i++) {
        ezld_sec_cfg_t  sec_cfg = g_self->i_cfg.cfg_sections.buf[i];
        ezld_mrg_sec_t *mrg     = ezld_array_push(g_self->i_mss);
        mrg->ms_ndx             = g_self->i_mss.len - 1;
        mrg->ms_vaddr           = sec_cfg.sc_vaddr;
        mrg->ms_name            = shstr_add(sec_cfg.sc_name);
        mrg->ms_memsz           = 0;
        mrg->ms_fileoff         = 0;
        ezld_array_init(mrg->ms_oss);
    }
}

static void open_output(void) {
    FILE *out = fopen(g_self->i_cfg.cfg_outpath, "wb");

    if (NULL == out) {
        ezld_runtime_exit(EZLD_ECODE_NOFILE,
                          "could not open output file '%s'",
                          g_self->i_cfg.cfg_outpath);
    }

    g_self->i_out.out_file = out;
}

static void open_objects(void) {
    for (size_t i = 0; i < g_self->i_cfg.cfg_objpaths.len; i++) {
        const char *obj_path = g_self->i_cfg.cfg_objpaths.buf[i];
        FILE       *file     = fopen(obj_path, "rb");

        if (NULL == file) {
            ezld_runtime_exit(
                EZLD_ECODE_NOFILE, "could not open input file '%s'", obj_path);
        }

        ezld_obj_t *obj   = ezld_array_push(g_self->i_objs);
        obj->obj_file     = file;
        obj->obj_filepath = obj_path;
        obj->obj_ndx      = i;
    }
}

static void free_instance(void) {
    ezld_array_free(g_self->i_globsymtab);
    ezld_array_free(g_self->i_globstrtab.gst_strs);
    ezld_array_free(g_self->i_shstrtab.gst_strs);

    for (size_t i = 0; i < g_self->i_objs.len; i++) {
        ezld_obj_t *obj = &g_self->i_objs.buf[i];
        for (size_t j = 0; j < obj->obj_oss.len; j++) {
            ezld_obj_sec_t *sec = &obj->obj_oss.buf[j];
            free(sec->os_data);
            sec->os_data = NULL;
        }
        ezld_array_free(obj->obj_oss);
        ezld_array_free(obj->obj_ost.ost_syms);
        fclose(obj->obj_file);
    }

    for (size_t i = 0; i < g_self->i_mss.len; i++) {
        ezld_mrg_sec_t *ms = &g_self->i_mss.buf[i];
        ezld_array_free(ms->ms_oss);
    }

    ezld_array_free(g_self->i_mss);
    ezld_array_free(g_self->i_objs);
    fclose(g_self->i_out.out_file);
}

// TODO: fix HUGE endianness UB here
/**
 * @brief Applies relocations to the final executable (after it has been written
 * to disk)
 *
 * @param data original data from the section (code or data) - not relocated
 * @param bufsz size of the data buffer
 * @param outfile_off offset in the output file where to store the relocation
 * @param type the type of relocation read from the ELF file
 * @param virt_addr the virtual address of the section (used for relative
 * values)
 * @param addend the added of the relotion
 * @param globsym global symbol table (in-memory)
 */
static void relocate(uint8_t  *data,
                     size_t    bufsz,
                     size_t    outfile_off,
                     size_t    type,
                     size_t    virt_addr,
                     size_t    addend,
                     Elf32_Sym globsym) {
#define REQUIRE(bytes)                                              \
    if (bufsz < bytes) {                                            \
        ezld_runtime_message(EZLD_EMSG_ERR,                         \
                             "out of bounds relocation, ignoring"); \
        break;                                                      \
    }
#define REGION(type)    *(type *)data
#define KEEP_HI32(bits) (mask32(~(uint32_t)(0) << (32 - bits)))
#define KEEP_LO32(bits) (~KEEP_HI32((32 - bits)))
#define WRITE(val)                                         \
    ezld_runtime_write_exact_at(&val,                      \
                                sizeof val,                \
                                outfile_off,               \
                                g_self->i_cfg.cfg_outpath, \
                                g_self->i_out.out_file)

    ezld_runtime_seek(
        outfile_off, g_self->i_cfg.cfg_outpath, g_self->i_out.out_file);

    switch (type) {
    case R_RISCV_BRANCH: {
        REQUIRE(4);
        uint32_t inst    = REGION(uint32_t);
        int32_t  val     = (int32_t)globsym.st_value + addend - virt_addr;
        uint32_t uval    = (uint32_t)val;
        uint32_t imm12   = (uval >> 12) & 0x1;
        uint32_t imm10_5 = (uval >> 5) & 0x3F;
        uint32_t imm4_1  = (uval >> 1) & 0xF;
        uint32_t imm11   = (uval >> 11) & 0x1;
        inst &= 0x01FFF07F;
        inst |= (imm12 << 31);
        inst |= (imm10_5 << 25);
        inst |= (imm4_1 << 8);
        inst |= (imm11 << 7);
        WRITE(inst);
        break;
    }

    case R_RISCV_JAL: {
        REQUIRE(4);
        uint32_t inst     = REGION(uint32_t);
        int32_t  val      = (int32_t)globsym.st_value + addend - virt_addr;
        uint32_t uval     = (uint32_t)val;
        uint32_t imm20    = (uval >> 20) & 0x1;
        uint32_t imm10_1  = (uval >> 1) & 0x3FF;
        uint32_t imm11    = (uval >> 11) & 0x1;
        uint32_t imm19_12 = (uval >> 12) & 0xFF;
        inst &= 0x00000FFF;
        inst |= (imm20 << 31);
        inst |= (imm10_1 << 21);
        inst |= (imm11 << 20);
        inst |= (imm19_12 << 12);
        WRITE(inst);
        break;
    }

    case R_RISCV_HI20: {
        REQUIRE(4);
        uint32_t inst = REGION(uint32_t);
        inst =
            (inst & KEEP_LO32(12)) | mask32(globsym.st_value & KEEP_HI32(20));
        WRITE(inst);
        break;
    }

    case R_RISCV_LO12_I: {
        REQUIRE(4);
        uint32_t inst = REGION(uint32_t);
        inst          = (inst & KEEP_LO32(20)) |
               mask32((globsym.st_value & KEEP_LO32(12)) << 20);
        WRITE(inst);
        break;
    }

    case R_RISCV_LO12_S: {
        REQUIRE(4);
        uint32_t inst    = REGION(uint32_t);
        uint32_t uval    = globsym.st_value + addend;
        uint32_t imm11_5 = (uval >> 5) & 0x7F;
        uint32_t imm4_0  = uval & 0x1F;
        inst &= 0x01FFF07F;
        inst |= (imm11_5 << 25);
        inst |= (imm4_0 << 7);
        WRITE(inst);
        break;
    }

    default:
        ezld_runtime_message(
            EZLD_EMSG_WARN, "unsupported relocation type %u, ignoring", type);
    }
#undef REQUIRE
#undef KEEP_HI32
#undef KEEP_LO32
#undef WRITE
}

static void rela_section(ezld_obj_sec_t *objsec) {
    // TODO: handle case in which symtab is wrong
    // size_t symtab_idx = objsec->os_shdr.sh_link;
    size_t          target_idx = objsec->os_shdr.sh_info;
    ezld_obj_sec_t *target     = &objsec->os_obj->obj_oss.buf[target_idx];
    read_section_contents(target);
    const char *target_name = shstr_from_idx(target->os_mrg->ms_name).gs_data;
    size_t num_entries = objsec->os_shdr.sh_size / objsec->os_shdr.sh_entsize;
    Elf32_Rela *relas  = (Elf32_Rela *)objsec->os_data;

    // TODO: do this just like symbols: read from file
    // instead of loading everything into memory
    // TODO: fix endianness here too
    for (size_t i = 0; i < num_entries; i++) {
        Elf32_Rela      entry   = relas[i];
        size_t          off     = entry.r_offset + target->os_mrg->ms_fileoff;
        size_t          sym_idx = ELF32_R_SYM(entry.r_info);
        size_t          type    = ELF32_R_TYPE(entry.r_info);
        ezld_obj_sym_t *sym = &objsec->os_obj->obj_ost.ost_syms.buf[sym_idx];
        Elf32_Sym       glob_sym;

        if (EZLD_GLOB_SYM_UNDEF == resolve_sym(&glob_sym, sym, 0, true)) {
            ezld_runtime_message(
                EZLD_EMSG_ERR,
                "in %s:%s+0x%x (%s:%s+0x%lx): undefined reference to '%s'",
                objsec->os_obj->obj_filepath,
                target_name,
                entry.r_offset,
                g_self->i_cfg.cfg_outpath,
                target_name,
                target->os_transl + entry.r_offset,
                sym->osy_name);
            continue;
        }

        relocate(&target->os_data[entry.r_offset],
                 target->os_shdr.sh_size - entry.r_offset,
                 off,
                 type,
                 target->os_mrg->ms_vaddr + target->os_transl + entry.r_offset,
                 entry.r_addend,
                 glob_sym);
    }
}

static void apply_relocations(void) {
    for (size_t i = 0; i < g_self->i_objs.len; i++) {
        ezld_obj_t *obj = &g_self->i_objs.buf[i];

        for (size_t j = 0; j < obj->obj_oss.len; j++) {
            ezld_obj_sec_t *objsec = &obj->obj_oss.buf[j];

            // TODO: support REL as well
            if (SHT_RELA == objsec->os_shdr.sh_type) {
                read_section_contents(objsec);
                rela_section(objsec);
            }
        }
    }
}

void ezld_link(ezld_config_t config) {
    ezld_instance_t instance = {0};
    ezld_array_init(instance.i_mss);
    ezld_array_init(instance.i_globsymtab);
    ezld_array_init(instance.i_globstrtab.gst_strs);
    ezld_array_init(instance.i_shstrtab.gst_strs);
    instance.i_osentry = NULL;
    instance.i_cfg     = config;
    instance.i_out     = (ezld_output_t){0};

    g_self = &instance;
    globstr_add(instance.i_cfg.cfg_entrysym);

    open_output();
    open_objects();
    setup_sections();
    read_objects();
    align_sections();
    virtualize_syms();

    write_exec();
    apply_relocations();
    free_instance();
}
