# ezld
[contributors-shield]: https://img.shields.io/github/contributors/Alessandro-Salerno/ezld.svg?style=flat-square
[contributors-url]: https://github.com/Alessandro-Salerno/ezld/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/Alessandro-Salerno/ezld.svg?style=flat-square
[forks-url]: https://github.com/Alessandro-Salerno/ezld/network/members
[stars-shield]: https://img.shields.io/github/stars/Alessandro-Salerno/ezld.svg?style=flat-square
[stars-url]: https://github.com/Alessandro-Salerno/ezld/stargazers
[issues-shield]: https://img.shields.io/github/issues/Alessandro-Salerno/ezld.svg?style=flat-square
[issues-url]: https://github.com/Alessandro-Salerno/ezld/issues
[license-shield]: https://img.shields.io/github/license/Alessandro-Salerno/ezld.svg?style=flat-square
[license-url]: https://github.com/Alessandro-Salerno/ezld/blob/master/LICENSE.txt

[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]
![](https://tokei.rs/b1/github/Alessandro-Salerno/ezld)

Tiny, simple, and portable static [Linker](https://en.wikipedia.org/wiki/Linker_(computing)) for the [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) format.

## How to use ezld
ezld features a small but functional CLI:
```
ezld by Alessandro Salerno
The EZ ELF Linker

Usage: ezld [<command>] [<options>] [<input files>]

COMMANDS:
    -h, --help       show this menu
    -v, --version    show version information

OPTIONS:
    -e, --entry-sym    set the entry point symbol (default: '_start')
    -s, --section      set the base virtual address for a given section (example: -s .text=0x4000)
    -a, --align        set the alignent of PT_LOAD segments (example: -a 0x1000)
    -o, --output       set the output file path (default: 'a.out')
```

## Design and portability
ezld is designed to be used as a tinkering playground for low-level newcomers as part of the [rarsjs]() project. This project is designed to be host-agnostic and portable: even though the linker targets ELF binaries (traditionally used in UNIX-Like systems), it can also be used on
Windows and the lack of non-standard libc dependencies makes it easy to port to any platform. In fact, ezld also runs on [SalernOS](https://github.com/Alessandro-Salerno/SalernOS-Kernel) using the [fresh CRT](https://githuh.com/Alessandro-Salerno/fresh) and can even be used as a library.

## Hot to build ezld
Using the included Makefile:
```bash
make debug   # Build with debug information, sanitiziers, -O0, etc.
make release # Build optimized bundle
```

## License and Copyright
This project contains redistributions of [musl libc](https://musl.libc.org/) and [tarman](https://github.com/Alessandro-Salerno/tarman). The source code for ezld is distributed under the MIT icense (see [LICENSE](LICENSE) for details) as is the source code taken from other projects, however
these may feature divergent copyright holders.

