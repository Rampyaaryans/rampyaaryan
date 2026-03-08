# ðŸŽ“ Rampyaaryan Tutorial â€” Learn by Doing

> A step-by-step guide to master the Rampyaaryan programming language.  
> Each lesson builds on the previous one. Type along!

---

## Lesson 1: Your First Program ðŸŒŸ

Create a file called `pehla.ram`:

```
likho "ðŸ™ Namaste, Duniya!"
```

Run it:
```bash
rampyaaryan pehla.ram
```

**Output:**
```
ðŸ™ Namaste, Duniya!
```

ðŸŽ‰ You just wrote your first Rampyaaryan program!

---

## Lesson 2: Variables â€” `maano` ðŸ“¦

Use `maano` (meaning "assume/let") to create variables:

```
maano naam = "Aryan"
maano umr = 20
maano chhatra = sach

likho naam       # â†’ Aryan
likho umr        # â†’ 20
likho chhatra    # â†’ sach
```

**Variable naming rules:**
- Start with a letter or underscore
- Can contain letters, numbers, underscores
- Case-sensitive (`naam` â‰  `Naam`)

---

## Lesson 3: Data Types ðŸ·ï¸

Rampyaaryan has 6 core data types:

```
# Numbers (sankhya)
maano purn_sankhya = 42
maano dashmlav = 3.14

# Strings (shabd)
maano sandesh = "Hello!"

# Booleans (boolean)
maano haan = sach       # true
maano nahi = jhooth     # false

# Null (khali)
maano kuch_nahi = khali

# Lists (suchi)
maano rang = ["lal", "hara", "neela"]

# Functions (kaam)
kaam namaste() {
    likho "Namaste!"
}
```

Check any type with `prakar()`:
```
likho prakar(42)         # â†’ sankhya
likho prakar("hello")    # â†’ shabd
likho prakar(sach)       # â†’ boolean
likho prakar(khali)      # â†’ khali
likho prakar([1,2])      # â†’ suchi
```

---

## Lesson 4: Arithmetic âž•âž–

```
maano a = 10
maano b = 3

likho a + b      # â†’ 13   (jod - addition)
likho a - b      # â†’ 7    (ghata - subtraction)
likho a * b      # â†’ 30   (guna - multiplication)
likho a / b      # â†’ 3.33 (bhag - division)
likho a % b      # â†’ 1    (shesha - modulo)

# String concatenation with +
likho "Hello " + "World"  # â†’ Hello World

# String repetition
likho dohrao("ha", 3)     # â†’ hahaha
```

---

## Lesson 5: User Input â€” `poocho` ðŸ“

```
maano naam = poocho("Aapka naam kya hai? ")
likho "Namaste, " + naam + "!"

maano umr_str = poocho("Umr batao: ")
maano umr = sankhya(umr_str)
likho format("Aap {} saal ke ho!", umr)
```

---

## Lesson 6: Conditions â€” `agar/warna agar/warna` ðŸ”€

```
maano marks = 85

agar marks >= 90 {
    likho "Grade: A+"
} warna agar marks >= 80 {
    likho "Grade: A"
} warna agar marks >= 70 {
    likho "Grade: B"
} warna {
    likho "Grade: C"
}
```

**Comparison operators:**
```
==    # equals (barabar)
!=    # not equals (alag)
<     # less than (se chhota)
>     # greater than (se bada)
<=    # less or equal (se chhota ya barabar)
>=    # greater or equal (se bada ya barabar)
```

**Logical operators:**
```
aur   # AND (dono sach)
ya    # OR (ek bhi sach)
nahi  # NOT (ulta)
```

Example:
```
maano umr = 25
maano license = sach

agar umr >= 18 aur license {
    likho "Gaadi chala sakte ho!"
}
```

---

## Lesson 7: Loops â€” `jabtak` and `har` ðŸ”

### While Loop â€” `jabtak`
```
maano i = 1
jabtak i <= 5 {
    likho format("Ginti: {}", i)
    i = i + 1
}
```

### For Loop â€” `har`
```
har i = 1; i <= 10; i = i + 1 {
    likho format("{} x 5 = {}", i, i * 5)
}
```

### Loop Control
```
# ruko = break (stop the loop)
# agla = continue (skip to next)

har i = 1; i <= 10; i = i + 1 {
    agar i == 5 {
        agla       # skip 5
    }
    agar i == 8 {
        ruko       # stop at 8
    }
    likho i
}
# Output: 1 2 3 4 6 7
```

---

## Lesson 8: Functions â€” `kaam` ðŸ”§

