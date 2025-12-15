#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef _MEMORY_H
#define _MEMORY_H

#define TAG_MAGIC 0 //(0xEAULL << 56)
#define TAG_MAGIC_MASK (0xFFULL << 56)

#define IS_NIL(cell) (cell == NULL || cell == &nil_cell || (cell->car == NULL && cell->cdr == NULL)) //|| (cell->car == &nil_cell && cell->cdr == &nil_cell))
#define NIL (&nil_cell)
#define IS_T(cell) (!IS_NIL(cell))
#define T (&true_cell)

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
#define TAG_TYPE_EXCEPTION  (0x09)
#define TAG_TYPE_HARDLINK   (0x0A)
#define TAG_TYPE_STREAM     (0x0B)

#define TAG_SPEC_MASK       (0xFF << 8)
#define OF_SPEC(tag, spec)  ((tag & ~TAG_SPEC_MASK) | spec)
#define IS_SPEC(tag, spec)  (((tag & TAG_SPEC_MASK) & spec) == spec)

#define TAG_SPEC_FUNLAZY    (0x01 << 8)
#define TAG_SPEC_FUNMACRO   (0x02 << 8)

#define TAG_SPEC_EX_LABEL   (0x01 << 8) // tryin some shit
#define TAG_SPEC_EX_EXIT    (0x02 << 8)
#define TAG_SPEC_EX_TYPE    (0x03 << 8) // wrong kind of object
#define TAG_SPEC_EX_VALUE   (0x04 << 8) // ???
#define TAG_SPEC_EX_DATA    (0x05 << 8) // wasn't found, etc.
#define TAG_SPEC_EX_IO      (0x06 << 8) // error in io op

#define EXCEPTION_OF(spec, str) (Cell){TAG_MAGIC | TAG_TYPE_EXCEPTION | TAG_STATIC | spec, &(Cell){TAG_MAGIC | TAG_TYPE_STRING | TAG_STATIC, str, NIL}, NIL}

// TODO: the whole stream interface is sus
#define TAG_SPEC_STM_FILE   (0x01 << 8) // file IO

#define TAG_MEMORY_MASK     (0xFF << 16)
#define TAG_MARKED          (0x01 << 16)
#define TAG_STATIC          (0x02 << 16)

// Unsure if exceptions should be their own type.

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
extern Cell nil_cell;
extern Cell true_cell;

extern Cell stex_interrupted;

void memory_init(int cells);

int memory_get_used();

Cell* memory_nth(Cell* begin, int place);
int memory_length(Cell* begin);

Cell* memory_single_copy(Cell* src);
Cell* memory_shallow_copy(Cell* begin);
Cell* memory_deep_copy(Cell* begin);
Cell* memory_extend(Cell* last);

Cell* memory_alloc_cons(Cell* ar, Cell* dr);

void memory_build_number(Cell* found, double value);
Cell* memory_alloc_number(double value);

void memory_build_string(Cell* found, char* src, int len);
Cell* memory_alloc_string(char* src, int len);

void memory_build_symbol(Cell* found, char* src);
Cell* memory_alloc_symbol(char* src);

void memory_build_lambda(Cell* found, Cell* args, Cell* body, int tags);
Cell* memory_alloc_lambda(Cell* args, Cell* body, int tags);
void memory_build_builtin(Cell* found, Cell* (*primfunc)(Cell*), int tags);
Cell* memory_alloc_builtin(Cell* (*primfunc)(Cell*), int tags);

void memory_build_exception(Cell* found, int kind, Cell* data);
Cell* memory_alloc_exception(int kind, Cell* data);

void memory_build_hardlink(Cell* found, Cell* data);
Cell* memory_alloc_hardlink(Cell* data);

void memory_build_stream(Cell* found, void* repr);
Cell* memory_alloc_stream(void* repr);

// ---

char* symbol_string_ptr(Cell* sym);
char* string_ptr(Cell* str);
bool symbol_is_keyword(Cell* sym);
bool symbol_matches(Cell* sym, char* name);

// ---

void memory_free(Cell* target);

int memory_mark(Cell* begin);
int memory_sweep();

void memory_add_root(Cell* nroot);
void memory_del_root(Cell* nroot);
int memory_gc();
int memory_gc_thresh(double pct);

#endif
