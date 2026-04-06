#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "vm.h"
#include "chunk.h"
#include "value.h"
#include "table.h"
#include "compiler.h"
#include "ast.h"
#include "parser.h"

// stack macros
#define PUSH(val) (*vm->stack_top++ = (val))
#define POP() (*--vm->stack_top)
#define PEEK() (*(vm->stack_top - 1))
#define PEEK2() (*(vm->stack_top - 2))

// current frame macros
#define FRAME (vm->frames[vm->frame_count - 1])
#define READ_BYTE() (*FRAME.ip++)
#define READ_CONST() (FRAME.chunk->constants[READ_BYTE()])
#define READ_SHORT() \
    (FRAME.ip += 2,  \
     (uint16_t)((FRAME.ip[-2] << 8) | FRAME.ip[-1]))

static void runtime_error(VM *vm, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Runtime error: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    // print stack trace
    for (int i = vm->frame_count - 1; i >= 0; i--)
    {
        CallFrame *frame = &vm->frames[i];
        int offset = (int)(frame->ip - frame->chunk->code - 1);
        int line = frame->chunk->lines[offset];
        fprintf(stderr, "  [line %d]\n", line);
    }
}

// native functions
static Value native_print(int arg_count, Value *args)
{
    for (int i = 0; i < arg_count; i++)
    {
        value_print(args[i]);
        if (i < arg_count - 1)
            printf(" ");
    }
    printf("\n");
    return NULL_VAL();
}

static Value native_len(int arg_count, Value *args)
{
    if (arg_count != 1)
    {
        fprintf(stderr, "len() takes 1 argument.\n");
        return NULL_VAL();
    }
    if (IS_LIST(args[0]))
    {
        return NUMBER_VAL(AS_LIST(args[0])->count);
    }
    if (IS_STRING(args[0]))
    {
        return NUMBER_VAL(strlen(AS_STRING(args[0])));
    }
    fprintf(stderr, "len() argument must be a list or string.\n");
    return NULL_VAL();
}

static Value native_push(int arg_count, Value *args)
{
    if (arg_count != 2)
    {
        fprintf(stderr, "push() takes 2 arguments.\n");
        return NULL_VAL();
    }
    if (!IS_LIST(args[0]))
    {
        fprintf(stderr, "First argument to push() must be a list.\n");
        return NULL_VAL();
    }
    list_push(AS_LIST(args[0]), args[1]);
    return NULL_VAL();
}

static Value native_type(int arg_count, Value *args)
{
    if (arg_count != 1)
    {
        fprintf(stderr, "type() takes 1 argument.\n");
        return NULL_VAL();
    }
    switch (args[0].type)
    {
    case VAL_NULL:
        return STRING_VAL(strdup("null"));
    case VAL_BOOL:
        return STRING_VAL(strdup("bool"));
    case VAL_NUMBER:
        return STRING_VAL(strdup("number"));
    case VAL_STRING:
        return STRING_VAL(strdup("string"));
    case VAL_LIST:
        return STRING_VAL(strdup("list"));
    case VAL_FUNCTION:
        return STRING_VAL(strdup("function"));
    case VAL_NATIVE:
        return STRING_VAL(strdup("native"));
    default:
        return STRING_VAL(strdup("unknown"));
    }
}

static void register_native(VM *vm, const char *name, NativeFn fn)
{
    table_set(&vm->globals, name, NATIVE_VAL(fn));
}

void vm_init(VM *vm)
{
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    table_init(&vm->globals);

    // register native functions
    register_native(vm, "print", native_print);
    register_native(vm, "len", native_len);
    register_native(vm, "push", native_push);
    register_native(vm, "type", native_type);
}

void vm_free(VM *vm)
{
    table_free(&vm->globals);
}

