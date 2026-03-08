# Changelog

All notable changes to Rampyaaryan will be documented in this file.

## [2.0.0] - 2025-07-12

### Added — New Data Types
- **Map/Dictionary type** (`shabdkosh`) with `{key: value}` literal syntax
- Map bracket access (`m["key"]`) and assignment (`m["key"] = val`)
- 9 map functions: `shabdkosh`, `chabi`, `mulya`, `jodi`, `map_hai`, `map_get`, `map_hatao`, `map_milao`, `map_lambai`
- `kya_map(x)` type checking function

### Added — Bitwise Operators
- AND `&`, OR `|`, XOR `^`, NOT `~`, Left Shift `<<`, Right Shift `>>`
- Base conversion: `hex_shabd(n)`, `oct_shabd(n)`, `bin_shabd(n)`

### Added — Higher-Order Functions (7)
- `naksha(list, fn)` — map
- `chhaano(list, fn)` — filter
- `ikkatha(list, fn, init)` — reduce/fold
- `sab(list)` / `koi(list)` — all/any
- `jodi_banao(a, b)` — zip
- `ginati_banao(list)` — enumerate

### Added — New String Functions (10)
- `title_case(s)`, `capitalize(s)`, `swapcase(s)` — case variants
- `center(s, width, [char])` — center padding
- `pad_left(s, width, [char])`, `pad_right(s, width, [char])` — padding
- `kya_ank(s)`, `kya_akshar(s)`, `kya_alnum(s)`, `kya_space(s)` — string checks

### Added — New Math Functions (12)
- `factorial(n)`, `kya_prime(n)`, `fib(n)` — number theory
- `hypot_val(a, b)`, `log2_val(x)` — math
- `is_nan(x)`, `is_inf(x)`, `INF()`, `NAN_VAL()` — special values
- `random_choice(list)`, `random_shuffle(list)`, `random_int(min, max)` — random

### Added — New List Functions (5)
- `flatten(list)` — flatten nested lists
- `tukda(list, size)` — chunk into groups
- `ghuma(list, n)` — rotate
- `copy_suchi(list)` — shallow copy
- `khali_karo(list)` — clear all elements

### Added — Date & Time Functions (7)
- `din()`, `mahina()`, `saal()` — day, month, year
- `ghanta()`, `minute()`, `second()` — hour, minute, second
- `hafta_din()` — day of week

### Added — Utility Functions
- `typeof_val(x)`, `bool_val(x)`, `print_type(x)`, `likho_line()`

### Added — Examples
- 5 new example programs (16-20): shabdkosh, bitwise, higher-order, mini-database, feature showcase

### Changed
- Total built-in functions: 30+ → **145**
- Total examples: 10 → **20**
- Documentation expanded with new tutorials and API sections

---

## [1.0.0] - 2025-01-01

### Added
- Initial release of Rampyaaryan compiler
- Complete Hinglish keyword set (maano, likho, pucho, agar, warna, jab tak, har, kaam, wapas do, ruko, agla)
- Bytecode compiler with Pratt parser
- Stack-based Virtual Machine
- Mark-sweep Garbage Collector
- String interning with FNV-1a hashing
- Closure and upvalue support
- Dynamic lists with indexing
- 30+ built-in functions (type, string, list, math, utility)
- Interactive REPL with multiline support
- ASCII art splash screen and animations
- Colored terminal output (ANSI + Windows)
- Error messages in Hinglish
- 10 example programs
- Cross-platform support (Windows, Linux, macOS)
- Makefile and CMake build system
- Rampyaaryan License
