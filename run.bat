@ECHO OFF
gcc -std=c99 -Wall -Wextra -pedantic -Iinclude src\main.c src\lexer.c src\parser.c src\ast.c src\chunk.c src\value.c src\object.c src\table.c src\compiler.c src\vm.c -o bin\kpp.exe
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
bin\kpp.exe run\main.kpp
