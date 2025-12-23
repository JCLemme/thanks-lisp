#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

#include "memory.h"
#include "frame.h"

#ifndef _REPL_H
#define _REPL_H

extern bool got_interrupt;

typedef int (*read_callback_t)(void*);
typedef int (*write_callback_t)(char*, int, void*);

extern Cell reader_macros;
Cell* _walk_sexps(char* buffer, int* index);
Cell* _recurse_sexps(char* buffer, int* index);
Cell* _parse_sexps(char* inbuf);

int _write_to_stdout(char* src, int len, void* state);
void _print_sexps(Cell* target);
void _print_sexps_to(char* dest, int size, Cell* target);
void _print_sexps_to_file(FILE* dest, Cell* target);

Cell* _lambda_list_define_all(Cell* env, Cell* list, Cell* args, int* expected);
bool _is_macro_expr(Cell* list);
Cell* _hardlinker(Cell* target, Cell* lambs);
Cell* _hardlink_dynamic(Cell* target);
Cell* _evaluate_sexp(Cell* target);
Cell* _macroexpand_one(Cell* expr);
Cell* _macroexpand(Cell* expr);

#endif
