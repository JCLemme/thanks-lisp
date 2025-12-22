#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "memory.h"
#include "frame.h"
#include "repl.h"

#ifndef _BUILTINS_H
#define _BUILTINS_H

// Thanks lisp - builtin functions

void _evaldef(char* code);
void _define_lisp();
void _define_c();
void builtins_init();

#endif
