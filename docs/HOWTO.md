# 🚀 How to Use Rampyaaryan — Complete Beginner's Guide

> **Rampyaaryan** is the world's first Hinglish programming language. Write code in Hindi-English, run it anywhere.

---

## 📦 Installation

### Windows
```powershell
# Option 1: Download from Releases
# Go to https://github.com/Rampyaaryans/rampyaaryan/releases
# Download rampyaaryan-windows-x64.exe

# Option 2: Run install script
powershell -ExecutionPolicy Bypass -File install.ps1
```

### Linux
```bash
# Option 1: Download binary
curl -L https://github.com/Rampyaaryans/rampyaaryan/releases/latest/download/rampyaaryan-linux-x64 -o rampyaaryan
chmod +x rampyaaryan
sudo mv rampyaaryan /usr/local/bin/

# Option 2: Build from source
git clone https://github.com/Rampyaaryans/rampyaaryan.git
cd rampyaaryan
make
sudo make install
```

### macOS
```bash
curl -L https://github.com/Rampyaaryans/rampyaaryan/releases/latest/download/rampyaaryan-macos-x64 -o rampyaaryan
chmod +x rampyaaryan
sudo mv rampyaaryan /usr/local/bin/
```

### Verify Installation
```bash
rampyaaryan --version
# Output: Rampyaaryan v1.0.0 (bytecode VM, built ...)
```

---

## 🏃 Running Programs

### Run a `.ram` file
```bash
rampyaaryan hello.ram
```

### Start the REPL (Interactive Mode)
```bash
rampyaaryan
```
Type code line by line. Use `.help` for REPL commands.

### REPL Commands
| Command | Description |
|---------|-------------|
| `.help` | Show all keywords and built-in functions |
| `.clear` | Clear screen |
| `.memory` | Show memory usage stats |
| `.version` | Show version |
| `.exit` | Quit REPL |

---

## ✍️ Your First Program

Create a file `namaste.ram`:
```
likho "🙏 Namaste Duniya!"
likho "Mera naam Rampyaaryan hai!"
```

Run it:
```bash
rampyaaryan namaste.ram
```

Output:
```
🙏 Namaste Duniya!
Mera naam Rampyaaryan hai!
```

---

## 📖 Language Basics

### Comments
```
# Yeh ek comment hai
// Yeh bhi comment hai
```

### Variables (maano)
```
maano naam = "Rampyaaryan"
maano umar = 21
maano active = sach
maano kuch_nahi = khali
```

### Data Types
| Type | Hindi Name | Examples |
|------|-----------|----------|
| Number | sankhya | `42`, `3.14`, `-7` |
| String | shabd | `"hello"`, `"namaste"` |
| Boolean | bool | `sach` (true), `jhooth` (false) |
| Null | khali | `khali` (null) |
| List | suchi | `[1, 2, 3]`, `["a", "b"]` |
| Function | kaam | `kaam add(a, b) { ... }` |

### Print (likho)
```
likho "Hello!"                    # Simple print
likho "Naam:", naam               # Multiple values
likho "Sum =", 2 + 3             # With expressions
likho ""                          # Empty line
```

### Input (pucho)
```
maano naam = pucho("Aapka naam: ")
maano umar = sankhya(pucho("Umar: "))
```

---

## 🔢 Operators

### Arithmetic
| Operator | Meaning | Example |
|----------|---------|---------|
| `+` | Addition | `5 + 3` → `8` |
| `-` | Subtraction | `10 - 4` → `6` |
| `*` | Multiplication | `6 * 7` → `42` |
| `/` | Division | `10 / 3` → `3.333` |
| `%` | Modulo | `10 % 3` → `1` |
| `**` | Power | `2 ** 10` → `1024` |

### Comparison
| Operator | Meaning |
|----------|---------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less or equal |
| `>=` | Greater or equal |

### Logical
| Keyword | Meaning | Example |
|---------|---------|---------|
| `aur` | AND | `x > 0 aur x < 10` |
| `ya` | OR | `x == 5 ya x == 10` |
| `nahi` | NOT | `nahi (x > 100)` |