### Basic function
```
kaam namaste(naam) {
    likho format("Namaste, {}!", naam)
}

namaste("Aryan")     # â†’ Namaste, Aryan!
```

### Return values â€” `wapas do`
```
kaam jod(a, b) {
    wapas do a + b
}

maano result = jod(10, 20)
likho result         # â†’ 30
```

### Default thinking in functions
```
kaam factorial(n) {
    agar n <= 1 {
        wapas do 1
    }
    wapas do n * factorial(n - 1)
}

likho factorial(10)  # â†’ 3628800
```

### Closures (functions inside functions)
```
kaam counter() {
    maano count = 0
    kaam badhao() {
        count = count + 1
        wapas do count
    }
    wapas do badhao
}

maano c = counter()
likho c()    # â†’ 1
likho c()    # â†’ 2
likho c()    # â†’ 3
```

---

## Lesson 9: Lists â€” `suchi` ðŸ“‹

### Creating lists
```
maano fruits = ["seb", "kela", "aam"]
maano numbers = [1, 2, 3, 4, 5]
maano mixed = [42, "hello", sach, khali]
maano khali_suchi = []
```

### Accessing elements (0-indexed)
```
likho fruits[0]      # â†’ seb
likho fruits[1]      # â†’ kela
likho fruits[-1]     # â†’ aam (last element)
```

### Modifying lists
```
fruits[0] = "santara"    # replace
joodo(fruits, "anaar")   # append
nikalo(fruits)           # remove last
nikalo(fruits, 0)        # remove first
daalo(fruits, 1, "angoor")  # insert at index 1
```

### List operations
```
maano nums = [3, 1, 4, 1, 5, 9, 2, 6]

likho lambai(nums)       # â†’ 8
likho jod(nums)          # â†’ 31
likho ausat(nums)        # â†’ 3.875
likho sabse_bada(nums)   # â†’ 9
likho sabse_chhota(nums) # â†’ 1
likho kram(nums)         # sorted: [1, 1, 2, 3, 4, 5, 6, 9]
likho ulta(nums)         # reversed
likho anokha(nums)       # unique: [3, 1, 4, 5, 9, 2, 6]
likho shamil(nums, 5)    # â†’ sach
likho index_of(nums, 4)  # â†’ 2
```

### Building lists dynamically
```
maano squares = []
har i = 1; i <= 10; i = i + 1 {
    joodo(squares, i * i)
}
likho squares    # â†’ [1, 4, 9, 16, 25, 36, 49, 64, 81, 100]
```

### Looping through lists
```
maano dost = ["Ram", "Shyam", "Mohan"]
har i = 0; i < lambai(dost); i = i + 1 {
    likho format("Dost #{}: {}", i + 1, dost[i])
}
```

---

## Lesson 10: String Magic ðŸª„

```
maano s = "  Namaste Duniya!  "

# Cleaning
likho saaf(s)                    # â†’ "Namaste Duniya!"

# Case conversion
likho bade_akshar("hello")       # â†’ "HELLO"
likho chhote_akshar("HELLO")     # â†’ "hello"

# Searching
likho dhundho("hello world", "world")  # â†’ 6
likho shamil("hello", "ell")           # â†’ sach
likho gino("banana", "a")             # â†’ 3

# Splitting and joining
maano parts = todo("a,b,c,d", ",")    # â†’ ["a","b","c","d"]
likho jodo_shabd(parts, " | ")        # â†’ "a | b | c | d"

# Checking
likho shuru_se("file.ram", "file")    # â†’ sach
likho ant_se("file.ram", ".ram")      # â†’ sach

# Substring
likho kato("Namaste", 0, 3)           # â†’ "Nam"

# Replacement
likho badlo("Namaste Duniya", "Duniya", "World")
# â†’ "Namaste World"

# Formatting
likho format("{} is {} years old", "Ram", 20)
# â†’ "Ram is 20 years old"
```

---

## Lesson 11: Math & Science ðŸ§®

```
# Basic math
likho abs_val(-42)       # â†’ 42
likho gol(3.7)           # â†’ 4
likho upar(3.1)          # â†’ 4
likho neeche(3.9)        # â†’ 3
likho sqrt_val(144)      # â†’ 12
likho power_val(2, 10)   # â†’ 1024

# Trigonometry
likho sin_val(PI() / 2)  # â†’ 1
likho cos_val(0)         # â†’ 1
likho tan_val(PI() / 4)  # â†’ 1

# Logarithms
likho log_val(E())       # â†’ 1
likho log10_val(1000)    # â†’ 3
likho exp_val(1)         # â†’ 2.71828

# Number theory
likho gcd(48, 18)        # â†’ 6
likho lcm(12, 8)         # â†’ 24

# Conversions
likho degrees(PI())      # â†’ 180
likho radians(180)       # â†’ 3.14159

# Random numbers
likho yaadrchik()        # â†’ 0.xxxx (random float)
likho yaadrchik(1, 100)  # â†’ random integer 1-100
```

