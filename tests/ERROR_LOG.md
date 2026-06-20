# KPP Error Log

This log tracks edge-case failures and language/runtime gaps found while testing.

## Open

- No open test failures yet. Add issues here when an edge-case test exposes a bug that should not be fixed immediately.

## Watchlist

- Strings do not support escape sequences yet, so `"a\nb"` is parsed as backslash plus `n`, not a newline.
- Functions do not implement closures. Nested functions can be called locally, but they cannot capture outer local variables.
- Runtime allocations are freed at VM shutdown, but there is no garbage collector for long-running programs.
- Globals are resolved at runtime, so misspelled globals are runtime errors rather than compile-time errors.

## Verified Edge Cases

- Plain terminal execution with `kpp.exe run/main.kpp`, including forward-slash paths.
- Arithmetic precedence and grouping.
- Local variable shadowing and block-scope cleanup.
- Loop reassignment.
- Recursive function calls.
- String concatenation and mixed string/number type errors.
- Logical short-circuiting.
- Undefined variables.
- Division by zero.
- Function arity mismatch.
- Top-level return compile error.
- Local variable self-initialization compile error.
- Invalid assignment targets.
- Unterminated strings.
