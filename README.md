# ðŸ™ Rampyaaryan

<div align="center">

### **à¤ªà¤¹à¤²à¥€ à¤¹à¤¿à¤‚à¤—à¥à¤²à¤¿à¤¶ à¤ªà¥à¤°à¥‹à¤—à¥à¤°à¤¾à¤®à¤¿à¤‚à¤— à¤­à¤¾à¤·à¤¾ â€” India's First Hinglish Programming Language**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Language: C99](https://img.shields.io/badge/Built%20with-C99-blue.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-green.svg)]()
[![Functions](https://img.shields.io/badge/Built--in%20Functions-145-orange.svg)](docs/API_REFERENCE.md)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](docs/CONTRIBUTING.md)

**Write code in the language you think in. No Python. No Node.js. No dependencies. Just one native binary.**

[Getting Started](docs/HOWTO.md) Â· [Tutorial](docs/TUTORIAL.md) Â· [API Reference](docs/API_REFERENCE.md) Â· [Examples](examples/)

</div>

---

```
likho "ðŸ™ Namaste Duniya!"

maano naam = poocho("Aapka naam: ")
likho "Swagat hai, " + naam + " ji!"

kaam factorial(n) {
    agar n <= 1 { wapas do 1 }
    wapas do n * factorial(n - 1)
}

likho format("10! = {}", factorial(10))
```

---

## âš¡ Features

| Category | Highlights |
|----------|-----------|
| ðŸ—ï¸ **Pure C (C99)** | Compiles to a single native binary â€” zero runtime dependencies |
| ðŸ‡®ðŸ‡³ **Hinglish Syntax** | Keywords in Hindi-English â€” `maano`, `likho`, `agar`, `jabtak`, `kaam` |
| âš™ï¸ **Bytecode VM** | Stack-based virtual machine with 44 opcodes for fast execution |
| ðŸ§¹ **Garbage Collector** | Tri-color mark-sweep GC with automatic heap management |
| ðŸ”— **Closures & First-Class Functions** | Full lexical scoping with upvalue capture |
| ðŸ“‹ **Dynamic Lists** | Built-in arrays with 23 operations (sort, slice, search, stats, flatten, chunk) |
| ðŸ”¤ **Rich String Library** | 27 string functions â€” split, join, format, replace, trim, title_case, pad |
| ðŸ§® **Math & Science** | 37 functions: trig, log, factorial, prime, Fibonacci, random |
| ðŸ“ **File I/O** | Read, write, append files â€” build real applications |
| 📖 **Dictionaries/Maps** | Key-value `{key: value}` syntax with 9 map functions |
| 🔢 **Bitwise Operators** | AND `&`, OR `|`, XOR `^`, NOT `~`, shifts `<<` `>>` |
| 🔄 **Higher-Order Functions** | `naksha`, `chhaano`, `ikkatha`, `sab`, `koi`, `jodi_banao` |
| 📅 **Date & Time** | `din`, `mahina`, `saal`, `ghanta`, `minute`, `second`, `hafta_din` |
| 🔀 **Base Conversion** | `hex_shabd`, `oct_shabd`, `bin_shabd` |
| ðŸŽ¯ **145 Built-in Functions** | Covers strings, math, lists, maps, files, types, date/time, system  |
| ðŸ’» **Interactive REPL** | Multiline editing, syntax colors, command history |
| ðŸŒ **Cross-Platform** | Windows, Linux, macOS â€” one codebase, three platforms |
| ðŸŽ¨ **ASCII Art & Animations** | Colorful splash screen, progress bars, styled errors |

---

## ðŸš€ Quick Start

### Option 1: Download Binary (Easiest)
Download the latest release for your platform from [Releases](https://github.com/Rampyaaryans/rampyaaryan/releases).

### Option 2: Build from Source

**Requirements:** A C compiler (GCC, Clang, or MSVC)

```bash
# Clone
git clone https://github.com/Rampyaaryans/rampyaaryan.git
cd rampyaaryan

# Build (pick one)
make                              # GNU Make
gcc -O2 -o rampyaaryan src/*.c -lm  # Direct GCC
```

<details>
<summary><b>Windows (MinGW/MSYS2)</b></summary>

```cmd
gcc -O2 -o rampyaaryan.exe src\memory.c src\value.c src\object.c src\table.c src\chunk.c src\lexer.c src\compiler.c src\vm.c src\native.c src\debug.c src\ascii_art.c src\main.c -lm
```
</details>

<details>
<summary><b>CMake</b></summary>

```bash
mkdir build && cd build
cmake ..
cmake --build .
```
</details>

### Run a Program
```bash
rampyaaryan examples/01_namaste_duniya.ram
```

### Interactive REPL
```bash
rampyaaryan
```

---

## ðŸ“– Language Guide

### Variables â€” `maano`
```
maano naam = "Rampyaaryan"
maano umar = 25
maano active = sach
maano data = khali
```

### Print â€” `likho`
```
likho "Namaste!"
likho "Sum:", 2 + 3
likho "Naam:", naam, "Umar:", umar
```

### Input â€” `pucho`
```
maano naam = pucho("Aapka naam: ")
maano umar = sankhya(pucho("Aapki umar: "))
```

### Conditions â€” `agar`, `warna agar`, `warna`
```
agar umar >= 18 {
    likho "Vote de sakte ho"
} warna agar umar >= 13 {
    likho "Teenager ho"
} warna {
    likho "Chote ho"
}
```

### While Loop â€” `jabtak`
```
maano i = 0
jabtak i < 5 {
    likho i
    i = i + 1
}
```

### For Loop â€” `har`
```
har i = 1; i <= 10; i = i + 1 {
    likho i
}
```

### Functions â€” `kaam`, `wapas do`
```
kaam namaste(naam) {
    likho "Namaste, " + naam
}

kaam jodo(a, b) {
    wapas do a + b
}

namaste("Duniya")
likho jodo(5, 3)
```

### Lists
```
maano fruits = ["seb", "kela", "aam"]
joodo(fruits, "santara")
likho fruits[0]
likho lambai(fruits)
```
### Maps / Dictionaries — `shabdkosh`
```
maano student = {"naam": "Aryan", "umar": 20, "active": sach}
likho student["naam"]
likho chabi(student)     # ["naam", "umar", "active"]
likho mulya(student)     # ["Aryan", 20, sach]
```
### Strings
```
maano msg = "Namaste" + " " + "Duniya"
likho lambai(msg)
likho bade_akshar(msg)
likho kato(msg, 0, 7)
```

### Break & Continue â€” `ruko`, `agla`
```
har i = 0; i < 10; i = i + 1 {
    agar i == 5 { ruko }
    agar i % 2 == 0 { agla }
    likho i
}
```

---

## ðŸ—ï¸ Keyword Reference

| Rampyaaryan | English | Usage |
|---|---|---|
| `maano` | let/var | Variable declaration |
| `likho` | print | Output to console |
| `poocho` | input | Read from user |
| `agar` | if | Condition |
| `warna agar` | else if | Else-if branch |
| `warna` | else | Else branch |
| `jabtak` | while | While loop |
| `har` | for | For loop |
| `kaam` | function | Function definition |
| `wapas do` | return | Return value |
| `ruko` | break | Break loop |
| `agla` | continue | Skip iteration |
| `sach` | true | Boolean true |
| `jhooth` | false | Boolean false |
| `khali` | null/none | Null value |
| `aur` | and | Logical AND |
| `ya` | or | Logical OR |
| `nahi` | not | Logical NOT |

---

## 🛠️ Built-in Functions (145)

> Full documentation: **[API Reference](docs/API_REFERENCE.md)**

### Type Conversion & Checking (16)
| Function | Description |
|---|---|
| `prakar(x)` / `typeof_val(x)` / `print_type(x)` | Type name / typeof / print type |
| `sankhya(x)` / `dashmlav(x)` / `purn(x)` / `bool_val(x)` | Convert to number/float/int/bool |
| `shabd(x)` | Convert to string |
| `kya_sankhya(x)` / `kya_shabd(x)` / `kya_suchi(x)` | Type checking |
| `kya_kaam(x)` / `kya_bool(x)` / `kya_khali(x)` / `kya_purn(x)` / `kya_map(x)` | Type checking |

### String Functions (27)
| Function | Description |
|---|---|
| `lambai(s)` | Length of string/list |
| `bade_akshar(s)` / `chhote_akshar(s)` | Case conversion |
| `title_case(s)` / `capitalize(s)` / `swapcase(s)` | Title Case / Capitalize / Swap case |
| `kato(s, start, end)` | Substring/slice |
| `dhundho(s, sub)` / `badlo(s, old, new)` | Find / Replace |
| `todo(s, sep)` / `jodo_shabd(list, sep)` | Split / Join |
| `saaf(s)` / `center(s, width, [char])` | Trim / Center |
| `pad_left(s, width, [char])` / `pad_right(s, width, [char])` | Left/Right padding |
| `shuru_se(s, prefix)` / `ant_se(s, suffix)` | Starts/Ends with |
| `kya_ank(s)` / `kya_akshar(s)` / `kya_alnum(s)` / `kya_space(s)` | String checks |
| `dohrao(s, n)` / `akshar(s, i)` | Repeat / Char at index |
| `ascii_code(c)` / `ascii_se(n)` | ASCII conversion |
| `format(template, ...)` / `gino(s, sub)` | Format / Count |

### List Functions (23)
| Function | Description |
|---|---|
| `joodo(list, item)` / `nikalo(list, idx)` | Append / Remove |
| `kram(list)` / `ulta(x)` | Sort / Reverse |
| `suchi(...)` / `range_(start, end, step)` | Create list / Range |
| `shamil(list, val)` / `index_of(list, val)` | Contains / Index of |
| `katao(list, start, end)` | Slice |
| `pahla(list)` / `aakhri(list)` | First / Last element |
| `milao(a, b)` / `daalo(list, idx, val)` | Concat / Insert |
| `anokha(list)` | Unique elements |
| `jod(list)` / `ausat(list)` | Sum / Average |
| `sabse_bada(list)` / `sabse_chhota(list)` | Max / Min |
| `flatten(list)` / `tukda(list, size)` | Flatten nested / Chunk into groups |
| `ghuma(list, n)` / `copy_suchi(list)` / `khali_karo(list)` | Rotate / Copy / Clear |

### Higher-Order Functions (7)
| Function | Description |
|---|---|
| `naksha(list, fn)` / `chhaano(list, fn)` | Map / Filter |
| `ikkatha(list, fn, init)` | Reduce/Fold |
| `sab(list)` / `koi(list)` | All truthy / Any truthy |
| `jodi_banao(a, b)` / `ginati_banao(list)` | Zip / Enumerate |

### Map/Dictionary Functions (9)
| Function | Description |
|---|---|
| `shabdkosh()` | Create empty map |
| `chabi(map)` / `mulya(map)` / `jodi(map)` | Keys / Values / Entries |
| `map_hai(map, key)` / `map_get(map, key, default)` | Has key / Get with default |
| `map_hatao(map, key)` | Delete key |
| `map_milao(map1, map2)` / `map_lambai(map)` | Merge / Size |

### Math & Science (37)
| Function | Description |
|---|---|
| `abs_val(x)` / `gol(x)` / `upar(x)` / `neeche(x)` | Abs / Round / Ceil / Floor |
| `sqrt_val(x)` / `power_val(b, e)` / `hypot_val(a, b)` | Sqrt / Power / Hypotenuse |
| `sin_val` / `cos_val` / `tan_val` | Trigonometry |
| `asin_val` / `acos_val` / `atan_val` | Inverse trig |
| `log_val(x)` / `log10_val(x)` / `log2_val(x)` / `exp_val(x)` | Logarithms |
| `PI()` / `E()` / `INF()` / `NAN_VAL()` | Math constants |
| `gcd(a, b)` / `lcm(a, b)` | Number theory |
| `factorial(n)` / `kya_prime(n)` / `fib(n)` | Factorial / Prime check / Fibonacci |
| `sign(x)` / `clamp(v, min, max)` | Sign / Clamp |
| `degrees(r)` / `radians(d)` | Unit conversion |
| `is_nan(x)` / `is_inf(x)` | NaN/Infinity check |
| `random_choice(list)` / `random_shuffle(list)` / `random_int(min, max)` | Random ops |

### Base Conversion (3)
| Function | Description |
|---|---|
| `hex_shabd(n)` / `oct_shabd(n)` / `bin_shabd(n)` | To hex/octal/binary string |

### Date & Time (7)
| Function | Description |
|---|---|
| `din()` / `mahina()` / `saal()` | Day / Month / Year |
| `ghanta()` / `minute()` / `second()` | Hour / Minute / Second |
| `hafta_din()` | Day of week (0=Sun) |

### File I/O (4)
| Function | Description |
|---|---|
| `padho_file(path)` | Read file |
| `likho_file(path, content)` | Write file |
| `joodo_file(path, content)` | Append to file |
| `file_hai(path)` | Check if file exists |

### System & Utility (11)
| Function | Description |
|---|---|
| `samay()` / `ghadi()` | CPU time / Wall clock |
| `ruko_samay(ms)` | Sleep |
| `yaadrchik([min, max])` | Random number |
| `platform()` / `env_var(name)` | OS info / Env vars |
| `taareekh()` / `waqt()` / `timestamp()` | Date / Time strings |
| `hash_val(x)` | Hash code |
| `bahar([code])` | Exit program |

## ðŸ—ï¸ Architecture

```
Source Code (.ram)
       â†“
   â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”
   â”‚  Lexer   â”‚  â†’ Tokens
   â””â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”˜
        â†“
   â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
   â”‚ Compiler  â”‚  â†’ Bytecode (Pratt parser + code gen)
   â””â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”˜
        â†“
   â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
   â”‚   VM     â”‚  â†’ Execution (stack-based)
   â””â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”˜
        â†“
   â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
   â”‚   GC     â”‚  â†’ Memory management (mark-sweep)
   â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

**Tech Stack:**
- **Language:** Pure C (C99)
- **Parser:** Pratt parser (precedence climbing)
- **VM:** Stack-based bytecode virtual machine
- **GC:** Mark-sweep garbage collector
- **Hashing:** FNV-1a, open-addressing hash table
- **String Interning:** All strings interned for fast comparison

---

## ðŸ“ Project Structure

```
rampyaaryan-compiler/
â”œâ”€â”€ src/
â”‚   â”œâ”€â”€ common.h         â€” Common includes & config
â”‚   â”œâ”€â”€ memory.h/c       â€” Memory allocator & GC
â”‚   â”œâ”€â”€ value.h/c        â€” Value types (number, bool, null, obj)
â”‚   â”œâ”€â”€ object.h/c       â€” Heap objects (string, function, list, closure)
â”‚   â”œâ”€â”€ table.h/c        â€” Hash table implementation
â”‚   â”œâ”€â”€ chunk.h/c        â€” Bytecode chunks
â”‚   â”œâ”€â”€ lexer.h/c        â€” Tokenizer (Hinglish keywords)
â”‚   â”œâ”€â”€ compiler.h/c     â€” Pratt parser + bytecode emitter
â”‚   â”œâ”€â”€ vm.h/c           â€” Virtual Machine
â”‚   â”œâ”€â”€ native.h/c       â€” Built-in functions (145)
â”‚   â”œâ”€â”€ debug.h/c        â€” Bytecode disassembler
â”‚   â”œâ”€â”€ ascii_art.h/c    â€” Terminal animations & colors
â”‚   â””â”€â”€ main.c           â€” CLI entry point & REPL
â”œâ”€â”€ examples/            â€” Example .ram programs (20)
â”œâ”€â”€ docs/
â”‚   â”œâ”€â”€ HOWTO.md         â€” How to Use guide
â”‚   â”œâ”€â”€ TUTORIAL.md      â€” Step-by-step tutorial
â”‚   â”œâ”€â”€ API_REFERENCE.md â€” All 145 built-in functions
â”‚   â”œâ”€â”€ LANGUAGE_SPEC.md â€” Grammar specification
â”‚   â”œâ”€â”€ CONTRIBUTING.md  â€” Contribution guide
â”‚   â””â”€â”€ CHANGELOG.md     â€” Version history
â”œâ”€â”€ scripts/             â€” Install scripts
â”œâ”€â”€ Makefile             â€” GNU Make build
â”œâ”€â”€ CMakeLists.txt       â€” CMake build
â”œâ”€â”€ LICENSE              â€” MIT License
â””â”€â”€ README.md            â€” This file
```

---

## ï¿½ Documentation

| Resource | Description |
|----------|-------------|
| **[How to Use](docs/HOWTO.md)** | Complete beginner's guide â€” installation to advanced |
| **[Tutorial](docs/TUTORIAL.md)** | Learn by doing â€” 15 lessons with projects |
| **[API Reference](docs/API_REFERENCE.md)** | All 145 built-in functions documented |
| **[Language Spec](docs/LANGUAGE_SPEC.md)** | Formal grammar & syntax rules |
| **[Examples](examples/)** | 20 ready-to-run .ram programs |
| **[Contributing](docs/CONTRIBUTING.md)** | How to contribute to Rampyaaryan |
| **[Changelog](docs/CHANGELOG.md)** | Version history |

---

## ðŸ¤ Contributing

Contributions welcome! See [CONTRIBUTING.md](docs/CONTRIBUTING.md).

```bash
git clone https://github.com/Rampyaaryans/rampyaaryan.git
cd rampyaaryan
make
# hack away!
```

---

## ðŸ“œ License

MIT License â€” See [LICENSE](LICENSE)

---

## ðŸ’¡ Inspiration

- [Crafting Interpreters](https://craftinginterpreters.com/) by Robert Nystrom
- Python, JavaScript, and Lua

---

<div align="center">

**Made with â¤ï¸ for India ðŸ‡®ðŸ‡³**

**[â­ Star this repo](https://github.com/Rampyaaryans/rampyaaryan)** if you like it!

</div>
