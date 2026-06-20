param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$projectBin = Join-Path $root "bin"
$exe = Join-Path $projectBin "kpp.exe"
$userBin = Join-Path $env:USERPROFILE ".local\bin"
$installedExe = Join-Path $userBin "kpp.exe"

function Add-UserPathFront {
    param([string]$PathToAdd)

    $resolved = (Resolve-Path $PathToAdd).Path.TrimEnd("\")
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $parts = @()
    if (-not [string]::IsNullOrWhiteSpace($userPath)) {
        $parts = $userPath -split ";" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    }

    $filtered = $parts | Where-Object {
        -not [string]::Equals($_.TrimEnd("\"), $resolved, [System.StringComparison]::OrdinalIgnoreCase)
    }

    [Environment]::SetEnvironmentVariable("Path", (@($resolved) + @($filtered)) -join ";", "User")

    $envParts = $env:Path -split ";" | Where-Object {
        -not [string]::IsNullOrWhiteSpace($_) -and
        -not [string]::Equals($_.TrimEnd("\"), $resolved, [System.StringComparison]::OrdinalIgnoreCase)
    }
    $env:Path = (@($resolved) + @($envParts)) -join ";"
}

if (-not $SkipBuild) {
    Push-Location $root
    try {
        & gcc -std=c99 -Wall -Wextra -pedantic -Iinclude src\main.c src\lexer.c src\parser.c src\ast.c src\chunk.c src\value.c src\object.c src\table.c src\compiler.c src\vm.c -o bin\kpp.exe
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed with exit code $LASTEXITCODE."
        }
    }
    finally {
        Pop-Location
    }
}

New-Item -ItemType Directory -Force -Path $projectBin, $userBin | Out-Null
Copy-Item -LiteralPath $exe -Destination $installedExe -Force

Add-UserPathFront $userBin
Add-UserPathFront $projectBin

Write-Host "Installed $installedExe"
Write-Host "Current resolution: $((Get-Command kpp.exe).Source)"
