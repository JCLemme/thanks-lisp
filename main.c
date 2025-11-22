#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "memory.h"
#include "util.h"

Cell* _evaluate_sexp(Cell* target);

// Holds mappings that we kept with define and w/e.
Cell* frame_top = NULL;

// TODO: the frame list: a list of frames to check. Functions add/remove lists to the top of the frame list as scope changes, and 
// new definitions go in the global environment (or a namespace)

// The actual work involved to make a definition.
Cell* frame_define(Cell* sym, Cell* val)
{
    // A definition is a list: first item is a symbol, second is *something*.
    Cell* new_def = memory_alloc_cons(sym, val);
    Cell* new_entry = memory_alloc_cons(new_def, frame_top);
    frame_top = new_entry;
    return frame_top;
}

Cell* frame_define_name(char* name, Cell* val)
{
    // A definition is a list: first item is a symbol, second is *something*.
    Cell* sym = memory_alloc_symbol(name);
    return frame_define(sym, val);
}

// Find the most recent definition for a symbol.
Cell* frame_find(Cell* sym)
{
    // Walk the list.
    Cell* frame_current = frame_top;

    while(frame_current != NULL)
    {
        // Somewhat convoluted
        Cell* found_def = frame_current->car;
        Cell* found_sym = found_def->car;
        if(found_sym->car == sym->car)
        {
            return found_def->cdr;
        }
        frame_current = frame_current->cdr;
    }

    // Eep
    printf("main: frame: couldn't find symbol\n");
    abort();

    return NULL;
}

Cell* frame_find_name(char* name)
{
    // Build a symbol locally.
    Cell sym;
    memory_build_symbol(&sym, name);
    return frame_find(&sym);
}

// Remove the last *count* definitions from the current frame.
void frame_pop(int count)
{
    // Not error checking lol.
    Cell* current = frame_top;
    for(int i = 0; i < count; i++)
    {
        current = current->cdr;
    }
    frame_top = current;
}

Cell* fn_def(Cell* args)
{
    Cell* evalarg = _evaluate_sexp(((Cell*)(args->cdr))->car);
    frame_define(args->car, evalarg);
    return args->car;
}

// Real simple primitives.
Cell* fn_car(Cell* args)
{
    Cell* first = args->car;
    if(!IS_TYPE(first->tag, TAG_TYPE_CONS))
    {
        printf("fn: car: argument not of type 'list'\n");
        abort();
    }

    return first->car;
}

Cell* fn_cdr(Cell* args)
{
    Cell* first = args->car;
    if(!IS_TYPE(first->tag, TAG_TYPE_CONS))
    {
        printf("fn: cdr: argument not of type 'list'\n");
        abort();
    }

    return first->cdr;
}

Cell* fn_list(Cell* args)
{
    return args;
}

Cell* fn_quote(Cell* args)
{
    return args->car;
}

Cell* fn_add(Cell* args)
{
    double acc = 0;
    while(args != NULL)
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            printf("fn: add: not a number\n");
            abort();
        }
        acc += arg->num;
        args = args->cdr;
    }

    return memory_alloc_number(acc);
}

Cell* fn_sub(Cell* args)
{
    double acc = 0;
    bool first = true;
    while(args != NULL)
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            printf("fn: add: not a number\n");
            abort();
        }
        if(first)
        {
            acc = arg->num;
            first = false;
        }
        else
        {
            acc -= arg->num;
        }

        args = args->cdr;
    }

    return memory_alloc_number(acc);
}

Cell* fn_mul(Cell* args)
{
    double acc = 1;
    while(args != NULL)
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            printf("fn: add: not a number\n");
            abort();
        }
        acc *= arg->num;
        args = args->cdr;
    }

    return memory_alloc_number(acc);
}

Cell* fn_div(Cell* args)
{
    double acc = 0;
    bool first = true;
    while(args != NULL)
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            printf("fn: add: not a number\n");
            abort();
        }
        if(first)
        {
            acc = arg->num;
            first = false;
        }
        else
        {
            acc /= arg->num;
        }

        args = args->cdr;
    }

    return memory_alloc_number(acc);
}

Cell* fn_lambda(Cell* args)
{
    // Args: a list of arguments, and a list of operations
    Cell* arglist = args->car;
    Cell* oplist = ((Cell*)(args->cdr))->car;
    // TODO: make sure the arglist is only symbols
    return memory_alloc_lambda(arglist, oplist, false);
}

// ----------------------------------------------

// Reader operations.
char replin[2048];
char scratch[256];

