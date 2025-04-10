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
#include <musl/elf.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define EZLD_ENTRY_NAME     1
#define EZLD_GLOB_SYM_UNDEF 0

// This can be global
static ezld_instance_t *g_self = NULL;

static ezld_merged_sec_t *find_mrg_sec(size_t name_idx) {
    for (size_t i = 0; i < g_self->mrg_sec.len; i++) {
        ezld_merged_sec_t *s = &g_self->mrg_sec.buf[i];

        if (s->name_idx == name_idx) {
            return s;
        }
    }

    return NULL;
}

static size_t
resolve_sym(Elf32_Sym *ret, ezld_obj_sym_t *objsym, size_t glob_stridx) {
    if (NULL != objsym && EZLD_GLOB_SYM_UNDEF != objsym->glob_idx) {
        *ret = g_self->glob_symtab.buf[objsym->glob_idx - 1];
        return objsym->glob_idx;
    }

    for (size_t i = 0; i < g_self->glob_symtab.len; i++) {
        Elf32_Sym s = g_self->glob_symtab.buf[i];

        if (s.st_name == glob_stridx) {
            if (NULL != objsym) {
                objsym->glob_idx = i + 1;
            }
            *ret = s;
            return i + 1;
        }
    }

    return EZLD_GLOB_SYM_UNDEF;
}

static ezld_global_str_t shstr_from_idx(size_t shstr_idx) {
    return g_self->sh_strtab.buf[shstr_idx];
}

static ezld_global_str_t globstr_from_idx(size_t glob_idx) {
    return g_self->glob_strtab.buf[glob_idx];
}

// A bit of code repetition here
static size_t shstr_add(const char *str) {
    size_t len = strlen(str);

    for (size_t i = 0; i < g_self->sh_strtab.len; i++) {
        ezld_global_str_t s = g_self->sh_strtab.buf[i];

        if (len == s.len && 0 == strcmp(str, s.str)) {
            return i;
        }
    }

    ezld_global_str_t s                 = {.str = str, .len = strlen(str)};
    *ezld_array_push(g_self->sh_strtab) = s;
    return g_self->sh_strtab.len - 1;
}

static size_t globstr_add(const char *str) {
    size_t len = strlen(str);

    for (size_t i = 0; i < g_self->glob_strtab.len; i++) {
        ezld_global_str_t s = g_self->glob_strtab.buf[i];

        if (len == s.len && 0 == strcmp(str, s.str)) {
            return i;
        }
    }

    ezld_global_str_t s                   = {.str = str, .len = strlen(str)};
    *ezld_array_push(g_self->glob_strtab) = s;
    return g_self->glob_strtab.len - 1;
}

static void read_section_contents(ezld_obj_sec_t *sec) {
    if (NULL == sec->buf) {
        sec->buf = ezld_runtime_alloc(1, sec->shdr.sh_size);
        ezld_runtime_read_exact_at(sec->buf,
                                   sec->shdr.sh_size,
                                   sec->shdr.sh_offset,
                                   sec->o_file->path,
                                   sec->o_file->file);
    }
}

static void merge_section(ezld_obj_sec_t *objsec, size_t objsec_name) {
    for (size_t i = 0; i < g_self->mrg_sec.len; i++) {
        ezld_merged_sec_t *mrg = &g_self->mrg_sec.buf[i];

        if (mrg->name_idx == objsec_name) {
            size_t next_idx = mrg->sub.len;

            if (0 == mrg->sub.len) {
                *ezld_array_push(mrg->sub) = objsec;
                objsec->merged_idx         = next_idx;
                objsec->merged_sec         = mrg;
                objsec->transl_off         = 0;
                return;
            }

            ezld_obj_sec_t *last = mrg->sub.buf[next_idx - 1];

            if (objsec->shdr.sh_type != last->shdr.sh_type) {
                ezld_runtime_exit(EZLD_ECODE_BADSEC,
                                  "section '%s' in '%s' has conflicting type "
                                  "with '%s' sections in other files",
                                  objsec_name,
                                  objsec->o_file->path,
                                  objsec_name);
            }

            if (objsec->shdr.sh_flags != last->shdr.sh_flags) {
                ezld_runtime_exit(EZLD_ECODE_BADSEC,
                                  "section '%s' in '%s' has conflicting flags "
                                  "with '%s' sections in other files",
                                  objsec_name,
                                  objsec->o_file->path,
                                  objsec_name);
            }

            if (objsec->shdr.sh_addralign != last->shdr.sh_addralign) {
                ezld_runtime_exit(
                    EZLD_ECODE_BADSEC,
                    "section '%s' in '%s' has conflicting alignment "
                    "with '%s' sections in other files",
                    shstr_from_idx(objsec_name).str,
                    objsec->o_file->path,
                    shstr_from_idx(objsec_name).str);
            }

            size_t transl_off          = last->transl_off + last->shdr.sh_size;
            *ezld_array_push(mrg->sub) = objsec;
            objsec->merged_idx         = next_idx;
            objsec->merged_sec         = mrg;
            objsec->transl_off         = transl_off;
            return;
        }
    }

    size_t             next_mrg_idx = g_self->mrg_sec.len;
    ezld_merged_sec_t *new_mrg      = ezld_array_push(g_self->mrg_sec);
    new_mrg->name_idx               = objsec_name;
    new_mrg->index                  = next_mrg_idx;
    ezld_array_init(new_mrg->sub);
    objsec->merged_idx             = 0;
    objsec->merged_sec             = new_mrg;
    objsec->transl_off             = 0;
    *ezld_array_push(new_mrg->sub) = objsec;
}

