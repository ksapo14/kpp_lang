#include "parser.h"

bool parseAndCompile(const char *source, ObjFunction **function)
{
    return compile(source, function);
}
