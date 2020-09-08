//---------------------------------------------------------------------------
// Add this define before including this file in only one of your source
// files
//
// #define FORTH_IMPLEMENT
// #include <forth/forth.h>
//
//---------------------------------------------------------------------------
// Integer number size. Defaults is 64 bits. Define one of these in your 
// preprocessors to use different size.
// 
// -DFORTH_INT_SIZE_32_BITS=1
// -DFORTH_INT_SIZE_16_BITS=1
// -DFORTH_INT_SIZE_8_BITS=1
//
//---------------------------------------------------------------------------
// Same thing with pointer types. Those are indices in the memory. If your
// have a memory max of 30k, you only need 16-bits pointer type for example.
// Default is 32 bits. Note: they are signed types
// 
// -DFORTH_POINTER_SIZE_64_BITS=1
// -DFORTH_POINTER_SIZE_16_BITS=1
// -DFORTH_POINTER_SIZE_8_BITS=1
//
//---------------------------------------------------------------------------
// You can redefine how many characters are store in the dictionnary per 
// entry. By default, it is 32. This doesn't mean word lengths are limited
// to this. It first checks the length, then compares those characters.
//
// i.e.: if you want 3 characters per word
// -DFORTH_DICT_CHAR_COUNT=3

#ifndef FORTH_H_INCLUDED
#define FORTH_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

//---------------------------------------------------------------------------
// PUBLIC
//---------------------------------------------------------------------------

#include <inttypes.h>
#include <stdlib.h>

#define FORTH_FAILURE 0
#define FORTH_SUCCESS 1

#if FORTH_INT_SIZE_8_BITS
typedef int8_t forth_pointer_type;
#elif DFORTH_POINTER_SIZE_16_BITS
typedef int16_t forth_pointer_type;
#elif DFORTH_POINTER_SIZE_64_BITS
typedef int64_t forth_pointer_type;
#else
#define DFORTH_POINTER_SIZE_32_BITS 1
typedef int32_t forth_pointer_type;
#endif

#if DFORTH_POINTER_SIZE_8_BITS
typedef int8_t forth_int_type;
#elif FORTH_INT_SIZE_16_BITS
typedef int16_t forth_int_type;
#elif FORTH_INT_SIZE_32_BITS
typedef int32_t forth_int_type;
#else
#define FORTH_INT_SIZE_64_BITS 1
typedef int64_t forth_int_type;
#endif

#ifndef FORTH_DICT_CHAR_COUNT
#define FORTH_DICT_CHAR_COUNT 32
#endif

typedef int (*forth_c_func)(struct ForthContext*);

typedef struct ForthCell
{
    union
    {
        forth_int_type int_value;
    };
} ForthCell;

typedef struct ForthContext
{
    uint8_t* memory;
    int memory_size;
    uint8_t memory_auto_resize;
    forth_pointer_type memory_pointer;
    forth_pointer_type program_pointer;

    ForthCell* stack;
    int stack_size;
    uint8_t stack_auto_resize;
    int stack_pointer;

    uint8_t* dict_name_lens;
    char* dict_names;
    forth_pointer_type* dict_pointers;
    int dict_size;
    uint8_t dict_auto_resize;
    int dict_pointer;

    int (*log)(struct ForthContext*, const char *fmt, ...);
    const char* code;
    int state;
} ForthContext;

// Create a context. Returns NULL if failed to create
//  memory_size : in bytes. -1 for infinite (Allocated in chunks of 1k)
//  stack_size  : in cell count. -1 for infinite (Allocated in chunks of 1k)
//  dict_size   : Dictionnary size, in word count. -1 for infinite (Allocated in chunks of 1k)
ForthContext* forth_createContext(int memory_size = -1, int stack_size = -1, int dict_size = -1);

// Destroy a context
void forth_destroyContext(ForthContext* ctx);

// Returns cell on top of the stack, or NULL if stack is empty
ForthCell* forth_top(ForthContext* ctx, int offset = 0);

// Evaluate code. Returns FORTH_SUCCESS on success
int forth_eval(ForthContext* ctx, const char* code);

// Add a C-function word to the dictionnary. Returns FORTH_SUCCESS on success
int forth_addCWord(ForthContext* ctx, const char* name, forth_c_func fn);

//---------------------------------------------------------------------------
// IMPLEMENTATION
//---------------------------------------------------------------------------

#if defined(FORTH_IMPLEMENT)

#include <math.h>

#define FORTHI_STATE_INTERPRET 0
#define FORTHI_STATE_COMPILE 1

#define FORTHI_MEM_ALLOC_CHUNK_SIZE 1024

#define FORTHI_INST_RETURN 0
#define FORTHI_INST_CALL_C_FUNCTION 1
#define FORTHI_INST_PUSH_INT_NUMBER 2
#define FORTHI_INST_WORD_CALL 3

#define FORTH_LOG(_ctx_, _fmt_, ...) \
{ \
    if (ctx && ctx->log) \
        ctx->log(_ctx_, _fmt_, __VA_ARGS__); \
    else \
        printf(_fmt_, __VA_ARGS__); \
}

static int forthi_pushCell(ForthContext* ctx, ForthCell cell)
{
    if (ctx->stack_pointer >= ctx->stack_size)
    {
        FORTH_LOG(ctx, "Stack overflow\n");
        return FORTH_FAILURE;
    }

    ctx->stack[ctx->stack_pointer++] = cell;
    return FORTH_SUCCESS;
}

static int forthi_pushIntNumber(ForthContext* ctx, forth_int_type number)
{
    ForthCell cell;
    cell.int_value = number;
    return forthi_pushCell(ctx, cell);
}

static int forthi_pop(ForthContext* ctx, int count = 1)
{
    if (ctx->stack_pointer < count)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    ctx->stack_pointer -= count;
    return FORTH_SUCCESS;
}

static int forthi_growMemory(ForthContext* ctx)
{
    int new_size = ctx->memory_size + FORTHI_MEM_ALLOC_CHUNK_SIZE;

    uint8_t* new_memory = (uint8_t*)malloc(new_size);
    if (!new_memory)
        return FORTH_FAILURE;

    memcpy(new_memory, ctx->memory, ctx->memory_size);
    free(ctx->memory);
    ctx->memory = new_memory;
    ctx->memory_size = new_size;

    return FORTH_SUCCESS;
}

static int forthi_growDictionnary(ForthContext* ctx)
{
    uint8_t* new_name_lens = (uint8_t*)malloc(ctx->dict_size + FORTHI_MEM_ALLOC_CHUNK_SIZE);
    if (!new_name_lens)
        return FORTH_FAILURE;
    memcpy(new_name_lens + FORTHI_MEM_ALLOC_CHUNK_SIZE, ctx->dict_name_lens, ctx->dict_size);
    free(ctx->dict_name_lens);
    ctx->dict_name_lens = new_name_lens;

    char* new_names = (char*)malloc((ctx->dict_size + FORTHI_MEM_ALLOC_CHUNK_SIZE) * FORTH_DICT_CHAR_COUNT);
    if (!new_names)
        return FORTH_FAILURE;
    memcpy(new_names + FORTHI_MEM_ALLOC_CHUNK_SIZE * FORTH_DICT_CHAR_COUNT, ctx->dict_names, ctx->dict_size * FORTH_DICT_CHAR_COUNT);
    free(ctx->dict_names);
    ctx->dict_names = new_names;

    forth_pointer_type* new_pointers = (forth_pointer_type*)malloc((ctx->dict_size + FORTHI_MEM_ALLOC_CHUNK_SIZE) * sizeof(forth_pointer_type));
    if (!new_pointers)
        return FORTH_FAILURE;
    memcpy(new_pointers + FORTHI_MEM_ALLOC_CHUNK_SIZE, ctx->dict_pointers, ctx->dict_size * sizeof(forth_pointer_type));
    free(ctx->dict_pointers);
    ctx->dict_pointers = new_pointers;

    ctx->dict_size += FORTHI_MEM_ALLOC_CHUNK_SIZE;

    return FORTH_SUCCESS;
}

static int forthi_reserveMemorySpace(ForthContext* ctx, int size)
{
    int space_left = ctx->memory_size - ctx->memory_pointer;
    while (size > space_left)
    {
        if (!ctx->memory_auto_resize)
        {
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }

        if (forthi_growMemory(ctx) == FORTH_FAILURE)
        {
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }

        space_left = ctx->memory_size - ctx->memory_pointer;
    }

    return FORTH_SUCCESS;
}

static int forthi_word_semicolon(ForthContext* ctx);

