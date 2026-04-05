param(
    # New version string, e.g. "1.2.0".  If omitted, reads current version from library.properties.
    [string]$Version = "",

    # Compiler to use for quality-gate tests (auto / gcc / clang / cl).
    [ValidateSet("auto", "gcc", "clang", "cl")]
    [string]$Compiler = "auto",

    # Memory limit (KB) for the benchmark gate.
    [int]$MemoryLimitKB = 16,

    # Skip tests (faster iteration; not recommended for actual release).
    [switch]$SkipTests,

    # Skip benchmark gate.
    [switch]$SkipBench,

    # Skip matrix build of precompiled .a files.
    [switch]$SkipMatrix,

    # Build the Arduino library ZIP package.
    [switch]$Package,

    # Create a git tag and commit the version bump (requires clean working tree).
    [switch]$Tag,

    # Print what would happen without making any changes.
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

Set-Location $Root

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
function Step([string]$label) {
    Write-Host ""
    Write-Host "=== $label ===" -ForegroundColor Cyan
}

function Ok([string]$msg)   { Write-Host "[ok]   $msg" -ForegroundColor Green  }
function Fail([string]$msg) { Write-Host "[FAIL] $msg" -ForegroundColor Red; exit 1 }
function Info([string]$msg) { Write-Host "[info] $msg" }
function Dry([string]$msg)  { Write-Host "[dry]  $msg" -ForegroundColor Yellow }

# ---------------------------------------------------------------------------
# Step 0: Resolve and validate version
# ---------------------------------------------------------------------------
Step "Version"

$PropsPath = Join-Path $Root "library.properties"
$propsLines = Get-Content $PropsPath -Encoding UTF8

$verLine    = $propsLines | Where-Object { $_ -match '^version=' } | Select-Object -First 1
$currentVer = ($verLine -replace '^version=', '').Trim()
Info "Current version in library.properties: $currentVer"

if (-not $Version) {
    $Version = $currentVer
    Info "Using existing version: $Version"
} else {
    if ($Version -notmatch '^\d+\.\d+\.\d+(-[a-zA-Z0-9._]+)?$') {
        Fail "Invalid version format '$Version'. Expected semver (e.g. 1.2.0 or 1.2.0-rc1)."
    }
    if ($Version -ne $currentVer) {
        Info "Version bump: $currentVer → $Version"
        if (-not $DryRun) {
            $newLines = $propsLines | ForEach-Object { if ($_ -match '^version=') { "version=$Version" } else { $_ } }
            Set-Content $PropsPath -Value $newLines -Encoding UTF8
            Ok "library.properties updated"
        } else {
            Dry "Would write version=$Version to library.properties"
        }
    }
}

$TagName = "v$Version"
Info "Release tag: $TagName"

# ---------------------------------------------------------------------------
# Step 1: Build + quality gate
# ---------------------------------------------------------------------------
if (-not $SkipTests) {
    Step "Quality Gate (test matrix)"
    if ($DryRun) {
        Dry "Would run: scripts\test.ps1 -Matrix -Compiler $Compiler"
    } else {
        & powershell -ExecutionPolicy Bypass -File (Join-Path $Root "scripts\test.ps1") -Matrix -Compiler $Compiler
        if ($LASTEXITCODE -ne 0) { Fail "Quality gate failed. Aborting release." }
        Ok "Quality gate passed"
        $reportPath = Join-Path $Root "build\quality_gate_report.md"
        if (Test-Path $reportPath) {
            $failed = Select-String -Path $reportPath -Pattern "FAIL" -SimpleMatch
            if ($failed) { Fail "Quality gate report contains FAIL entries. Aborting." }
        }
    }
} else {
    Info "Skipping tests (-SkipTests)"
}

# ---------------------------------------------------------------------------
# Step 2: Benchmark gate
# ---------------------------------------------------------------------------
if (-not $SkipBench) {
    Step "Benchmark Gate"
    if ($DryRun) {
        Dry "Would run: scripts\bench.ps1 -Compiler $Compiler -MemoryLimitKB $MemoryLimitKB"
    } else {
        & powershell -ExecutionPolicy Bypass -File (Join-Path $Root "scripts\bench.ps1") -Compiler $Compiler -MemoryLimitKB $MemoryLimitKB
        if ($LASTEXITCODE -ne 0) { Fail "Benchmark gate failed. Aborting release." }
        Ok "Benchmark gate passed"
    }
} else {
    Info "Skipping bench (-SkipBench)"
}

# ---------------------------------------------------------------------------
# Step 3: Matrix build (precompiled .a)
# ---------------------------------------------------------------------------
if (-not $SkipMatrix) {
    Step "Matrix Build (precompiled .a)"
    if ($DryRun) {
        Dry "Would run: scripts\build.ps1 -Matrix"
    } else {
        & powershell -ExecutionPolicy Bypass -File (Join-Path $Root "scripts\build.ps1") -Matrix
        if ($LASTEXITCODE -ne 0) { Fail "Matrix build failed. Aborting release." }
        $libs = Get-ChildItem $Root -Recurse -Filter "libOSynapticFX.a" | Where-Object { $_.FullName -notmatch '\\build\\' }
        Ok "Precompiled libraries: $($libs.Count) target(s)"
        $libs | ForEach-Object { Info "  $($_.FullName.Replace($Root,''))" }
    }
} else {
    Info "Skipping matrix build (-SkipMatrix)"
}

# ---------------------------------------------------------------------------
# Step 4: Package
# ---------------------------------------------------------------------------
if ($Package) {
    Step "Package Arduino Library ZIP"

    $pkgName = "OSynaptic-FX"
    $zipName = "${pkgName}-${Version}.zip"
    $zipPath = Join-Path $Root $zipName
    $stagingDir = Join-Path $Root "_pkg_staging\$pkgName"

    if ($DryRun) {
        Dry "Would create $zipName"
    } else {
        # Clean staging
        if (Test-Path (Join-Path $Root "_pkg_staging")) {
            Remove-Item (Join-Path $Root "_pkg_staging") -Recurse -Force
        }
        New-Item -ItemType Directory -Path $stagingDir -Force | Out-Null

        # Copy Arduino library files
        Copy-Item (Join-Path $Root "library.properties")    $stagingDir
        Copy-Item (Join-Path $Root "README.md")             $stagingDir
        Copy-Item (Join-Path $Root "LICENSE")               $stagingDir
        Copy-Item (Join-Path $Root "keywords.txt")          $stagingDir
        Copy-Item (Join-Path $Root "src")    -Destination $stagingDir -Recurse
        Copy-Item (Join-Path $Root "include") -Destination $stagingDir -Recurse
        Copy-Item (Join-Path $Root "examples") -Destination $stagingDir -Recurse

        # Build artifacts in staging root for extras
        $reportSrc = Join-Path $Root "build\quality_gate_report.md"
        if (Test-Path $reportSrc) {
            Copy-Item $reportSrc $stagingDir
        }

        # Remove build artefacts that shouldn't ship
        Get-ChildItem $stagingDir -Recurse -Filter "*.o"   | Remove-Item -Force
        Get-ChildItem $stagingDir -Recurse -Filter "*.obj" | Remove-Item -Force

        if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
        Compress-Archive -Path $stagingDir -DestinationPath $zipPath -Force

        Remove-Item (Join-Path $Root "_pkg_staging") -Recurse -Force

        $kb = [math]::Round((Get-Item $zipPath).Length / 1024, 1)
        Ok "Package created: $zipName ($kb KB)"
    }
}

# ---------------------------------------------------------------------------
# Step 5: Git tag
# ---------------------------------------------------------------------------
if ($Tag) {
    Step "Git Tag"

    # Require clean working tree
    $status = git status --porcelain 2>&1
    if ($status -and (-not $DryRun)) {
        # If only library.properties changed (version bump), commit it
        $changed = git diff --name-only HEAD 2>&1
        if ($changed -and ($changed | Where-Object { $_ -ne "library.properties" })) {
            Fail "Working tree is not clean (beyond library.properties). Commit or stash changes first."
        }
        if (-not $DryRun) {
            git add library.properties
            git commit -m "chore: bump version to $Version"
            Ok "Committed version bump"
        }
    }

    $existingTag = git tag -l $TagName 2>&1
    if ($existingTag) {
        Fail "Tag $TagName already exists. Delete it first or choose a different version."
    }

    if ($DryRun) {
        Dry "Would create git tag: $TagName"
        Dry "Would run: git tag -a $TagName -m 'Release $TagName'"
    } else {
        git tag -a $TagName -m "Release $TagName"
        if ($LASTEXITCODE -ne 0) { Fail "git tag failed." }
        Ok "Created tag: $TagName"
        Info "Push with: git push origin $TagName"
        Info "This will trigger the GitHub Actions release workflow."
    }
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
Step "Release Summary"
Info "Version  : $Version"
Info "Tag      : $TagName"
Info "Tests    : $(if ($SkipTests) {'skipped'} else {'passed'})"
Info "Bench    : $(if ($SkipBench) {'skipped'} else {'passed'})"
Info "Matrix   : $(if ($SkipMatrix) {'skipped'} else {'built'})"
Info "Package  : $(if ($Package) {'created'} else {'--'})"
Info "Git tag  : $(if ($Tag) {'created'} else {'--'})"
if ($DryRun) { Info "(DRY RUN — no changes made)" }

Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
if ($Tag -and -not $DryRun) {
    Write-Host "  git push origin $TagName"
    Write-Host "  → GitHub Actions release workflow will build, package, and publish."
} else {
    Write-Host "  scripts\release.ps1 -Version $Version -Package -Tag"
    Write-Host "  git push origin $TagName"
}