---

## Lesson 12: File Operations ðŸ“

```
# Write to a file
likho_file("notes.txt", "Line 1\n")

# Append to a file
joodo_file("notes.txt", "Line 2\n")
joodo_file("notes.txt", "Line 3\n")

# Check if file exists
agar file_hai("notes.txt") {
    likho "File exists!"
}

# Read entire file
maano content = padho_file("notes.txt")
likho content

# Process line by line
maano lines = todo(content, "\n")
har i = 0; i < lambai(lines); i = i + 1 {
    agar lambai(lines[i]) > 0 {
        likho format("Line {}: {}", i + 1, lines[i])
    }
}
```

---

## Lesson 13: Practical Projects ðŸš€

### Project 1: Calculator
```
likho "=== Rampyaaryan Calculator ==="

maano num1 = sankhya(poocho("Pehla number: "))
maano op = poocho("Operation (+, -, *, /): ")
maano num2 = sankhya(poocho("Doosra number: "))

maano result = 0
agar op == "+" {
    result = num1 + num2
} warna agar op == "-" {
    result = num1 - num2
} warna agar op == "*" {
    result = num1 * num2
} warna agar op == "/" {
    agar num2 != 0 {
        result = num1 / num2
    } warna {
        likho "Error: Zero se divide nahi kar sakte!"
        bahar(1)
    }
}

likho format("{} {} {} = {}", num1, op, num2, result)
```

### Project 2: Number Guessing Game
```
maano secret = yaadrchik(1, 100)
maano attempts = 0

likho "ðŸŽ¯ Number Guessing Game!"
likho "Maine 1 se 100 ke beech ek number socha hai."

maano found = jhooth
jabtak nahi found {
    maano guess = sankhya(poocho("Aapka guess: "))
    attempts = attempts + 1

    agar guess == secret {
        likho format("ðŸŽ‰ Sahi jawaab! {} attempts mein!", attempts)
        found = sach
    } warna agar guess < secret {
        likho "â¬†ï¸ Zyada socho!"
    } warna {
        likho "â¬‡ï¸ Kam socho!"
    }
}
```

### Project 3: Todo List Manager
```
maano tasks = []
maano running = sach

jabtak running {
    likho "\nðŸ“ Todo Manager"
    likho "1. Task joodo"
    likho "2. Tasks dekho"
    likho "3. Task hatao"
    likho "4. Bahar"

    maano choice = poocho("Choice: ")

    agar choice == "1" {
        maano task = poocho("Naya task: ")
        joodo(tasks, task)
        likho "âœ… Task added!"
    } warna agar choice == "2" {
        agar lambai(tasks) == 0 {
            likho "Koi task nahi hai!"
        } warna {
            har i = 0; i < lambai(tasks); i = i + 1 {
                likho format("  {}. {}", i + 1, tasks[i])
            }
        }
    } warna agar choice == "3" {
        maano idx = sankhya(poocho("Task number: ")) - 1
        agar idx >= 0 aur idx < lambai(tasks) {
            nikalo(tasks, idx)
            likho "ðŸ—‘ï¸ Task removed!"
        } warna {
            likho "Invalid number!"
        }
    } warna agar choice == "4" {
        running = jhooth
        likho "ðŸ‘‹ Alvida!"
    }
}
```

### Project 4: Fibonacci Sequence
```
kaam fibonacci(n) {
    agar n <= 0 {
        wapas do []
    }
    agar n == 1 {
        wapas do [0]
    }
    maano fib = [0, 1]
    har i = 2; i < n; i = i + 1 {
        maano next = fib[i-1] + fib[i-2]
        joodo(fib, next)
    }
    wapas do fib
}

maano result = fibonacci(20)
likho "First 20 Fibonacci numbers:"
likho result
likho format("Sum: {}", jod(result))
```

