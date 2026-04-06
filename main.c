#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        fprintf(stderr, "Could not open file '%s'.\n", path);
        exit(1);
    }

    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *buffer = malloc(size + 1);
    if (!buffer)
    {
        fprintf(stderr, "Not enough memory.\n");
        exit(1);
    }

    size_t bytes_read = fread(buffer, sizeof(char), size, file);
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

static void run_file(VM *vm, const char *path)
{
    char *source = read_file(path);
    InterpretResult result = vm_interpret(vm, source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

static void run_repl(VM *vm)
{
    char line[1024];
    printf("kpp v0.1\n");

    while (true)
    {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }
        if (strcmp(line, "exit\n") == 0)
            break;
        vm_interpret(vm, line);
    }
}

int main(int argc, char *argv[])
{
    VM vm;
    vm_init(&vm);

    if (argc == 1)
    {
        run_repl(&vm);
    }
    else if (argc == 2)
    {
        run_file(&vm, argv[1]);
    }
    else
    {
        fprintf(stderr, "Usage: kpp [file.kpp]\n");
        exit(1);
    }

    vm_free(&vm);
    return 0;
}