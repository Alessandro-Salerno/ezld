> [!IMPORTANT]
> The official ezld repository is https://github.com/Alessandro-Salerno/ezld. If you're seeing this on any other website (like github-zh.com), switch to the official repository! I have **never** uploaded this project to other source hosting services.

# ⚙️ ezld — Tiny Static ELF Linker

[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]
![Lines of Code](https://tokei.rs/b1/github/Alessandro-Salerno/ezld)

> A tiny, simple, and portable static [Linker](https://en.wikipedia.org/wiki/Linker_(computing)) for the [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) format.

---

## 🛠 Features
- Command-line interface for direct usage  
- No OS-specific libc dependency (portable across platforms)  
- Supports section alignment, entry point overrides, and more  
- Usable as a library or standalone binary  
- Designed with simplicity and clarity in mind

---

## 🚀 Usage

```
ezld by Alessandro Salerno
The EZ ELF Linker

Usage: ezld [<command>] [<options>] [<input files>]

COMMANDS:
    -h, --help         Show this menu
    -v, --version      Show version information

OPTIONS:
    -e, --entry-sym    Set the entry point symbol (default: '_start')
    -s, --section      Set base virtual address for section (e.g., -s .text=0x4000)
    -a, --align        Set PT_LOAD segment alignment (e.g., -a 0x1000)
    -o, --output       Set output file path (default: 'a.out')
```

---

## ⚙️ Building from Source

Use the included `Makefile` to build:

```bash
make debug    # Build with debug info, sanitizers, -O0
make release  # Build optimized release version
make library  # Build as a static library
```

---

## 📦 Installing via tarman

Install with [`tarman`](https://github.com/Alessandro-Salerno/tarman) from the [tarman user repository](https://github.com/Alessandro-Salerno/tarman-user-repository):

```bash
tarman install -r ezld
```

---

## 🌍 Portability

> 💡 **ezld is platform-agnostic.**

Though it links **ELF binaries**, it's designed to run on any host, including:
- 🪟 Windows
- 🐧 Linux
- 🍎 macOS
- 🌈 [SalernOS](https://github.com/Alessandro-Salerno/SalernOS-Kernel)

ezld avoids non-standard libc functions, improving portability and making it suitable for low-level projects, OS userspace testing, or educational purposes.

---

## 👨‍💻 Project Context

ezld was created as part of the [`rarsjs`](https://github.com/ldlaur/rarsjs) project. It’s meant to be a **learning tool** and **experimentation ground** for understanding the inner workings of static linking and ELF formats.

---

## ⚖️ License

This project includes code from:
- [musl libc](https://musl.libc.org/)
- [tarman](https://github.com/Alessandro-Salerno/tarman)

ezld is licensed under the [MIT License](LICENSE). Third-party code may have distinct copyright.

---

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
