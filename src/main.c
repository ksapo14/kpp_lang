#include <stdio.h>
#include <stdlib.h>

#include "vm.h"

static char *readFile(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file '%s'.\n", path);
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0)
    {
        fprintf(stderr, "Could not read file '%s'.\n", path);
        fclose(file);
        return NULL;
    }

    long fileSize = ftell(file);
    if (fileSize < 0)
    {
        fprintf(stderr, "Could not read file '%s'.\n", path);
        fclose(file);
        return NULL;
    }
    rewind(file);

    char *buffer = (char *)malloc((size_t)fileSize + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Out of memory reading '%s'.\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, sizeof(char), (size_t)fileSize, file);
    if (bytesRead < (size_t)fileSize)
    {
        fprintf(stderr, "Could not read file '%s'.\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "run\\main.kpp";
    char *source = readFile(path);
    if (source == NULL)
        return 74;

    initVM();
    InterpretResult result = interpret(source);
    free(source);
    freeVM();

    if (result == INTERPRET_COMPILE_ERROR)
        return 65;
    if (result == INTERPRET_RUNTIME_ERROR)
        return 70;
    return 0;
}
