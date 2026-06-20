# KPP Language

KPP is a small programming language project written in C. It started as a simple lexer and parser experiment and is being built out into a bytecode language with a virtual machine.

This project was made for fun by a student. It is not meant to be a production language yet; it is a learning project for understanding how programming languages are scanned, parsed, compiled to bytecode, and executed by an interpreter.

## Overview

KPP source files use the `.kpp` extension. The interpreter reads a KPP file, compiles it to bytecode, and executes that bytecode in a stack-based virtual machine written in C.

The current implementation includes:

- A lexer/scanner with line and column tracking.
- A parser/compiler that emits bytecode.
- A bytecode chunk format with constants and source locations.
- A stack-based VM.
- Global and local variables.
- Functions, calls, and return values.
- Control flow with `if`, `else`, and `while`.
- Runtime values for numbers, strings, booleans, and `nil`.
- Runtime and compile-time error messages.
- A small PowerShell test suite for edge cases.

## Project Layout

```text
bin/
  kpp.exe              Built executable

include/
  *.h                  C headers

src/
  *.c                  C source files

run/
  main.kpp             Default sample KPP program

scripts/
  install-kpp.ps1      Installs kpp.exe into PATH

tests/
  run-tests.ps1        Edge-case test runner
  ERROR_LOG.md         Known issues and future fixes

run.bat                Builds and runs run/main.kpp
```

## Building

This project currently targets Windows with GCC.

Build and run the default sample:

```powershell
.\run.bat
```

That command compiles the interpreter into:

```text
bin\kpp.exe
```

and then runs:

```text
run\main.kpp
```

## Installing The Command

To make `kpp.exe` available from a terminal:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\install-kpp.ps1
```

After installing, new terminals should be able to run:

```powershell
kpp.exe run/main.kpp
```

You can also run any KPP file by passing its path:

```powershell
kpp.exe path\to\program.kpp
```

If no file is passed, the interpreter defaults to:

```text
run\main.kpp
```

## Running Tests

Run the test suite with:

```powershell
powershell -ExecutionPolicy Bypass -File tests\run-tests.ps1
```

The tests cover arithmetic, scoping, loops, recursion, string behavior, short-circuit logic, runtime errors, compile errors, and command-line execution.

Known issues and future fixes are tracked in:

```text
tests\ERROR_LOG.md
```

## Syntax Examples

### Printing

```kpp
print("helloworld");
print(123);
print(true);
```

### Variables

Variables are declared with `let`.

```kpp
let name = "KPP";
let version = 1;

print(name);
print(version);
```

Variables can be reassigned:

```kpp
let count = 0;
count = count + 1;
print(count);
```

### Values

KPP currently supports:

```kpp
let numberValue = 42;
let stringValue = "hello";
let trueValue = true;
let falseValue = false;
let emptyValue = nil;
```

### Arithmetic

```kpp
print(1 + 2);
print(10 - 3);
print(4 * 5);
print(8 / 2);
print((1 + 2) * 3);
```

### Strings

Strings use double quotes.

```kpp
print("hello");
print("kp" + "p");
```

String concatenation only works between two strings. KPP does not currently coerce numbers into strings automatically.

```kpp
print("answer " + 42); // Runtime error
```

### Comparisons

```kpp
print(3 > 2);
print(3 >= 3);
print(2 < 3);
print(2 <= 2);
print(1 == 1);
print(1 != 2);
```

### Logic

```kpp
print(true and false);
print(true or false);
print(!false);
```

`and` and `or` short-circuit:

```kpp
print(false and missingVariable);
print(true or missingVariable);
```

### Blocks And Scope

Blocks use braces.

```kpp
let value = "global";

{
    let value = "local";
    print(value);
}

print(value);
```

Output:

```text
local
global
```

### If / Else

```kpp
let score = 90;

if (score >= 60) {
    print("pass");
} else {
    print("fail");
}
```

### While Loops

```kpp
let i = 1;
let total = 0;

while (i <= 5) {
    total = total + i;
    i = i + 1;
}

print(total);
```

### Functions

Functions are declared with `fun`.

```kpp
fun add(a, b) {
    return a + b;
}

print(add(20, 22));
```

Functions can call themselves recursively:

```kpp
fun fact(n) {
    if (n <= 1) {
        return 1;
    }

    return n * fact(n - 1);
}

print(fact(5));
```

### Comments

Line comments start with `//`.

```kpp
// This is a comment.
print("comments work");
```

## Current Limitations

KPP is still early. Some known limitations:

- No arrays or maps yet.
- No classes or modules yet.
- No closures yet.
- No garbage collector yet.
- Strings do not support escape sequences yet.
- Globals are resolved at runtime, so misspelled global names become runtime errors.
- The VM executes bytecode; it does not generate native machine code directly.

## Example Program

```kpp
print("helloworld");

let total = 0;
let i = 1;

while (i <= 5) {
    total = total + i;
    i = i + 1;
}

fun add(a, b) {
    return a + b;
}

print(total);
print(add(1, 1));
print("kp" + "p");
```

Expected output:

```text
helloworld
15
2
kpp
```

## Exit Codes

The interpreter currently uses these exit codes:

```text
0   Success
65  Compile error
70  Runtime error
74  File read error
```

## Project Goal

The goal of KPP is to learn by building. The language is intentionally small, but the internals are structured like a real interpreter: scanner, compiler, bytecode representation, runtime values, and VM execution.

Future work could include arrays, maps, modules, better diagnostics, closures, a garbage collector, and eventually a native-code backend or JIT experiment.
