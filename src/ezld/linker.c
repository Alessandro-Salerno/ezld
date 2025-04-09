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
#include <string.h>

// This can be global
static ezld_instance_t *g_self = NULL;

static void merge_section(ezld_obj_sec_t *objsec, const char *objsec_name) {
    for (size_t i = 0; i < g_self->mrg_sec.len; i++) {
        ezld_merged_sec_t *mrg = &g_self->mrg_sec.buf[i];

        if (0 == strcmp(mrg->name, objsec_name)) {
            size_t          next_idx = mrg->sub.len;
            ezld_obj_sec_t *last     = mrg->sub.buf[next_idx - 1];

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
                    objsec_name,
                    objsec->o_file->path,
                    objsec_name);
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
    new_mrg->name                   = objsec_name;
    new_mrg->index                  = next_mrg_idx;
    ezld_array_init(new_mrg->sub);
    objsec->merged_idx   = 0;
    objsec->merged_sec   = new_mrg;
    objsec->transl_off   = 0;
    ezld_obj_sec_t **new = ezld_array_push(new_mrg->sub);
    fprintf(stderr,
            "adding section %p to array %p in position %zu with addr %p\n",
            objsec,
            new_mrg->sub.buf,
            next_mrg_idx,
            new);
    *new = objsec;
}

static void merge_symtabs(ezld_obj_t *obj) {
    ezld_obj_sec_t *obj_symtab = obj->symtab.sec;

    if (!(SHF_INFO_LINK & obj_symtab->shdr.sh_flags)) {
        ezld_runtime_message(EZLD_EMSG_WARN,
                             "section '%s' in '%s' is of type SHT_SYMTAB "
                             "but is missing flag SHF_LINK_INFO",
                             obj_symtab->merged_sec->name,
                             obj->path);
    }

    ezld_obj_sec_t *strtab_sec = &obj->sections.buf[obj_symtab->shdr.sh_link];
    size_t          num_entires =
        obj_symtab->shdr.sh_size / obj_symtab->shdr.sh_entsize -
        obj_symtab->shdr.sh_info;
    size_t skip_bytes = obj_symtab->shdr.sh_info * obj_symtab->shdr.sh_entsize;
    ezld_runtime_seek(
        obj_symtab->shdr.sh_offset + skip_bytes, obj->path, obj->file);

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

        // Adding 1 for NULL entry
        obj_sym->glob_idx           = g_self->glob_symtab.len + 1;
        ezld_global_sym_t *glob_sym = ezld_array_push(g_self->glob_symtab);
        glob_sym->sym.st_shndx      = glob_shidx;
        glob_sym->sym.st_value      = glob_sym_off;
        glob_sym->sym.st_name       = entry.st_name;
        glob_sym->sym.st_size       = entry.st_size;
        glob_sym->sym.st_info       = entry.st_info;
        glob_sym->strtab            = strtab_sec;
    }
}

static void read_object(ezld_obj_t *obj) {
    ezld_array_init(obj->sections);
    obj->symtab.sec = NULL;
    ezld_array_init(obj->symtab.syms);

    Elf32_Ehdr ehdr = {0};
    ezld_runtime_read_exact(&ehdr, sizeof(Elf32_Ehdr), obj->path, obj->file);

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

        // Cannot skip these here because indicies may be broken
        /*if (SHT_PROGBITS != shdr.sh_type && SHT_NOBITS != shdr.sh_type &&
            SHT_STRTAB != shdr.sh_type && SHT_SYMTAB != shdr.sh_type) {
            continue;
        }*/

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

        const char *objsec_name = &shstrtab[shdr.sh_name];
        merge_section(objsec, objsec_name);
    }

    merge_symtabs(obj);
}

static void read_objects(void) {
    for (size_t i = 0; i < g_self->o_files.len; i++) {
        read_object(&g_self->o_files.buf[i]);
    }
}

void ezld_link(ezld_instance_t *instance, FILE *output_file) {
    g_self = instance;
    read_objects();

    for (size_t i = 0; i < g_self->mrg_sec.len; i++) {
        ezld_merged_sec_t *m = &g_self->mrg_sec.buf[i];
        fprintf(stderr, "merged section: %s\n", m->name);

        for (size_t j = 0; j < m->sub.len; j++) {
            ezld_obj_sec_t *s = m->sub.buf[i];
            fprintf(stderr, "\t SEC: %p ", s);
            fprintf(stderr,
                    "%s (elems: %zu, transl: %zu)\n",
                    s->o_file->path,
                    s->sec_elems,
                    s->transl_off);
        }
    }

    for (size_t i = 0; i < g_self->glob_symtab.len; i++) {
        ezld_global_sym_t *s = &g_self->glob_symtab.buf[i];
        fprintf(
            stderr, "SYM: %s = %u\n", s->strtab->o_file->path, s->sym.st_value);
    }
}
