param(
    [ValidateSet("auto", "gcc", "clang", "cl")]
    [string]$Compiler = "auto",
    [string]$ConfigPath = "",
    # Run CMake cross-compile matrix for all supported architectures.
    # Requires toolchain compilers to be on PATH; missing toolchains are skipped.
    [switch]$Matrix
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$SrcDir = Join-Path $Root "src"
$Inc = Join-Path $Root "include"
$BuildDir = Join-Path $Root "build"
$BuildConfigHeader = Join-Path $Inc "osfx_build_config.h"
$LibraryDataHeader = Join-Path $SrcDir "osfx_library_data.generated.h"
if (-not $ConfigPath) {
    $ConfigPath = Join-Path $Root "Config.json"
}

# ---------------------------------------------------------------------------
# CMake cross-compile matrix
# ---------------------------------------------------------------------------
if ($Matrix) {
    # Each entry: [preset, toolchain_file, output_subdir, required_compiler]
    # toolchain_file = "" means use the host compiler (x86_64 native build).
    $MatrixTargets = @(
        @{ Preset="x86_64";      Toolchain="";                          OutDir="src/host";             Compiler="gcc"              },
        @{ Preset="cortexm0plus";Toolchain="arm-cortexm0plus.cmake";    OutDir="src/cortex-m0plus";    Compiler="arm-none-eabi-gcc" },
        @{ Preset="cortexm3";    Toolchain="arm-cortexm3.cmake";        OutDir="src/cortex-m3";        Compiler="arm-none-eabi-gcc" },
        @{ Preset="cortexm4";    Toolchain="arm-cortexm4.cmake";        OutDir="src/cortex-m4";        Compiler="arm-none-eabi-gcc" },
        @{ Preset="cortexm7";    Toolchain="arm-cortexm7.cmake";        OutDir="src/cortex-m7";        Compiler="arm-none-eabi-gcc" },
        @{ Preset="cortexm33";   Toolchain="arm-cortexm33.cmake";       OutDir="src/cortex-m33";       Compiler="arm-none-eabi-gcc" },
        @{ Preset="riscv32";     Toolchain="riscv32-elf.cmake";         OutDir="src/riscv32";          Compiler="riscv64-unknown-elf-gcc" },
        @{ Preset="esp32";       Toolchain="xtensa-esp32.cmake";        OutDir="src/esp32";            Compiler="xtensa-esp32-elf-gcc" },
        @{ Preset="avr328p";     Toolchain="avr-atmega328p.cmake";      OutDir="src/avr/atmega328p";   Compiler="avr-gcc"           },
        @{ Preset="avr2560";     Toolchain="avr-atmega2560.cmake";      OutDir="src/avr/atmega2560";   Compiler="avr-gcc"           }
    )

    $Results = @()
    foreach ($t in $MatrixTargets) {
        $compilerExe = $t.Compiler
        $available = [bool](Get-Command $compilerExe -ErrorAction SilentlyContinue)
        if (-not $available) {
            Write-Host "[skip] $($t.Preset) — $compilerExe not found on PATH"
            $Results += [PSCustomObject]@{ Target=$t.Preset; Status="SKIPPED"; Reason="$compilerExe not found" }
            continue
        }

        $buildSub = Join-Path $Root "build_matrix_$($t.Preset)"
        $outDir   = Join-Path $Root $t.OutDir
        New-Item -ItemType Directory -Path $outDir -Force | Out-Null

        $cmakeArgs = @(
            "-B", $buildSub,
            "-DOSFX_BUILD_EASY_API=ON",
            "-DOSFX_ARCH_PRESET=$($t.Preset)"
        )
        if ($t.Toolchain -ne "") {
            $toolchainPath = Join-Path $Root "cmake/toolchains/$($t.Toolchain)"
            $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$toolchainPath"
        }

        Write-Host "[cmake] configuring $($t.Preset)..."
        & cmake $Root @cmakeArgs 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[FAIL] cmake configure failed for $($t.Preset)"
            $Results += [PSCustomObject]@{ Target=$t.Preset; Status="FAILED"; Reason="cmake configure" }
            Remove-Item $buildSub -Recurse -Force -ErrorAction SilentlyContinue
            continue
        }

        Write-Host "[cmake] building $($t.Preset)..."
        & cmake --build $buildSub 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[FAIL] cmake build failed for $($t.Preset)"
            $Results += [PSCustomObject]@{ Target=$t.Preset; Status="FAILED"; Reason="cmake build" }
            Remove-Item $buildSub -Recurse -Force -ErrorAction SilentlyContinue
            continue
        }

        # Copy .a to precompiled output directory
        $lib = Get-ChildItem $buildSub -Recurse -Filter "*.a" | Select-Object -First 1
        if ($lib) {
            $dest = Join-Path $outDir "libOSynapticFX.a"
            Copy-Item $lib.FullName $dest -Force
            $kb = [math]::Round($lib.Length / 1024, 1)
            Write-Host "[ok] $($t.Preset) → $($t.OutDir)/libOSynapticFX.a ($kb KB)"
            $Results += [PSCustomObject]@{ Target=$t.Preset; Status="OK"; Reason="$kb KB" }
        }

        Remove-Item $buildSub -Recurse -Force -ErrorAction SilentlyContinue
    }

    Write-Host ""
    Write-Host "=== Matrix Build Summary ==="
    $Results | Format-Table Target, Status, Reason -AutoSize
    exit 0
}