static int forthi_addFunctionInst(ForthContext* ctx, forth_c_func fn)
{
    if (ctx->state == FORTHI_STATE_COMPILE && fn == forthi_word_semicolon)
        return forthi_word_semicolon(ctx);

    if (forthi_reserveMemorySpace(ctx, sizeof(fn) + 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    ctx->memory[ctx->memory_pointer++] = FORTHI_INST_CALL_C_FUNCTION;
    memcpy(ctx->memory + ctx->memory_pointer, &fn, sizeof(forth_c_func));
    ctx->memory_pointer += (int)sizeof(fn);

    return FORTH_SUCCESS;
}

static int forthi_addReturnInst(ForthContext* ctx)
{
    ctx->state = FORTHI_STATE_INTERPRET;

    if (forthi_reserveMemorySpace(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    ctx->memory[ctx->memory_pointer++] = FORTHI_INST_RETURN;

    return FORTH_SUCCESS;
}

static int forthi_addPushIntNumberInst(ForthContext* ctx, forth_int_type number)
{
    if (forthi_reserveMemorySpace(ctx, sizeof(forth_int_type) + 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    ctx->memory[ctx->memory_pointer++] = FORTHI_INST_PUSH_INT_NUMBER;
    *(forth_int_type*)&ctx->memory[ctx->memory_pointer] = number;
    ctx->memory_pointer += (int)sizeof(forth_int_type);

    return FORTH_SUCCESS;
}

static int forthi_addWordCallInst(ForthContext* ctx, forth_pointer_type pointer)
{
    if (forthi_reserveMemorySpace(ctx, sizeof(forth_pointer_type) + 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    ctx->memory[ctx->memory_pointer++] = FORTHI_INST_WORD_CALL;
    *(forth_pointer_type*)&ctx->memory[ctx->memory_pointer] = pointer;
    ctx->memory_pointer += (int)sizeof(forth_pointer_type);

    return FORTH_SUCCESS;
}

static int forthi_addWord(ForthContext* ctx, const char* name, int name_len, forth_pointer_type memory_offset)
{
    if (ctx->dict_pointer >= ctx->dict_size)
    {
        if (!ctx->dict_auto_resize)
        {
            FORTH_LOG(ctx, "Dictionnary full\n");
            return FORTH_FAILURE;
        }

        if (forthi_growDictionnary(ctx) == FORTH_FAILURE)
        {
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }
    }

    int index = ctx->dict_size - ctx->dict_pointer - 1;
    ctx->dict_name_lens[index] = name_len;
    ctx->dict_pointers[index] = memory_offset;
    int name_copy_len = name_len > FORTH_DICT_CHAR_COUNT ? FORTH_DICT_CHAR_COUNT : name_len;
    memcpy(ctx->dict_names + index * FORTH_DICT_CHAR_COUNT, name, name_copy_len);

    ctx->dict_pointer++;

    return FORTH_SUCCESS;
}

int forth_addCWord(ForthContext* ctx, const char* name, forth_c_func fn)
{
    forth_pointer_type memory_pointer = ctx->memory_pointer;
    if (forthi_addFunctionInst(ctx, fn) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forthi_addWord(ctx, name, (int)strlen(name), memory_pointer);

    return FORTH_SUCCESS;
}

static int forthi_isSpace(char c)
{
    return !c || c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static const char* forthi_trimCode(ForthContext* ctx)
{
    while (*ctx->code && forthi_isSpace(*ctx->code))
        ctx->code++;

    return ctx->code;
}

static const char* forthi_getNextToken(ForthContext* ctx, size_t* token_len)
{
    const char* token_start = forthi_trimCode(ctx);

    while (*ctx->code)
    {
        ctx->code++;
        if (forthi_isSpace(*ctx->code))
        {
            *token_len = ctx->code - token_start;
            return token_start;
        }
    }

    return NULL;
}

static int forthi_isTokenNumber(const char* token, size_t token_len)
{
    if (*token >= '0' && *token <= '9')
        return 1;

    if (*token == '-' && token_len > 1)
        if (token[1] >= '0' && token[1] <= '9')
            return 1;

    return 0;
}

static forth_pointer_type forthi_getWord(ForthContext* ctx, const char* name, size_t name_len)
{
    int index = ctx->dict_size - ctx->dict_pointer;
    if (ctx->state == FORTHI_STATE_COMPILE)
        index++; // Skip word being compiled

    size_t compare_len = name_len > FORTH_DICT_CHAR_COUNT ? FORTH_DICT_CHAR_COUNT : name_len;
    while (index < ctx->dict_size)
    {
        if (ctx->dict_name_lens[index] == name_len)
            if (strncmp(ctx->dict_names + index * FORTH_DICT_CHAR_COUNT, name, compare_len) == 0)
                return ctx->dict_pointers[index];

        index++;
    }

    return -1;
}

static int forthi_execute(ForthContext* ctx, forth_pointer_type offset)
{
    forth_pointer_type previous_offset = ctx->program_pointer;
    ctx->program_pointer = offset;

    uint8_t inst = ctx->memory[ctx->program_pointer++];
    while (ctx->program_pointer < (forth_pointer_type)ctx->memory_size)
    {
        if (inst == FORTHI_INST_RETURN)
        {
            break;
        }
        else if (inst == FORTHI_INST_CALL_C_FUNCTION)
        {
            forth_c_func fn = NULL;
            memcpy(&fn, ctx->memory + ctx->program_pointer, sizeof(forth_c_func));
            ctx->program_pointer += sizeof(forth_c_func);
            if (fn(ctx) == FORTH_FAILURE)
                return FORTH_FAILURE;
        }
        else if (inst == FORTHI_INST_PUSH_INT_NUMBER)
        {
            forth_int_type number = *(forth_int_type*)&ctx->memory[ctx->program_pointer];
            ctx->program_pointer += sizeof(forth_int_type);
            if (forthi_pushIntNumber(ctx, number) == FORTH_FAILURE)
                return FORTH_FAILURE;
        }
        else if (inst == FORTHI_INST_WORD_CALL)
        {
            forth_pointer_type pointer = *(forth_pointer_type*)&ctx->memory[ctx->program_pointer];
            ctx->program_pointer += sizeof(forth_pointer_type);
            if (forthi_execute(ctx, pointer) == FORTH_FAILURE)
                return FORTH_FAILURE;
        }

        inst = ctx->memory[ctx->program_pointer++];
    }

    ctx->program_pointer = previous_offset;
    return FORTH_SUCCESS;
}

static int forthi_interpretToken(ForthContext* ctx, const char* token, size_t token_len)
{
    forth_pointer_type memory_pointer = forthi_getWord(ctx, token, token_len);
    if (memory_pointer >= 0)
    {
        uint8_t word_type = ctx->memory[memory_pointer];
        if (word_type == FORTHI_INST_CALL_C_FUNCTION)
        {
            forth_c_func fn = NULL;
            memcpy(&fn, ctx->memory + memory_pointer + 1, sizeof(forth_c_func));

            if (ctx->state == FORTHI_STATE_INTERPRET)
                return fn(ctx);
            else
                return forthi_addFunctionInst(ctx, fn);
        }
        else
        {
            if (ctx->state == FORTHI_STATE_INTERPRET)
                return forthi_execute(ctx, memory_pointer);
            else
                return forthi_addWordCallInst(ctx, memory_pointer);
        }

        return FORTH_FAILURE;
    }

    if (forthi_isTokenNumber(token, token_len))
    {
        forth_int_type number = (forth_int_type)atoi(token);
        if (ctx->state == FORTHI_STATE_INTERPRET)
        {
            return forthi_pushIntNumber(ctx, number);
        }
        else  if (ctx->state == FORTHI_STATE_COMPILE)
        {
            return forthi_addPushIntNumberInst(ctx, number);
        }

        return FORTH_FAILURE;
    }

    return FORTH_FAILURE;
}

static int forthi_interpret(ForthContext* ctx)
{
    const char* token;
    size_t token_len;
    while (token = forthi_getNextToken(ctx, &token_len))
    {
        if (forthi_interpretToken(ctx, token, token_len) == FORTH_FAILURE)
            return FORTH_FAILURE;
    }

    return FORTH_SUCCESS;
}

//---------------------------------------------------------------------------
// STANDARD WORDS
//---------------------------------------------------------------------------

static int forthi_word_store(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_number_sign(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_number_sign_greater(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_number_sign_s(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_tick(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_paren(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_paren_local_paren(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_star(ForthContext* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int_type n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_pushIntNumber(ctx, n1 * n2);
}

static int forthi_word_star_slash(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_star_slash_mod(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_plus(ForthContext* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int_type n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_pushIntNumber(ctx, n1 + n2);
}

static int forthi_word_plus_store(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_plus_field(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_plus_loop(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_plus_x_string(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_comma(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_minus(ForthContext* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int_type n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_pushIntNumber(ctx, n1 - n2);
}

static int forthi_word_dash_trailing(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dash_trailing_garbage(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dot(ForthContext* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n = ctx->stack[ctx->stack_pointer].int_value;
    FORTH_LOG(ctx, "%" PRId64 " ", n);
    return FORTH_SUCCESS;
}

static int forthi_word_dot_quote(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dot_paren(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dot_r(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dot_s(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_slash(ForthContext* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int_type n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_pushIntNumber(ctx, (forth_int_type)floor((double)n1 / (double)n2));
}

static int forthi_word_slash_mod(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_slash_string(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_zero_less(ForthContext* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_pushIntNumber(ctx, n < 0 ? -1 : 0);
}

static int forthi_word_zero_not_equals(ForthContext* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_pushIntNumber(ctx, n != 0 ? -1 : 0);
}

static int forthi_word_zero_equals(ForthContext* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_pushIntNumber(ctx, n == 0 ? -1 : 0);
}

static int forthi_word_zero_greater(ForthContext* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_pushIntNumber(ctx, n > 0 ? -1 : 0);
}

static int forthi_word_one_plus(ForthContext* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_pushIntNumber(ctx, n + 1);
}

static int forthi_word_one_minus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_store(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_star(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_slash(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_to_r(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_fetch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_constant(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_drop(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_dupe(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_literal(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_over(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_r_from(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_r_fetch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_rote(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_swap(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_value(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_variable(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_colon(ForthContext* ctx)
{
    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        FORTH_LOG(ctx, "Unexpected ':'\n");
        return FORTH_FAILURE;
    }

    size_t word_name_len;
    const char* word_name = forthi_getNextToken(ctx, &word_name_len);
    if (!word_name || !word_name_len)
    {
        FORTH_LOG(ctx, "Expected name after ':'\n");
        return FORTH_FAILURE;
    }

    if (forthi_addWord(ctx, word_name, (int)word_name_len, ctx->memory_pointer) == FORTH_FAILURE)
        return FORTH_FAILURE;

    ctx->state = FORTHI_STATE_COMPILE;

    return FORTH_SUCCESS;
}

static int forthi_word_colon_no_name(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_semicolon(ForthContext* ctx)
{
    if (ctx->state != FORTHI_STATE_COMPILE)
    {
        FORTH_LOG(ctx, "Unexpected ';'\n");
        return FORTH_FAILURE;
    }

    ctx->state = FORTHI_STATE_INTERPRET;

    return forthi_addReturnInst(ctx);
}

static int forthi_word_semicolon_code(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_less_than(ForthContext* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int_type n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_pushIntNumber(ctx, n1 < n2 ? -1 : 0);
}

static int forthi_word_less_number_sign(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_not_equals(ForthContext* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int_type n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_pushIntNumber(ctx, n1 != n2 ? -1 : 0);
}

static int forthi_word_equals(ForthContext* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int_type n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_pushIntNumber(ctx, n1 == n2 ? -1 : 0);
}

static int forthi_word_greater_than(ForthContext* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int_type n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int_type n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_pushIntNumber(ctx, n1 > n2 ? -1 : 0);
}

static int forthi_word_to_body(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_to_float(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_to_in(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_to_number(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_to_r(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_question(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_question_do(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_question_dupe(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_fetch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ABORT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_abort_quote(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_abs(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ACCEPT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ACTION_OF(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_AGAIN(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_AHEAD(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALIGN(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALIGNED(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALLOCATE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALLOT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALSO(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_AND(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ASSEMBLER(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_at_x_y(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BASE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BEGIN(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BEGIN_STRUCTURE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BIN(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_b_l(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BLANK(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_b_l_k(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BLOCK(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BUFFER(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_buffer_colon(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BYE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_store(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_quote(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_comma(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_fetch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CASE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CATCH(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_cell_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CELLS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_field_colon(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_char(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_char_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_chars(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CLOSE_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_move(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_move_up(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CODE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_COMPARE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_compile_comma(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CONSTANT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_COUNT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_r(ForthContext* ctx)
{
    FORTH_LOG(ctx, "\n");
    return FORTH_SUCCESS;
}

static int forthi_word_CREATE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CREATE_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_s_pick(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_s_roll(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_minus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_dot(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_dot_r(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_zero_less(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_zero_equals(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_two_star(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_two_slash(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_less_than(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_equals(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_to_f(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_to_s(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_abs(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DECIMAL(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DEFER(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_defer_store(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_defer_fetch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DEFINITIONS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DELETE_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DEPTH(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_store(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_fetch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_align(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_aligned(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_field_colon(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_float_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_floats(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_max(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_min(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_negate(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DO(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_does(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DROP(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_u_less(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DUMP(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dupe(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_EDITOR(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key_to_char(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key_to_f_key(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key_to_x_char(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key_question(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ELSE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_EMIT(ForthContext* ctx)
{
     if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    char c = (char)ctx->stack[ctx->stack_pointer].int_value;
    FORTH_LOG(ctx, "%c", c);
    return FORTH_SUCCESS;
}

static int forthi_word_emit_question(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_EMPTY_BUFFERS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_END_STRUCTURE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_end_case(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_end_of(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_environment_query(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ERASE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_EVALUATE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_EXECUTE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_EXIT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_store(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_star(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_star_star(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_minus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_dot(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_slash(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_zero_less_than(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_zero_equals(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_to_d(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_F_to_S(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_fetch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_abs(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_cos(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_cosh(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_align(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_aligned(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_log(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FALSE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_sine(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_cinch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_tan(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_tan_two(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_tan_h(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_constant(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_cos(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_cosh(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_depth(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_drop(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_dupe(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_e_dot(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_e_x_p(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_e_x_p_m_one(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_field_colon(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_field_colon(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FILE_POSITION(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FILE_SIZE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FILE_STATUS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FILL(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FIND(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_literal(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_l_n(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_l_n_p_one(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_float_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FLOATS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_log(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FLOOR(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FLUSH(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FLUSH_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_m_slash_mod(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_max(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_min(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_negate(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FORGET(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FORTH(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FORTH_WORDLIST(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_over(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FREE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_rote(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_round(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_s_dot(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_sine(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_sine_cos(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_cinch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_square_root(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_swap(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_tan(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_tan_h(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_trunc(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_value(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_variable(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_proximate(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_GET_CURRENT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_GET_ORDER(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_HERE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_HEX(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_HOLD(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_HOLDS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_I(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_IF(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_IMMEDIATE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_INCLUDE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_INCLUDE_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_INCLUDED(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_INVERT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_IS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_J(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_ALT_MASK(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_CTRL_MASK(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_DELETE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_DOWN(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_END(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_1(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_10(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_11(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_12(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_2(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_3(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_4(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_5(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_6(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_7(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_8(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_9(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_HOME(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_INSERT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_LEFT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_NEXT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_PRIOR(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_RIGHT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_SHIFT_MASK(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_UP(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_KEY(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_key_question(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_LEAVE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_LIST(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_LITERAL(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_LOAD(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_locals_bar(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_LOOP(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_l_shift(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_m_star(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_m_star_slash(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_m_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_MARKER(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_MAX(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_MIN(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_MOD(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_MOVE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_MS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_n_to_r(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_name_to_compile(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_name_to_interpret(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_name_to_string(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_NEGATE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_NIP(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_n_r_from(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_OF(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ONLY(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_OPEN_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_OR(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ORDER(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_OVER(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PAD(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PAGE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PARSE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PARSE_NAME(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PICK(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_POSTPONE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PRECISION(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PREVIOUS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_QUIT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_r_o(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_r_w(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_r_from(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_r_fetch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_READ_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_READ_LINE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RECURSE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REFILL(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RENAME_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REPEAT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REPLACES(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REPOSITION_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REPRESENT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REQUIRE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REQUIRED(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RESIZE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RESIZE_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RESTORE_INPUT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ROLL(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_rote(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_r_shift(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_quote(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_to_d(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_to_F(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SAVE_BUFFERS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SAVE_INPUT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_c_r(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SEARCH(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SEARCH_WORDLIST(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SEE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SET_CURRENT(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SET_ORDER(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SET_PRECISION(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_store(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_fetch(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_align(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_aligned(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_field_colon(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_float_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_floats(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SIGN(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SLITERAL(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_m_slash_rem(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SOURCE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_source_i_d(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SPACE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SPACES(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_STATE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SUBSTITURE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SWAP(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SYNONYM(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_backslash_quote(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_THEN(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_THROW(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_THRU(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_time_and_date(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TO(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TRAVERSE_WORDLIST(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TRUE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TUCK(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TYPE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_u_dot(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_u_dot_r(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_u_less_than(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_u_greater_than(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_u_m_star(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_u_m_slash_mod(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_UNESCAPE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_UNLOOP(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_UNTIL(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_UNUSED(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_UPDATE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_VALUE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_VARIABLE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_w_o(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WHILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WITHIN(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WORD(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WORDLIST(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WORDS(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WRITE_FILE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WRITE_LINE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_X_SIZE(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_X_WIDTH(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_store_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_store_plus_query(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_comma(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_size(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_width(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_fetch_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_char_plus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_char_minus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_emit(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_hold(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_key(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_key_query(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_or(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_string_minus(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_left_bracket(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_tick(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_char(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_compile(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_defined(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_else(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_if(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_then(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_undefined(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_backslash(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_right_bracket(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_brace_colon(ForthContext* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

//---------------------------------------------------------------------------

static int forthi_defineStandardWords(ForthContext* ctx)
{
#define FORTHI_DEFINE_STANDARD_WORD(_word_, _func_) \
    if (forth_addCWord(ctx, _word_, _func_) == FORTH_FAILURE) \
        return FORTH_FAILURE;
    
    FORTHI_DEFINE_STANDARD_WORD("!", forthi_word_store);
    FORTHI_DEFINE_STANDARD_WORD("#", forthi_word_number_sign);
    FORTHI_DEFINE_STANDARD_WORD("#>", forthi_word_number_sign_greater);
    FORTHI_DEFINE_STANDARD_WORD("#S", forthi_word_number_sign_s);
    FORTHI_DEFINE_STANDARD_WORD("'", forthi_word_tick);
    FORTHI_DEFINE_STANDARD_WORD("(", forthi_word_paren);
    FORTHI_DEFINE_STANDARD_WORD("(LOCAL)", forthi_word_paren_local_paren);
    FORTHI_DEFINE_STANDARD_WORD("*", forthi_word_star);
    FORTHI_DEFINE_STANDARD_WORD("*/", forthi_word_star_slash);
    FORTHI_DEFINE_STANDARD_WORD("*/MOD", forthi_word_star_slash_mod);
    FORTHI_DEFINE_STANDARD_WORD("+", forthi_word_plus);
    FORTHI_DEFINE_STANDARD_WORD("+!", forthi_word_plus_store);
    FORTHI_DEFINE_STANDARD_WORD("+FIELD", forthi_word_plus_field);
    FORTHI_DEFINE_STANDARD_WORD("+LOOP", forthi_word_plus_loop);
    FORTHI_DEFINE_STANDARD_WORD("+X/STRING", forthi_word_plus_x_string);
    FORTHI_DEFINE_STANDARD_WORD(",", forthi_word_comma);
    FORTHI_DEFINE_STANDARD_WORD("-", forthi_word_minus);
    FORTHI_DEFINE_STANDARD_WORD("-TRAILING", forthi_word_dash_trailing);
    FORTHI_DEFINE_STANDARD_WORD("-TRAILING-GARBAGE", forthi_word_dash_trailing_garbage);
    FORTHI_DEFINE_STANDARD_WORD(".", forthi_word_dot);
    FORTHI_DEFINE_STANDARD_WORD(".\"", forthi_word_dot_quote);
    FORTHI_DEFINE_STANDARD_WORD(".(", forthi_word_dot_paren);
    FORTHI_DEFINE_STANDARD_WORD(".R", forthi_word_dot_r);
    FORTHI_DEFINE_STANDARD_WORD(".S", forthi_word_dot_s);
    FORTHI_DEFINE_STANDARD_WORD("/", forthi_word_slash);
    FORTHI_DEFINE_STANDARD_WORD("/MOD", forthi_word_slash_mod);
    FORTHI_DEFINE_STANDARD_WORD("/STRING", forthi_word_slash_string);
    FORTHI_DEFINE_STANDARD_WORD("0<", forthi_word_zero_less);
    FORTHI_DEFINE_STANDARD_WORD("0<>", forthi_word_zero_not_equals);
    FORTHI_DEFINE_STANDARD_WORD("0=", forthi_word_zero_equals);
    FORTHI_DEFINE_STANDARD_WORD("0>", forthi_word_zero_greater);
    FORTHI_DEFINE_STANDARD_WORD("1+", forthi_word_one_plus);
    FORTHI_DEFINE_STANDARD_WORD("1-", forthi_word_one_minus);
    FORTHI_DEFINE_STANDARD_WORD("2!", forthi_word_two_store);
    FORTHI_DEFINE_STANDARD_WORD("2*", forthi_word_two_star);
    FORTHI_DEFINE_STANDARD_WORD("2/", forthi_word_two_slash);
    FORTHI_DEFINE_STANDARD_WORD("2>R", forthi_word_two_to_r);
    FORTHI_DEFINE_STANDARD_WORD("2@", forthi_word_two_fetch);
    FORTHI_DEFINE_STANDARD_WORD("2CONSTANT", forthi_word_two_constant);
    FORTHI_DEFINE_STANDARD_WORD("2DROP", forthi_word_two_drop);
    FORTHI_DEFINE_STANDARD_WORD("2DUP", forthi_word_two_dupe);
    FORTHI_DEFINE_STANDARD_WORD("2LITERAL", forthi_word_two_literal);
    FORTHI_DEFINE_STANDARD_WORD("2OVER", forthi_word_two_over);
    FORTHI_DEFINE_STANDARD_WORD("2R>", forthi_word_two_r_from);
    FORTHI_DEFINE_STANDARD_WORD("2R@", forthi_word_two_r_fetch);
    FORTHI_DEFINE_STANDARD_WORD("2ROT", forthi_word_two_rote);
    FORTHI_DEFINE_STANDARD_WORD("2SWAP", forthi_word_two_swap);
    FORTHI_DEFINE_STANDARD_WORD("2VALUE", forthi_word_two_value);
    FORTHI_DEFINE_STANDARD_WORD("2VARIABLE", forthi_word_two_variable);
    FORTHI_DEFINE_STANDARD_WORD(":", forthi_word_colon);
    FORTHI_DEFINE_STANDARD_WORD(":NONAME", forthi_word_colon_no_name);
    FORTHI_DEFINE_STANDARD_WORD(";", forthi_word_semicolon);
    FORTHI_DEFINE_STANDARD_WORD(";CODE", forthi_word_semicolon_code);
    FORTHI_DEFINE_STANDARD_WORD("<", forthi_word_less_than);
    FORTHI_DEFINE_STANDARD_WORD("<#", forthi_word_less_number_sign);
    FORTHI_DEFINE_STANDARD_WORD("<>", forthi_word_not_equals);
    FORTHI_DEFINE_STANDARD_WORD("=", forthi_word_equals);
    FORTHI_DEFINE_STANDARD_WORD(">", forthi_word_greater_than);
    FORTHI_DEFINE_STANDARD_WORD(">BODY", forthi_word_to_body);
    FORTHI_DEFINE_STANDARD_WORD(">FLOAT", forthi_word_to_float);
    FORTHI_DEFINE_STANDARD_WORD(">IN", forthi_word_to_in);
    FORTHI_DEFINE_STANDARD_WORD(">NUMBER", forthi_word_to_number);
    FORTHI_DEFINE_STANDARD_WORD(">R", forthi_word_to_r);
    FORTHI_DEFINE_STANDARD_WORD("?", forthi_word_question);
    FORTHI_DEFINE_STANDARD_WORD("?DO", forthi_word_question_do);
    FORTHI_DEFINE_STANDARD_WORD("?DUP", forthi_word_question_dupe);
    FORTHI_DEFINE_STANDARD_WORD("@", forthi_word_fetch);
    FORTHI_DEFINE_STANDARD_WORD("ABORT", forthi_word_ABORT);
    FORTHI_DEFINE_STANDARD_WORD("ABORT\"", forthi_word_abort_quote);
    FORTHI_DEFINE_STANDARD_WORD("ABS", forthi_word_abs);
    FORTHI_DEFINE_STANDARD_WORD("ACCEPT", forthi_word_ACCEPT);
    FORTHI_DEFINE_STANDARD_WORD("ACTION-OF", forthi_word_ACTION_OF);
    FORTHI_DEFINE_STANDARD_WORD("AGAIN", forthi_word_AGAIN);
    FORTHI_DEFINE_STANDARD_WORD("AHEAD", forthi_word_AHEAD);
    FORTHI_DEFINE_STANDARD_WORD("ALIGN", forthi_word_ALIGN);
    FORTHI_DEFINE_STANDARD_WORD("ALIGNED", forthi_word_ALIGNED);
    FORTHI_DEFINE_STANDARD_WORD("ALLOCATE", forthi_word_ALLOCATE);
    FORTHI_DEFINE_STANDARD_WORD("ALLOT", forthi_word_ALLOT);
    FORTHI_DEFINE_STANDARD_WORD("ALSO", forthi_word_ALSO);
    FORTHI_DEFINE_STANDARD_WORD("AND", forthi_word_AND);
    FORTHI_DEFINE_STANDARD_WORD("ASSEMBLER", forthi_word_ASSEMBLER);
    FORTHI_DEFINE_STANDARD_WORD("AT-XY", forthi_word_at_x_y);
    FORTHI_DEFINE_STANDARD_WORD("BASE", forthi_word_BASE);
    FORTHI_DEFINE_STANDARD_WORD("BEGIN", forthi_word_BEGIN);
    FORTHI_DEFINE_STANDARD_WORD("BEGIN-STRUCTURE", forthi_word_BEGIN_STRUCTURE);
    FORTHI_DEFINE_STANDARD_WORD("BIN", forthi_word_BIN);
    FORTHI_DEFINE_STANDARD_WORD("BL", forthi_word_b_l);
    FORTHI_DEFINE_STANDARD_WORD("BLANK", forthi_word_BLANK);
    FORTHI_DEFINE_STANDARD_WORD("BLK", forthi_word_b_l_k);
    FORTHI_DEFINE_STANDARD_WORD("BLOCK", forthi_word_BLOCK);
    FORTHI_DEFINE_STANDARD_WORD("BUFFER", forthi_word_BUFFER);
    FORTHI_DEFINE_STANDARD_WORD("BUFFER:", forthi_word_buffer_colon);
    FORTHI_DEFINE_STANDARD_WORD("BYE", forthi_word_BYE);
    FORTHI_DEFINE_STANDARD_WORD("C!", forthi_word_c_store);
    FORTHI_DEFINE_STANDARD_WORD("C\"", forthi_word_c_quote);
    FORTHI_DEFINE_STANDARD_WORD("C,", forthi_word_c_comma);
    FORTHI_DEFINE_STANDARD_WORD("C@", forthi_word_c_fetch);
    FORTHI_DEFINE_STANDARD_WORD("CASE", forthi_word_CASE);
    FORTHI_DEFINE_STANDARD_WORD("CATCH", forthi_word_CATCH);
    FORTHI_DEFINE_STANDARD_WORD("CELL+", forthi_word_cell_plus);
    FORTHI_DEFINE_STANDARD_WORD("CELLS", forthi_word_CELLS);
    FORTHI_DEFINE_STANDARD_WORD("CFIELD:", forthi_word_c_field_colon);
    FORTHI_DEFINE_STANDARD_WORD("CHAR", forthi_word_char);
    FORTHI_DEFINE_STANDARD_WORD("CHAR+", forthi_word_char_plus);
    FORTHI_DEFINE_STANDARD_WORD("CHARS", forthi_word_chars);
    FORTHI_DEFINE_STANDARD_WORD("CLOSE-FILE", forthi_word_CLOSE_FILE);
    FORTHI_DEFINE_STANDARD_WORD("CMOVE", forthi_word_c_move);
    FORTHI_DEFINE_STANDARD_WORD("CMOVE>", forthi_word_c_move_up);
    FORTHI_DEFINE_STANDARD_WORD("CODE", forthi_word_CODE);
    FORTHI_DEFINE_STANDARD_WORD("COMPARE", forthi_word_COMPARE);
    FORTHI_DEFINE_STANDARD_WORD("COMPILE,", forthi_word_compile_comma);
    FORTHI_DEFINE_STANDARD_WORD("CONSTANT", forthi_word_CONSTANT);
    FORTHI_DEFINE_STANDARD_WORD("COUNT", forthi_word_COUNT);
    FORTHI_DEFINE_STANDARD_WORD("CR", forthi_word_c_r);
    FORTHI_DEFINE_STANDARD_WORD("CREATE", forthi_word_CREATE);
    FORTHI_DEFINE_STANDARD_WORD("CREATE-FILE", forthi_word_CREATE_FILE);
    FORTHI_DEFINE_STANDARD_WORD("CS-PICK", forthi_word_c_s_pick);
    FORTHI_DEFINE_STANDARD_WORD("CS-ROLL", forthi_word_c_s_roll);
    FORTHI_DEFINE_STANDARD_WORD("D+", forthi_word_d_plus);
    FORTHI_DEFINE_STANDARD_WORD("D-", forthi_word_d_minus);
    FORTHI_DEFINE_STANDARD_WORD("D.", forthi_word_d_dot);
    FORTHI_DEFINE_STANDARD_WORD("D.R", forthi_word_d_dot_r);
    FORTHI_DEFINE_STANDARD_WORD("D0<", forthi_word_d_zero_less);
    FORTHI_DEFINE_STANDARD_WORD("D0=", forthi_word_d_zero_equals);
    FORTHI_DEFINE_STANDARD_WORD("D2*", forthi_word_d_two_star);
    FORTHI_DEFINE_STANDARD_WORD("D2/", forthi_word_d_two_slash);
    FORTHI_DEFINE_STANDARD_WORD("D<", forthi_word_d_less_than);
    FORTHI_DEFINE_STANDARD_WORD("D=", forthi_word_d_equals);
    FORTHI_DEFINE_STANDARD_WORD("D>F", forthi_word_d_to_f);
    FORTHI_DEFINE_STANDARD_WORD("D>S", forthi_word_d_to_s);
    FORTHI_DEFINE_STANDARD_WORD("DABS", forthi_word_d_abs);
    FORTHI_DEFINE_STANDARD_WORD("DECIMAL", forthi_word_DECIMAL);
    FORTHI_DEFINE_STANDARD_WORD("DEFER", forthi_word_DEFER);
    FORTHI_DEFINE_STANDARD_WORD("DEFER!", forthi_word_defer_store);
    FORTHI_DEFINE_STANDARD_WORD("DEFER@", forthi_word_defer_fetch);
    FORTHI_DEFINE_STANDARD_WORD("DEFINITIONS", forthi_word_DEFINITIONS);
    FORTHI_DEFINE_STANDARD_WORD("DELETE-FILE", forthi_word_DELETE_FILE);
    FORTHI_DEFINE_STANDARD_WORD("DEPTH", forthi_word_DEPTH);
    FORTHI_DEFINE_STANDARD_WORD("DF!", forthi_word_d_f_store);
    FORTHI_DEFINE_STANDARD_WORD("DF@", forthi_word_d_f_fetch);
    FORTHI_DEFINE_STANDARD_WORD("DFALIGN", forthi_word_d_f_align);
    FORTHI_DEFINE_STANDARD_WORD("DFALIGNED", forthi_word_d_f_aligned);
    FORTHI_DEFINE_STANDARD_WORD("DFFIELD:", forthi_word_d_f_field_colon);
    FORTHI_DEFINE_STANDARD_WORD("DFLOAT+", forthi_word_d_float_plus);
    FORTHI_DEFINE_STANDARD_WORD("DFLOATS", forthi_word_d_floats);
    FORTHI_DEFINE_STANDARD_WORD("DMAX", forthi_word_d_max);
    FORTHI_DEFINE_STANDARD_WORD("DMIN", forthi_word_d_min);
    FORTHI_DEFINE_STANDARD_WORD("DNEGATE", forthi_word_d_negate);
    FORTHI_DEFINE_STANDARD_WORD("DO", forthi_word_DO);
    FORTHI_DEFINE_STANDARD_WORD("DOES>", forthi_word_does);
    FORTHI_DEFINE_STANDARD_WORD("DROP", forthi_word_DROP);
    FORTHI_DEFINE_STANDARD_WORD("DU<", forthi_word_d_u_less);
    FORTHI_DEFINE_STANDARD_WORD("DUMP", forthi_word_DUMP);
    FORTHI_DEFINE_STANDARD_WORD("DUP", forthi_word_dupe);
    FORTHI_DEFINE_STANDARD_WORD("EDITOR", forthi_word_EDITOR);
    FORTHI_DEFINE_STANDARD_WORD("EKEY", forthi_word_e_key);
    FORTHI_DEFINE_STANDARD_WORD("EKEY>CHAR", forthi_word_e_key_to_char);
    FORTHI_DEFINE_STANDARD_WORD("EKEY>FKEY", forthi_word_e_key_to_f_key);
    FORTHI_DEFINE_STANDARD_WORD("EKEY>XCHAR", forthi_word_e_key_to_x_char);
    FORTHI_DEFINE_STANDARD_WORD("EKEY?", forthi_word_e_key_question);
    FORTHI_DEFINE_STANDARD_WORD("ELSE", forthi_word_ELSE);
    FORTHI_DEFINE_STANDARD_WORD("EMIT", forthi_word_EMIT);
    FORTHI_DEFINE_STANDARD_WORD("EMIT?", forthi_word_emit_question);
    FORTHI_DEFINE_STANDARD_WORD("EMPTY-BUFFERS", forthi_word_EMPTY_BUFFERS);
    FORTHI_DEFINE_STANDARD_WORD("END-STRUCTURE", forthi_word_END_STRUCTURE);
    FORTHI_DEFINE_STANDARD_WORD("ENDCASE", forthi_word_end_case);
    FORTHI_DEFINE_STANDARD_WORD("ENDOF", forthi_word_end_of);
    FORTHI_DEFINE_STANDARD_WORD("ENVIRONMENT?", forthi_word_environment_query);
    FORTHI_DEFINE_STANDARD_WORD("ERASE", forthi_word_ERASE);
    FORTHI_DEFINE_STANDARD_WORD("EVALUATE", forthi_word_EVALUATE);
    FORTHI_DEFINE_STANDARD_WORD("EXECUTE", forthi_word_EXECUTE);
    FORTHI_DEFINE_STANDARD_WORD("EXIT", forthi_word_EXIT);
    FORTHI_DEFINE_STANDARD_WORD("F!", forthi_word_f_store);
    FORTHI_DEFINE_STANDARD_WORD("F*", forthi_word_f_star);
    FORTHI_DEFINE_STANDARD_WORD("F**", forthi_word_f_star_star);
    FORTHI_DEFINE_STANDARD_WORD("F+", forthi_word_f_plus);
    FORTHI_DEFINE_STANDARD_WORD("F-", forthi_word_f_minus);
    FORTHI_DEFINE_STANDARD_WORD("F.", forthi_word_f_dot);
    FORTHI_DEFINE_STANDARD_WORD("F/", forthi_word_f_slash);
    FORTHI_DEFINE_STANDARD_WORD("F0<", forthi_word_f_zero_less_than);
    FORTHI_DEFINE_STANDARD_WORD("F0=", forthi_word_f_zero_equals);
    FORTHI_DEFINE_STANDARD_WORD("F>D", forthi_word_f_to_d);
    FORTHI_DEFINE_STANDARD_WORD("F>S", forthi_word_F_to_S);
    FORTHI_DEFINE_STANDARD_WORD("F@", forthi_word_f_fetch);
    FORTHI_DEFINE_STANDARD_WORD("FABS", forthi_word_f_abs);
    FORTHI_DEFINE_STANDARD_WORD("FACOS", forthi_word_f_a_cos);
    FORTHI_DEFINE_STANDARD_WORD("FACOSH", forthi_word_f_a_cosh);
    FORTHI_DEFINE_STANDARD_WORD("FALIGN", forthi_word_f_align);
    FORTHI_DEFINE_STANDARD_WORD("FALIGNED", forthi_word_f_aligned);
    FORTHI_DEFINE_STANDARD_WORD("FALOG", forthi_word_f_a_log);
    FORTHI_DEFINE_STANDARD_WORD("FALSE", forthi_word_FALSE);
    FORTHI_DEFINE_STANDARD_WORD("FASIN", forthi_word_f_a_sine);
    FORTHI_DEFINE_STANDARD_WORD("FASINH", forthi_word_f_a_cinch);
    FORTHI_DEFINE_STANDARD_WORD("FATAN", forthi_word_f_a_tan);
    FORTHI_DEFINE_STANDARD_WORD("FATAN2", forthi_word_f_a_tan_two);
    FORTHI_DEFINE_STANDARD_WORD("FATANH", forthi_word_f_a_tan_h);
    FORTHI_DEFINE_STANDARD_WORD("FCONSTANT", forthi_word_f_constant);
    FORTHI_DEFINE_STANDARD_WORD("FCOS", forthi_word_f_cos);
    FORTHI_DEFINE_STANDARD_WORD("FCOSH", forthi_word_f_cosh);
    FORTHI_DEFINE_STANDARD_WORD("FDEPTH", forthi_word_f_depth);
    FORTHI_DEFINE_STANDARD_WORD("FDROP", forthi_word_f_drop);
    FORTHI_DEFINE_STANDARD_WORD("FDUP", forthi_word_f_dupe);
    FORTHI_DEFINE_STANDARD_WORD("FE.", forthi_word_f_e_dot);
    FORTHI_DEFINE_STANDARD_WORD("FEXP", forthi_word_f_e_x_p);
    FORTHI_DEFINE_STANDARD_WORD("FEXPM1", forthi_word_f_e_x_p_m_one);
    FORTHI_DEFINE_STANDARD_WORD("FFIELD:", forthi_word_f_field_colon);
    FORTHI_DEFINE_STANDARD_WORD("FIELD:", forthi_word_field_colon);
    FORTHI_DEFINE_STANDARD_WORD("FILE-POSITION", forthi_word_FILE_POSITION);
    FORTHI_DEFINE_STANDARD_WORD("FILE-SIZE", forthi_word_FILE_SIZE);
    FORTHI_DEFINE_STANDARD_WORD("FILE-STATUS", forthi_word_FILE_STATUS);
    FORTHI_DEFINE_STANDARD_WORD("FILL", forthi_word_FILL);
    FORTHI_DEFINE_STANDARD_WORD("FIND", forthi_word_FIND);
    FORTHI_DEFINE_STANDARD_WORD("FLITERAL", forthi_word_f_literal);
    FORTHI_DEFINE_STANDARD_WORD("FLN", forthi_word_f_l_n);
    FORTHI_DEFINE_STANDARD_WORD("FLNP1", forthi_word_f_l_n_p_one);
    FORTHI_DEFINE_STANDARD_WORD("FLOAT+", forthi_word_float_plus);
    FORTHI_DEFINE_STANDARD_WORD("FLOATS", forthi_word_FLOATS);
    FORTHI_DEFINE_STANDARD_WORD("FLOT", forthi_word_f_log);
    FORTHI_DEFINE_STANDARD_WORD("FLOOR", forthi_word_FLOOR);
    FORTHI_DEFINE_STANDARD_WORD("FLUSH", forthi_word_FLUSH);
    FORTHI_DEFINE_STANDARD_WORD("FLUSH-FILE", forthi_word_FLUSH_FILE);
    FORTHI_DEFINE_STANDARD_WORD("FM/MOD", forthi_word_f_m_slash_mod);
    FORTHI_DEFINE_STANDARD_WORD("FMAX", forthi_word_f_max);
    FORTHI_DEFINE_STANDARD_WORD("FMIN", forthi_word_f_min);
    FORTHI_DEFINE_STANDARD_WORD("FNEGATE", forthi_word_f_negate);
    FORTHI_DEFINE_STANDARD_WORD("FORGET", forthi_word_FORGET);
    FORTHI_DEFINE_STANDARD_WORD("FORTH", forthi_word_FORTH);
    FORTHI_DEFINE_STANDARD_WORD("FORTH-WORDLIST", forthi_word_FORTH_WORDLIST);
    FORTHI_DEFINE_STANDARD_WORD("FOVER", forthi_word_f_over);
    FORTHI_DEFINE_STANDARD_WORD("FREE", forthi_word_FREE);
    FORTHI_DEFINE_STANDARD_WORD("FROT", forthi_word_f_rote);
    FORTHI_DEFINE_STANDARD_WORD("FROUND", forthi_word_f_round);
    FORTHI_DEFINE_STANDARD_WORD("FS.", forthi_word_f_s_dot);
    FORTHI_DEFINE_STANDARD_WORD("FSIN", forthi_word_f_sine);
    FORTHI_DEFINE_STANDARD_WORD("FSINCOS", forthi_word_f_sine_cos);
    FORTHI_DEFINE_STANDARD_WORD("FSINH", forthi_word_f_cinch);
    FORTHI_DEFINE_STANDARD_WORD("FSQRT", forthi_word_f_square_root);
    FORTHI_DEFINE_STANDARD_WORD("FSWAP", forthi_word_f_swap);
    FORTHI_DEFINE_STANDARD_WORD("FTAN", forthi_word_f_tan);
    FORTHI_DEFINE_STANDARD_WORD("FTANH", forthi_word_f_tan_h);
    FORTHI_DEFINE_STANDARD_WORD("FTRUNC", forthi_word_f_trunc);
    FORTHI_DEFINE_STANDARD_WORD("FVALUE", forthi_word_f_value);
    FORTHI_DEFINE_STANDARD_WORD("FVARIABLE", forthi_word_f_variable);
    FORTHI_DEFINE_STANDARD_WORD("F~", forthi_word_f_proximate);
    FORTHI_DEFINE_STANDARD_WORD("GET-CURRENT", forthi_word_GET_CURRENT);
    FORTHI_DEFINE_STANDARD_WORD("GET-ORDER", forthi_word_GET_ORDER);
    FORTHI_DEFINE_STANDARD_WORD("HERE", forthi_word_HERE);
    FORTHI_DEFINE_STANDARD_WORD("HEX", forthi_word_HEX);
    FORTHI_DEFINE_STANDARD_WORD("HOLD", forthi_word_HOLD);
    FORTHI_DEFINE_STANDARD_WORD("HOLDS", forthi_word_HOLDS);
    FORTHI_DEFINE_STANDARD_WORD("I", forthi_word_I);
    FORTHI_DEFINE_STANDARD_WORD("IF", forthi_word_IF);
    FORTHI_DEFINE_STANDARD_WORD("IMMEDIATE", forthi_word_IMMEDIATE);
    FORTHI_DEFINE_STANDARD_WORD("INCLUDE", forthi_word_INCLUDE);
    FORTHI_DEFINE_STANDARD_WORD("INCLUDE-FILE", forthi_word_INCLUDE_FILE);
    FORTHI_DEFINE_STANDARD_WORD("INCLUDED", forthi_word_INCLUDED);
    FORTHI_DEFINE_STANDARD_WORD("INVERT", forthi_word_INVERT);
    FORTHI_DEFINE_STANDARD_WORD("IS", forthi_word_IS);
    FORTHI_DEFINE_STANDARD_WORD("J", forthi_word_J);
    FORTHI_DEFINE_STANDARD_WORD("K-ALT-MASK", forthi_word_K_ALT_MASK);
    FORTHI_DEFINE_STANDARD_WORD("K-CTRL-MASK", forthi_word_K_CTRL_MASK);
    FORTHI_DEFINE_STANDARD_WORD("K-DELETE", forthi_word_K_DELETE);
    FORTHI_DEFINE_STANDARD_WORD("K-DOWN", forthi_word_K_DOWN);
    FORTHI_DEFINE_STANDARD_WORD("K-END", forthi_word_K_END);
    FORTHI_DEFINE_STANDARD_WORD("K-F1", forthi_word_k_f_1);
    FORTHI_DEFINE_STANDARD_WORD("K-F10", forthi_word_k_f_10);
    FORTHI_DEFINE_STANDARD_WORD("K-F11", forthi_word_k_f_11);
    FORTHI_DEFINE_STANDARD_WORD("K-F12", forthi_word_k_f_12);
    FORTHI_DEFINE_STANDARD_WORD("K-F2", forthi_word_k_f_2);
    FORTHI_DEFINE_STANDARD_WORD("K-F3", forthi_word_k_f_3);
    FORTHI_DEFINE_STANDARD_WORD("K-F4", forthi_word_k_f_4);
    FORTHI_DEFINE_STANDARD_WORD("K-F5", forthi_word_k_f_5);
    FORTHI_DEFINE_STANDARD_WORD("K-F6", forthi_word_k_f_6);
    FORTHI_DEFINE_STANDARD_WORD("K-F7", forthi_word_k_f_7);
    FORTHI_DEFINE_STANDARD_WORD("K-F8", forthi_word_k_f_8);
    FORTHI_DEFINE_STANDARD_WORD("K-F9", forthi_word_k_f_9);
    FORTHI_DEFINE_STANDARD_WORD("K_HOME", forthi_word_K_HOME);
    FORTHI_DEFINE_STANDARD_WORD("K_INSERT", forthi_word_K_INSERT);
    FORTHI_DEFINE_STANDARD_WORD("K_LEFT", forthi_word_K_LEFT);
    FORTHI_DEFINE_STANDARD_WORD("K_NEXT", forthi_word_K_NEXT);
    FORTHI_DEFINE_STANDARD_WORD("K_PRIOR", forthi_word_K_PRIOR);
    FORTHI_DEFINE_STANDARD_WORD("K_RIGHT", forthi_word_K_RIGHT);
    FORTHI_DEFINE_STANDARD_WORD("K_SHIFT_MASK", forthi_word_K_SHIFT_MASK);
    FORTHI_DEFINE_STANDARD_WORD("K_UP", forthi_word_K_UP);
    FORTHI_DEFINE_STANDARD_WORD("KEY", forthi_word_KEY);
    FORTHI_DEFINE_STANDARD_WORD("KEY?", forthi_word_key_question);
    FORTHI_DEFINE_STANDARD_WORD("LEAVE", forthi_word_LEAVE);
    FORTHI_DEFINE_STANDARD_WORD("LIST", forthi_word_LIST);
    FORTHI_DEFINE_STANDARD_WORD("LITERAL", forthi_word_LITERAL);
    FORTHI_DEFINE_STANDARD_WORD("LOAD", forthi_word_LOAD);
    FORTHI_DEFINE_STANDARD_WORD("LOCALS|", forthi_word_locals_bar);
    FORTHI_DEFINE_STANDARD_WORD("LOOP", forthi_word_LOOP);
    FORTHI_DEFINE_STANDARD_WORD("LSHIFT", forthi_word_l_shift);
    FORTHI_DEFINE_STANDARD_WORD("M*", forthi_word_m_star);
    FORTHI_DEFINE_STANDARD_WORD("M*/", forthi_word_m_star_slash);
    FORTHI_DEFINE_STANDARD_WORD("M+", forthi_word_m_plus);
    FORTHI_DEFINE_STANDARD_WORD("MARKER", forthi_word_MARKER);
    FORTHI_DEFINE_STANDARD_WORD("MAX", forthi_word_MAX);
    FORTHI_DEFINE_STANDARD_WORD("MIN", forthi_word_MIN);
    FORTHI_DEFINE_STANDARD_WORD("MOD", forthi_word_MOD);
    FORTHI_DEFINE_STANDARD_WORD("MOVE", forthi_word_MOVE);
    FORTHI_DEFINE_STANDARD_WORD("MS", forthi_word_MS);
    FORTHI_DEFINE_STANDARD_WORD("N>R", forthi_word_n_to_r);
    FORTHI_DEFINE_STANDARD_WORD("NAME>COMPILE", forthi_word_name_to_compile);
    FORTHI_DEFINE_STANDARD_WORD("NAME>INTERPRET", forthi_word_name_to_interpret);
    FORTHI_DEFINE_STANDARD_WORD("NAME>STRING", forthi_word_name_to_string);
    FORTHI_DEFINE_STANDARD_WORD("NEGATE", forthi_word_NEGATE);
    FORTHI_DEFINE_STANDARD_WORD("NIP", forthi_word_NIP);
    FORTHI_DEFINE_STANDARD_WORD("NR>", forthi_word_n_r_from);
    FORTHI_DEFINE_STANDARD_WORD("OF", forthi_word_OF);
    FORTHI_DEFINE_STANDARD_WORD("ONLY", forthi_word_ONLY);
    FORTHI_DEFINE_STANDARD_WORD("OPEN-FILE", forthi_word_OPEN_FILE);
    FORTHI_DEFINE_STANDARD_WORD("OR", forthi_word_OR);
    FORTHI_DEFINE_STANDARD_WORD("ORDER", forthi_word_ORDER);
    FORTHI_DEFINE_STANDARD_WORD("OVER", forthi_word_OVER);
    FORTHI_DEFINE_STANDARD_WORD("PAD", forthi_word_PAD);
    FORTHI_DEFINE_STANDARD_WORD("PAGE", forthi_word_PAGE);
    FORTHI_DEFINE_STANDARD_WORD("PARSE", forthi_word_PARSE);
    FORTHI_DEFINE_STANDARD_WORD("PARSE-NAME", forthi_word_PARSE_NAME);
    FORTHI_DEFINE_STANDARD_WORD("PICK", forthi_word_PICK);
    FORTHI_DEFINE_STANDARD_WORD("POSTPONE", forthi_word_POSTPONE);
    FORTHI_DEFINE_STANDARD_WORD("PRECISION", forthi_word_PRECISION);
    FORTHI_DEFINE_STANDARD_WORD("PREVIOUS", forthi_word_PREVIOUS);
    FORTHI_DEFINE_STANDARD_WORD("QUIT", forthi_word_QUIT);
    FORTHI_DEFINE_STANDARD_WORD("R/O", forthi_word_r_o);
    FORTHI_DEFINE_STANDARD_WORD("R/W", forthi_word_r_w);
    FORTHI_DEFINE_STANDARD_WORD("R>", forthi_word_r_from);
    FORTHI_DEFINE_STANDARD_WORD("R@", forthi_word_r_fetch);
    FORTHI_DEFINE_STANDARD_WORD("READ-FILE", forthi_word_READ_FILE);
    FORTHI_DEFINE_STANDARD_WORD("READ-LINE", forthi_word_READ_LINE);
    FORTHI_DEFINE_STANDARD_WORD("RECURSE", forthi_word_RECURSE);
    FORTHI_DEFINE_STANDARD_WORD("REFILL", forthi_word_REFILL);
    FORTHI_DEFINE_STANDARD_WORD("RENAME_FILE", forthi_word_RENAME_FILE);
    FORTHI_DEFINE_STANDARD_WORD("REPEAT", forthi_word_REPEAT);
    FORTHI_DEFINE_STANDARD_WORD("REPLACES", forthi_word_REPLACES);
    FORTHI_DEFINE_STANDARD_WORD("REPOSITION-FILE", forthi_word_REPOSITION_FILE);
    FORTHI_DEFINE_STANDARD_WORD("REPRESENT", forthi_word_REPRESENT);
    FORTHI_DEFINE_STANDARD_WORD("REQUIRE", forthi_word_REQUIRE);
    FORTHI_DEFINE_STANDARD_WORD("REQUIRED", forthi_word_REQUIRED);
    FORTHI_DEFINE_STANDARD_WORD("RESIZE", forthi_word_RESIZE);
    FORTHI_DEFINE_STANDARD_WORD("RESIZE-FILE", forthi_word_RESIZE_FILE);
    FORTHI_DEFINE_STANDARD_WORD("RESTORE-INPUT", forthi_word_RESTORE_INPUT);
    FORTHI_DEFINE_STANDARD_WORD("ROLL", forthi_word_ROLL);
    FORTHI_DEFINE_STANDARD_WORD("ROT", forthi_word_rote);
    FORTHI_DEFINE_STANDARD_WORD("RSHIFT", forthi_word_r_shift);
    FORTHI_DEFINE_STANDARD_WORD("S\"", forthi_word_s_quote);
    FORTHI_DEFINE_STANDARD_WORD("S>D", forthi_word_s_to_d);
    FORTHI_DEFINE_STANDARD_WORD("S>F", forthi_word_s_to_F);
    FORTHI_DEFINE_STANDARD_WORD("SAVE-BUFFERS", forthi_word_SAVE_BUFFERS);
    FORTHI_DEFINE_STANDARD_WORD("SAVE-INPUT", forthi_word_SAVE_INPUT);
    FORTHI_DEFINE_STANDARD_WORD("SCR", forthi_word_s_c_r);
    FORTHI_DEFINE_STANDARD_WORD("SEARCH", forthi_word_SEARCH);
    FORTHI_DEFINE_STANDARD_WORD("SEARCH-WORDLIST", forthi_word_SEARCH_WORDLIST);
    FORTHI_DEFINE_STANDARD_WORD("SEE", forthi_word_SEE);
    FORTHI_DEFINE_STANDARD_WORD("SET-CURRENT", forthi_word_SET_CURRENT);
    FORTHI_DEFINE_STANDARD_WORD("SET-ORDER", forthi_word_SET_ORDER);
    FORTHI_DEFINE_STANDARD_WORD("SET-PRECISION", forthi_word_SET_PRECISION);
    FORTHI_DEFINE_STANDARD_WORD("SF!", forthi_word_s_f_store);
    FORTHI_DEFINE_STANDARD_WORD("SF@", forthi_word_s_f_fetch);
    FORTHI_DEFINE_STANDARD_WORD("SFALIGN", forthi_word_s_f_align);
    FORTHI_DEFINE_STANDARD_WORD("SFALIGNED", forthi_word_s_f_aligned);
    FORTHI_DEFINE_STANDARD_WORD("SFFIELD:", forthi_word_s_f_field_colon);
    FORTHI_DEFINE_STANDARD_WORD("SFLOAT+", forthi_word_s_float_plus);
    FORTHI_DEFINE_STANDARD_WORD("SFLOATS", forthi_word_s_floats);
    FORTHI_DEFINE_STANDARD_WORD("SIGN", forthi_word_SIGN);
    FORTHI_DEFINE_STANDARD_WORD("SLITERAL", forthi_word_SLITERAL);
    FORTHI_DEFINE_STANDARD_WORD("SM/REM", forthi_word_s_m_slash_rem);
    FORTHI_DEFINE_STANDARD_WORD("SOURCE", forthi_word_SOURCE);
    FORTHI_DEFINE_STANDARD_WORD("SOURCE_ID", forthi_word_source_i_d);
    FORTHI_DEFINE_STANDARD_WORD("SPACE", forthi_word_SPACE);
    FORTHI_DEFINE_STANDARD_WORD("SPACES", forthi_word_SPACES);
    FORTHI_DEFINE_STANDARD_WORD("STATE", forthi_word_STATE);
    FORTHI_DEFINE_STANDARD_WORD("SUBSTITURE", forthi_word_SUBSTITURE);
    FORTHI_DEFINE_STANDARD_WORD("SWAP", forthi_word_SWAP);
    FORTHI_DEFINE_STANDARD_WORD("SYNONYM", forthi_word_SYNONYM);
    FORTHI_DEFINE_STANDARD_WORD("S\\", forthi_word_s_backslash_quote);
    FORTHI_DEFINE_STANDARD_WORD("THEN", forthi_word_THEN);
    FORTHI_DEFINE_STANDARD_WORD("THROW", forthi_word_THROW);
    FORTHI_DEFINE_STANDARD_WORD("THRU", forthi_word_THRU);
    FORTHI_DEFINE_STANDARD_WORD("TIME&DATE", forthi_word_time_and_date);
    FORTHI_DEFINE_STANDARD_WORD("TO", forthi_word_TO);
    FORTHI_DEFINE_STANDARD_WORD("TRAVERSE-WORDLIST", forthi_word_TRAVERSE_WORDLIST);
    FORTHI_DEFINE_STANDARD_WORD("TRUE", forthi_word_TRUE);
    FORTHI_DEFINE_STANDARD_WORD("TUCK", forthi_word_TUCK);
    FORTHI_DEFINE_STANDARD_WORD("TYPE", forthi_word_TYPE);
    FORTHI_DEFINE_STANDARD_WORD("U.", forthi_word_u_dot);
    FORTHI_DEFINE_STANDARD_WORD("U.R", forthi_word_u_dot_r);
    FORTHI_DEFINE_STANDARD_WORD("U<", forthi_word_u_less_than);
    FORTHI_DEFINE_STANDARD_WORD("U>", forthi_word_u_greater_than);
    FORTHI_DEFINE_STANDARD_WORD("UM*", forthi_word_u_m_star);
    FORTHI_DEFINE_STANDARD_WORD("UM/MOD", forthi_word_u_m_slash_mod);
    FORTHI_DEFINE_STANDARD_WORD("UNESCAPE", forthi_word_UNESCAPE);
    FORTHI_DEFINE_STANDARD_WORD("UNLOOP", forthi_word_UNLOOP);
    FORTHI_DEFINE_STANDARD_WORD("UNTIL", forthi_word_UNTIL);
    FORTHI_DEFINE_STANDARD_WORD("UNUSED", forthi_word_UNUSED);
    FORTHI_DEFINE_STANDARD_WORD("UPDATE", forthi_word_UPDATE);
    FORTHI_DEFINE_STANDARD_WORD("VALUE", forthi_word_VALUE);
    FORTHI_DEFINE_STANDARD_WORD("VARIABLE", forthi_word_VARIABLE);
    FORTHI_DEFINE_STANDARD_WORD("W/O", forthi_word_w_o);
    FORTHI_DEFINE_STANDARD_WORD("WHILE", forthi_word_WHILE);
    FORTHI_DEFINE_STANDARD_WORD("WITHIN", forthi_word_WITHIN);
    FORTHI_DEFINE_STANDARD_WORD("WORD", forthi_word_WORD);
    FORTHI_DEFINE_STANDARD_WORD("WORDLIST", forthi_word_WORDLIST);
    FORTHI_DEFINE_STANDARD_WORD("WORDS", forthi_word_WORDS);
    FORTHI_DEFINE_STANDARD_WORD("WRITE-FILE", forthi_word_WRITE_FILE);
    FORTHI_DEFINE_STANDARD_WORD("WRITE-LINE", forthi_word_WRITE_LINE);
    FORTHI_DEFINE_STANDARD_WORD("X-SIZE", forthi_word_X_SIZE);
    FORTHI_DEFINE_STANDARD_WORD("X-WIDTH", forthi_word_X_WIDTH);
    FORTHI_DEFINE_STANDARD_WORD("XC!+", forthi_word_x_c_store_plus);
    FORTHI_DEFINE_STANDARD_WORD("XC!+?", forthi_word_x_c_store_plus_query);
    FORTHI_DEFINE_STANDARD_WORD("XC,", forthi_word_x_c_comma);
    FORTHI_DEFINE_STANDARD_WORD("XC-SIZE", forthi_word_x_c_size);
    FORTHI_DEFINE_STANDARD_WORD("XC-WIDTH", forthi_word_x_c_width);
    FORTHI_DEFINE_STANDARD_WORD("XC@+", forthi_word_x_c_fetch_plus);
    FORTHI_DEFINE_STANDARD_WORD("XCHAR+", forthi_word_x_char_plus);
    FORTHI_DEFINE_STANDARD_WORD("XCHAR-", forthi_word_x_char_minus);
    FORTHI_DEFINE_STANDARD_WORD("XEMIT", forthi_word_x_emit);
    FORTHI_DEFINE_STANDARD_WORD("XHOLD", forthi_word_x_hold);
    FORTHI_DEFINE_STANDARD_WORD("XKEY", forthi_word_x_key);
    FORTHI_DEFINE_STANDARD_WORD("XKEY?", forthi_word_x_key_query);
    FORTHI_DEFINE_STANDARD_WORD("XOR", forthi_word_x_or);
    FORTHI_DEFINE_STANDARD_WORD("X\\STRING-", forthi_word_x_string_minus);
    FORTHI_DEFINE_STANDARD_WORD("[", forthi_word_left_bracket);
    FORTHI_DEFINE_STANDARD_WORD("[']", forthi_word_bracket_tick);
    FORTHI_DEFINE_STANDARD_WORD("[CHAR]", forthi_word_bracket_char);
    FORTHI_DEFINE_STANDARD_WORD("[COMPILE]", forthi_word_bracket_compile);
    FORTHI_DEFINE_STANDARD_WORD("[DEFINED]", forthi_word_bracket_defined);
    FORTHI_DEFINE_STANDARD_WORD("[ELSE]", forthi_word_bracket_else);
    FORTHI_DEFINE_STANDARD_WORD("[IF]", forthi_word_bracket_if);
    FORTHI_DEFINE_STANDARD_WORD("[THEN]", forthi_word_bracket_then);
    FORTHI_DEFINE_STANDARD_WORD("[UNDEFINED]", forthi_word_bracket_undefined);
    FORTHI_DEFINE_STANDARD_WORD("\\", forthi_word_backslash);
    FORTHI_DEFINE_STANDARD_WORD("]", forthi_word_right_bracket);
    FORTHI_DEFINE_STANDARD_WORD("{:", forthi_word_brace_colon);

    return FORTH_SUCCESS;
}

ForthContext* forth_createContext(int memory_size, int stack_size, int dict_size)
{
    if (memory_size < -1 || memory_size == 0)
        return NULL;

    if (stack_size < -1 || stack_size == 0)
        return NULL;

    if (dict_size < -1 || dict_size == 0)
        return NULL;

    ForthContext* ctx = (ForthContext*)malloc(sizeof(ForthContext));
    if (!ctx)
        return NULL;

    memset(ctx, 0, sizeof(ForthContext));

    ctx->memory_auto_resize = memory_size == -1 ? 1 : 0;
    ctx->memory_size = ctx->memory_auto_resize ? FORTHI_MEM_ALLOC_CHUNK_SIZE : memory_size;
    ctx->memory = (uint8_t*)malloc(ctx->memory_size);
    if (!ctx->memory)
    {
        forth_destroyContext(ctx);
        return NULL;
    }

    ctx->stack_auto_resize = stack_size == -1 ? 1 : 0;
    ctx->stack_size = ctx->stack_auto_resize ? FORTHI_MEM_ALLOC_CHUNK_SIZE : stack_size;
    ctx->stack = (ForthCell*)malloc(sizeof(ForthCell) * ctx->stack_size);
    if (!ctx->stack)
    {
        forth_destroyContext(ctx);
        return NULL;
    }

    ctx->dict_auto_resize = dict_size == -1 ? 1 : 0;
    ctx->dict_size = ctx->dict_auto_resize ? FORTHI_MEM_ALLOC_CHUNK_SIZE : dict_size;
    ctx->dict_name_lens = (uint8_t*)malloc(ctx->dict_size);
    if (!ctx->dict_name_lens)
    {
        forth_destroyContext(ctx);
        return NULL;
    }
    ctx->dict_names = (char*)malloc(FORTH_DICT_CHAR_COUNT * ctx->dict_size);
    if (!ctx->dict_names)
    {
        forth_destroyContext(ctx);
        return NULL;
    }
    ctx->dict_pointers = (forth_pointer_type*)malloc(sizeof(forth_pointer_type) * ctx->dict_size);
    if (!ctx->dict_pointers)
    {
        forth_destroyContext(ctx);
        return NULL;
    }

    if (forthi_defineStandardWords(ctx) == FORTH_FAILURE)
    {
        forth_destroyContext(ctx);
        return NULL;
    }

    return ctx;
}

void forth_destroyContext(ForthContext* ctx)
{
    if (!ctx)
        return;

    if (ctx->memory)
        free(ctx->memory);

    if (ctx->stack)
        free(ctx->stack);

    if (ctx->dict_name_lens)
        free(ctx->dict_name_lens);

    if (ctx->dict_names)
        free(ctx->dict_names);

    if (ctx->dict_pointers)
        free(ctx->dict_pointers);

    free(ctx);
}

ForthCell* forth_top(ForthContext* ctx, int offset)
{
    if (!ctx)
        return NULL;

    if (ctx->stack_pointer <= offset)
        return NULL;

    return &ctx->stack[ctx->stack_pointer - offset - 1];
}

int forth_eval(ForthContext* ctx, const char* code)
{
    if (!ctx)
        return FORTH_FAILURE;

    if (!code)
        return FORTH_FAILURE;

    ctx->code = code;
    ctx->state = FORTHI_STATE_INTERPRET;

    return forthi_interpret(ctx);
}

#endif

#if defined(__cplusplus)
};
#endif

#endif /* FORTH_H_INCLUDED */
