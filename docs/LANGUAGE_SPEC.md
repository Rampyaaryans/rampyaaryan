# Rampyaaryan Language Specification

**Version 1.0.0**

## 1. Lexical Structure

### 1.1 Tokens
- **Identifiers**: `[a-zA-Z_][a-zA-Z0-9_]*`
- **Numbers**: Integer (`42`) and float (`3.14`)
- **Strings**: Double-quoted (`"hello"`) with escape sequences (`\n`, `\t`, `\\`, `\"`)
- **Comments**: `#` or `//` (single-line)

### 1.2 Keywords
```
maano, likho, pucho, agar, warna, warna agar, 
jab tak, har, kaam, wapas do, ruko, agla,
sach, jhooth, khali, aur, ya, nahi
```

### 1.3 Operators
```
+  -  *  /  %  **                    (arithmetic)
== != > < >= <=                      (comparison)
aur  ya  nahi                        (logical)
=  +=  -=  *=  /=                    (assignment)
```

## 2. Data Types

| Type | Hinglish Name | Examples |
|---|---|---|
| Number | sankhya | `42`, `3.14`, `-7` |
| String | shabd | `"hello"`, `"namaste"` |
| Boolean | haan_na | `sach`, `jhooth` |
| Null | khali | `khali` |
| List | suchi | `[1, 2, 3]` |
| Function | kaam | `kaam add(a, b) { ... }` |

## 3. Statements

### Variable Declaration
```
maano <name> = <expression>
```

### Print Statement
```
likho <expression> [, <expression>]*
```
Multiple expressions are printed space-separated with a trailing newline.
A `;` separator prints without newline.

### If Statement
```
agar <condition> {
    <statements>
} warna agar <condition> {
    <statements>  
} warna {
    <statements>
}
```

### While Loop
```
jab tak <condition> {
    <statements>
}
```

### For Loop
```
har <init>; <condition>; <update> {
    <statements>
}
```

### Function
```
kaam <name>(<params>) {
    <statements>
    wapas do <expression>
}
```

### Break/Continue
```
ruko      # break out of loop
agla      # skip to next iteration
```

## 4. Expressions

### Precedence (low to high)
1. `ya` (or)
2. `aur` (and)
3. `== !=` (equality)
4. `> < >= <=` (comparison)
5. `+ -` (addition)
6. `* / %` (multiplication)
7. `**` (power)
8. `nahi -` (unary)
9. `() [] .` (call, index)

### Function Calls
```
<function>(<args>)
```

### List Indexing
```
<list>[<index>]
<list>[<index>] = <value>
```

### Input Expression
```
pucho(<prompt>)
```

## 5. Closures

Functions capture variables from enclosing scopes:
```
kaam counter() {
    maano n = 0
    kaam badhao() {
        n = n + 1
        wapas do n
    }
    wapas do badhao
}
```

## 6. Truthiness

- `jhooth` and `khali` are falsy
- `0` is falsy
- Empty string `""` is falsy
- Empty list `[]` is falsy
- Everything else is truthy

## 7. String Operations

- Concatenation: `"a" + "b"` → `"ab"`
- Repetition: `"ha" * 3` → `"hahaha"`
- Indexing: `"hello"[0]` → `"h"`
- Length: `lambai("hello")` → `5`

## 8. Runtime Architecture

- **Compilation**: Single-pass Pratt parser → bytecode
- **Execution**: Stack-based virtual machine
- **Memory**: Mark-sweep garbage collector
- **Strings**: Interned (hash table based)
- **Variables**: Hash table (global), stack slots (local)
