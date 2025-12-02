#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "memory.h"

#ifndef _FRAME_H
#define _FRAME_H

extern Cell env_top;
extern Cell env_root;

extern Cell temp_root;

Cell* frame_push_in(Cell* pkg, Cell* def);
Cell* frame_push_def(Cell* name, Cell* value);
Cell* frame_push_defn(char* name, Cell* value);
Cell* frame_push_def_in(Cell* pkg, Cell* name, Cell* value);
Cell* frame_push_defn_in(Cell* pkg, char* name, Cell* value);

Cell* frame_pop_in(Cell* pkg, int count);
Cell* frame_pop(int count);

Cell* frame_make_package_in(Cell* pkg, Cell* name);
Cell* frame_make_package(Cell* name);
Cell* frame_make_packagen(char* name);
Cell* frame_make_packagen_in(Cell* pkg, char* name);

Cell* frame_free_package(Cell* pkg);

Cell* frame_find_def_in(Cell* pkg, Cell* name);
Cell* frame_find_defn_in(Cell* pkg, char* name);
Cell* frame_find_def_from(Cell* pkg, Cell* name);
Cell* frame_find_defn_from(Cell* pkg, char* name);

Cell* frame_find_def(Cell* name);
Cell* frame_find_defn(char* name);
Cell* frame_find_package(Cell* name);
Cell* frame_find_packagen(char* name);

Cell* frame_set_def(Cell* name, Cell* nvalue);
Cell* frame_set_defn(char* name, Cell* nvalue);

Cell* frame_init();

#endif