static InterpretResult run(VM *vm)
{
    while (true)
    {
        // add this debug print here
        int offset = (int)(FRAME.ip - FRAME.chunk->code);
        printf("offset: %d opcode: %d\n", offset, FRAME.chunk->code[offset]);

        uint8_t instruction = READ_BYTE();

        switch (instruction)
        {

        case OP_PUSH:
        {
            Value constant = READ_CONST();
            PUSH(constant);
            break;
        }

        case OP_POP:
            POP();
            break;

        case OP_NULL:
            PUSH(NULL_VAL());
            break;
        case OP_TRUE:
            PUSH(BOOL_VAL(true));
            break;
        case OP_FALSE:
            PUSH(BOOL_VAL(false));
            break;

        // arithmetic
        case OP_ADD:
        {
            Value b = POP();
            Value a = POP();
            if (IS_NUMBER(a) && IS_NUMBER(b))
            {
                PUSH(NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b)));
            }
            else if (IS_STRING(a) && IS_STRING(b))
            {
                // string concatenation
                size_t len = strlen(AS_STRING(a)) + strlen(AS_STRING(b)) + 1;
                char *result = malloc(len);
                strcpy(result, AS_STRING(a));
                strcat(result, AS_STRING(b));
                PUSH(STRING_VAL(result));
            }
            else
            {
                runtime_error(vm, "Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }

        case OP_SUB:
        {
            Value b = POP();
            Value a = POP();
            if (!IS_NUMBER(a) || !IS_NUMBER(b))
            {
                runtime_error(vm, "Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(NUMBER_VAL(AS_NUMBER(a) - AS_NUMBER(b)));
            break;
        }

        case OP_MUL:
        {
            Value b = POP();
            Value a = POP();
            if (!IS_NUMBER(a) || !IS_NUMBER(b))
            {
                runtime_error(vm, "Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(NUMBER_VAL(AS_NUMBER(a) * AS_NUMBER(b)));
            break;
        }

        case OP_DIV:
        {
            Value b = POP();
            Value a = POP();
            if (!IS_NUMBER(a) || !IS_NUMBER(b))
            {
                runtime_error(vm, "Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (AS_NUMBER(b) == 0)
            {
                runtime_error(vm, "Division by zero.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(NUMBER_VAL(AS_NUMBER(a) / AS_NUMBER(b)));
            break;
        }

        case OP_MOD:
        {
            Value b = POP();
            Value a = POP();
            if (!IS_NUMBER(a) || !IS_NUMBER(b))
            {
                runtime_error(vm, "Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(NUMBER_VAL((int)AS_NUMBER(a) % (int)AS_NUMBER(b)));
            break;
        }

        case OP_NEGATE:
        {
            if (!IS_NUMBER(PEEK()))
            {
                runtime_error(vm, "Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(NUMBER_VAL(-AS_NUMBER(POP())));
            break;
        }

        // comparison
        case OP_EQ:
        {
            Value b = POP();
            Value a = POP();
            PUSH(BOOL_VAL(value_equal(a, b)));
            break;
        }

        case OP_NEQ:
        {
            Value b = POP();
            Value a = POP();
            PUSH(BOOL_VAL(!value_equal(a, b)));
            break;
        }

        case OP_LT:
        {
            Value b = POP();
            Value a = POP();
            if (!IS_NUMBER(a) || !IS_NUMBER(b))
            {
                runtime_error(vm, "Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(BOOL_VAL(AS_NUMBER(a) < AS_NUMBER(b)));
            break;
        }

        case OP_GT:
        {
            Value b = POP();
            Value a = POP();
            if (!IS_NUMBER(a) || !IS_NUMBER(b))
            {
                runtime_error(vm, "Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(BOOL_VAL(AS_NUMBER(a) > AS_NUMBER(b)));
            break;
        }

        case OP_LTE:
        {
            Value b = POP();
            Value a = POP();
            if (!IS_NUMBER(a) || !IS_NUMBER(b))
            {
                runtime_error(vm, "Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(BOOL_VAL(AS_NUMBER(a) <= AS_NUMBER(b)));
            break;
        }

        case OP_GTE:
        {
            Value b = POP();
            Value a = POP();
            if (!IS_NUMBER(a) || !IS_NUMBER(b))
            {
                runtime_error(vm, "Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(BOOL_VAL(AS_NUMBER(a) >= AS_NUMBER(b)));
            break;
        }

        case OP_NOT:
        {
            Value a = POP();
            PUSH(BOOL_VAL(!value_truthy(a)));
            break;
        }

        // variables
        case OP_DEFINE_GLOBAL:
        {
            Value name_val = READ_CONST();
            const char *name = AS_STRING(name_val);
            table_set(&vm->globals, name, PEEK());
            POP();
            break;
        }

        case OP_GET_GLOBAL:
        {
            Value name_val = READ_CONST();
            const char *name = AS_STRING(name_val);
            Value value;
            if (!table_get(&vm->globals, name, &value))
            {
                runtime_error(vm, "Undefined variable '%s'.", name);
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(value);
            break;
        }

        case OP_SET_GLOBAL:
        {
            Value name_val = READ_CONST();
            const char *name = AS_STRING(name_val);
            if (!table_has(&vm->globals, name))
            {
                runtime_error(vm, "Undefined variable '%s'.", name);
                return INTERPRET_RUNTIME_ERROR;
            }
            table_set(&vm->globals, name, PEEK());
            // don't pop — assignment is an expression, leaves value on stack
            break;
        }

        // control flow
        case OP_JUMP:
        {
            uint16_t offset = READ_SHORT();
            FRAME.ip += offset;
            break;
        }

        case OP_JUMP_IF_FALSE:
        {
            uint16_t offset = READ_SHORT();
            if (!value_truthy(PEEK()))
            {
                FRAME.ip += offset;
            }
            break;
        }

        case OP_LOOP:
        {
            uint16_t offset = READ_SHORT();
            FRAME.ip -= offset;
            break;
        }

        // lists
        case OP_BUILD_LIST:
        {
            int count = READ_BYTE();
            ObjList *list = list_new();

            // elements are on the stack in order, collect them
            Value *start = vm->stack_top - count;
            for (int i = 0; i < count; i++)
            {
                list_push(list, start[i]);
            }
            vm->stack_top -= count;
            PUSH(LIST_VAL(list));
            break;
        }

        case OP_INDEX_GET:
        {
            Value index = POP();
            Value list = POP();

            if (!IS_LIST(list))
            {
                runtime_error(vm, "Cannot index into a non-list value.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_NUMBER(index))
            {
                runtime_error(vm, "List index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }

            int i = (int)AS_NUMBER(index);
            ObjList *obj = AS_LIST(list);

            if (i < 0 || i >= obj->count)
            {
                runtime_error(vm, "List index %d out of bounds.", i);
                return INTERPRET_RUNTIME_ERROR;
            }

            PUSH(obj->elements[i]);
            break;
        }

        case OP_INDEX_SET:
        {
            Value value = POP();
            Value index = POP();
            Value list = POP();

            if (!IS_LIST(list))
            {
                runtime_error(vm, "Cannot index into a non-list value.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_NUMBER(index))
            {
                runtime_error(vm, "List index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }

            int i = (int)AS_NUMBER(index);
            ObjList *obj = AS_LIST(list);

            if (i < 0 || i >= obj->count)
            {
                runtime_error(vm, "List index %d out of bounds.", i);
                return INTERPRET_RUNTIME_ERROR;
            }

            obj->elements[i] = value;
            PUSH(value);
            break;
        }

        // functions
        case OP_CALL:
        {
            int arg_count = READ_BYTE();
            Value callee = *(vm->stack_top - arg_count - 1);
            printf("callee type: %d value type in globals: \n", callee.type);

            // print entire stack
            printf("stack: ");
            for (Value *v = vm->stack; v < vm->stack_top; v++)
            {
                printf("[type %d] ", v->type);
            }
            printf("\n");

            // native function
            if (IS_NATIVE(callee))
            {
                NativeFn fn = (NativeFn)AS_NATIVE(callee);
                Value result = fn(arg_count, vm->stack_top - arg_count);
                vm->stack_top -= arg_count + 1; // pop args and callee
                PUSH(result);
                break;
            }

            // user defined function
            if (IS_FUNCTION(callee))
            {
                ObjFunction *fn = AS_FUNCTION(callee);

                if (arg_count != fn->param_count)
                {
                    runtime_error(vm, "Expected %d arguments but got %d.",
                                  fn->param_count, arg_count);
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (vm->frame_count == FRAMES_MAX)
                {
                    runtime_error(vm, "Stack overflow.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // set up new call frame
                CallFrame *frame = &vm->frames[vm->frame_count++];
                frame->chunk = (Chunk *)fn->chunk;
                frame->ip = frame->chunk->code;
                frame->slots = vm->stack_top - arg_count;
                frame->params = fn->params;
                frame->param_count = fn->param_count;

                // bind arguments to parameter names in globals
                // (proper locals handled in future scope upgrade)
                for (int i = 0; i < fn->param_count; i++)
                {
                    Value arg = *(vm->stack_top - arg_count + i);
                    table_set(&vm->globals, fn->params[i], arg);
                }

                break;
            }

            runtime_error(vm, "Can only call functions.");
            return INTERPRET_RUNTIME_ERROR;
        }

        case OP_RETURN:
        {
            Value result = POP();

            vm->frame_count--;

            if (vm->frame_count == 0)
            {
                // top level return, done
                POP();
                return INTERPRET_OK;
            }

            // restore stack to before the call
            vm->stack_top = FRAME.slots;
            PUSH(result);
            break;
        }

        case OP_HALT:
            return INTERPRET_OK;

        default:
            runtime_error(vm, "Unknown opcode %d.", instruction);
            return INTERPRET_RUNTIME_ERROR;
        }
    }
}

InterpretResult vm_interpret(VM *vm, const char *source)
{
    // parse
    ASTNode *program = parser_parse(source);
    if (!program)
        return INTERPRET_COMPILE_ERROR;

    // compile
    Chunk *chunk = compiler_compile(program);
    ast_free(program);

    if (!chunk)
        return INTERPRET_COMPILE_ERROR;

    // set up top level call frame
    CallFrame *frame = &vm->frames[vm->frame_count++];
    frame->chunk = chunk;
    frame->ip = chunk->code;
    frame->slots = vm->stack;

    // DEBUGGING: chunk_disassemble(chunk, "main");

    InterpretResult result = run(vm);

    chunk_free(chunk);
    free(chunk);

    return result;
}