static void merge_symtabs(ezld_obj_t *obj) {
    ezld_obj_sec_t *obj_symtab = obj->symtab.sec;

    if (!(SHF_INFO_LINK & obj_symtab->shdr.sh_flags)) {
        ezld_runtime_message(
            EZLD_EMSG_WARN,
            "section '%s' in '%s' is of type SHT_SYMTAB "
            "but is missing flag SHF_LINK_INFO",
            shstr_from_idx(obj_symtab->merged_sec->name_idx).str,
            obj->path);
    }

    ezld_obj_sec_t *strtab_sec = &obj->sections.buf[obj_symtab->shdr.sh_link];
    size_t num_entires = obj_symtab->shdr.sh_size / obj_symtab->shdr.sh_entsize;
    read_section_contents(strtab_sec);
    ezld_runtime_seek(obj_symtab->shdr.sh_offset, obj->path, obj->file);

    ezld_array_alloc(obj->symtab.syms, num_entires);

    for (size_t i = 0; i < num_entires; i++) {
        Elf32_Sym entry = {0};
        ezld_runtime_read_exact(
            &entry, sizeof(Elf32_Sym), obj->path, obj->file);

        ezld_obj_sym_t *obj_sym = ezld_array_push(obj->symtab.syms);
        obj_sym->sym            = entry;

        if (SHN_UNDEF == entry.st_shndx) {
            obj_sym->glob_idx = 0;
            continue;
        }

        ezld_obj_sec_t *sym_sec =
            &obj_symtab->o_file->sections.buf[entry.st_shndx];
        uint32_t glob_shidx   = sym_sec->merged_sec->index;
        uint32_t glob_sym_off = entry.st_value + sym_sec->transl_off;

        Elf32_Sym *glob_sym = ezld_array_push(g_self->glob_symtab);
        glob_sym->st_shndx  = glob_shidx;
        glob_sym->st_value  = glob_sym_off;
        glob_sym->st_name   = globstr_add(&strtab_sec->buf[entry.st_name]);
        glob_sym->st_size   = entry.st_size;
        glob_sym->st_info   = entry.st_info;

        // This starts at 1 to use 0 as NULL
        obj_sym->glob_idx = g_self->glob_symtab.len;

        if (NULL == g_self->entry_sym && EZLD_ENTRY_NAME == glob_sym->st_name) {
            g_self->entry_sym = obj_sym;
        }
    }
}