Cell* _recurse_sexps(char* buffer, int* index)
{
    // TODO: the readlist
    Cell* this_object = NULL;
    char c = buffer[(*index)];
    int scridx = 0;

    if(c == '\0')
    {
        return NULL;
    }
    else if(c == '(')
    {
        Cell* next_object = NULL;
        (*index)++;
        while(buffer[(*index)] == ' ') {(*index)++;}
        c = buffer[(*index)];

        while(c != ')')
        {
            if(next_object == NULL)
            {
                next_object = memory_alloc_cons(NULL, NULL);
                this_object = next_object;
            }
            else
            {
                next_object->cdr = memory_alloc_cons(NULL, NULL);
                next_object = next_object->cdr;
            }

            next_object->car = _recurse_sexps(buffer, index);

            while(buffer[(*index)] == ' ') {(*index)++;}
            c = buffer[(*index)];
        }

        // Read off the end
        (*index)++;
    }
    else if(c == '-' || c == '+' || isdigit(c))
    {
        // The cursed logic in this function:
        // * symbols can *also* start with a plus/minus (but not numbers)
        // * numbers don't always start with a sign
        // * goto considered fun and interesting
        int backup_index = *index;
        char backup_c = c;
        char first_dig = (isdigit(c)) ? c : 0;

        do
        {
            scratch[scridx++] = c;
            (*index)++;
            c = buffer[*index];
            if(first_dig == 0) first_dig = c;
        }
        while(c == '.' || isdigit(c));
        scratch[scridx] = '\0';

        if(!isdigit(first_dig) && first_dig != '\0')
        {
            *index = backup_index;
            c = backup_c;
            scridx = 0;
            printf("%c\n", scratch[1]);
            goto was_symbol_actually;
        }

        this_object = memory_alloc_number(atof(scratch));
    }
    else if(c == '"')
    {
        // TODO: ew
        (*index)++;
        c = buffer[*index];
        bool escaped = false;
        bool looking = true;

        do
        {
            if(escaped)
            {
                escaped = false;
                if(c == 'n')
                    scratch[scridx++] = '\n';
                else if(c == '0')
                    scratch[scridx++] = '\0';
                else if(c == 'r')
                    scratch[scridx++] = '\r';
                else if(c == 't')
                    scratch[scridx++] = '\t';
                else if(c == '"')
                    scratch[scridx++] = '"';
                else if(c == '\\')
                    scratch[scridx++] = '\\';
                else
                    scratch[scridx++] = '!';
            }
            else
            {
                if(c == '\\')
                    escaped = true;
                else if(c == '"')
                    looking = false;
                else
                    scratch[scridx++] = c;
            }

            (*index)++;
            c = buffer[*index];
        }
        while(looking && c != '\0');
        scratch[scridx] = '\0';

        this_object = memory_alloc_string(scratch);
    }
    else if(c == '\'')
    {
        (*index)++;
        this_object = memory_alloc_cons(memory_alloc_symbol("quote"), memory_alloc_cons(_recurse_sexps(buffer, index), NULL));
    }
    else
    {
was_symbol_actually:
        do
        {
            scratch[scridx++] = c;
            (*index)++;
            c = buffer[*index];
        }
        while(c != ' ' && c != ')' && c != '\0');
        scratch[scridx] = '\0';

        this_object = memory_alloc_symbol(scratch);
    }

    return this_object;
}

Cell* fn_read(Cell* args)
{
    int idx = 0;
    return _recurse_sexps(replin, &idx);
}

void _print_sexps(Cell* target)
{
    if(IS_TYPE(target->tag, TAG_TYPE_CONS))
    {
        printf("(");
        while(target != NULL)
        {
            _print_sexps(target->car);
            printf(" ");
            target = target->cdr;
        }
        printf("\b)");
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_NUMBER))
    {
        print_double(target->num);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_LAMBDA))
    {
        printf("<lambda at %08x>", target->car);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_SYMBOL))
    {
        Cell* symstr = target->car;
        // TODO: auto type crushing for strings
        if(IS_TYPE(symstr->tag, TAG_TYPE_PSTRING))
            printf("%s", (char*)&(symstr->car));
        else
            printf("%s", (char*)symstr->car);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_PSTRING))
    {
        printf("\"%s\"", (char*)&(target->car));
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_STRING))
    {
        printf("\"%s\"", (char*)target->car);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_ARRAY))
    {
        printf("<array at %08x>", target->car);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_BUILTIN))
    {
        printf("<builtin at %08x>", target->car);
    }
    else
    {
        printf("<unknown type %d at %08x>", target->tag, target);
    }
}