### Compound Assignment
```
x += 5      # x = x + 5
x -= 3      # x = x - 3
x *= 2      # x = x * 2
x /= 4      # x = x / 4
```

---

## 🔀 Control Flow

### If-Else (agar / warna)
```
agar umar >= 18 {
    likho "Vote de sakte ho!"
} warna agar umar >= 13 {
    likho "Teenager ho!"
} warna {
    likho "Bachche ho!"
}
```

### While Loop (jab tak)
```
maano i = 5
jab tak i > 0 {
    likho i
    i = i - 1
}
```

### For Loop (har)
```
har i = 1; i <= 10; i = i + 1 {
    likho i
}
```

### Break and Continue
```
har i = 1; i <= 100; i = i + 1 {
    agar i % 2 == 0 { agla }      # Skip even (continue)
    agar i > 20 { ruko }          # Stop at 20 (break)
    likho i
}
```

---

## ⚡ Functions (kaam)

### Define a Function
```
kaam namaste(naam) {
    likho "🙏 Namaste,", naam, "ji!"
}

namaste("Rampyaaryan")
```

### Return Values (wapas do)
```
kaam jodo(a, b) {
    wapas do a + b
}

maano result = jodo(5, 3)
likho "Result:", result     # 8
```

### Recursion
```
kaam factorial(n) {
    agar n <= 1 { wapas do 1 }
    wapas do n * factorial(n - 1)
}

likho factorial(10)    # 3628800
```

### Closures
```
kaam counter() {
    maano count = 0
    kaam increment() {
        count = count + 1
        wapas do count
    }
    wapas do increment
}

maano c = counter()
likho c()    # 1
likho c()    # 2
likho c()    # 3
```

---

## 📋 Lists (Suchi)

### Create and Access
```
maano fruits = ["seb", "kela", "aam"]
likho fruits[0]      # "seb"
likho fruits[2]      # "aam"
fruits[1] = "angoor"  # Modify
```

### List Operations
```
joodo(fruits, "santara")        # Append
nikalo(fruits, 0)               # Remove at index
likho lambai(fruits)            # Length
likho shamil(fruits, "seb")     # Contains?
likho index_of(fruits, "aam")   # Find index
```

### Slicing and Advanced
```
maano nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
likho katao(nums, 0, 3)    # [1, 2, 3]
likho katao(nums, -3)      # [8, 9, 10]
likho pahla(nums)           # 1
likho aakhri(nums)          # 10
likho ulta(nums)            # [10, 9, ..., 1]
likho kram(nums)            # Sort
likho anokha([1,2,2,3,3])  # [1, 2, 3]
```

### Statistics
```
maano data = [10, 20, 30, 40, 50]
likho jod(data)              # Sum: 150
likho ausat(data)            # Average: 30
likho sabse_bada(data)       # Max: 50
likho sabse_chhota(data)     # Min: 10
```

---

## 📁 File I/O

```
# Write file
likho_file("output.txt", "Hello World!\n")

# Append to file
joodo_file("output.txt", "Line 2\n")

# Read file
maano content = padho_file("output.txt")
likho content

# Check if file exists
agar file_hai("data.csv") {
    likho "File mil gayi!"
}
```

---

## 🔤 String Operations

```
maano s = "  Namaste Duniya  "

lambai(s)                    # Length
bade_akshar(s)               # UPPERCASE
chhote_akshar(s)             # lowercase
saaf(s)                      # Trim whitespace
kato(s, 2, 9)                # Substring
dhundho(s, "Duniya")         # Find (returns index)
badlo(s, "Duniya", "World")  # Replace
todo(s, " ")                 # Split → list
jodo_shabd(list, ", ")       # Join list → string
shuru_se(s, "Nam")           # Starts with?
ant_se(s, "iya")             # Ends with?
dohrao("ha", 3)              # "hahaha"
akshar(s, 0)                 # Character at index
ascii_code("A")              # 65
ascii_se(65)                 # "A"
format("{} is {}", "Pi", 3.14)  # String formatting
gino(s, "a")                 # Count occurrences
```

---

## 🧮 Math Functions

