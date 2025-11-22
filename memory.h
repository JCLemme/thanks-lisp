#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef _MEMORY_H
#define _MEMORY_H

#define TAG_MAGIC (0xEAULL << 56)
#define TAG_MAGIC_MASK (0xFFULL << 56)

#define TAG_TYPE_MASK       (0xFF)
#define OF_TYPE(tag, type) ((tag & ~TAG_TYPE_MASK) | type)
#define IS_TYPE(tag, type) ((tag & TAG_TYPE_MASK) == type)

#define TAG_TYPE_EMPTY      (0x00)
#define TAG_TYPE_CONS       (0x01)
#define TAG_TYPE_NUMBER     (0x02)
#define TAG_TYPE_LAMBDA     (0x03) 
#define TAG_TYPE_SYMBOL     (0x04)
#define TAG_TYPE_PSTRING    (0x05)
#define TAG_TYPE_STRING     (0x06)
#define TAG_TYPE_ARRAY      (0x07)
#define TAG_TYPE_BUILTIN    (0x08)

#define TAG_LAZY            (0x100)
#define TAG_MARKED          (0x200)


typedef struct Cell
{
    // Broken out bc we're here to learn
    uint64_t tag;
    
    union
    {
        void* car;
        uint64_t data;
        double num;
    };

    union 
    {
        void* cdr;
        uint64_t size;
    };

} Cell;

extern Cell* sym_top;

void memory_init(int cells);

int memory_get_used();

Cell* memory_alloc_cons(Cell* ar, Cell* dr);

void memory_build_number(Cell* found, double value);
Cell* memory_alloc_number(double value);

void memory_build_string(Cell* found, char* src);
Cell* memory_alloc_string(char* src);

void memory_build_symbol(Cell* found, char* src);
Cell* memory_alloc_symbol(char* src);

Cell* memory_alloc_lambda(Cell* args, Cell* body, bool lazy);
Cell* memory_alloc_builtin(Cell* (*primfunc)(Cell*), bool lazy);

int memory_mark(Cell* begin);
int memory_sweep();

#endif
