# ============================================
# Rampyaaryan Installer (Windows PowerShell)
# ============================================

Write-Host ""
Write-Host "  Rampyaaryan Installer" -ForegroundColor Cyan
Write-Host "  ======================" -ForegroundColor Cyan
Write-Host ""

# Check for GCC (MinGW)
$gcc = Get-Command gcc -ErrorAction SilentlyContinue
$cl = Get-Command cl -ErrorAction SilentlyContinue

if ($gcc) {
    Write-Host "  [OK] GCC found: $($gcc.Source)" -ForegroundColor Green

    Write-Host "  Building Rampyaaryan..." -ForegroundColor Yellow

    $srcFiles = @(
        "src\memory.c", "src\value.c", "src\object.c", "src\table.c",
        "src\chunk.c", "src\lexer.c", "src\compiler.c", "src\vm.c",
        "src\native.c", "src\debug.c", "src\ascii_art.c", "src\main.c"
    )

    & gcc -O2 -o rampyaaryan.exe $srcFiles -lm

    if ($LASTEXITCODE -eq 0) {
        Write-Host "  [OK] Build successful!" -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] Build failed!" -ForegroundColor Red
        exit 1
    }
}
elseif ($cl) {
    Write-Host "  [OK] MSVC found" -ForegroundColor Green
    Write-Host "  Building with MSVC..." -ForegroundColor Yellow

    & cl /O2 /Fe:rampyaaryan.exe src\*.c

    if ($LASTEXITCODE -eq 0) {
        Write-Host "  [OK] Build successful!" -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] Build failed!" -ForegroundColor Red
        exit 1
    }
}
else {
    Write-Host "  [FAIL] C compiler nahi mila!" -ForegroundColor Red
    Write-Host "  Install karo:" -ForegroundColor Yellow
    Write-Host "    - MinGW: https://www.mingw-w64.org/" -ForegroundColor Gray
    Write-Host "    - Visual Studio: https://visualstudio.microsoft.com/" -ForegroundColor Gray
    Write-Host "    - Or: winget install -e --id GnuWin32.Make" -ForegroundColor Gray
    exit 1
}

# Add to PATH suggestion
$currentDir = Get-Location
Write-Host ""
Write-Host "  [OK] rampyaaryan.exe built successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "  Next steps:" -ForegroundColor Cyan
Write-Host "    1. Move rampyaaryan.exe to a permanent location" -ForegroundColor Gray
Write-Host "    2. Add that location to your system PATH" -ForegroundColor Gray
Write-Host "    3. Run: rampyaaryan --help" -ForegroundColor Gray
Write-Host ""
Write-Host "  Or run directly: .\rampyaaryan.exe examples\01_namaste_duniya.ram" -ForegroundColor Yellow
Write-Host ""
