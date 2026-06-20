param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -Scope Global -ErrorAction SilentlyContinue) {
    $Global:PSNativeCommandUseErrorActionPreference = $false
}

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$exe = Join-Path $root "bin\kpp.exe"
$projectBin = Join-Path $root "bin"

function Normalize-Newlines {
    param([string]$Text)
    if ($null -eq $Text) {
        return ""
    }
    return ($Text -replace "`r`n", "`n").TrimEnd()
}

function Add-Test {
    param(
        [string]$Name,
        [string]$Source,
        [int]$ExitCode,
        [string]$Stdout = "",
        [string[]]$StderrContains = @()
    )

    [pscustomobject]@{
        Name = $Name
        Source = $Source
        ExitCode = $ExitCode
        Stdout = $Stdout
        StderrContains = $StderrContains
    }
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

$tests = @(
    Add-Test "arithmetic precedence and grouping" @'
print(1 + 2 * 3);
print((1 + 2) * 3);
print(8 / 2 - 3);
'@ 0 @'
7
9
1
'@

    Add-Test "locals shadow globals and unwind scopes" @'
let value = "global";
{
    let value = "local";
    print(value);
}
print(value);
'@ 0 @'
local
global
'@

    Add-Test "while loop and reassignment" @'
let i = 0;
let sum = 0;
while (i < 4) {
    sum = sum + i;
    i = i + 1;
}
print(sum);
'@ 0 @'
6
'@

    Add-Test "recursion and return values" @'
fun fact(n) {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}
print(fact(5));
'@ 0 @'
120
'@

    Add-Test "strings concatenate but do not coerce numbers" @'
print("kp" + "p");
print("answer " + 42);
'@ 70 @'
kpp
'@ @("Runtime error: Operands must be two numbers or two strings.")

    Add-Test "logical short circuit avoids runtime errors" @'
print(false and missing);
print(true or missing);
'@ 0 @'
false
true
'@

    Add-Test "undefined variable reports runtime error" @'
print(missingName);
'@ 70 "" @("Runtime error: Undefined variable 'missingName'.", "[line 1:7] in script")

    Add-Test "division by zero reports runtime error" @'
print(10 / 0);
'@ 70 "" @("Runtime error: Division by zero.")

    Add-Test "function arity mismatch reports runtime error" @'
fun add(a, b) {
    return a + b;
}
print(add(1));
'@ 70 "" @("Runtime error: Expected 2 arguments but got 1.")

    Add-Test "top level return is compile error" @'
return 1;
'@ 65 "" @("Cannot return from top-level code.")

    Add-Test "local self initializer is compile error" @'
{
    let value = value;
}
'@ 65 "" @("Cannot read local variable in its own initializer.")

    Add-Test "invalid assignment target is compile error" @'
1 = 2;
'@ 65 "" @("Invalid assignment target.")

    Add-Test "unterminated string is lexer error" @'
print("missing end);
'@ 65 "" @("Unterminated string.")
)

$cliTests = @(
    [pscustomobject]@{
        Name = "plain kpp.exe command accepts forward-slash project path"
        WorkingDirectory = $root
        Arguments = @("run/main.kpp")
        ExitCode = 0
        Stdout = @'
helloworld
15
42
kpp
'@
        StderrContains = @()
    }
)

$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("kpp-tests-" + [System.Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempDir | Out-Null

$failed = 0

try {
    foreach ($test in $tests) {
        $path = Join-Path $tempDir (($test.Name -replace '[^A-Za-z0-9_-]', '_') + ".kpp")
        Set-Content -Path $path -Value $test.Source -NoNewline

        $stdoutPath = Join-Path $tempDir "stdout.txt"
        $stderrPath = Join-Path $tempDir "stderr.txt"

        $previousErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = "Continue"
        & $exe $path > $stdoutPath 2> $stderrPath
        $actualExitCode = $LASTEXITCODE
        $ErrorActionPreference = $previousErrorActionPreference
        $actualStdout = Normalize-Newlines (Get-Content -Raw $stdoutPath)
        $actualStderr = Normalize-Newlines (Get-Content -Raw $stderrPath)
        $expectedStdout = Normalize-Newlines $test.Stdout

        $messages = @()
        if ($actualExitCode -ne $test.ExitCode) {
            $messages += "exit expected $($test.ExitCode), got $actualExitCode"
        }
        if ($actualStdout -ne $expectedStdout) {
            $messages += "stdout expected [$expectedStdout], got [$actualStdout]"
        }
        foreach ($needle in $test.StderrContains) {
            if (-not $actualStderr.Contains($needle)) {
                $messages += "stderr missing [$needle], got [$actualStderr]"
            }
        }

        if ($messages.Count -eq 0) {
            Write-Host "PASS $($test.Name)"
        }
        else {
            $failed++
            Write-Host "FAIL $($test.Name)"
            foreach ($message in $messages) {
                Write-Host "  $message"
            }
        }
    }

    foreach ($test in $cliTests) {
        $stdoutPath = Join-Path $tempDir "cli-stdout.txt"
        $stderrPath = Join-Path $tempDir "cli-stderr.txt"
        $originalPath = $env:Path
        $env:Path = "$projectBin;$originalPath"

        try {
            Push-Location $test.WorkingDirectory
            try {
                $previousErrorActionPreference = $ErrorActionPreference
                $ErrorActionPreference = "Continue"
                & kpp.exe @($test.Arguments) > $stdoutPath 2> $stderrPath
                $actualExitCode = $LASTEXITCODE
                $ErrorActionPreference = $previousErrorActionPreference
            }
            finally {
                Pop-Location
            }
        }
        finally {
            $env:Path = $originalPath
        }

        $actualStdout = Normalize-Newlines (Get-Content -Raw $stdoutPath)
        $actualStderr = Normalize-Newlines (Get-Content -Raw $stderrPath)
        $expectedStdout = Normalize-Newlines $test.Stdout
        $resolved = (& powershell -NoProfile -Command "`$env:Path='$projectBin;' + `$env:Path; (Get-Command kpp.exe).Source").Trim()

        $messages = @()
        if ($resolved -ne $exe) {
            $messages += "kpp.exe resolved to [$resolved], expected [$exe]"
        }
        if ($actualExitCode -ne $test.ExitCode) {
            $messages += "exit expected $($test.ExitCode), got $actualExitCode"
        }
        if ($actualStdout -ne $expectedStdout) {
            $messages += "stdout expected [$expectedStdout], got [$actualStdout]"
        }
        foreach ($needle in $test.StderrContains) {
            if (-not $actualStderr.Contains($needle)) {
                $messages += "stderr missing [$needle], got [$actualStderr]"
            }
        }

        if ($messages.Count -eq 0) {
            Write-Host "PASS $($test.Name)"
        }
        else {
            $failed++
            Write-Host "FAIL $($test.Name)"
            foreach ($message in $messages) {
                Write-Host "  $message"
            }
        }
    }
}
finally {
    Remove-Item -LiteralPath $tempDir -Recurse -Force
}

if ($failed -ne 0) {
    Write-Host "$failed test(s) failed."
    exit 1
}

$total = $tests.Count + $cliTests.Count
Write-Host "$total test(s) passed."
