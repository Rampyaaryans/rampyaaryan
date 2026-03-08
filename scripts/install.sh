#!/bin/bash
# ============================================
# Rampyaaryan Installer (Linux/macOS)
# ============================================

set -e

echo ""
echo "  🙏 Rampyaaryan Installer"
echo "  ========================"
echo ""

# Check for C compiler
CC=""
if command -v gcc &> /dev/null; then
    CC="gcc"
elif command -v clang &> /dev/null; then
    CC="clang"
elif command -v cc &> /dev/null; then
    CC="cc"
else
    echo "  ❌ C compiler nahi mila! gcc ya clang install karo."
    echo "     Ubuntu/Debian: sudo apt install gcc"
    echo "     macOS: xcode-select --install"
    exit 1
fi

echo "  ✓ C compiler found: $CC"

# Build
echo "  ⏳ Building Rampyaaryan..."
$CC -O2 -o rampyaaryan \
    src/memory.c src/value.c src/object.c src/table.c \
    src/chunk.c src/lexer.c src/compiler.c src/vm.c \
    src/native.c src/debug.c src/ascii_art.c src/main.c \
    -lm

echo "  ✓ Build successful!"

# Install
INSTALL_DIR="/usr/local/bin"
echo "  ⏳ Installing to $INSTALL_DIR..."

if [ -w "$INSTALL_DIR" ]; then
    cp rampyaaryan "$INSTALL_DIR/"
else
    sudo cp rampyaaryan "$INSTALL_DIR/"
fi

chmod +x "$INSTALL_DIR/rampyaaryan"
echo "  ✓ Installed!"

# Cleanup
rm -f rampyaaryan

echo ""
echo "  🎉 Rampyaaryan install ho gaya!"
echo "  Run: rampyaaryan --help"
echo ""