static void read_object(ezld_obj_t *obj) {
    ezld_array_init(obj->sections);
    obj->symtab.sec = NULL;
    ezld_array_init(obj->symtab.syms);

    Elf32_Ehdr ehdr = {0};
    ezld_runtime_read_exact(&ehdr, sizeof(Elf32_Ehdr), obj->path, obj->file);
    obj->ehdr = ehdr;

    if (ELFMAG0 != ehdr.e_ident[EI_MAG0] || ELFMAG1 != ehdr.e_ident[EI_MAG1] ||
        ELFMAG2 != ehdr.e_ident[EI_MAG2] || ELFMAG3 != ehdr.e_ident[EI_MAG3]) {
        ezld_runtime_exit(
            EZLD_ECODE_BADFILE, "'%s' is not an ELF file", obj->path);
    }

    if (ELFCLASS32 != ehdr.e_ident[EI_CLASS]) {
        ezld_runtime_exit(
            EZLD_ECODE_BADFILE, "'%s' is not a 32-bit ELF", obj->path);
    }

    if (ET_REL != ehdr.e_type) {
        ezld_runtime_exit(EZLD_ECODE_BADFILE,
                          "'%s' is not a relocatable object file",
                          obj->path);
    }

    Elf32_Shdr shstrtab_sh = {0};
    ezld_runtime_read_exact_at(&shstrtab_sh,
                               sizeof(Elf32_Shdr),
                               ehdr.e_shstrndx * sizeof(Elf32_Shdr) +
                                   ehdr.e_shoff,
                               obj->path,
                               obj->file);

    if (SHT_STRTAB != shstrtab_sh.sh_type) {
        ezld_runtime_message(
            EZLD_EMSG_WARN,
            "section header string section in '%s' is not of type SHT_STRTAB",
            obj->path);
    }

    char *shstrtab = (char *)ezld_runtime_alloc(1, shstrtab_sh.sh_size);
    ezld_runtime_read_exact_at(shstrtab,
                               shstrtab_sh.sh_size,
                               shstrtab_sh.sh_offset,
                               obj->path,
                               obj->file);

    ezld_runtime_seek(ehdr.e_shoff, obj->path, obj->file);
    for (size_t i = 0; i < ehdr.e_shnum; i++) {
        Elf32_Shdr shdr = {0};
        ezld_runtime_read_exact(
            &shdr, sizeof(Elf32_Shdr), obj->path, obj->file);

        ezld_obj_sec_t *objsec = ezld_array_push(obj->sections);
        objsec->o_file         = obj;
        objsec->shdr           = shdr;
        objsec->sec_elems      = shdr.sh_size;
        objsec->buf            = NULL;

        if (0 != shdr.sh_entsize) {
            objsec->sec_elems /= shdr.sh_entsize;
        }

        if (ehdr.e_shstrndx == i) {
            objsec->buf = shstrtab;
        }

        if (SHT_SYMTAB == shdr.sh_type) {
            if (NULL != obj->symtab.sec) {
                ezld_runtime_message(EZLD_EMSG_WARN,
                                     "object file '%s' has more than one "
                                     "section of type SHT_SYMTAB",
                                     obj->path);
            } else {
                obj->symtab.sec    = objsec;
                objsec->merged_sec = NULL;
                objsec->merged_idx = 0;
            }
            continue;
        }

        if (SHT_PROGBITS == shdr.sh_type || SHT_NOBITS == shdr.sh_type ||
            SHT_SYMTAB == shdr.sh_type) {
            const char *objsec_name = &shstrtab[shdr.sh_name];
            merge_section(objsec, shstr_add(objsec_name));
        }
    }

    merge_symtabs(obj);
}

static void read_objects(void) {
    for (size_t i = 0; i < g_self->o_files.len; i++) {
        read_object(&g_self->o_files.buf[i]);
    }
}

static void write_exec(FILE *out) {
    Elf32_Ehdr ehdr             = {0};
    ehdr.e_ident[EI_MAG0]       = ELFMAG0;
    ehdr.e_ident[EI_MAG1]       = ELFMAG1;
    ehdr.e_ident[EI_MAG2]       = ELFMAG2;
    ehdr.e_ident[EI_MAG3]       = ELFMAG3;
    ehdr.e_ident[EI_CLASS]      = ELFCLASS32;
    ehdr.e_ident[EI_DATA]       = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION]    = 1;
    ehdr.e_ident[EI_OSABI]      = ELFOSABI_SYSV;
    ehdr.e_ident[EI_ABIVERSION] = 1;

    ehdr.e_type    = ET_EXEC;
    ehdr.e_machine = EM_RISCV;
    ehdr.e_version = EV_CURRENT;

    if (NULL == g_self->entry_sym ||
        EZLD_GLOB_SYM_UNDEF == g_self->entry_sym->glob_idx) {
        ezld_runtime_message(EZLD_EMSG_WARN,
                             "could not resolve entry point symbol '%s', "
                             "defaulting to base of '.text' section",
                             g_self->entry_label);
        // TODO: actually default to that
    } else {
        Elf32_Sym gsym;
        resolve_sym(&gsym, g_self->entry_sym, 0);
        ehdr.e_entry = gsym.st_value;
    }

    // Header will be added later
    ezld_runtime_seek(sizeof(Elf32_Ehdr), "<output>", out);

    ehdr.e_phoff     = sizeof(Elf32_Ehdr);
    ehdr.e_phentsize = sizeof(Elf32_Phdr);
    ehdr.e_shnum     = g_self->mrg_sec.len + 2;
    ehdr.e_shentsize = sizeof(Elf32_Shdr);

    for (size_t i = 0; i < g_self->mrg_sec.len; i++) {
        ezld_merged_sec_t *mrg = &g_self->mrg_sec.buf[i];

        if (0 != mrg->sub.len && (SHF_ALLOC & mrg->sub.buf[0]->shdr.sh_flags)) {
            Elf32_Shdr base = mrg->sub.buf[0]->shdr;
            ehdr.e_phnum++;
            size_t sz = mrg->sub.buf[mrg->sub.len - 1]->transl_off +
                        mrg->sub.buf[mrg->sub.len - 1]->shdr.sh_size;
            Elf32_Phdr phdr = {0};
            phdr.p_align    = base.sh_addralign;
            phdr.p_vaddr    = mrg->virt_addr;
            phdr.p_paddr    = mrg->virt_addr;
            phdr.p_memsz    = sz;

            if (SHT_NOBITS != base.sh_type) {
                phdr.p_filesz = sz;
            }

            // TODO: set flags
            // TODO: actually do this well
        }
    }
}

