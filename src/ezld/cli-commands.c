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

#include <ctype.h>
#include <ezld/cli-commands.h>
#include <ezld/linker.h>
#include <ezld/runtime.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#define STR_HELPER(x) #x
#define STR(x)        STR_HELPER(x)

#ifndef EXT_EZLD_BUILD
#define EXT_EZLD_BUILD "unknown"
#endif

#ifndef EXT_EZLD_OS
#define EXT_EZLD_OS "unknown"
#endif

#ifndef EXT_EZLD_COMPILER
#if defined(__GNUC__) && defined(__GNUC_MINOR__) &&        \
    defined(__GNUC_PATCHLEVEL__) && !defined(__clang__) && \
    !defined(__llvm__) && !defined(__INTEL_COMPILER)
#define EXT_EZLD_COMPILER \
    "gcc " STR(__GNUC__) "." STR(__GNUC_MINOR__) "." STR(__GNUC_PATCHLEVEL__)
#elif defined(__clang_minor__) && defined(__clang_major__)
#define EXT_EZLD_COMPILER "clang " STR(__clang_major__) "." STR(__clang_minor__)
#elif defined(__clang__)
#define EXT_EZLD_COMPILER "clang (unknown version)"
#elif defined(__llvm__)
#define EXT_EZLD_COMPILER "generic llvm"
#elif defined(_MSC_VER)
#define EXT_EZLD_COMPILER "msc " _MSC_VER
#else
#define EXT_EZLD_COMPILER "unknown"
#endif
#endif // EXT_EZLD_COMPILER

static size_t parse_digit(char digit, size_t base, size_t pos) {
    size_t mult  = pow(base, pos);
    size_t value = digit - '0';

    if (!isalpha(digit)) {
        if (digit < '0' || digit > ('0' + (char)base)) {
            ezld_runtime_exit(
                EZLD_ECODE_BADPARAM,
                "'%c' is not a valid input for number in base %zu",
                digit,
                base);
        }

        return value * mult;
    }

    char start = isupper(digit) ? 'A' : 'a';

    if (digit > (start + (char)base)) {
        ezld_runtime_exit(EZLD_ECODE_BADPARAM,
                          "'%c' is not a valid input for number in base %zu",
                          digit,
                          base);
    }

    value = digit - start;
    return value * mult;
}

static size_t parse_number(const char *str) {
    size_t base  = 10;
    size_t len   = strlen(str);
    size_t start = 0;
    size_t value = 0;

    if ('0' == str[0]) {
        start = 2;
        switch (tolower(str[1])) {
        case 'x':
            base = 16;
            break;
        case 'b':
            base = 2;
            break;
        default:
            start = 0;
            break;
        }
    }

    if (start >= len) {
        return 0;
    }

    for (size_t i = len - 1; start <= i; i--) {
        size_t pos = len - i - 1;
        value += parse_digit(str[i], base, pos);
    }

    return value;
}

static void parse_assignment(const char *str, char **key, char **value) {
    for (size_t i = 0; str[i]; i++) {
        if ('=' == str[i]) {
            *(char *)&str[i] = '\0';
            *key             = (char *)str;
            *value           = (char *)&str[i + 1];
            return;
        }
    }

    ezld_runtime_exit(EZLD_ECODE_BADPARAM, "expected '=' in '%s'", str);
}

void ezld_clicmd_version(ezld_config_t config) {
    (void)config;

    puts("ezld version " EXT_EZLD_BUILD);
    puts("target: " EXT_EZLD_OS);
    puts("compiled with: " EXT_EZLD_COMPILER);
}

void ezld_clicmd_entrysym(ezld_config_t *config, const char *next) {
    config->cfg_entrysym = next;
}

void ezld_clicmd_section(ezld_config_t *config, const char *next) {
    char *key = NULL, *value = NULL;
    parse_assignment(next, &key, &value);

    for (size_t i = 0; i < config->cfg_sections.len; i++) {
        if (0 == strcmp(config->cfg_sections.buf[i].sc_name, key)) {
            config->cfg_sections.buf[i].sc_vaddr = parse_number(value);
            return;
        }
    }

    ezld_sec_cfg_t *s = ezld_array_push(config->cfg_sections);
    s->sc_name        = key;
    s->sc_vaddr       = parse_number(value);
}

void ezld_clicmd_align(ezld_config_t *config, const char *next) {
    config->cfg_segalign = parse_number(next);
}

void ezld_clicmd_output(ezld_config_t *config, const char *next) {
    config->cfg_outpath = next;
}
