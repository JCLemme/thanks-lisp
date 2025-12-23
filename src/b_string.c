#include "memory.h"
#include "frame.h"
#include "repl.h"
#include "builtins.h"

/* Lisp operations
 * Math related
 */

Cell* fn_symbol_name(Cell* args) // (symbol-name s) Returns a string containing the name of symbol s
{
    // We don't want the user fucking with the symbol table.
    return memory_single_copy(((Cell*)args->car)->car);
}

Cell* fn_read_from_string(Cell* args) // (read-from-string s) Reads and evaluates one s-expression from stream s
{
    return _parse_sexps(string_ptr(args->car)); 
}

Cell* fn_substr(Cell* args) // (substr s i l) Copies and returns a substring from string s, from index i and with length l
{
    Cell* str = memory_nth(args, 0)->car;
    Cell* cstart = memory_nth(args, 1)->car;
    Cell* clen = memory_nth(args, 2)->car;
    
    int start = cstart->num;
    int len = (IS_NIL(clen)) ? -1 : clen->num;

    if(start >= str->size || len > str->size || (start+len) > str->size) 
        return memory_alloc_exception(TAG_SPEC_EX_VALUE, memory_alloc_string("substr: invalid start/len", -1));

    return memory_alloc_string(string_ptr(str) + start, len);
}