static void align_sections(void) {
    for (size_t i = 0; i < g_self->mrg_sec.len; i++) {
        ezld_merged_sec_t *mrg      = &g_self->mrg_sec.buf[i];
        const char        *sec_name = shstr_from_idx(mrg->name_idx).str;

        if (0 == mrg->sub.len) {
            ezld_runtime_message(EZLD_EMSG_WARN, "section '%s' is empty");
            continue;
        }

        if (0 == mrg->virt_addr) {
            ezld_runtime_message(EZLD_EMSG_WARN,
                                 "section '%s' has virtual address %08x",
                                 sec_name,
                                 0);
        } else if (0 != mrg->virt_addr % mrg->sub.buf[0]->shdr.sh_addralign) {
            size_t aligned_virt =
                mrg->virt_addr +
                (mrg->virt_addr % mrg->sub.buf[0]->shdr.sh_addralign);
            ezld_runtime_message(
                EZLD_EMSG_WARN,
                "section '%s' has misaligned virtual address %08x (requires "
                "alignment of %u byte(s)), changing to %08x",
                mrg->virt_addr,
                mrg->sub.buf[0]->shdr.sh_addralign,
                aligned_virt);
            mrg->virt_addr = aligned_virt;
        }

        if (i > 0) {
            ezld_merged_sec_t *prev_mrg = &g_self->mrg_sec.buf[i - 1];
            size_t             prev_mrg_sz =
                prev_mrg->sub.buf[prev_mrg->sub.len - 1]->transl_off +
                prev_mrg->sub.buf[prev_mrg->sub.len - 1]->shdr.sh_size;

            if (mrg->virt_addr < prev_mrg->virt_addr + prev_mrg_sz) {
                size_t diff =
                    prev_mrg->virt_addr + prev_mrg_sz - mrg->virt_addr;
                size_t old_virt = mrg->virt_addr;
                mrg->virt_addr += diff;
                mrg->virt_addr +=
                    mrg->virt_addr % mrg->sub.buf[0]->shdr.sh_addralign;

                ezld_runtime_message(EZLD_EMSG_WARN,
                                     "section '%s' has overlapping virtual "
                                     "address %08x, changing to %08x",
                                     old_virt,
                                     mrg->virt_addr);
            }
        }
    }
}

void virtualize_syms(void) {
    for (size_t i = 0; i < g_self->glob_symtab.len; i++) {
        Elf32_Sym         *sym = &g_self->glob_symtab.buf[i];
        ezld_merged_sec_t *sec = &g_self->mrg_sec.buf[sym->st_shndx];
        sym->st_value += sec->virt_addr;
    }
}

void ezld_link(ezld_instance_t *instance, FILE *output_file) {
    ezld_array_init(instance->mrg_sec);
    ezld_array_init(instance->glob_symtab);
    ezld_array_init(instance->glob_strtab);
    ezld_array_init(instance->sh_strtab);

    instance->entry_sym = NULL;
    g_self              = instance;
    globstr_add(instance->entry_label);

    for (size_t i = 0; i < instance->config.sections.len; i++) {
        ezld_sec_cfg_t     sec_cfg = instance->config.sections.buf[i];
        ezld_merged_sec_t *mrg     = ezld_array_push(instance->mrg_sec);
        mrg->index                 = instance->mrg_sec.len - 1;
        mrg->virt_addr             = sec_cfg.virt_addr;
        mrg->name_idx              = shstr_add(sec_cfg.name);
        ezld_array_init(mrg->sub);
    }

    read_objects();
    align_sections();
    virtualize_syms();

    for (size_t i = 0; i < g_self->mrg_sec.len; i++) {
        ezld_merged_sec_t *m = &g_self->mrg_sec.buf[i];
        fprintf(stderr,
                "merged section: %s (%zu entries, virt addr: %08lx)\n",
                shstr_from_idx(m->name_idx).str,
                m->sub.len,
                m->virt_addr);

        for (size_t j = 0; j < m->sub.len; j++) {
            ezld_obj_sec_t *s = m->sub.buf[j];
            fprintf(stderr, "\t SEC: %p ", s);
            fprintf(stderr,
                    "%s (elems: %zu, transl: %zu)\n",
                    s->o_file->path,
                    s->sec_elems,
                    s->transl_off);
        }
    }

    for (size_t i = 0; i < g_self->glob_symtab.len; i++) {
        Elf32_Sym s = g_self->glob_symtab.buf[i];
        fprintf(stderr,
                "SYM: %s = %x\n",
                globstr_from_idx(s.st_name).str,
                s.st_value);
    }

    write_exec(output_file);
}
