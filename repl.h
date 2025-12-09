#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

#include "memory.h"
#include "frame.h"
#include "util.h"

#ifndef _REPL_H
#define _REPL_H

Cell* _recurse_sexps(char* buffer, int* index);
Cell* _parse_sexps(char* inbuf);

void _print_sexps(Cell* target);

int _lambda_list_define_all(Cell* env, Cell* list, Cell* args);
bool _is_macro_expr(Cell* list);
Cell* _hardlinker(Cell* target);
Cell* _evaluate_sexp(Cell* target);
Cell* _macroexpand_one(Cell* expr);
Cell* _macroexpand(Cell* expr);

#endif