```
abs_val(-42)        # 42
sqrt_val(144)       # 12
power_val(2, 10)    # 1024
gol(3.7)            # 4 (round)
upar(3.2)           # 4 (ceil)
neeche(3.9)         # 3 (floor)
bada(5, 10)         # 10 (max)
chhota(5, 10)       # 5 (min)
sign(-42)           # -1
clamp(150, 0, 100)  # 100

# Trigonometry
sin_val(angle)      # Sine
cos_val(angle)      # Cosine
tan_val(angle)      # Tangent
asin_val(x)         # Arc sine
acos_val(x)         # Arc cosine
atan_val(x)         # Arc tangent

# Logarithms
log_val(x)          # Natural log
log10_val(x)        # Log base 10
exp_val(x)          # e^x

# Constants
PI()                # 3.14159...
E()                 # 2.71828...

# Number Theory
gcd(48, 18)         # 6
lcm(12, 8)          # 24

# Angle Conversion
degrees(PI())       # 180
radians(180)        # 3.14159...
```

---

## 🔍 Type System

### Type Checking Functions
```
kya_sankhya(42)     # sach
kya_shabd("hi")     # sach
kya_suchi([1,2])    # sach
kya_kaam(fn)        # sach (is it a function?)
kya_bool(sach)      # sach
kya_khali(khali)    # sach
kya_purn(42)        # sach (is integer?)
kya_purn(3.14)      # jhooth
prakar(42)          # "sankhya"
```

### Type Conversion
```
sankhya("42")       # 42
shabd(42)           # "42"
purn(3.99)          # 3
dashmlav("3.14")    # 3.14
```

---

## ⏱️ System Functions

```
samay()             # Execution time (seconds)
ghadi()             # High-precision clock
ruko_samay(1000)    # Sleep 1 second
yaadrchik()         # Random 0-1
yaadrchik(1, 100)   # Random 1-100
platform()          # "windows" / "linux" / "macos"
taareekh()          # "2026-03-08"
waqt()              # "14:30:45"
timestamp()         # Unix timestamp
env_var("PATH")     # Environment variable
hash_val("hello")   # Hash of value
bahar(0)            # Exit program
```

---

## 🎓 Complete Keyword Reference

| Keyword | English | Usage |
|---------|---------|-------|
| `maano` | let/var | Variable declaration |
| `likho` | print | Print output |
| `pucho` | input | Read user input |
| `agar` | if | Condition |
| `warna` | else | Else branch |
| `warna agar` | else if | Else-if branch |
| `jab tak` | while | While loop |
| `har` | for | For loop |
| `kaam` | function | Function definition |
| `wapas do` | return | Return from function |
| `ruko` | break | Break from loop |
| `agla` | continue | Skip to next iteration |
| `sach` | true | Boolean true |
| `jhooth` | false | Boolean false |
| `khali` | null | Null value |
| `aur` | and | Logical AND |
| `ya` | or | Logical OR |
| `nahi` | not | Logical NOT |

---

## 💡 Tips & Tricks

1. **Use `#` for comments** — they're ignored by the compiler
2. **Lists can hold mixed types** — `[1, "hello", sach, [2, 3]]`
3. **Functions are first-class** — pass them as arguments, return them
4. **Closures work** — inner functions can access outer variables
5. **Use `format()` for string interpolation** — `format("{} + {} = {}", 1, 2, 3)`
6. **Use `ghadi()` for benchmarking** — high-precision timer
7. **File I/O works cross-platform** — read/write/append/check
8. **Use compound assignment** — `x += 5` instead of `x = x + 5`

---

## 🆘 Common Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `Variable define nahi hai` | Using undefined variable | Add `maano` declaration |
| `Index seema se bahar` | List index out of range | Check `lambai()` |
| `Number convert nahi kar sakte` | Invalid string to number | Validate input |
| `'{' lagao` | Missing opening brace | Add `{` after condition/loop |
| `'}' lagao` | Missing closing brace | Add `}` |

---

Made with ❤️ by Rampyaaryan | [GitHub](https://github.com/Rampyaaryans/rampyaaryan)
