# Rampyaaryan VS Code Extension

Syntax highlighting, file icons, and language support for **Rampyaaryan** (`.ram`) — India's First Hinglish Programming Language.

## Features

- **Syntax Highlighting** — Full TextMate grammar for all Rampyaaryan keywords, built-in functions, strings, numbers, comments, and operators
- **File Icons** — `.ram` files show the Rampyaaryan logo icon in the explorer
- **Language Configuration** — Auto-closing brackets, comment toggling (`#`), indentation rules
- **File Association** — `.ram` files are automatically recognized as Rampyaaryan

## Supported Keywords

| Rampyaaryan | English |
|------------|---------|
| `maano` | let/var |
| `likho` | print |
| `agar` / `warna` | if/else |
| `jab tak` | while |
| `har` | for |
| `kaam` | function |
| `kaksha` | class |
| `wapas do` | return |
| `sach` / `jhooth` | true/false |
| `khali` | null |
| `ganana` | enum |
| `pakka` | assert |
| `koshish` / `pakdo` | try/catch |
| `shamil karo` | import |

## 215+ Built-in Functions

All highlighted with distinct colors by category: I/O, strings, lists, maps, math, higher-order, OOP, dates, and more.

## Installation

### From VSIX (GitHub Releases)

1. Download the `.vsix` file from [Releases](https://github.com/Rampyaaryans/rampyaaryan/releases)
2. In VS Code: `Ctrl+Shift+P` → "Extensions: Install from VSIX..."
3. Select the downloaded `.vsix` file
4. Reload VS Code

### From Source

```bash
cd vscode-extension
npm install -g @vscode/vsce
vsce package
code --install-extension rampyaaryan-1.0.0.vsix
```

## Links

- [Documentation](https://rampyaaryans.github.io/rampyaaryan/)
- [GitHub](https://github.com/Rampyaaryans/rampyaaryan)
- [API Reference](https://rampyaaryans.github.io/rampyaaryan/api.html)