### Project 5: Simple Statistics
```
kaam stats(data) {
    maano n = lambai(data)
    maano sum = jod(data)
    maano mean = ausat(data)
    maano mx = sabse_bada(data)
    maano mn = sabse_chhota(data)

    # Variance
    maano sq_diff = 0
    har i = 0; i < n; i = i + 1 {
        maano diff = data[i] - mean
        sq_diff = sq_diff + diff * diff
    }
    maano variance = sq_diff / n
    maano std_dev = sqrt_val(variance)

    likho format("Count:    {}", n)
    likho format("Sum:      {}", sum)
    likho format("Mean:     {}", gol(mean * 100) / 100)
    likho format("Min:      {}", mn)
    likho format("Max:      {}", mx)
    likho format("Range:    {}", mx - mn)
    likho format("Std Dev:  {}", gol(std_dev * 100) / 100)
}

maano data = [23, 45, 12, 67, 34, 89, 56, 78, 41, 63]
stats(data)
```

---

## Lesson 14: Advanced Techniques ðŸ§ 

### Recursive data structures
```
# Binary search
kaam binary_search(list, target, low, high) {
    agar low > high {
        wapas do -1
    }
    maano mid = purn((low + high) / 2)
    agar list[mid] == target {
        wapas do mid
    } warna agar list[mid] < target {
        wapas do binary_search(list, target, mid + 1, high)
    } warna {
        wapas do binary_search(list, target, low, mid - 1)
    }
}

maano sorted = [2, 5, 8, 12, 16, 23, 38, 56, 72, 91]
likho binary_search(sorted, 23, 0, lambai(sorted) - 1)  # â†’ 5
```

### Higher-order functions
```
kaam apply(list, func) {
    maano result = []
    har i = 0; i < lambai(list); i = i + 1 {
        joodo(result, func(list[i]))
    }
    wapas do result
}

kaam square(x) { wapas do x * x }
kaam double(x) { wapas do x * 2 }

maano nums = [1, 2, 3, 4, 5]
likho apply(nums, square)    # â†’ [1, 4, 9, 16, 25]
likho apply(nums, double)    # â†’ [2, 4, 6, 8, 10]
```

### Memoization
```
# Cache for Fibonacci (much faster for large n)
maano cache = []

kaam fib_fast(n) {
    agar n <= 1 { wapas do n }

    # Build cache iteratively
    jabtak lambai(cache) <= n {
        maano len = lambai(cache)
        agar len == 0 {
            joodo(cache, 0)
        } warna agar len == 1 {
            joodo(cache, 1)
        } warna {
            joodo(cache, cache[len-1] + cache[len-2])
        }
    }
    wapas do cache[n]
}

likho fib_fast(50)   # instant!
```

---

## Lesson 15: System Integration âš™ï¸

```
# Get current date and time
likho format("Date: {}", taareekh())
likho format("Time: {}", waqt())
likho format("Timestamp: {}", timestamp())

# Platform info
likho format("OS: {}", platform())

# Environment variables
maano home = env_var("HOME")
agar kya_khali(home) {
    home = env_var("USERPROFILE")   # Windows fallback
}
likho format("Home: {}", home)

# Benchmarking
maano start = ghadi()
maano sum = 0
har i = 0; i < 1000000; i = i + 1 {
    sum = sum + i
}
maano elapsed = ghadi() - start
likho format("1M iterations in {} seconds", gol(elapsed * 1000) / 1000)

# Hashing
likho format("Hash of 'hello': {}", hash_val("hello"))
```

---

## Quick Keyword Reference ðŸ“–

| Hinglish | English | Usage |
|----------|---------|-------|
| `maano` | let/var | Variable declaration |
| `likho` | print | Output |
| `poocho` | input | User input |
| `agar` | if | Condition |
| `warna agar` | else if | Alternative condition |
| `warna` | else | Default branch |
| `jabtak` | while | While loop |
| `har` | for | For loop |
| `kaam` | function | Function definition |
| `wapas do` | return | Return value |
| `sach` | true | Boolean true |
| `jhooth` | false | Boolean false |
| `khali` | null | Null value |
| `aur` | and | Logical AND |
| `ya` | or | Logical OR |
| `nahi` | not | Logical NOT |
| `ruko` | break | Exit loop |
| `agla` | continue | Skip iteration |
| `bahar` | exit | Exit program |

---

## Next Steps ðŸš€

- Read the [API Reference](API_REFERENCE.md) for all 65+ built-in functions
- Check out the [examples/](../examples/) folder for more programs
- Read the [Language Specification](LANGUAGE_SPEC.md) for the full grammar
- Join us on [GitHub](https://github.com/Rampyaaryans/rampyaaryan)!

---

> Made with â¤ï¸ by Rampyaaryan