Cell* fn_print(Cell* args)
{
    Cell* arg = args->car;
    _print_sexps(arg);
    return arg;
}

Cell* _evaluate_sexp(Cell* target)
{
    if(!IS_TYPE(target->tag, TAG_TYPE_CONS))
    {
        if(IS_TYPE(target->tag, TAG_TYPE_SYMBOL))
        {
            // Look it up first
            return frame_find(target);
        }
        else
        {
            // It's an atom - return it
            return target;
        }
    }
    else
    {
        // It's a list - evaluate
        Cell* func = target->car;
        Cell* args = target->cdr;
        int argc = 0;

        func = _evaluate_sexp(func);

        if((func->tag & TAG_LAZY) == 0)
        {
            // Normal function - evaluate arguments first
            Cell* proc_args = NULL;
            Cell* curr_arg = NULL;
 
            while(args != NULL)
            {
                if(curr_arg == NULL)
                {
                    curr_arg = memory_alloc_cons(NULL, NULL);
                    proc_args = curr_arg;
                }
                else
                {
                    curr_arg->cdr = memory_alloc_cons(NULL, NULL);
                    curr_arg = curr_arg->cdr;
                }

                curr_arg->car = _evaluate_sexp(args->car);
                args = args->cdr;
            }

            args = proc_args;
            argc++;
        }
        else
        {
            Cell* backup_args = args;
            while(backup_args != NULL) 
            {
                backup_args = backup_args->cdr;
                argc++;
            }
        }

        if(IS_TYPE(func->tag, TAG_TYPE_BUILTIN))
        {
            return ((Cell* (*)(Cell*))func->car)(args);
        }
        else if(IS_TYPE(func->tag, TAG_TYPE_LAMBDA))
        {
            Cell* sig_list = func->car;
            Cell* body = func->cdr;
            Cell* todef = args;
            int expected = 0;

            while(sig_list != NULL)
            {
                frame_define(sig_list->car, todef->car);
                expected++;
                sig_list = sig_list->cdr;
                todef = todef->cdr;
            }

            Cell* result = _evaluate_sexp(body);
            frame_pop(expected);
            return result;
        }
        else
        {
            printf("fn: eval: not a function\n");
            abort();
        }
    }
}

Cell* fn_eval(Cell* args)
{
    return _evaluate_sexp(args->car);
}

int main(int argc, char** argv) 
{
    // Start up core.
    memory_init(4096);

    // Start up the frame machinery.
    frame_define_name("def", memory_alloc_builtin(fn_def, true));
    frame_define_name("car", memory_alloc_builtin(fn_car, false)); 
    frame_define_name("cdr", memory_alloc_builtin(fn_cdr, false)); 
    frame_define_name("list", memory_alloc_builtin(fn_list, false));
    frame_define_name("quote", memory_alloc_builtin(fn_quote, true));
    frame_define_name("+", memory_alloc_builtin(fn_add, false));
    frame_define_name("-", memory_alloc_builtin(fn_sub, false));
    frame_define_name("*", memory_alloc_builtin(fn_mul, false));
    frame_define_name("/", memory_alloc_builtin(fn_div, false));
    frame_define_name("lambda", memory_alloc_builtin(fn_lambda, true));
    frame_define_name("read", memory_alloc_builtin(fn_read, false));
    frame_define_name("print", memory_alloc_builtin(fn_print, false));
    frame_define_name("eval", memory_alloc_builtin(fn_eval, false));

    printf("Thanks lisp v1\n");
    printf("Now speaking to the lisp listener.\n");
    
    bool running = true;
    Cell* todo;

    while(running)
    {
        int idx = 0;
        int next = 0;

        printf("* ");

        while(idx < sizeof(replin))
        {
            next = getchar();
            if(next == EOF)
            {
                running = false;
                break;
            }
            else if(next == '\n')
            {
                break;
            }
            else
            {
                replin[idx++] = next; 
            }
        }
        replin[idx] = '\0';
        
        if(replin[0] != '\0')
        {
            todo = fn_read(NULL);
            _print_sexps(_evaluate_sexp(todo));
            printf("\n");
        }

        memory_mark(frame_top);
        memory_mark(sym_top);
        memory_sweep();
    }

    printf("main: used %d/%d cells\n", memory_get_used(), 4096);

    // TODO: exceptions/debugger?
    //
    return 0;
}