# ---------------------------------------------------------------------------
# Single native build (original behaviour)
# ---------------------------------------------------------------------------

New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
Get-ChildItem -Path $BuildDir -File -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue

if (-not (Test-Path $LibraryDataHeader)) {
    throw "Missing required generated header: $LibraryDataHeader"
}
if (-not (Test-Path $BuildConfigHeader)) {
    throw "Missing required build config header: $BuildConfigHeader"
}

function Resolve-Compiler([string]$Preferred) {
    if ($Preferred -ne "auto") {
        return $Preferred
    }
    if (Get-Command clang -ErrorAction SilentlyContinue) { return "clang" }
    if (Get-Command gcc -ErrorAction SilentlyContinue) { return "gcc" }
    if (Get-Command cl -ErrorAction SilentlyContinue) { return "cl" }
    throw "No C compiler found (clang/gcc/cl)."
}

$Selected = Resolve-Compiler $Compiler
Write-Host "[build] compiler=$Selected"

if ($Selected -eq "gcc" -or $Selected -eq "clang") {
    $ObjFiles = @()
    $Lib = Join-Path $BuildDir "libosfx_core.a"

    Get-ChildItem -Path $SrcDir -Filter "*.c" | ForEach-Object {
        $Obj = Join-Path $BuildDir ($_.BaseName + ".o")
        & $Selected -std=c99 -O3 -D_CRT_SECURE_NO_WARNINGS -I $Inc -c $_.FullName -o $Obj
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $ObjFiles += $Obj
    }

    $Ar = Get-Command ar -ErrorAction SilentlyContinue
    if (-not $Ar) { throw "ar not found for static archive creation." }

    & $Ar.Source rcs $Lib $ObjFiles
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "[ok] built $Lib"
    exit 0
}

if ($Selected -eq "cl") {
    $ObjFiles = @()
    $Lib = Join-Path $BuildDir "osfx_core.lib"

    Get-ChildItem -Path $SrcDir -Filter "*.c" | ForEach-Object {
        $Obj = Join-Path $BuildDir ($_.BaseName + ".obj")
        & cl /nologo /TC /O2 /D_CRT_SECURE_NO_WARNINGS /I $Inc /c $_.FullName /Fo$Obj
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $ObjFiles += $Obj
    }

    $LibExe = Get-Command lib -ErrorAction SilentlyContinue
    if (-not $LibExe) { throw "lib.exe not found for static archive creation." }

    & $LibExe.Source /nologo /OUT:$Lib $ObjFiles
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "[ok] built $Lib"
    exit 0
}

throw "Unsupported compiler: $Selected"

