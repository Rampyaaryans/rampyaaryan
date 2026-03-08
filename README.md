# Rampyaaryan

<div align="center">

<img src="docs/logo.png" alt="Rampyaaryan" width="120">

### **India's First Hinglish Programming Language**

[![License: Rampyaaryan](https://img.shields.io/badge/License-Rampyaaryan-red.svg)](LICENSE)
[![Language: C99](https://img.shields.io/badge/Built%20with-C99-blue.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-green.svg)]()
[![Functions](https://img.shields.io/badge/Built--in%20Functions-145-orange.svg)](https://rampyaaryans.github.io/rampyaaryan/api.html)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://rampyaaryans.github.io/rampyaaryan/contributing.html)
[![VS Code Extension](https://img.shields.io/badge/VS%20Code-Extension-blueviolet.svg)](https://marketplace.visualstudio.com/items?itemName=Rampyaaryans.rampyaaryan)

**Write code in the language you think in. No Python. No Node.js. No dependencies. Just one native binary.**

[Documentation](https://rampyaaryans.github.io/rampyaaryan/) | [Getting Started](https://rampyaaryans.github.io/rampyaaryan/getting-started.html) | [API Reference](https://rampyaaryans.github.io/rampyaaryan/api.html) | [Examples](https://rampyaaryans.github.io/rampyaaryan/examples.html)

</div>

---

```
likho "Namaste Duniya!"

maano naam = pucho("Aapka naam: ")
likho "Swagat hai, " + naam + " ji!"

kaam factorial(n) {
    agar n <= 1 { wapas do 1 }
    wapas do n * factorial(n - 1)
}

likho format("10! = {}", factorial(10))
```

---

## Features

| Category | Highlights |
|----------|-----------|
| **Pure C (C99)** | Compiles to a single native binary -- zero runtime dependencies |
| **Hinglish Syntax** | Keywords in Hindi-English -- `maano`, `likho`, `agar`, `jab tak`, `kaam` |
| **Bytecode VM** | Stack-based virtual machine with 44 opcodes for fast execution |
| **Garbage Collector** | Tri-color mark-sweep GC with automatic heap management |
| **Closures** | Full lexical scoping with upvalue capture |
| **Dynamic Lists** | 23 operations (sort, slice, search, stats, flatten, chunk) |
| **Rich Strings** | 27 string functions -- split, join, format, replace, title_case, pad |
| **Math & Science** | 37 functions: trig, log, factorial, prime, Fibonacci, random |
| **File I/O** | Read, write, append files -- build real applications |
| **Dictionaries** | `{key: value}` syntax with 9 map functions |
| **Bitwise Operators** | AND `&`, OR `\|`, XOR `^`, NOT `~`, shifts `<<` `>>` |
| **Higher-Order** | `naksha`, `chhaano`, `ikkatha`, `sab`, `koi`, `jodi_banao` |
| **Date & Time** | 7 functions for date, time, day of week |
| **145 Built-in Functions** | Covers strings, math, lists, maps, files, types, date/time |
| **Interactive REPL** | Multiline editing, syntax colors, command history |
| **Cross-Platform** | Windows, Linux, macOS -- one codebase, three platforms |

---

## Architecture

<div align="center">

**Compilation Pipeline**

<img src="docs/architecture-pipeline.svg" alt="Architecture Pipeline" width="700">

**VM Architecture**

<img src="docs/vm-architecture.svg" alt="VM Architecture" width="700">

**Object Hierarchy**

<img src="docs/object-hierarchy.svg" alt="Object Hierarchy" width="700">

</div>

---

## Quick Start

### Download Binary
Download the latest release for your platform from [Releases](https://github.com/Rampyaaryans/rampyaaryan/releases).

### Build from Source

```bash
git clone https://github.com/Rampyaaryans/rampyaaryan.git
cd rampyaaryan
make
```

### Run

```bash
# Run a program
rampyaaryan examples/01_namaste_duniya.ram

# Start interactive REPL
rampyaaryan
```

---

## Documentation

Full documentation is available at **[rampyaaryans.github.io/rampyaaryan](https://rampyaaryans.github.io/rampyaaryan/)**

| Page | Description |
|------|-------------|
| [Home](https://rampyaaryans.github.io/rampyaaryan/) | Overview and quick start |
| [Getting Started](https://rampyaaryans.github.io/rampyaaryan/getting-started.html) | Installation and first program |
| [Tutorial](https://rampyaaryans.github.io/rampyaaryan/tutorial.html) | 19 lessons from basics to advanced |
| [API Reference](https://rampyaaryans.github.io/rampyaaryan/api.html) | All 145 built-in functions |
| [Language Spec](https://rampyaaryans.github.io/rampyaaryan/spec.html) | Formal grammar and syntax rules |
| [Examples](https://rampyaaryans.github.io/rampyaaryan/examples.html) | 20 ready-to-run programs |
| [How-To Guide](https://rampyaaryans.github.io/rampyaaryan/howto.html) | Complete cookbook |
| [Changelog](https://rampyaaryans.github.io/rampyaaryan/changelog.html) | Version history |
| [Contributing](https://rampyaaryans.github.io/rampyaaryan/contributing.html) | How to contribute |

---

## Project Structure

```
rampyaaryan/
├── src/              -- C99 source (compiler, VM, GC, 145 native functions)
├── examples/         -- 20 example .ram programs
├── docs/             -- GitHub Pages documentation website
├── vscode-extension/ -- VS Code extension (themes, WebNeko, Hindi UI)
├── scripts/          -- Install scripts
├── Makefile          -- GNU Make build
├── CMakeLists.txt    -- CMake build
├── LICENSE           -- Rampyaaryan License
└── README.md         -- This file
```

---

## Contributing

Contributions welcome! See the [Contributing Guide](https://rampyaaryans.github.io/rampyaaryan/contributing.html).

---

## License

Rampyaaryan License -- See [LICENSE](LICENSE)

---

## Inspiration

- [Crafting Interpreters](https://craftinginterpreters.com/) by Robert Nystrom
- Python, JavaScript, and Lua

---

<div align="center">

**Made in India | Built with Pride**

**[Star this repo](https://github.com/Rampyaaryans/rampyaaryan)** if you like it!

</div>
