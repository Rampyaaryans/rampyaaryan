# 📚 Rampyaaryan API Reference — All 65+ Built-in Functions

> Complete reference for every built-in function in the Rampyaaryan language.

---

## Table of Contents

1. [Type Conversion (5)](#-type-conversion)
2. [Type Checking (7)](#-type-checking)
3. [String Operations (14)](#-string-operations)
4. [List Operations (15)](#-list-operations)
5. [Math Functions (20)](#-math-functions)
6. [File I/O (4)](#-file-io)
7. [System & Utility (11)](#-system--utility)

---

## 🔄 Type Conversion

### `prakar(value)` → string
Returns the type name of a value as a string.
```
prakar(42)        # → "sankhya"
prakar("hello")   # → "shabd"
prakar(sach)      # → "boolean"
prakar(khali)     # → "khali"
prakar([1,2])     # → "suchi"
```

### `sankhya(value)` → number
Converts a value to a number. Strings are parsed.
```
sankhya("42")     # → 42
sankhya("3.14")   # → 3.14
sankhya(sach)     # → 1
sankhya(jhooth)   # → 0
sankhya(khali)    # → 0
```

### `dashmlav(value)` → number
Converts to a decimal/float number. Same as `sankhya()`.
```
dashmlav("3.14")  # → 3.14
```

### `shabd(value)` → string
Converts any value to its string representation.
```
shabd(42)         # → "42"
shabd(3.14)       # → "3.14"
shabd(sach)       # → "sach"
shabd([1,2,3])    # → "[1, 2, 3]"
```

### `purn(value)` → number
Converts to an integer (truncates decimals).
```
purn(3.99)        # → 3
purn("42")        # → 42
purn(-3.7)        # → -3
```

---

## 🔍 Type Checking

### `kya_sankhya(value)` → boolean
Returns `sach` if the value is a number.
```
kya_sankhya(42)      # → sach
kya_sankhya("42")    # → jhooth
```

### `kya_shabd(value)` → boolean
Returns `sach` if the value is a string.
```
kya_shabd("hello")   # → sach
kya_shabd(42)        # → jhooth
```

### `kya_suchi(value)` → boolean
Returns `sach` if the value is a list.
```
kya_suchi([1,2,3])   # → sach
kya_suchi("hello")   # → jhooth
```

### `kya_kaam(value)` → boolean
Returns `sach` if the value is a function (user-defined, closure, or native).
```
kaam greet() { likho "hi" }
kya_kaam(greet)      # → sach
kya_kaam(42)         # → jhooth
```

### `kya_bool(value)` → boolean
Returns `sach` if the value is a boolean.
```
kya_bool(sach)       # → sach
kya_bool(42)         # → jhooth
```

### `kya_khali(value)` → boolean
Returns `sach` if the value is `khali` (null).
```
kya_khali(khali)     # → sach
kya_khali(0)         # → jhooth
```

### `kya_purn(value)` → boolean
Returns `sach` if the value is an integer (whole number).
```
kya_purn(42)         # → sach
kya_purn(3.14)       # → jhooth
kya_purn(5.0)        # → sach
```

---

## 🔤 String Operations

### `lambai(value)` → number
Returns the length of a string or list.
```
lambai("hello")      # → 5
lambai([1,2,3])      # → 3
```

### `bade_akshar(str)` → string
Converts string to UPPERCASE.
```
bade_akshar("hello") # → "HELLO"
```

### `chhote_akshar(str)` → string
Converts string to lowercase.
```
chhote_akshar("HELLO") # → "hello"
```

### `kato(str, start, [end])` → string
Extracts a substring. Supports negative indices.
```
kato("Namaste", 0, 3)   # → "Nam"
kato("Namaste", -4)     # → "aste"
kato("Namaste", 2, 5)   # → "mas"
```

### `dhundho(str, substr)` → number
Finds the first occurrence of `substr`. Returns -1 if not found.
```
dhundho("hello world", "world")  # → 6
dhundho("hello world", "xyz")    # → -1
```

### `badlo(str, old, new)` → string
Replaces ALL occurrences of `old` with `new`.
```
badlo("hello world", "world", "duniya")  # → "hello duniya"
badlo("aaa", "a", "b")                  # → "bbb"
```

### `todo(str, [separator])` → list
Splits a string into a list. Default separator: space.
```
todo("a,b,c", ",")      # → ["a", "b", "c"]
todo("hello world")     # → ["hello", "world"]
todo("abc", "")          # → ["a", "b", "c"]
```

### `saaf(str)` → string
Removes whitespace from both ends of a string.
```
saaf("  hello  ")       # → "hello"
saaf("\t hi \n")        # → "hi"
```

### `shuru_se(str, prefix)` → boolean
Checks if string starts with the given prefix.
```
shuru_se("hello", "hel")   # → sach
shuru_se("hello", "xyz")   # → jhooth
```

### `ant_se(str, suffix)` → boolean
Checks if string ends with the given suffix.
```
ant_se("file.ram", ".ram")  # → sach
ant_se("file.txt", ".ram")  # → jhooth
```

### `dohrao(str, count)` → string
Repeats a string `count` times.
```
dohrao("ha", 3)          # → "hahaha"
dohrao("⭐", 5)          # → "⭐⭐⭐⭐⭐"
```

### `akshar(str, index)` → string
Returns the character at the given index. Supports negative indices.
```
akshar("hello", 0)       # → "h"
akshar("hello", -1)      # → "o"
```

### `ascii_code(char)` → number
Returns the ASCII code of the first character.
```
ascii_code("A")          # → 65
ascii_code("0")          # → 48
```

### `ascii_se(code)` → string
Returns the character for an ASCII code.
```
ascii_se(65)             # → "A"
ascii_se(48)             # → "0"
```

### `jodo_shabd(list, [separator])` → string
Joins list elements into a string with a separator.
```
jodo_shabd(["a","b","c"], ",")  # → "a,b,c"
jodo_shabd([1, 2, 3], " + ")   # → "1 + 2 + 3"
```

### `format(template, args...)` → string
String formatting with `{}` placeholders.
```
format("{} + {} = {}", 2, 3, 5)     # → "2 + 3 = 5"
format("Hi, {}!", "Ram")            # → "Hi, Ram!"
```

### `gino(str, substr)` → number
Counts non-overlapping occurrences of `substr` in `str`.
```
gino("banana", "a")     # → 3
gino("hello", "ll")     # → 1
```

---

## 📋 List Operations

### `joodo(list, item)` → list
Appends an item to the end of a list. Modifies in-place.
```
maano l = [1, 2]
joodo(l, 3)              # l → [1, 2, 3]
```

### `nikalo(list, [index])` → value
Removes and returns the item at index. Default: last item.
```
maano l = [1, 2, 3]
nikalo(l)                # → 3, l → [1, 2]
nikalo(l, 0)             # → 1, l → [2]
```

### `ulta(value)` → list|string
Reverses a list or string.
```
ulta([1, 2, 3])          # → [3, 2, 1]
ulta("hello")            # → "olleh"
```

### `kram(list)` → list
Sorts a list in-place (numbers ascending, strings alphabetical).
```
kram([3, 1, 2])          # → [1, 2, 3]
kram(["c", "a", "b"])    # → ["a", "b", "c"]
```

### `suchi(args...)` → list
Creates a new list from the given arguments.
```
suchi(1, 2, 3)           # → [1, 2, 3]
```

### `range_(start_or_end, [end], [step])` → list
Creates a list of numbers in a range.
```
range_(5)                # → [0, 1, 2, 3, 4]
range_(1, 6)             # → [1, 2, 3, 4, 5]
range_(0, 10, 2)         # → [0, 2, 4, 6, 8]
range_(10, 0, -1)        # → [10, 9, 8, ..., 1]
```

### `shamil(collection, value)` → boolean
Checks if a value exists in a list or substring in a string.
```
shamil([1,2,3], 2)       # → sach
shamil("hello", "ell")   # → sach
```

### `katao(list, [start], [end])` → list
Returns a slice of the list. Supports negative indices.
```
katao([1,2,3,4,5], 1, 3) # → [2, 3]
katao([1,2,3,4,5], -2)   # → [4, 5]
```

### `pahla(list)` → value
Returns the first element of a list.
```
pahla([10, 20, 30])      # → 10
```

### `aakhri(list)` → value
Returns the last element of a list.
```
aakhri([10, 20, 30])     # → 30
```

### `milao(list1, list2)` → list
Concatenates two lists into a new list.
```
milao([1,2], [3,4])      # → [1, 2, 3, 4]
```

### `index_of(list, value)` → number
Returns the index of the first occurrence, or -1 if not found.
```
index_of([10,20,30], 20) # → 1
index_of([10,20,30], 99) # → -1
```

### `anokha(list)` → list
Returns a new list with duplicate elements removed.
```
anokha([1,2,2,3,3])     # → [1, 2, 3]
```

### `daalo(list, index, value)` → list
Inserts a value at the specified index. Modifies in-place.
```
maano l = [1, 3]
daalo(l, 1, 2)           # l → [1, 2, 3]
```

### `jod(list)` → number
Returns the sum of all numbers in a list.
```
jod([10, 20, 30])        # → 60
```

### `sabse_bada(list)` → number
Returns the maximum number in a list.
```
sabse_bada([3, 7, 1, 9]) # → 9
```

### `sabse_chhota(list)` → number
Returns the minimum number in a list.
```
sabse_chhota([3, 7, 1, 9]) # → 1
```

### `ausat(list)` → number
Returns the average (mean) of numbers in a list.
```
ausat([10, 20, 30])      # → 20
```

---

## 🧮 Math Functions

### `abs_val(x)` → number
Absolute value.
```
abs_val(-42)             # → 42
```

### `gol(x)` → number
Round to nearest integer.
```
gol(3.7)                 # → 4
gol(3.2)                 # → 3
```

### `upar(x)` → number
Ceiling — round up.
```
upar(3.1)                # → 4
```

### `neeche(x)` → number
Floor — round down.
```
neeche(3.9)              # → 3
```

### `sqrt_val(x)` → number
Square root.
```
sqrt_val(144)            # → 12
```

### `bada(a, b)` → number
Returns the larger of two numbers.
```
bada(5, 10)              # → 10
```

### `chhota(a, b)` → number
Returns the smaller of two numbers.
```
chhota(5, 10)            # → 5
```

### `sin_val(x)`, `cos_val(x)`, `tan_val(x)` → number
Trigonometric functions (input in radians).
```
sin_val(PI() / 2)        # → 1
cos_val(0)               # → 1
```

### `asin_val(x)`, `acos_val(x)`, `atan_val(x)` → number
Inverse trigonometric functions.
```
asin_val(1)              # → 1.5708 (PI/2)
```

### `log_val(x)` → number
Natural logarithm (ln).
```
log_val(E())             # → 1
```

### `log10_val(x)` → number
Base-10 logarithm.
```
log10_val(1000)          # → 3
```

### `exp_val(x)` → number
Exponential function (e^x).
```
exp_val(1)               # → 2.71828...
```

### `power_val(base, exp)` → number
Power function.
```
power_val(2, 10)         # → 1024
```

### `PI()` → number
Returns the value of Pi (3.14159...).

### `E()` → number
Returns Euler's number (2.71828...).

### `gcd(a, b)` → number
Greatest Common Divisor.
```
gcd(48, 18)              # → 6
```

### `lcm(a, b)` → number
Least Common Multiple.
```
lcm(12, 8)               # → 24
```

### `sign(x)` → number
Returns -1, 0, or 1 based on the sign of x.
```
sign(-42)                # → -1
sign(0)                  # → 0
sign(99)                 # → 1
```

### `clamp(value, min, max)` → number
Clamps a value between min and max.
```
clamp(150, 0, 100)       # → 100
clamp(-5, 0, 100)        # → 0
```

### `degrees(radians)` → number
Converts radians to degrees.
```
degrees(PI())            # → 180
```

### `radians(degrees)` → number
Converts degrees to radians.
```
radians(180)             # → 3.14159...
```

---

## 📁 File I/O

### `padho_file(path)` → string
Reads the entire contents of a file.
```
maano data = padho_file("input.txt")
```

### `likho_file(path, content)` → boolean
Writes content to a file (creates or overwrites).
```
likho_file("output.txt", "Hello!\n")
```

### `joodo_file(path, content)` → boolean
Appends content to a file.
```
joodo_file("log.txt", "New entry\n")
```

### `file_hai(path)` → boolean
Checks if a file exists.
```
agar file_hai("config.txt") {
    likho "Config found!"
}
```

---

## ⚙️ System & Utility

### `samay()` → number
Returns CPU time since program start (seconds).

### `ghadi()` → number
Returns high-resolution wall clock time (seconds). Best for benchmarking.

### `ruko_samay(ms)` → null
Pauses execution for the specified milliseconds.
```
ruko_samay(1000)         # Sleep 1 second
```

### `yaadrchik([min, max])` → number
Random number. No args: 0-1 float. Two args: integer in range.
```
yaadrchik()              # → 0.7234... (random float)
yaadrchik(1, 100)        # → 42 (random integer)
```

### `platform()` → string
Returns the operating system name.
```
platform()               # → "windows" / "linux" / "macos"
```

### `taareekh()` → string
Returns today's date as "YYYY-MM-DD".
```
taareekh()               # → "2026-03-08"
```

### `waqt()` → string
Returns the current time as "HH:MM:SS".
```
waqt()                   # → "14:30:45"
```

### `timestamp()` → number
Returns the current Unix timestamp (seconds since 1970).

### `env_var(name)` → string|null
Returns the value of an environment variable, or `khali` if not set.
```
env_var("HOME")          # → "/home/user"
env_var("PATH")          # → system PATH
```

### `hash_val(value)` → number
Returns a hash code for a string or number.
```
hash_val("hello")        # → numeric hash
```

### `bahar([code])` → (exits)
Exits the program with the given exit code (default: 0).
```
bahar()                  # Exit normally
bahar(1)                 # Exit with error code 1
```

---

> **Total: 65+ built-in functions** across 7 categories.
> 
> Made with ❤️ by Rampyaaryan | [GitHub](https://github.com/Rampyaaryans/rampyaaryan)
