/*
MIT License

Copyright (c) 2020 David St-Louis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
// You can redefine how many characters are store in the dictionnary per 
// entry. By default, it is 32. This doesn't mean word lengths are limited
// to this. It first checks the length, then compares those characters.
//
// i.e.: if you want 3 characters per word
// -DFORTH_DICT_CHAR_COUNT=3
//
// The total size of a dictionnary is:
// word_count * (FORTH_DICT_CHAR_COUNT + sizeof(forth_pointer) + 1)
//
//---------------------------------------------------------------------------
// In this file, public API constants, typdefs, structs and functions
// start with FORTH. Internal with FORTHI, the I stands for "internal".
//
//---------------------------------------------------------------------------

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
#define FORTH_MEM_INFINITE -1
#define FORTH_FALSE 0
#define FORTH_TRUE -1

typedef uintptr_t forth_pointer;

#if FORTH_INT_SIZE_8_BITS
typedef int8_t forth_int;
typedef int16_t forth_double_length_int;
typedef uint8_t forth_uint;
typedef uint16_t forth_double_length_uint;
#elif FORTH_INT_SIZE_16_BITS
typedef int16_t forth_int;
typedef int32_t forth_double_length_int;
typedef uint16_t forth_uint;
typedef uint32_t forth_double_length_uint;
#elif FORTH_INT_SIZE_32_BITS
typedef int32_t forth_int;
typedef int64_t forth_double_length_int;
typedef uint32_t forth_uint;
typedef uint64_t forth_double_length_uint;
#else
#define FORTH_INT_SIZE_64_BITS 1
typedef int64_t forth_int;
typedef uint64_t forth_double_length_int; // We don't use 128 bits maths
typedef uint64_t forth_uint;
typedef uint64_t forth_double_length_uint;
#endif

#ifndef FORTH_DICT_CHAR_COUNT
#define FORTH_DICT_CHAR_COUNT 32
#endif

typedef int (*forth_c_func)(struct forth_context*);
typedef int (*forth_log_func)(struct forth_context*, const char *fmt, ...);

typedef struct forth_cell
{
    union
    {
        forth_int int_value;
        forth_uint uint_value;
        forth_pointer pointer_value;
    };
} forth_cell;

typedef struct forth_context
{
    uint8_t* memory;
    int memory_size;
    uint8_t memory_auto_resize;
    forth_pointer memory_pointer;
    forth_pointer program_pointer;

    forth_cell* stack;
    int stack_size;
    uint8_t stack_auto_resize;
    int stack_pointer;

    forth_cell* return_stack;
    int return_stack_size;
    uint8_t return_stack_auto_resize;
    int return_stack_pointer;

    uint8_t* dict_name_lens;
    char* dict_names;
    forth_pointer* dict_pointers;
    int dict_size;
    uint8_t dict_auto_resize;
    int dict_pointer;
    int default_dict_pointer;

    forth_log_func log;
    const char* code;
    int state;
    const char* token;
    size_t token_len;

    forth_pointer base;
} forth_context;

// Create a context. Returns NULL if failed to create
//  memory_size         : in bytes
//  stack_size          : in cell count
//  return_stack_size   : in pointer count
//  dict_size           : Dictionnary size, in word count
forth_context* forth_create_context(int memory_size       = FORTH_MEM_INFINITE,
                                    int stack_size        = FORTH_MEM_INFINITE,
                                    int return_stack_size = FORTH_MEM_INFINITE,
                                    int dict_size         = FORTH_MEM_INFINITE);

// Destroy a context
void forth_destroy_context(forth_context* ctx);

// Returns cell on top of the stack, or NULL if stack is empty
forth_cell* forth_get_top(forth_context* ctx, int offset = 0);

// Evaluate code. Returns FORTH_SUCCESS on success
int forth_eval(forth_context* ctx, const char* code);

// Add a C-function word to the dictionnary. Returns FORTH_SUCCESS on success
int forth_add_c_word(forth_context* ctx, const char* name, forth_c_func fn);

//---------------------------------------------------------------------------
// IMPLEMENTATION
//---------------------------------------------------------------------------

#if defined(FORTH_IMPLEMENT)

#include <math.h>

#define FORTHI_MEM_ALLOC_CHUNK_SIZE 1024

#define FORTHI_STATE_INTERPRET 0
#define FORTHI_STATE_COMPILE 1
#define FORTHI_STATE_EXECUTE 2

#define FORTHI_INST_RETURN 0
#define FORTHI_INST_CALL_C_FUNCTION 1
#define FORTHI_INST_PUSH_INT_NUMBER 2
#define FORTHI_INST_CALL_WORD 3
#define FORTHI_INST_PRINT_TEXT 4
#define FORTHI_INST_EXECUTE 5

#if FORTH_INT_SIZE_8_BITS
#define FORTHI_INT_PRINT_CODE PRId8
#define FORTHI_UINT_PRINT_CODE PRIu8
#define FORTHI_OCTAL_PRINT_CODE PRIo8
#define FORTHI_HEX_PRINT_CODE PRIX8
#elif FORTH_INT_SIZE_16_BITS
#define FORTHI_INT_PRINT_CODE PRId16
#define FORTHI_UINT_PRINT_CODE PRIu16
#define FORTHI_OCTAL_PRINT_CODE PRIo16
#define FORTHI_HEX_PRINT_CODE PRIX16
#elif FORTH_INT_SIZE_32_BITS
#define FORTHI_INT_PRINT_CODE PRId32
#define FORTHI_UINT_PRINT_CODE PRIu32
#define FORTHI_OCTAL_PRINT_CODE PRIo32
#define FORTHI_HEX_PRINT_CODE PRIX32
#elif FORTH_INT_SIZE_64_BITS
#define FORTHI_INT_PRINT_CODE PRId64
#define FORTHI_UINT_PRINT_CODE PRIu64
#define FORTHI_OCTAL_PRINT_CODE PRIo64
#define FORTHI_HEX_PRINT_CODE PRIX64
#endif

#define FORTH_LOG(_ctx_, _fmt_, ...) \
{ \
    if (ctx && ctx->log) \
        ctx->log(_ctx_, _fmt_, ##__VA_ARGS__); \
    else \
        printf(_fmt_, ##__VA_ARGS__); \
}

//---------------------------------------------------------------------------
// PROTOTYPES
//---------------------------------------------------------------------------

// Memory
static int forthi_grow_memory(forth_context* ctx);
static int forthi_grow_stack(forth_context* ctx);
static int forthi_grow_return_stack(forth_context* ctx);
static int forthi_grow_dictionnary(forth_context* ctx);
static int forthi_reserve_memory_space(forth_context* ctx, int size);
static int forthi_check_valid_memory_range(forth_context* ctx, forth_pointer at, forth_pointer size = 1);
static int forthi_write_byte(forth_context* ctx, uint8_t data);
static int forthi_write_number(forth_context* ctx, forth_int data);
static int forthi_write_number_at(forth_context* ctx, forth_int data, forth_pointer at);
static int forthi_write_pointer(forth_context* ctx, forth_pointer data);
static int forthi_write_function(forth_context* ctx, forth_c_func fn);
static int forthi_write_text(forth_context* ctx, const char* text, size_t len);
static int forthi_read_byte(forth_context* ctx, uint8_t* data);
static int forthi_read_function(forth_context* ctx, forth_c_func* data);
static int forthi_read_number(forth_context* ctx, forth_int* data);
static int forthi_read_number_at(forth_context* ctx, forth_int* data, forth_pointer at);
static int forthi_read_pointer(forth_context* ctx, forth_pointer* data);
static const char* forthi_read_text(forth_context* ctx, forth_int* len);

// Stack
static int forthi_push_cell(forth_context* ctx, forth_cell cell);
static int forthi_push_int_number(forth_context* ctx, forth_int number);
static int forthi_push_double_length_uint(forth_context* ctx, forth_double_length_uint u);
static forth_double_length_uint forthi_to_double_length_uint(forth_uint u1, forth_uint u2);
static int forthi_push_pointer(forth_context* ctx, forth_pointer pointer);
static int forthi_pop(forth_context* ctx, int count = 1);
forth_cell* forth_get_top(forth_context* ctx, int offset);

// Return stack
static int forthi_push_return_cell(forth_context* ctx, forth_cell cell);
static int forthi_push_return_int_number(forth_context* ctx, forth_int number);
static int forthi_push_return_pointer(forth_context* ctx, forth_pointer pointer);
static int forthi_pop_return(forth_context* ctx, int count = 1);
forth_cell* forth_get_return_top(forth_context* ctx, int offset);

// Compile
static int forthi_compile_function_call(forth_context* ctx, forth_c_func fn);
static int forthi_compile_push_int_number(forth_context* ctx, forth_int number);
static int forthi_compile_word_call(forth_context* ctx, forth_pointer pointer);

// Dictionnary
static int forthi_add_word(forth_context* ctx, const char* name, int name_len, forth_pointer memory_offset);
int forth_add_c_word(forth_context* ctx, const char* name, forth_c_func fn);
static forth_pointer forthi_get_word(forth_context* ctx, const char* name, size_t name_len);
static int forthi_get_word_index(forth_context* ctx, const char* name, size_t name_len);

// Interpreting
static int forthi_is_space(char c);
static const char* forthi_trim_code(forth_context* ctx);
static const char* forthi_read_until(forth_context* ctx, char delim);
static const char* forthi_get_next_token(forth_context* ctx, size_t* token_len);
static int forthi_interpret_token(forth_context* ctx);
static int forthi_interpret(forth_context* ctx);
int forth_eval(forth_context* ctx, const char* code);

// Standard words (Only those requiring forward declaration)
static int forthi_word_abort_quote(forth_context* ctx);
static int forthi_word_BEGIN(forth_context* ctx);
static int forthi_word_DO(forth_context* ctx);
static int forthi_word_dot_quote(forth_context* ctx);
static int forthi_word_ELSE(forth_context* ctx);
static int forthi_word_EXECUTE(forth_context* ctx);
static int forthi_word_IF(forth_context* ctx);
static int forthi_word_LOOP(forth_context* ctx);
static int forthi_word_NUMBER(forth_context* ctx);
static int forthi_word_paren(forth_context* ctx);
static int forthi_word_plus_loop(forth_context* ctx);
static int forthi_word_slash_loop(forth_context* ctx);
static int forthi_word_REPEAT(forth_context* ctx);
static int forthi_word_semicolon(forth_context* ctx);
static int forthi_word_THEN(forth_context* ctx);
static int forthi_word_UNTIL(forth_context* ctx);
static int forthi_word_WHILE(forth_context* ctx);

//---------------------------------------------------------------------------
// MEMORY
//---------------------------------------------------------------------------

static int forthi_grow_memory(forth_context* ctx)
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

static int forthi_grow_stack(forth_context* ctx)
{
    int new_size = sizeof(forth_cell) * (ctx->stack_size + FORTHI_MEM_ALLOC_CHUNK_SIZE);

    forth_cell* new_stack = (forth_cell*)malloc(new_size);
    if (!new_stack)
        return FORTH_FAILURE;

    memcpy(new_stack, ctx->stack, ctx->stack_size * sizeof(forth_cell));
    free(ctx->stack);
    ctx->stack = new_stack;
    ctx->stack_size = new_size;

    return FORTH_SUCCESS;
}

static int forthi_grow_return_stack(forth_context* ctx)
{
    int new_size = sizeof(forth_cell) * (ctx->return_stack_size + FORTHI_MEM_ALLOC_CHUNK_SIZE);

    forth_cell* new_stack = (forth_cell*)malloc(new_size);
    if (!new_stack)
        return FORTH_FAILURE;

    memcpy(new_stack, ctx->return_stack, ctx->return_stack_size * sizeof(forth_cell));
    free(ctx->return_stack);
    ctx->return_stack = new_stack;
    ctx->return_stack_size = new_size;

    return FORTH_SUCCESS;
}

static int forthi_grow_dictionnary(forth_context* ctx)
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
    memcpy(new_names + FORTHI_MEM_ALLOC_CHUNK_SIZE * FORTH_DICT_CHAR_COUNT, 
        ctx->dict_names, ctx->dict_size * FORTH_DICT_CHAR_COUNT);
    free(ctx->dict_names);
    ctx->dict_names = new_names;

    forth_pointer* new_pointers = 
        (forth_pointer*)malloc((ctx->dict_size + FORTHI_MEM_ALLOC_CHUNK_SIZE) * sizeof(forth_pointer));
    if (!new_pointers)
        return FORTH_FAILURE;
    memcpy(new_pointers + FORTHI_MEM_ALLOC_CHUNK_SIZE, ctx->dict_pointers, ctx->dict_size * sizeof(forth_pointer));
    free(ctx->dict_pointers);
    ctx->dict_pointers = new_pointers;

    ctx->dict_size += FORTHI_MEM_ALLOC_CHUNK_SIZE;

    return FORTH_SUCCESS;
}

static int forthi_reserve_memory_space(forth_context* ctx, int size)
{
    int space_left = (int)ctx->memory_size - (int)ctx->memory_pointer;
    while (size > space_left)
    {
        if (!ctx->memory_auto_resize)
        {
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }

        if (forthi_grow_memory(ctx) == FORTH_FAILURE)
        {
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }

        space_left = (int)ctx->memory_size - (int)ctx->memory_pointer;
    }

    return FORTH_SUCCESS;
}

static int forthi_reserve_aligned_memory_space(forth_context* ctx, forth_pointer size)
{
    forth_pointer reminder = ctx->memory_pointer % sizeof(uintptr_t);
    if (reminder > 0)
        size += sizeof(uintptr_t) - reminder;

    forth_pointer space_left = ctx->memory_size - ctx->memory_pointer;
    while (size > space_left)
    {
        if (!ctx->memory_auto_resize)
        {
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }

        if (forthi_grow_memory(ctx) == FORTH_FAILURE)
        {
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }

        space_left = ctx->memory_size - ctx->memory_pointer;
    }

    if (reminder > 0)
        ctx->memory_pointer += sizeof(uintptr_t) - reminder;

    return FORTH_SUCCESS;
}

static int forthi_check_valid_memory_range(forth_context* ctx, forth_pointer at, forth_pointer size)
{
    if (at + size > ctx->memory_pointer)
    {
        FORTH_LOG(ctx, "Invalid memory address\n");
        return FORTH_FAILURE;
    }
    return FORTH_SUCCESS;
}

static int forthi_check_valid_aligned_memory_range(forth_context* ctx, forth_pointer at, forth_pointer size)
{
    int reminder = ctx->memory_pointer % sizeof(uintptr_t);
    if (reminder > 0)
        size += sizeof(uintptr_t) - reminder;

    if (at + size > ctx->memory_pointer)
    {
        FORTH_LOG(ctx, "Invalid memory address\n");
        return FORTH_FAILURE;
    }
    
    if (reminder > 0)
        ctx->memory_pointer += sizeof(uintptr_t) - reminder;

    return FORTH_SUCCESS;
}

static int forthi_write_byte(forth_context* ctx, uint8_t data)
{
    if (forthi_reserve_memory_space(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    ctx->memory[ctx->memory_pointer++] = data;

    return FORTH_SUCCESS;
}

static int forthi_write_number(forth_context* ctx, forth_int data)
{
    if (forthi_reserve_memory_space(ctx, sizeof(forth_int)) == FORTH_FAILURE)
        return FORTH_FAILURE;

    *(forth_int*)&ctx->memory[ctx->memory_pointer] = data;
    ctx->memory_pointer += (int)sizeof(forth_int);

    return FORTH_SUCCESS;
}

static int forthi_write_number_at(forth_context* ctx, forth_int data, forth_pointer at)
{
    if (forthi_check_valid_memory_range(ctx, at, (forth_pointer)sizeof(forth_int)) == FORTH_FAILURE)
        return FORTH_FAILURE;

    *(forth_int*)&ctx->memory[at] = data;

    return FORTH_SUCCESS;
}

static int forthi_write_pointer(forth_context* ctx, forth_pointer data)
{
    if (forthi_reserve_memory_space(ctx, sizeof(forth_pointer)) == FORTH_FAILURE)
        return FORTH_FAILURE;

    *(forth_pointer*)&ctx->memory[ctx->memory_pointer] = data;
    ctx->memory_pointer += (forth_pointer)sizeof(forth_pointer);

    return FORTH_SUCCESS;
}

static int forthi_write_function(forth_context* ctx, forth_c_func fn)
{
    if (forthi_reserve_memory_space(ctx, sizeof(fn)) == FORTH_FAILURE)
        return FORTH_FAILURE;

    memcpy(ctx->memory + ctx->memory_pointer, &fn, sizeof(forth_c_func));
    ctx->memory_pointer += (forth_pointer)sizeof(forth_c_func);
    //*(forth_c_func*)&ctx->memory[ctx->memory_pointer] = fn;
    //ctx->memory_pointer += (int)sizeof(forth_c_func);

    return FORTH_SUCCESS;
}

static int forthi_write_text(forth_context* ctx, const char* text, size_t len)
{
    if (forthi_write_number(ctx, (forth_int)len) == FORTH_FAILURE)
        return FORTH_FAILURE;

    if (forthi_reserve_memory_space(ctx, (int)len) == FORTH_FAILURE)
        return FORTH_FAILURE;

    memcpy(ctx->memory + ctx->memory_pointer, text, len);
    ctx->memory_pointer += (forth_pointer)len;

    return FORTH_SUCCESS;
}

static int forthi_read_byte(forth_context* ctx, uint8_t* data)
{
    if (forthi_check_valid_memory_range(ctx, ctx->program_pointer) == FORTH_FAILURE)
        return FORTH_FAILURE;

    *data = ctx->memory[ctx->program_pointer++];
    return FORTH_SUCCESS;
}

static int forthi_read_function(forth_context* ctx, forth_c_func* data)
{
    if (forthi_check_valid_memory_range(ctx, ctx->program_pointer, sizeof(forth_c_func)) == FORTH_FAILURE)
        return FORTH_FAILURE;

    memcpy(data, ctx->memory + ctx->program_pointer, sizeof(forth_c_func));
    ctx->program_pointer += (forth_pointer)sizeof(forth_c_func);
    //*data = *(forth_c_func*)&ctx->memory[ctx->program_pointer];
    //ctx->program_pointer += sizeof(forth_c_func);

    return FORTH_SUCCESS;
}

static int forthi_read_number(forth_context* ctx, forth_int* data)
{
    if (forthi_check_valid_memory_range(ctx, ctx->program_pointer, sizeof(forth_int)) == FORTH_FAILURE)
        return FORTH_FAILURE;

    *data = *(forth_int*)&ctx->memory[ctx->program_pointer];
    ctx->program_pointer += sizeof(forth_int);

    return FORTH_SUCCESS;
}

static int forthi_read_number_at(forth_context* ctx, forth_int* data, forth_pointer at)
{
    if (forthi_check_valid_memory_range(ctx, at, sizeof(forth_int)) == FORTH_FAILURE)
        return FORTH_FAILURE;

    *data = *(forth_int*)&ctx->memory[at];

    return FORTH_SUCCESS;
}

static int forthi_read_pointer(forth_context* ctx, forth_pointer* data)
{
    if (forthi_check_valid_memory_range(ctx, ctx->program_pointer, sizeof(forth_pointer)) == FORTH_FAILURE)
        return FORTH_FAILURE;

    *data = *(forth_pointer*)&ctx->memory[ctx->program_pointer];
    ctx->program_pointer += (forth_pointer)sizeof(forth_pointer);

    return FORTH_SUCCESS;
}

static const char* forthi_read_text(forth_context* ctx, forth_int* len)
{
    const char* text = NULL;

    if (forthi_read_number(ctx, len) == FORTH_FAILURE)
        return NULL;

    if (forthi_check_valid_memory_range(ctx, ctx->program_pointer, (forth_pointer)*len) == FORTH_FAILURE)
        return FORTH_FAILURE;

    text = (const char*)&ctx->memory[ctx->program_pointer];
    ctx->program_pointer += (forth_pointer)*len;

    return text;
}

//---------------------------------------------------------------------------
// STACK
//---------------------------------------------------------------------------

static int forthi_push_cell(forth_context* ctx, forth_cell cell)
{
    if (ctx->stack_pointer >= ctx->stack_size)
    {
        if (!ctx->stack_auto_resize)
        {
            FORTH_LOG(ctx, "Stack overflow\n");
            return FORTH_FAILURE;
        }

        if (forthi_grow_stack(ctx) == FORTH_FAILURE)
        {
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }
    }

    ctx->stack[ctx->stack_pointer++] = cell;
    return FORTH_SUCCESS;
}

static int forthi_push_int_number(forth_context* ctx, forth_int n)
{
    forth_cell cell = {0};
    cell.int_value = n;
    return forthi_push_cell(ctx, cell);
}

static int forthi_push_double_length_uint(forth_context* ctx, forth_double_length_uint u)
{
    forth_cell cell1 = {0};
    forth_cell cell2 = {0};

    cell1.uint_value = (forth_uint)u;
#if FORTH_INT_SIZE_64_BITS
    cell2.uint_value = 0;
#else
    cell2.uint_value = (forth_uint)(u >> (sizeof(forth_uint) * 8));
#endif

    if (forthi_push_cell(ctx, cell1) == FORTH_FAILURE)
        return FORTH_FAILURE;
    return forthi_push_cell(ctx, cell2);
}

static forth_double_length_uint forthi_to_double_length_uint(forth_uint u1, forth_uint u2)
{
#if FORTH_INT_SIZE_8_BITS
    return (forth_double_length_uint)u1 |
        ((forth_double_length_uint)u2 << 8);
#elif FORTH_INT_SIZE_16_BITS
    return (forth_double_length_uint)u1 |
        ((forth_double_length_uint)u2 << 16);
#elif FORTH_INT_SIZE_32_BITS
    return (forth_double_length_uint)u1 |
        ((forth_double_length_uint)u2 << 32);
#else // FORTH_INT_SIZE_64_BITS
    return (forth_double_length_uint)u1;
#endif
}

static int forthi_push_uint_number(forth_context* ctx, forth_uint u)
{
    forth_cell cell = {0};
    cell.uint_value = u;
    return forthi_push_cell(ctx, cell);
}

static int forthi_push_pointer(forth_context* ctx, forth_pointer pointer)
{
    forth_cell cell = {0};
    cell.pointer_value = pointer;
    return forthi_push_cell(ctx, cell);
}

static int forthi_pop(forth_context* ctx, int count)
{
    if (ctx->stack_pointer < count)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    ctx->stack_pointer -= count;
    return FORTH_SUCCESS;
}

forth_cell* forth_get_top(forth_context* ctx, int offset)
{
    if (!ctx)
        return NULL;

    if (ctx->stack_pointer <= offset)
        return NULL;

    return &ctx->stack[ctx->stack_pointer - offset - 1];
}

//---------------------------------------------------------------------------
// RETURN STACK
//---------------------------------------------------------------------------

static int forthi_push_return_cell(forth_context* ctx, forth_cell cell)
{
    if (ctx->return_stack_pointer >= ctx->return_stack_size)
    {
        if (!ctx->return_stack_auto_resize)
        {
            FORTH_LOG(ctx, "Return stack overflow\n");
            return FORTH_FAILURE;
        }

        if (forthi_grow_return_stack(ctx) == FORTH_FAILURE)
        {
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }
    }

    ctx->return_stack[ctx->return_stack_pointer++] = cell;
    return FORTH_SUCCESS;
}

static int forthi_push_return_int_number(forth_context* ctx, forth_int number)
{
    forth_cell cell;
    cell.int_value = number;
    return forthi_push_return_cell(ctx, cell);
}

static int forthi_push_return_pointer(forth_context* ctx, forth_pointer pointer)
{
    forth_cell cell;
    cell.pointer_value = pointer;
    return forthi_push_return_cell(ctx, cell);
}

static int forthi_pop_return(forth_context* ctx, int count)
{
    if (ctx->return_stack_pointer < count)
    {
        FORTH_LOG(ctx, "Return stack underflow\n");
        return FORTH_FAILURE;
    }

    ctx->return_stack_pointer -= count;
    return FORTH_SUCCESS;
}

forth_cell* forth_get_return_top(forth_context* ctx, int offset)
{
    if (!ctx)
        return NULL;

    if (ctx->return_stack_pointer <= offset)
        return NULL;

    return &ctx->return_stack[ctx->return_stack_pointer - offset - 1];
}


//---------------------------------------------------------------------------
// COMPILE
//---------------------------------------------------------------------------

static int forthi_compile_function_call(forth_context* ctx, forth_c_func fn)
{
    // Comments
    if (fn == forthi_word_paren ||
        fn == forthi_word_THEN)
        return fn(ctx);

    if (forthi_write_byte(ctx, FORTHI_INST_CALL_C_FUNCTION) == FORTH_FAILURE)
        return FORTH_FAILURE;

    if (forthi_write_function(ctx, fn) == FORTH_FAILURE)
        return FORTH_FAILURE;

    // Special cases for WORDs that need to do special things at compile
    // time, like: allocate memory, set state flag, etc.
    // Call them at compile time
    if (fn == forthi_word_abort_quote ||
        fn == forthi_word_dot_quote ||
        fn == forthi_word_semicolon ||
        fn == forthi_word_IF ||
        fn == forthi_word_ELSE ||
        fn == forthi_word_DO ||
        fn == forthi_word_LOOP ||
        fn == forthi_word_plus_loop ||
        fn == forthi_word_slash_loop ||
        fn == forthi_word_BEGIN ||
        fn == forthi_word_UNTIL ||
        fn == forthi_word_REPEAT ||
        fn == forthi_word_WHILE)
    {
        return fn(ctx);
    }

    return FORTH_SUCCESS;
}

static int forthi_compile_push_int_number(forth_context* ctx, forth_int number)
{
    if (forthi_write_byte(ctx, FORTHI_INST_PUSH_INT_NUMBER) == FORTH_FAILURE)
        return FORTH_FAILURE;

    return forthi_write_number(ctx, number);
}

static int forthi_compile_word_call(forth_context* ctx, forth_pointer pointer)
{
    if (forthi_write_byte(ctx, FORTHI_INST_CALL_WORD) == FORTH_FAILURE)
        return FORTH_FAILURE;

    return forthi_write_pointer(ctx, pointer);
}

//---------------------------------------------------------------------------
// DICTIONNARY
//---------------------------------------------------------------------------

static int forthi_add_word(forth_context* ctx, const char* name, int name_len, forth_pointer memory_offset)
{
    if (ctx->dict_pointer >= ctx->dict_size)
    {
        if (!ctx->dict_auto_resize)
        {
            FORTH_LOG(ctx, "Dictionnary full\n");
            return FORTH_FAILURE;
        }

        if (forthi_grow_dictionnary(ctx) == FORTH_FAILURE)
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

int forth_add_c_word(forth_context* ctx, const char* name, forth_c_func fn)
{
    forth_pointer memory_pointer = ctx->memory_pointer;

    if (forthi_write_byte(ctx, FORTHI_INST_CALL_C_FUNCTION) == FORTH_FAILURE)
        return FORTH_FAILURE;

    if (forthi_write_function(ctx, fn) == FORTH_FAILURE)
        return FORTH_FAILURE;

    return forthi_add_word(ctx, name, (int)strlen(name), memory_pointer);
}

static forth_pointer forthi_get_word(forth_context* ctx, const char* name, size_t name_len)
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

    return (forth_pointer)-1;
}

static int forthi_get_word_index(forth_context* ctx, const char* name, size_t name_len)
{
    int index = ctx->dict_size - ctx->dict_pointer;
    if (ctx->state == FORTHI_STATE_COMPILE)
        index++; // Skip word being compiled

    size_t compare_len = name_len > FORTH_DICT_CHAR_COUNT ? FORTH_DICT_CHAR_COUNT : name_len;
    while (index < ctx->dict_size)
    {
        if (ctx->dict_name_lens[index] == name_len)
            if (strncmp(ctx->dict_names + index * FORTH_DICT_CHAR_COUNT, name, compare_len) == 0)
                return index;

        index++;
    }

    return index;
}

//---------------------------------------------------------------------------
// INTERPRETING
//---------------------------------------------------------------------------

static int forthi_is_space(char c)
{
    return !c || c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static const char* forthi_trim_code(forth_context* ctx)
{
    while (*ctx->code && forthi_is_space(*ctx->code))
        ctx->code++;

    return ctx->code;
}

static const char* forthi_read_until(forth_context* ctx, char delim)
{
    while (*ctx->code && *ctx->code != delim)
        ctx->code++;
    if (!*ctx->code)
        return ctx->code;

    return ctx->code;
}

static const char* forthi_get_next_token(forth_context* ctx, size_t* token_len)
{
    const char* token_start = forthi_trim_code(ctx);

    while (*ctx->code)
    {
        ctx->code++;
        if (forthi_is_space(*ctx->code))
        {
            *token_len = ctx->code - token_start;
            return token_start;
        }
    }

    return NULL;
}

static int forthi_interpret_token(forth_context* ctx)
{
    forth_pointer memory_pointer = forthi_get_word(ctx, ctx->token, ctx->token_len);
    if (memory_pointer != (forth_pointer)-1)
    {
        uint8_t word_type = ctx->memory[memory_pointer];
        if (word_type == FORTHI_INST_CALL_C_FUNCTION)
        {
            forth_c_func fn = NULL;
            memcpy(&fn, ctx->memory + memory_pointer + 1, sizeof(forth_c_func));
            if (ctx->state == FORTHI_STATE_INTERPRET)
                return fn(ctx);
            else
                return forthi_compile_function_call(ctx, fn);
        }
        else if (word_type == FORTHI_INST_EXECUTE)
        {
            if (ctx->state == FORTHI_STATE_INTERPRET)
            {
                forthi_push_pointer(ctx, (forth_pointer)(memory_pointer + 1));
                return forthi_word_EXECUTE(ctx);
            }
            else
            {
                return forthi_compile_word_call(ctx, memory_pointer + 1);
            }
        }

        return FORTH_FAILURE;
    }

    if (forthi_word_NUMBER(ctx) == FORTH_FAILURE)
        return FORTH_FAILURE;

    return FORTH_SUCCESS;
}

static int forthi_interpret(forth_context* ctx)
{
    while ((ctx->token = forthi_get_next_token(ctx, &ctx->token_len)))
    {
        if (forthi_interpret_token(ctx) == FORTH_FAILURE)
            return FORTH_FAILURE;
    }

    return FORTH_SUCCESS;
}

int forth_eval(forth_context* ctx, const char* code)
{
    if (!ctx)
        return FORTH_FAILURE;

    if (!code)
        return FORTH_FAILURE;

    ctx->code = code;
    ctx->state = FORTHI_STATE_INTERPRET;

    if (forthi_interpret(ctx) == FORTH_FAILURE)
    {
        ctx->stack_pointer = 0;
        ctx->return_stack_pointer = 0;
        return FORTH_FAILURE;
    }
    return FORTH_SUCCESS;
}

//---------------------------------------------------------------------------
// STANDARD WORDS
//---------------------------------------------------------------------------

static int forthi_word_store(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_number_sign(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_number_sign_greater(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_number_sign_s(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_tick(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_paren(forth_context* ctx)
{
    forthi_read_until(ctx, ')');

    if (*ctx->code)
        ctx->code++;

    return FORTH_SUCCESS;
}

static int forthi_word_paren_local_paren(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_star(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 * n2);
}

static int forthi_word_star_slash(forth_context* ctx)
{
    if (forthi_pop(ctx, 3) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    forth_int n3 = ctx->stack[ctx->stack_pointer + 2].int_value;

    forth_double_length_int intermediate = (forth_double_length_int)n1 * (forth_double_length_int)n2;
    return forthi_push_int_number(ctx, (forth_int)floor((double)intermediate / (double)n3));
}

static int forthi_word_star_slash_mod(forth_context* ctx)
{
    if (forthi_pop(ctx, 3) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_uint u1 = ctx->stack[ctx->stack_pointer].uint_value;
    forth_uint u2 = ctx->stack[ctx->stack_pointer + 1].uint_value;
    forth_uint u3 = ctx->stack[ctx->stack_pointer + 2].uint_value;

    forth_double_length_uint intermediate = (forth_double_length_uint)u1 * (forth_double_length_uint)u2;

    if (forthi_push_uint_number(ctx, (forth_uint)(intermediate % (forth_double_length_uint)u3)) == FORTH_FAILURE)
        return FORTH_FAILURE;
    
    return forthi_push_uint_number(ctx, (forth_uint)(intermediate / (forth_double_length_uint)u3));
}

static int forthi_word_plus(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 + n2);
}

static int forthi_word_plus_store(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_plus_field(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_plus_loop(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        return forthi_write_pointer(ctx, ctx->stack[ctx->stack_pointer].pointer_value);
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        if (ctx->return_stack_pointer < 2)
        {
            FORTH_LOG(ctx, "Return stack underflow\n");
            return FORTH_FAILURE;
        }

        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;
        
        forth_int i = ctx->return_stack[ctx->return_stack_pointer - 1].int_value;
        forth_int prev_i = i;
        forth_int i_tick = ctx->return_stack[ctx->return_stack_pointer - 2].int_value;
        forth_int inc = ctx->stack[ctx->stack_pointer].int_value;
        forth_int diff = i_tick - i;
        i += inc;

        if (inc > 0 && prev_i > i)
        {
            ctx->program_pointer += sizeof(forth_pointer);
            return forthi_pop_return(ctx, 2);
        }

        if ((diff > 0 && i < i_tick) || (diff <= 0 && i >= i_tick))
        {
            ctx->return_stack[ctx->return_stack_pointer - 1].int_value = i;

            if (forthi_check_valid_memory_range(ctx, ctx->program_pointer, sizeof(forth_pointer)) == FORTH_FAILURE)
                return FORTH_FAILURE;

            ctx->program_pointer = *(forth_pointer*)&ctx->memory[ctx->program_pointer];
            return FORTH_SUCCESS;
        }

        ctx->program_pointer += sizeof(forth_pointer);
        return forthi_pop_return(ctx, 2);
    }

    return FORTH_FAILURE;
}

static int forthi_word_slash_loop(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        return forthi_write_pointer(ctx, ctx->stack[ctx->stack_pointer].pointer_value);
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        if (ctx->return_stack_pointer < 2)
        {
            FORTH_LOG(ctx, "Return stack underflow\n");
            return FORTH_FAILURE;
        }

        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        forth_uint i = ctx->return_stack[ctx->return_stack_pointer - 1].uint_value;
        forth_uint i_tick = ctx->return_stack[ctx->return_stack_pointer - 2].uint_value;
        forth_uint inc = ctx->stack[ctx->stack_pointer].uint_value;
        i += inc;
        if (i < i_tick)
        {
            ctx->return_stack[ctx->return_stack_pointer - 1].uint_value = i;

            if (forthi_check_valid_memory_range(ctx, ctx->program_pointer, sizeof(forth_pointer)) == FORTH_FAILURE)
                return FORTH_FAILURE;

            ctx->program_pointer = *(forth_pointer*)&ctx->memory[ctx->program_pointer];
            return FORTH_SUCCESS;
        }

        ctx->program_pointer += sizeof(forth_pointer);
        return forthi_pop_return(ctx, 2);
    }

    return FORTH_FAILURE;
}

static int forthi_word_plus_x_string(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_comma(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_minus(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 - n2);
}

static int forthi_word_dash_trailing(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dash_trailing_garbage(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dot(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;

    forth_int base;
    if (forthi_read_number_at(ctx, &base, ctx->base) == FORTH_FAILURE)
        return FORTH_FAILURE;

    if (base == 10)
        FORTH_LOG(ctx, "%" FORTHI_INT_PRINT_CODE " ", n)
    else if (base == 8)
        FORTH_LOG(ctx, "%" FORTHI_OCTAL_PRINT_CODE " ", (forth_uint)n)
    else if (base == 16)
        FORTH_LOG(ctx, "%" FORTHI_HEX_PRINT_CODE " ", (forth_uint)n)
    else
        return FORTH_FAILURE;

    return FORTH_SUCCESS;
}

static int forthi_word_dot_quote(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        forth_int len;
        const char* text = forthi_read_text(ctx, &len);
        if (text == NULL)
            return FORTH_FAILURE;

        FORTH_LOG(ctx, "%.*s", (unsigned int)len, text);
        return FORTH_SUCCESS;
    }

    if (*ctx->code)
        ctx->code++;

    const char* string_start = ctx->code;
    const char* string_end = forthi_read_until(ctx, '\"');

    if (*ctx->code)
        ctx->code++;

    size_t len = string_end - string_start;

    if (ctx->state == FORTHI_STATE_COMPILE)
        return forthi_write_text(ctx, string_start, len);

    FORTH_LOG(ctx, "%.*s", (unsigned int)len, string_start);
    return FORTH_SUCCESS;
}

static int forthi_word_dot_paren(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dot_r(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dot_s(forth_context* ctx)
{
    forth_int base;
    if (forthi_read_number_at(ctx, &base, ctx->base) == FORTH_FAILURE)
        return FORTH_FAILURE;

    for (int i = 0; i < ctx->stack_pointer; i++)
    {
        forth_int n = ctx->stack[i].int_value;

        if (base == 10)
            FORTH_LOG(ctx, "%" FORTHI_INT_PRINT_CODE " ", n)
        else if (base == 8)
            FORTH_LOG(ctx, "%" FORTHI_OCTAL_PRINT_CODE " ", n)
        else if (base == 16)
            FORTH_LOG(ctx, "%" FORTHI_HEX_PRINT_CODE " ", n)
        else
            return FORTH_FAILURE;
    }

    return FORTH_SUCCESS;
}

static int forthi_word_slash(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, (forth_int)floor((double)n1 / (double)n2));
}

static int forthi_word_slash_mod(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    if (forthi_push_int_number(ctx, n1 % n2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    return forthi_push_int_number(ctx, (forth_int)floor((double)n1 / (double)n2));
}

static int forthi_word_slash_string(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_zero_less(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n < 0 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_zero_not_equals(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n != 0 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_zero_equals(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n == 0 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_zero_greater(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n > 0 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_one_plus(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n + 1);
}

static int forthi_word_one_minus(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n - 1);
}

static int forthi_word_two_plus(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n + 2);
}

static int forthi_word_two_minus(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n - 2);
}

static int forthi_word_two_store(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_star(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n << 1);
}

static int forthi_word_two_slash(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n = ctx->stack[ctx->stack_pointer].int_value;
    return forthi_push_int_number(ctx, n / 2);
}

static int forthi_word_two_to_r(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_fetch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_constant(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_drop(forth_context* ctx)
{
    return forthi_pop(ctx, 2);
}

static int forthi_word_two_dupe(forth_context* ctx)
{
    if (ctx->stack_pointer < 2)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    if (forthi_push_cell(ctx, ctx->stack[ctx->stack_pointer - 2]) == FORTH_FAILURE)
        return FORTH_FAILURE;
    return forthi_push_cell(ctx, ctx->stack[ctx->stack_pointer - 2]);
}

static int forthi_word_two_literal(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_over(forth_context* ctx)
{
    if (ctx->stack_pointer < 4)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    if (forthi_push_cell(ctx, ctx->stack[ctx->stack_pointer - 4]) == FORTH_FAILURE)
        return FORTH_FAILURE;
    return forthi_push_cell(ctx, ctx->stack[ctx->stack_pointer - 4]);
}

static int forthi_word_two_r_from(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_r_fetch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_rote(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_swap(forth_context* ctx)
{
    if (forthi_pop(ctx, 4) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    forth_int n3 = ctx->stack[ctx->stack_pointer + 2].int_value;
    forth_int n4 = ctx->stack[ctx->stack_pointer + 3].int_value;

    forthi_push_int_number(ctx, n3);
    forthi_push_int_number(ctx, n4);
    forthi_push_int_number(ctx, n1);
    return forthi_push_int_number(ctx, n2);
}

static int forthi_word_two_value(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_two_variable(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_colon(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        FORTH_LOG(ctx, "Unexpected ':'\n");
        return FORTH_FAILURE;
    }

    size_t word_name_len;
    const char* word_name = forthi_get_next_token(ctx, &word_name_len);
    if (!word_name || !word_name_len)
    {
        FORTH_LOG(ctx, "Expected name after ':'\n");
        return FORTH_FAILURE;
    }

    if (forthi_add_word(ctx, word_name, (int)word_name_len, ctx->memory_pointer) == FORTH_FAILURE)
        return FORTH_FAILURE;

    ctx->state = FORTHI_STATE_COMPILE;

    if (forthi_reserve_memory_space(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;
    ctx->memory[ctx->memory_pointer++] = FORTHI_INST_EXECUTE;

    return FORTH_SUCCESS;
}

static int forthi_word_colon_no_name(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_semicolon(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        ctx->state = FORTHI_STATE_INTERPRET;
        return FORTH_SUCCESS;
    }
    else if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        if (forthi_pop_return(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;
        ctx->program_pointer = ctx->return_stack[ctx->return_stack_pointer].pointer_value;
        return FORTH_SUCCESS;
    }
    
    FORTH_LOG(ctx, "Interpreting a compile-only word\n");
    return FORTH_FAILURE;
}

static int forthi_word_semicolon_code(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_less_than(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 < n2 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_less_number_sign(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_not_equals(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 != n2 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_equals(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 == n2 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_greater_than(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 > n2 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_to_body(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_to_float(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_to_in(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_to_number(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_to_r(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    return forthi_push_return_pointer(ctx, (forth_pointer)ctx->stack[ctx->stack_pointer].int_value);
}

static int forthi_word_question(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_question_do(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_question_dupe(forth_context* ctx)
{
    forth_cell* top = forth_get_top(ctx);
    if (!top)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    if (top->int_value == FORTH_FALSE)
        return FORTH_SUCCESS;

    return forthi_push_cell(ctx, *top);
}

static int forthi_word_fetch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ABORT(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int number = ctx->stack[ctx->stack_pointer].int_value;
    if (number != FORTH_FALSE)
        return FORTH_FAILURE;

    return FORTH_SUCCESS;
}

static int forthi_word_abort_quote(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        forth_int number = ctx->stack[ctx->stack_pointer].int_value;
        if (number == FORTH_FALSE)
            return FORTH_SUCCESS;

        forth_int len;
        const char* text = forthi_read_text(ctx, &len);
        if (text == NULL)
            return FORTH_FAILURE;

        FORTH_LOG(ctx, "%.*s", (unsigned int)len, text);
        return FORTH_FAILURE;
    }

    if (*ctx->code)
        ctx->code++;

    const char* string_start = ctx->code;
    const char* string_end = forthi_read_until(ctx, '\"');

    if (*ctx->code)
        ctx->code++;

    size_t len = string_end - string_start;

    if (ctx->state == FORTHI_STATE_COMPILE)
        return forthi_write_text(ctx, string_start, len);

    FORTH_LOG(ctx, "%.*s", (unsigned int)len, string_start);

    return FORTH_SUCCESS;
}

static int forthi_word_abs(forth_context* ctx)
{
    if (ctx->stack_pointer < 1)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    forth_int n = ctx->stack[ctx->stack_pointer - 1].int_value;
    if (n < 0) n = -n;
    ctx->stack[ctx->stack_pointer - 1].int_value = n;

    return FORTH_SUCCESS;
}

static int forthi_word_ACCEPT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ACTION_OF(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_AGAIN(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_AHEAD(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALIGN(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALIGNED(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALLOCATE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALLOT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ALSO(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_AND(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 & n2);
}

static int forthi_word_ASSEMBLER(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_at_x_y(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BASE(forth_context* ctx)
{
    return forthi_push_pointer(ctx, ctx->base);
}

static int forthi_word_BEGIN(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        return forthi_push_pointer(ctx, ctx->memory_pointer);
    }

    return FORTH_SUCCESS;
}

static int forthi_word_BEGIN_STRUCTURE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BIN(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_b_l(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BLANK(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_b_l_k(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BLOCK(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BUFFER(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_buffer_colon(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_BYE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_store(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_quote(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_comma(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_fetch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CASE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CATCH(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_cell_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CELLS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_field_colon(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_char(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_char_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_chars(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CLOSE_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_move(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_move_up(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CODE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_COMPARE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_compile_comma(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CONSTANT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_COUNT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_r(forth_context* ctx)
{
    FORTH_LOG(ctx, "\n");
    return FORTH_SUCCESS;
}

static int forthi_word_CREATE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_CREATE_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_s_pick(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_c_s_roll(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_minus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_dot(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_dot_r(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_zero_less(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_zero_equals(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_two_star(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_two_slash(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_less_than(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_equals(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_to_f(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_to_s(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_abs(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DECIMAL(forth_context* ctx)
{
    return forthi_write_number_at(ctx, 10, ctx->base);
}

static int forthi_word_DEFER(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_defer_store(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_defer_fetch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DEFINITIONS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DELETE_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DEPTH(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_store(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_fetch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_align(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_aligned(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_f_field_colon(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_float_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_floats(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_max(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_min(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_d_negate(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DO(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        return forthi_push_pointer(ctx, ctx->memory_pointer);
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        if (forthi_pop(ctx, 2) == FORTH_FAILURE)
            return FORTH_FAILURE;

        forth_int limit = ctx->stack[ctx->stack_pointer].int_value;
        forth_int first = ctx->stack[ctx->stack_pointer + 1].int_value;

        if (forthi_push_return_pointer(ctx, (forth_pointer)limit) == FORTH_FAILURE)
            return FORTH_FAILURE;
        return forthi_push_return_pointer(ctx, (forth_pointer)first);
    }
    
    return FORTH_FAILURE;
}

static int forthi_word_does(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DROP(forth_context* ctx)
{
    return forthi_pop(ctx, 1);
}

static int forthi_word_d_u_less(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_DUMP(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_dupe(forth_context* ctx)
{
    forth_cell* top = forth_get_top(ctx);
    if (!top)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    return forthi_push_cell(ctx, *top);
}

static int forthi_word_EDITOR(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key_to_char(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key_to_f_key(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key_to_x_char(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_e_key_question(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ELSE(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        forth_pointer false_branch_pointer = ctx->stack[ctx->stack_pointer].pointer_value;

        if (forthi_check_valid_memory_range(ctx, false_branch_pointer, sizeof(forth_pointer)) == FORTH_FAILURE)
            return FORTH_FAILURE;

        if (forthi_push_pointer(ctx, ctx->memory_pointer) == FORTH_FAILURE)
            return FORTH_FAILURE;

        if (forthi_write_pointer(ctx, 0) == FORTH_FAILURE)
            return FORTH_FAILURE;

        *(forth_pointer*)&ctx->memory[false_branch_pointer] = ctx->memory_pointer;

        return FORTH_SUCCESS;
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        forth_pointer then_pointer;
        if (forthi_read_pointer(ctx, &then_pointer) == FORTH_FAILURE)
            return FORTH_FAILURE;

        ctx->program_pointer = then_pointer;
        return FORTH_SUCCESS;
    }
    
    return FORTH_FAILURE;
}

static int forthi_word_EMIT(forth_context* ctx)
{
     if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    char c = (char)ctx->stack[ctx->stack_pointer].int_value;
    FORTH_LOG(ctx, "%c", c);
    return FORTH_SUCCESS;
}

static int forthi_word_emit_question(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_EMPTY(forth_context* ctx)
{
    ctx->dict_pointer = ctx->default_dict_pointer;
    return FORTH_SUCCESS;
}

static int forthi_word_EMPTY_BUFFERS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_END_STRUCTURE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_end_case(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_end_of(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_environment_query(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ERASE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_EVALUATE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_EXECUTE(forth_context* ctx)
{
    uint8_t inst;
    forth_c_func fn;
    forth_int number;
    forth_pointer pointer;

    if (forthi_pop(ctx) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forthi_push_return_pointer(ctx, ctx->program_pointer);
    ctx->program_pointer = ctx->stack[ctx->stack_pointer].pointer_value;
    ctx->state = FORTHI_STATE_EXECUTE;

    while (ctx->return_stack_pointer > 0)
    {
        if (forthi_read_byte(ctx, &inst) == FORTH_FAILURE)
            return FORTH_FAILURE;

        if (inst == FORTHI_INST_CALL_C_FUNCTION)
        {
            if (forthi_read_function(ctx, &fn) == FORTH_FAILURE)
                return FORTH_FAILURE;

            if (fn(ctx) == FORTH_FAILURE)
                return FORTH_FAILURE;
        }
        else if (inst == FORTHI_INST_PUSH_INT_NUMBER)
        {
            if (forthi_read_number(ctx, &number) == FORTH_FAILURE)
                return FORTH_FAILURE;

            if (forthi_push_int_number(ctx, number) == FORTH_FAILURE)
                return FORTH_FAILURE;
        }
        else if (inst == FORTHI_INST_CALL_WORD)
        {
            if (forthi_read_pointer(ctx, &pointer) == FORTH_FAILURE)
                return FORTH_FAILURE;

            if (forthi_push_return_pointer(ctx, ctx->program_pointer) == FORTH_FAILURE)
                return FORTH_FAILURE;

            ctx->program_pointer = pointer;
        }
    }

    ctx->state = FORTHI_STATE_INTERPRET;
    return FORTH_SUCCESS;
}

static int forthi_word_EXIT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_store(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_star(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_star_star(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_minus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_dot(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_slash(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_zero_less_than(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_zero_equals(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_to_d(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_F_to_S(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_fetch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_abs(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_cos(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_cosh(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_align(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_aligned(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_log(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FALSE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_sine(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_cinch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_tan(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_tan_two(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_a_tan_h(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_constant(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_cos(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_cosh(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_depth(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_drop(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_dupe(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_e_dot(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_e_x_p(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_e_x_p_m_one(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_field_colon(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_field_colon(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FILE_POSITION(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FILE_SIZE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FILE_STATUS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FILL(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FIND(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_literal(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_l_n(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_l_n_p_one(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_float_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FLOATS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_log(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FLOOR(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FLUSH(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FLUSH_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_m_slash_mod(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_max(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_min(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_negate(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FORGET(forth_context* ctx)
{
    size_t word_len;
    const char* word_name = forthi_get_next_token(ctx, &word_len);
    if (!word_name)
    {
        FORTH_LOG(ctx, "Undefined word\n");
        return FORTH_FAILURE;
    }

    int index = forthi_get_word_index(ctx, word_name, word_len);;
    if (index == ctx->dict_size)
    {
        FORTH_LOG(ctx, "Undefined word\n");
        return FORTH_FAILURE;
    }

    ctx->dict_pointer = ctx->dict_size - index - 1;
    return FORTH_SUCCESS;
}

static int forthi_word_FORTH(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FORTH_WORDLIST(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_over(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_FREE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_rote(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_round(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_s_dot(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_sine(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_sine_cos(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_cinch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_square_root(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_swap(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_tan(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_tan_h(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_trunc(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_value(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_variable(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_f_proximate(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_GET_CURRENT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_GET_ORDER(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_HERE(forth_context* ctx)
{
    return forthi_push_pointer(ctx, ctx->memory_pointer);
}

static int forthi_word_HEX(forth_context* ctx)
{
    return forthi_write_number_at(ctx, 16, ctx->base);
}

static int forthi_word_HOLD(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_HOLDS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_I(forth_context* ctx)
{
    if (ctx->return_stack_pointer < 1)
    {
        FORTH_LOG(ctx, "Return stack underflow\n");
        return FORTH_FAILURE;
    }

    return forthi_push_pointer(ctx, ctx->return_stack[ctx->return_stack_pointer - 1].pointer_value);
}

static int forthi_word_i_tick(forth_context* ctx)
{
    if (ctx->return_stack_pointer < 2)
    {
        FORTH_LOG(ctx, "Return stack underflow\n");
        return FORTH_FAILURE;
    }

    return forthi_push_pointer(ctx, ctx->return_stack[ctx->return_stack_pointer - 2].pointer_value);
}

static int forthi_word_IF(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        if (forthi_push_pointer(ctx, ctx->memory_pointer) == FORTH_FAILURE)
            return FORTH_FAILURE;

        return forthi_write_pointer(ctx, 0);
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        forth_pointer false_branch_pointer;
        if (forthi_read_pointer(ctx, &false_branch_pointer) == FORTH_FAILURE)
            return FORTH_FAILURE;

        if (ctx->stack[ctx->stack_pointer].int_value == FORTH_FALSE)
            ctx->program_pointer = false_branch_pointer;

        return FORTH_SUCCESS;
    }
    
    return FORTH_FAILURE;
}

static int forthi_word_IMMEDIATE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_INCLUDE(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        size_t filename_len;
        const char* filename = forthi_get_next_token(ctx, &filename_len);
        if (!filename || filename_len > 259)
        {
            FORTH_LOG(ctx, "No such file or directory\n");
            return FORTH_FAILURE;
        }

        char zs_filename[260];
        memcpy(zs_filename, filename, filename_len);
        zs_filename[filename_len] = '\0';

        FILE* file = fopen(zs_filename, "rb");
        if (!file)
        {
            FORTH_LOG(ctx, "No such file or directory\n");
            return FORTH_FAILURE;
        }

        fseek(file, 0, SEEK_END);
        size_t file_size = (size_t)ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size == 0)
        {
            fclose(file);
            return FORTH_SUCCESS; // Nothing to load, its a success nothing gets copied
        }

        char* file_content = (char*)malloc(file_size + 1);
        if (!file_content)
        {
            fclose(file);
            FORTH_LOG(ctx, "Out of memory\n");
            return FORTH_FAILURE;
        }
        size_t byte_read = fread(file_content, 1, file_size, file);
        file_content[file_size] = '\0';
        fclose(file);

        if (byte_read != file_size)
        {
            free(file_content);
            FORTH_LOG(ctx, "Error reading file\n");
            return FORTH_FAILURE;
        }
        
        const char* previous_code = ctx->code;

        int result = forth_eval(ctx, file_content);
        
        free(file_content);
        ctx->code = previous_code;

        return result;
    }

    FORTH_LOG(ctx, "Interpret-only word\n");
    return FORTH_FAILURE;
}

static int forthi_word_INCLUDE_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_INCLUDED(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_INVERT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_IS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_J(forth_context* ctx)
{
    if (ctx->return_stack_pointer < 3)
    {
        FORTH_LOG(ctx, "Return stack underflow\n");
        return FORTH_FAILURE;
    }

    return forthi_push_pointer(ctx, ctx->return_stack[ctx->return_stack_pointer - 3].pointer_value);
}

static int forthi_word_K_ALT_MASK(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_CTRL_MASK(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_DELETE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_DOWN(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_END(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_1(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_10(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_11(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_12(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_2(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_3(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_4(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_5(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_6(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_7(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_8(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_k_f_9(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_HOME(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_INSERT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_LEFT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_NEXT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_PRIOR(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_RIGHT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_SHIFT_MASK(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_K_UP(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_KEY(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_key_question(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_LEAVE(forth_context* ctx)
{
    if (ctx->return_stack_pointer < 2)
    {
        FORTH_LOG(ctx, "Return stack underflow\n");
        return FORTH_FAILURE;
    }

    ctx->return_stack[ctx->return_stack_pointer - 1] = ctx->return_stack[ctx->return_stack_pointer - 2];
    return FORTH_SUCCESS;
}

static int forthi_word_LIST(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_LITERAL(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_LOAD(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_locals_bar(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_LOOP(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        return forthi_write_pointer(ctx, ctx->stack[ctx->stack_pointer].pointer_value);
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        if (ctx->return_stack_pointer < 2)
        {
            FORTH_LOG(ctx, "Return stack underflow\n");
            return FORTH_FAILURE;
        }

        forth_int i = ctx->return_stack[ctx->return_stack_pointer - 1].int_value;
        i++;
        if (i < ctx->return_stack[ctx->return_stack_pointer - 2].int_value)
        {
            ctx->return_stack[ctx->return_stack_pointer - 1].int_value = i;

            if (forthi_check_valid_memory_range(ctx, ctx->program_pointer, sizeof(forth_pointer)) == FORTH_FAILURE)
                return FORTH_FAILURE;

            ctx->program_pointer = *(forth_pointer*)&ctx->memory[ctx->program_pointer];
            return FORTH_SUCCESS;
        }

        ctx->program_pointer += sizeof(forth_pointer);
        return forthi_pop_return(ctx, 2);
    }

    return FORTH_FAILURE;
}

static int forthi_word_l_shift(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_m_star(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_m_star_slash(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_m_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_MARKER(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_MAX(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 > n2 ? n1 : n2);
}

static int forthi_word_MIN(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 < n2 ? n1 : n2);
}

static int forthi_word_MOD(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 % n2);
}

static int forthi_word_MOVE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_MS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_n_to_r(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_name_to_compile(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_name_to_interpret(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_name_to_string(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_NEGATE(forth_context* ctx)
{
    if (ctx->stack_pointer < 1)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    ctx->stack[ctx->stack_pointer - 1].int_value = -ctx->stack[ctx->stack_pointer - 1].int_value;

    return FORTH_SUCCESS;
}

static int forthi_word_NIP(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_n_r_from(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_NUMBER(forth_context* ctx)
{
    if (ctx->token_len == 0 || ctx->token == NULL)
    {
        FORTH_LOG(ctx, "Invalid memory\n");
        return FORTH_FAILURE;
    }

    forth_int base;
    if (forthi_read_number_at(ctx, &base, ctx->base) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int number;

    forth_int sign = 1;
    if (ctx->token[0] == '-')
    {
        sign = -1;
        ctx->token++;
        ctx->token_len--;
    }

    if (base == 10)
    {
        for (size_t i = 0; i < ctx->token_len; i++)
        {
            char c = ctx->token[i];
            if (c < '0' || c > '9')
            {
                FORTH_LOG(ctx, "Undefined word\n");
                return FORTH_FAILURE;
            }
        }

        number = (forth_int)atoi(ctx->token);
    }
    else if (base == 8)
    {
        for (size_t i = 0; i < ctx->token_len; i++)
        {
            char c = ctx->token[i];
            if (c < '0' || c > '7')
            {
                FORTH_LOG(ctx, "Undefined word\n");
                return FORTH_FAILURE;
            }
        }

        number = (forth_int)strtol(ctx->token, NULL, 8);
    }
    else if (base == 16)
    {
        if (ctx->token_len > 2 && ctx->token[0] == '0' && toupper(ctx->token[1]) == 'X')
        {
            ctx->token += 2;
            ctx->token_len -= 2;
        }

        for (size_t i = 0; i < ctx->token_len; i++)
        {
            char c = ctx->token[i];
            if ((c < '0' || c > '9') &&
                (toupper(c) < 'A' || toupper(c) > 'F'))
            {
                FORTH_LOG(ctx, "Undefined word\n");
                return FORTH_FAILURE;
            }
        }

        number = (forth_int)strtol(ctx->token, NULL, 16);
    }
    else
    {
        FORTH_LOG(ctx, "Unsupported base\n");
        return FORTH_FAILURE;
    }

    number *= sign;

    if (ctx->state == FORTHI_STATE_INTERPRET)
        return forthi_push_int_number(ctx, number);
    else if (ctx->state == FORTHI_STATE_COMPILE)
        return forthi_compile_push_int_number(ctx, number);
    
    FORTH_LOG(ctx, "Undefined word\n");
    return FORTH_FAILURE;
}

static int forthi_word_OCTAL(forth_context* ctx)
{
    return forthi_write_number_at(ctx, 8, ctx->base);
}

static int forthi_word_OF(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ONLY(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_OPEN_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_OR(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    return forthi_push_int_number(ctx, n1 | n2);
}

static int forthi_word_ORDER(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_OVER(forth_context* ctx)
{
    if (ctx->stack_pointer < 2)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    return forthi_push_cell(ctx, ctx->stack[ctx->stack_pointer - 2]);
}

static int forthi_word_PAD(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PAGE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PARSE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PARSE_NAME(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PICK(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_POSTPONE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PRECISION(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_PREVIOUS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_QUIT(forth_context* ctx)
{
    return FORTH_FAILURE;
}

static int forthi_word_r_o(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_r_w(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_r_from(forth_context* ctx)
{
    if (forthi_pop_return(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    return forthi_push_pointer(ctx, ctx->return_stack[ctx->return_stack_pointer].pointer_value);
}

static int forthi_word_r_fetch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_READ_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_READ_LINE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RECURSE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REFILL(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RENAME_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REPEAT(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        if (forthi_pop(ctx, 2) == FORTH_FAILURE)
            return FORTH_FAILURE;

        forth_pointer begin_pointer = ctx->stack[ctx->stack_pointer].pointer_value;
        forth_pointer while_pointer = ctx->stack[ctx->stack_pointer + 1].pointer_value;

        if (forthi_write_pointer(ctx, begin_pointer) == FORTH_FAILURE)
            return FORTH_FAILURE;

        if (forthi_check_valid_memory_range(ctx, while_pointer, sizeof(forth_pointer)) == FORTH_FAILURE)
            return FORTH_FAILURE;

        *(forth_pointer*)&ctx->memory[while_pointer] = ctx->memory_pointer;

        return FORTH_SUCCESS;
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        forth_pointer begin_pointer;
        if (forthi_read_pointer(ctx, &begin_pointer) == FORTH_FAILURE)
            return FORTH_FAILURE;

        ctx->program_pointer = begin_pointer;
        return FORTH_SUCCESS;
    }

    return FORTH_FAILURE;
}

static int forthi_word_REPLACES(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REPOSITION_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REPRESENT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REQUIRE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_REQUIRED(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RESIZE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RESIZE_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_RESTORE_INPUT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_ROLL(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_rote(forth_context* ctx)
{
    if (ctx->stack_pointer < 3)
    {
        FORTH_LOG(ctx, "Stack underflow\n");
        return FORTH_FAILURE;
    }

    forth_cell bottom = ctx->stack[ctx->stack_pointer - 3];
    ctx->stack[ctx->stack_pointer - 3] = ctx->stack[ctx->stack_pointer - 2];
    ctx->stack[ctx->stack_pointer - 2] = ctx->stack[ctx->stack_pointer - 1];
    ctx->stack[ctx->stack_pointer - 1] = bottom;

    return FORTH_SUCCESS;
}

static int forthi_word_r_shift(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_quote(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_to_d(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_to_F(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SAVE_BUFFERS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SAVE_INPUT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_c_r(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SEARCH(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SEARCH_WORDLIST(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SEE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SET_CURRENT(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SET_ORDER(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SET_PRECISION(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_store(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_fetch(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_align(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_aligned(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_f_field_colon(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_float_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_floats(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SIGN(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SLITERAL(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_m_slash_rem(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SOURCE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_source_i_d(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SPACE(forth_context* ctx)
{
    FORTH_LOG(ctx, " ");
    return FORTH_SUCCESS;
}

static int forthi_word_SPACES(forth_context* ctx)
{
    if (forthi_pop(ctx) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int count = ctx->stack[ctx->stack_pointer].int_value;
    if (count > 0)
        FORTH_LOG(ctx, "%*s", (int)count, "");

    return FORTH_SUCCESS;
}

static int forthi_word_STATE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SUBSTITURE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_SWAP(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    if (forthi_push_int_number(ctx, n2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    return forthi_push_int_number(ctx, n1);
}

static int forthi_word_SYNONYM(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_s_backslash_quote(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_THEN(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        forth_pointer if_else_branch_pointer = ctx->stack[ctx->stack_pointer].pointer_value;

        if (forthi_check_valid_memory_range(ctx, if_else_branch_pointer, sizeof(forth_pointer)) == FORTH_FAILURE)
            return FORTH_FAILURE;

        *(forth_pointer*)&ctx->memory[if_else_branch_pointer] = ctx->memory_pointer;

        return FORTH_SUCCESS;
    }
    
    return FORTH_SUCCESS;
}

static int forthi_word_THROW(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int number = ctx->stack[ctx->stack_pointer].int_value;
    if (number != FORTH_FALSE)
        return FORTH_FAILURE;

    return FORTH_SUCCESS;
}

static int forthi_word_THRU(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_time_and_date(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TO(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TRAVERSE_WORDLIST(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TRUE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TUCK(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_TYPE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_u_dot(forth_context* ctx)
{
    if (forthi_pop(ctx, 1) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_uint u = ctx->stack[ctx->stack_pointer].uint_value;
    
    forth_int base;
    if (forthi_read_number_at(ctx, &base, ctx->base) == FORTH_FAILURE)
        return FORTH_FAILURE;

    if (base == 10)
        FORTH_LOG(ctx, "%" FORTHI_UINT_PRINT_CODE " ", u)
    else if (base == 8)
        FORTH_LOG(ctx, "%" FORTHI_OCTAL_PRINT_CODE " ", u)
    else if (base == 16)
        FORTH_LOG(ctx, "%" FORTHI_HEX_PRINT_CODE " ", u)
    else
        return FORTH_FAILURE;

    return FORTH_SUCCESS;
}

static int forthi_word_u_star(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_uint u1 = ctx->stack[ctx->stack_pointer].uint_value;
    forth_uint u2 = ctx->stack[ctx->stack_pointer + 1].uint_value;

    forth_double_length_uint ud = (forth_double_length_uint)u1 * (forth_double_length_uint)u2;
    return forthi_push_double_length_uint(ctx, ud);
}

static int forthi_word_u_slash_mod(forth_context* ctx)
{
    if (forthi_pop(ctx, 3) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_double_length_uint ud = forthi_to_double_length_uint(
        ctx->stack[ctx->stack_pointer].uint_value,
        ctx->stack[ctx->stack_pointer + 1].uint_value);
    forth_double_length_uint u1 = (forth_double_length_uint)ctx->stack[ctx->stack_pointer + 2].uint_value;

    if (forthi_push_uint_number(ctx, (forth_uint)(ud % u1)) == FORTH_FAILURE)
        return FORTH_FAILURE;

    return forthi_push_uint_number(ctx, (forth_uint)(ud / u1));
}

static int forthi_word_u_dot_r(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_uint u = ctx->stack[ctx->stack_pointer].uint_value;
    forth_int amount = ctx->stack[ctx->stack_pointer + 1].int_value;
    
    forth_int base;
    if (forthi_read_number_at(ctx, &base, ctx->base) == FORTH_FAILURE)
        return FORTH_FAILURE;

    if (base == 10)
        FORTH_LOG(ctx, "%*" FORTHI_UINT_PRINT_CODE, (int)amount, u)
    else if (base == 8)
        FORTH_LOG(ctx, "%*" FORTHI_OCTAL_PRINT_CODE, (int)amount, u)
    else if (base == 16)
        FORTH_LOG(ctx, "%*" FORTHI_HEX_PRINT_CODE, (int)amount, u)
    else
        return FORTH_FAILURE;

    return FORTH_SUCCESS;
}

static int forthi_word_u_less_than(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_uint u1 = ctx->stack[ctx->stack_pointer].uint_value;
    forth_uint u2 = ctx->stack[ctx->stack_pointer + 1].uint_value;

    return forthi_push_int_number(ctx, u1 < u2 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_u_greater_than(forth_context* ctx)
{
    if (forthi_pop(ctx, 2) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_uint u1 = ctx->stack[ctx->stack_pointer].uint_value;
    forth_uint u2 = ctx->stack[ctx->stack_pointer + 1].uint_value;

    return forthi_push_int_number(ctx, u1 > u2 ? FORTH_TRUE : FORTH_FALSE);
}

static int forthi_word_u_m_star(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_u_m_slash_mod(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_UNESCAPE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_UNLOOP(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_UNTIL(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        return forthi_write_pointer(ctx, ctx->stack[ctx->stack_pointer].pointer_value);
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        forth_int f = ctx->stack[ctx->stack_pointer].int_value;
        if (f == FORTH_FALSE)
        {
            if (forthi_check_valid_memory_range(ctx, ctx->program_pointer, sizeof(forth_pointer)) == FORTH_FAILURE)
                return FORTH_FAILURE;

            ctx->program_pointer = *(forth_pointer*)&ctx->memory[ctx->program_pointer];
            return FORTH_SUCCESS;
        }

        ctx->program_pointer += sizeof(forth_pointer);
        return FORTH_SUCCESS;
    }

    return FORTH_FAILURE;
}

static int forthi_word_UNUSED(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_UPDATE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_VALUE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_VARIABLE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_w_o(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WHILE(forth_context* ctx)
{
    if (ctx->state == FORTHI_STATE_INTERPRET)
    {
        FORTH_LOG(ctx, "Interpreting a compile-only word\n");
        return FORTH_FAILURE;
    }

    if (ctx->state == FORTHI_STATE_COMPILE)
    {
        if (forthi_push_pointer(ctx, ctx->memory_pointer) == FORTH_FAILURE)
            return FORTH_FAILURE;

        ctx->memory_pointer += sizeof(forth_pointer);

        return FORTH_SUCCESS;
    }

    if (ctx->state == FORTHI_STATE_EXECUTE)
    {
        if (forthi_pop(ctx, 1) == FORTH_FAILURE)
            return FORTH_FAILURE;

        forth_int f = ctx->stack[ctx->stack_pointer].int_value;
        if (f == FORTH_FALSE)
        {
            forth_pointer pointer;
            if (forthi_read_pointer(ctx, &pointer) == FORTH_FAILURE)
                return FORTH_FAILURE;

            ctx->program_pointer = pointer;
            return FORTH_SUCCESS;
        }

        ctx->program_pointer += sizeof(forth_pointer);
        return FORTH_SUCCESS;
    }

    return FORTH_FAILURE;
}

static int forthi_word_WITHIN(forth_context* ctx)
{
    if (forthi_pop(ctx, 3) == FORTH_FAILURE)
        return FORTH_FAILURE;

    forth_int n1 = ctx->stack[ctx->stack_pointer].int_value;
    forth_int n2 = ctx->stack[ctx->stack_pointer + 1].int_value;
    forth_int n3 = ctx->stack[ctx->stack_pointer + 2].int_value;

    forth_int result = FORTH_FALSE;

    if (n2 < n3 && n1 >= n2 && n1 < n3)
        result = FORTH_TRUE;

    return forthi_push_int_number(ctx, result);
}

static int forthi_word_WORD(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WORDLIST(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WORDS(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WRITE_FILE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_WRITE_LINE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_X_SIZE(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_X_WIDTH(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_store_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_store_plus_query(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_comma(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_size(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_width(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_c_fetch_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_char_plus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_char_minus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_emit(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_hold(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_key(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_key_query(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_or(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_x_string_minus(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_left_bracket(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_tick(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_char(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_compile(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_defined(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_else(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_if(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_then(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_bracket_undefined(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_backslash(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_right_bracket(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

static int forthi_word_brace_colon(forth_context* ctx)
{
    FORTH_LOG(ctx, "Unimplemented\n");
    return FORTH_FAILURE;
}

//---------------------------------------------------------------------------
// INIT AND SHUTDOWN
//---------------------------------------------------------------------------

static int forthi_defineStandardWords(forth_context* ctx)
{
#define FORTHI_DEFINE_STANDARD_WORD(_word_, _func_) \
    if (forth_add_c_word(ctx, _word_, _func_) == FORTH_FAILURE) \
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
    FORTHI_DEFINE_STANDARD_WORD("/LOOP", forthi_word_slash_loop);
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
    FORTHI_DEFINE_STANDARD_WORD("2+", forthi_word_two_plus);
    FORTHI_DEFINE_STANDARD_WORD("2-", forthi_word_two_minus);
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
    FORTHI_DEFINE_STANDARD_WORD("EMPTY", forthi_word_EMPTY);
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
    //FORTHI_DEFINE_STANDARD_WORD("FLUSH", forthi_word_FLUSH); // Undefined word, block related
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
    FORTHI_DEFINE_STANDARD_WORD("I'", forthi_word_i_tick);
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
    //FORTHI_DEFINE_STANDARD_WORD("LIST", forthi_word_LIST); // Undefined word, block related
    FORTHI_DEFINE_STANDARD_WORD("LITERAL", forthi_word_LITERAL);
    //FORTHI_DEFINE_STANDARD_WORD("LOAD", forthi_word_LOAD); // Undefined word, block related
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
    FORTHI_DEFINE_STANDARD_WORD("NOT", forthi_word_zero_equals); // For convenience
    FORTHI_DEFINE_STANDARD_WORD("NR>", forthi_word_n_r_from);
    FORTHI_DEFINE_STANDARD_WORD("NUMBER", forthi_word_NUMBER);
    FORTHI_DEFINE_STANDARD_WORD("OCTAL", forthi_word_OCTAL);
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
    FORTHI_DEFINE_STANDARD_WORD("U*", forthi_word_u_star);
    FORTHI_DEFINE_STANDARD_WORD("U/MOD", forthi_word_u_slash_mod);
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

    ctx->default_dict_pointer = ctx->dict_pointer;

    //forth_pointer reminder = ctx->memory_pointer % sizeof(uintptr_t);
    //if (reminder > 0)
    //    ctx->memory_pointer += (forth_pointer)sizeof(uintptr_t) - reminder;

    return FORTH_SUCCESS;
}

forth_context* forth_create_context(int memory_size, int stack_size, int return_stack_size, int dict_size)
{
    if (memory_size <= 0 && memory_size != FORTH_MEM_INFINITE)
        return NULL;

    if (stack_size <= 0 && stack_size != FORTH_MEM_INFINITE)
        return NULL;

    if (return_stack_size <= 0 && return_stack_size != FORTH_MEM_INFINITE)
        return NULL;

    if (dict_size <= 0 && dict_size != FORTH_MEM_INFINITE)
        return NULL;

    forth_context* ctx = (forth_context*)malloc(sizeof(forth_context));
    if (!ctx)
        return NULL;

    memset(ctx, 0, sizeof(forth_context));

    ctx->memory_auto_resize = memory_size == FORTH_MEM_INFINITE ? 1 : 0;
    ctx->memory_size = ctx->memory_auto_resize 
        ? (435 * (sizeof(forth_c_func) + 2) / FORTHI_MEM_ALLOC_CHUNK_SIZE * FORTHI_MEM_ALLOC_CHUNK_SIZE + 
            FORTHI_MEM_ALLOC_CHUNK_SIZE) 
        : memory_size;
    ctx->memory = (uint8_t*)malloc(ctx->memory_size);
    if (!ctx->memory)
    {
        forth_destroy_context(ctx);
        return NULL;
    }

    ctx->stack_auto_resize = stack_size == FORTH_MEM_INFINITE ? 1 : 0;
    ctx->stack_size = ctx->stack_auto_resize ? FORTHI_MEM_ALLOC_CHUNK_SIZE : stack_size;
    ctx->stack = (forth_cell*)malloc(sizeof(forth_cell) * ctx->stack_size);
    if (!ctx->stack)
    {
        forth_destroy_context(ctx);
        return NULL;
    }

    ctx->return_stack_auto_resize = return_stack_size == FORTH_MEM_INFINITE ? 1 : 0;
    ctx->return_stack_size = ctx->return_stack_auto_resize ? FORTHI_MEM_ALLOC_CHUNK_SIZE : return_stack_size;
    ctx->return_stack = (forth_cell*)malloc(sizeof(forth_cell) * ctx->return_stack_size);
    if (!ctx->return_stack)
    {
        forth_destroy_context(ctx);
        return NULL;
    }

    ctx->dict_auto_resize = dict_size == FORTH_MEM_INFINITE ? 1 : 0;
    ctx->dict_size = ctx->dict_auto_resize ? FORTHI_MEM_ALLOC_CHUNK_SIZE : dict_size;
    ctx->dict_name_lens = (uint8_t*)malloc(ctx->dict_size);
    if (!ctx->dict_name_lens)
    {
        forth_destroy_context(ctx);
        return NULL;
    }
    ctx->dict_names = (char*)malloc(FORTH_DICT_CHAR_COUNT * ctx->dict_size);
    if (!ctx->dict_names)
    {
        forth_destroy_context(ctx);
        return NULL;
    }
    ctx->dict_pointers = (forth_pointer*)malloc(sizeof(forth_pointer) * ctx->dict_size);
    if (!ctx->dict_pointers)
    {
        forth_destroy_context(ctx);
        return NULL;
    }

    ctx->base = ctx->memory_pointer;
    if (forthi_write_number(ctx, 10) == FORTH_FAILURE)
    {
        forth_destroy_context(ctx);
        return NULL;
    }

    if (forthi_defineStandardWords(ctx) == FORTH_FAILURE)
    {
        forth_destroy_context(ctx);
        return NULL;
    }

    return ctx;
}

void forth_destroy_context(forth_context* ctx)
{
    if (!ctx)
        return;

    if (ctx->memory)
        free(ctx->memory);

    if (ctx->stack)
        free(ctx->stack);

    if (ctx->return_stack)
        free(ctx->return_stack);

    if (ctx->dict_name_lens)
        free(ctx->dict_name_lens);

    if (ctx->dict_names)
        free(ctx->dict_names);

    if (ctx->dict_pointers)
        free(ctx->dict_pointers);

    free(ctx);
}

#endif

#if defined(__cplusplus)
};
#endif

#endif /* FORTH_H_INCLUDED */
