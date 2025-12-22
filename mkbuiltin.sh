#!/bin/bash

# Extracts builtins from source files and defines  
for sf in math.c io.c lisp.c string.c; do
    docstrs=$(cat $sf | grep ^Cell | awk -F '// ' {'print $2;'})
    names=$(cat $sf | grep ^Cell | awk -F '// ' {'print $2;'} | awk {'print $1;'} | sed 's/(//')
    echo "$names"
done


# "static Cell* _fn$t = &(Cell){BCTAG, &(Cell){BCTAG, &(Cell){BSTAG, $name, $size}, &(Cell){BBTAG, $func, NIL}}, _fn$l}"
#define BCTAG TAG_MAGIC | TAG_STATIC | TAG_TYPE_CONS
#define BSTAG TAG_MAGIC | TAG_STATIC | TAG_TYPE_CONS
#define BBTAG TAG_MAGIC | TAG_STATIC | TAG_TYPE_CONS

#define BUILTIN_ENT(name, func, docs) &(Cell){BCTAG, &(Cell){BCTAG, 
