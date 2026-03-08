# Contributing to Rampyaaryan

Rampyaaryan mein contribute karne ke liye shukriya! 🙏

## How to Contribute

### 1. Fork & Clone
```bash
git clone https://github.com/YOUR_USERNAME/rampyaaryan-compiler.git
cd rampyaaryan-compiler
```

### 2. Build
```bash
make
```

### 3. Test
```bash
./bin/rampyaaryan examples/01_namaste_duniya.ram
```

### 4. Make Changes
- New features → `src/` directory
- New examples → `examples/` directory
- Bug fixes → Open an issue first

### 5. Submit PR
1. Create a branch: `git checkout -b feature/my-feature`
2. Commit: `git commit -m "Added: my feature"`
3. Push: `git push origin feature/my-feature`
4. Open a Pull Request

## Code Style

- C99 standard
- 4-space indentation
- Meaningful variable names
- Comments in English (code), Hinglish (user-facing strings)

## Areas for Contribution

- [ ] More built-in functions
- [ ] File I/O support
- [x] HashMap/Dictionary type
- [ ] Module/import system
- [ ] Error recovery improvements
- [ ] More examples
- [ ] VS Code syntax highlighting extension
- [ ] Online playground (WASM)
- [ ] Standard library in .ram files
- [ ] Performance benchmarks

## Reporting Issues

Use GitHub Issues. Include:
1. Rampyaaryan version (`rampyaaryan --version`)
2. OS and compiler version
3. Minimal `.ram` code to reproduce
4. Expected vs actual output

## License

By contributing, you agree that your contributions will be licensed under the Rampyaaryan License.
