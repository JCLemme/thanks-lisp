#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "memory.h"
#include "frame.h"
#include "util.h"
#include "repl.h"

#ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif

#if defined(USE_READLINE)
#include <readline/readline.h>
#include <readline/history.h>
#elif defined(USE_SHIM)
#include "shim.h"
#endif

#if defined(USE_READLINE) || defined(USE_SHIM)
char* last_line = NULL;

char* thanks_gets(char* prompt)
{
    if(last_line) { free(last_line); }
    last_line = readline(prompt);

    if(last_line && *last_line) { add_history(last_line); }

    return last_line;
}

#define HIST_FILE "./.thanks_history"

void history_saver()
{
    write_history(HIST_FILE);
    exit(0);
}

#else

char replin[2048];

char* thanks_gets(char* prompt)
{
    printf("%s", prompt);
    fflush(stdout);

    int idx = 0;
    int next = 0;

    while(idx < sizeof(replin))
    {
        next = getchar();
        if(next == EOF)
        {
            return NULL;
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
        
    return replin;
}

#endif

#define CELL_AREA 409600

static char errorzone[256];

Cell* fn_def(Cell* args)
{
    Cell* evalarg = _evaluate_sexp(((Cell*)(args->cdr))->car);
    frame_push_def_in(&env_root, args->car, evalarg);
    return args->car;
}

Cell* fn_ldef(Cell* args)
{
    Cell* evalarg = _evaluate_sexp(((Cell*)(args->cdr))->car);
    frame_push_def(args->car, evalarg);
    return args->car;
}


// Real simple primitives.
Cell* fn_car(Cell* args)
{
    Cell* first = args->car;
    if(!IS_TYPE(first->tag, TAG_TYPE_CONS))
    {
        _print_sexps(first);
        return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("car: argument not of type 'list'"));
    }

    return first->car;
}

Cell* fn_cdr(Cell* args)
{
    // TODO: we shouldn't have to special case this...
    if(args == NULL || args == NIL) { return NIL;}
    if(args->car == NULL || args->car == NIL) { return NIL;}

    Cell* first = args->car;
    if(!IS_TYPE(first->tag, TAG_TYPE_CONS))
    {
        return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("car: argument not of type 'list'"));
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

Cell* fn_cons(Cell* args)
{
    Cell* new_car = memory_nth(args, 0)->car;
    Cell* new_cdr = memory_nth(args, 1)->car;
    return memory_alloc_cons(new_car, new_cdr);
}

Cell* fn_append(Cell* args)
{
    // Trying something different.
    // (Bad)
    int len = memory_length(args);

    if(len == 0) { return NIL; }
    if(len == 1) { return args->car; }
    Cell* built = NULL;
    Cell* save = NULL;

    for(int i = 0; i < len - 1; i++)
    {
        if(args->car != NIL) 
        {
            Cell* copy = memory_shallow_copy(args->car);

            if(built == NULL) 
            { 
                built = copy; 
                save = copy;
            }
            else 
            {
                built->cdr = copy; 
            }

            int clen = memory_length(save);
            built = memory_nth(save, clen - 1); // ignore -> // might break - check shallow copy algo
        }

        args = args->cdr;
    }

    built->cdr = args->car;

    return save;
}

Cell* fn_null(Cell* args)
{
    // TODO: nearly time to flip case
    if(args == NULL || args == NIL) { return T; }
    return (IS_NIL(((Cell*)args->car))) ? T : NIL;
}

Cell* fn_add(Cell* args)
{
    double acc = 0;
    while(!IS_NIL(args))
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("add: not a number"));
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
    while(!IS_NIL(args))
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("sub: not a number"));
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
    while(!IS_NIL(args))
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("mul: not a number"));
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
    while(!IS_NIL(args))
    {
        Cell* arg = args->car;
        if(!IS_TYPE(arg->tag, TAG_TYPE_NUMBER))
        {
            return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("div: not a number"));
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

Cell* fn_mod(Cell* args)
{
    Cell* num = memory_nth(args, 0)->car;
    Cell* den = memory_nth(args, 1)->car;

    return memory_alloc_number((int)num->num % (int)den->num);
}

Cell* fn_lambda(Cell* args)
{
    // Args: a list of arguments, and a list of operations
    Cell* arglist = args->car;
    Cell* oplist = ((Cell*)(args->cdr))->car;
    // TODO: make sure the arglist is only symbols
    Cell* result = memory_alloc_lambda(arglist, oplist, 0);            

    // Closure conversion: capture anything from the lexical environment that we need
    result->cdr = _hardlinker(result->cdr);

    return result;
}

Cell* fn_macro(Cell* args)
{
    // Args: a list of arguments, and a list of operations
    Cell* arglist = args->car;
    Cell* oplist = ((Cell*)(args->cdr))->car;
    // TODO: make sure the arglist is only symbols
    return memory_alloc_lambda(arglist, oplist, TAG_SPEC_FUNLAZY | TAG_SPEC_FUNMACRO); // TODO: use case for splitting? or at least not implying lazy
}

// ----------------------------------------------

// Reader operations.

// TODO: SCREAMING
#if defined(USE_READLINE) || defined(USE_SHIM)
char replin[] = "(print \"don't use me\")";
#endif

Cell* fn_read(Cell* args)
{
    // TODO: merge with above?
    int idx = 0;
    return _recurse_sexps(replin, &idx);
}

Cell* fn_print(Cell* args)
{
    Cell* arg = args->car;
    _print_sexps(arg);
    printf("\n");
    return arg;
}


Cell* fn_eval(Cell* args)
{
    return _evaluate_sexp(args->car);
}

Cell* fn_eq(Cell* args)
{
    // Semantics - in CL, "eq" is a pointer comparison - is this the same object?
    Cell* a = memory_nth(args, 0);
    Cell* b = memory_nth(args, 1);
    return (a->car == b->car) ? T : NIL;
}

Cell* fn_eql(Cell* args)
{
    // However, "eql" is a value comparison - do these objects mean the same thing?
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->data == b->data) ? T : NIL;
}

// TODO: the below functions compared against "car", which somehow worked on macos but failed in emscripten. why?
Cell* fn_gt(Cell* args)
{
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num > b->num) ? T : NIL;
}

Cell* fn_gteq(Cell* args)
{
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num >= b->num) ? T : NIL;
}

Cell* fn_lt(Cell* args)
{
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num < b->num) ? T : NIL;
}

Cell* fn_lteq(Cell* args)
{
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num <= b->num) ? T : NIL;
}

Cell* fn_numeq(Cell* args)
{
    // These are here for compat with CL.
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num == b->num) ? T : NIL;
}

Cell* fn_numneq(Cell* args)
{
    // I mean I guess they could point to the same builtin but 
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->num != b->num) ? T : NIL;
}

Cell* fn_cond(Cell* args)
{
    while(!IS_NIL(args))
    {
        Cell* test = memory_nth(args->car, 0)->car;
        Cell* body = memory_nth(args->car, 1)->car;

        Cell* result = _evaluate_sexp(test);
        if(IS_TYPE(result->tag, TAG_TYPE_EXCEPTION)) { return result; }

        if(!IS_NIL(result)) 
        { 
            Cell* returned = _evaluate_sexp(body);
            return returned;
        }

        args = args->cdr;
    }

    return NIL;
}

Cell* fn_go(Cell* args)
{
    Cell* target = args->car;
    if(!IS_TYPE(target->tag, TAG_TYPE_SYMBOL)) 
    {
        return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("go: label wasn't a symbol"));
    }
    else
    {
        return memory_alloc_exception(TAG_SPEC_EX_LABEL, args->car);
    }
}

Cell* fn_tagbody(Cell* args)
{
    Cell* hunting_for = NULL;
    Cell* current_step = args;
    
    // TODO: needs a cache so bad
    
    while(!IS_NIL(current_step))
    {
        Cell* todo = current_step->car;

        if(hunting_for == NULL)
        {
            // Run statements.
            if(IS_TYPE(todo->tag, TAG_TYPE_SYMBOL))
            {
                // forget it jake it's tagbody
                current_step = current_step->cdr;
            }
            else
            {
                Cell* result = _evaluate_sexp(current_step->car);
                current_step = current_step->cdr;

                // An exception should be thrown, *unless* it's for us
                if(IS_TYPE(result->tag, TAG_TYPE_EXCEPTION)) 
                { 
                    if(IS_SPEC(result->tag, TAG_SPEC_EX_LABEL))
                    {
                        hunting_for = result->car;
                        current_step = args;
                    }
                    else
                    {
                        return result; 
                    }
                }
            }
        }
        else
        {
            if(hunting_for->car == todo->car)
            {
                hunting_for = NULL;
            }

            current_step = current_step->cdr;
        }
        
    }

    if(hunting_for != NULL)
    {
        return memory_alloc_exception(TAG_SPEC_EX_DATA, memory_alloc_string("tagbody: label not found"));
    }
    else
    {
        return NIL;
    }
}

Cell* fn_setq(Cell* args)
{
    Cell* sym = memory_nth(args, 0)->car;
    Cell* newval = memory_nth(args, 1)->car;

    newval = _evaluate_sexp(newval);
    if(IS_TYPE(newval->tag, TAG_TYPE_EXCEPTION)) { return newval; }
    Cell* result = NULL;

    if(IS_TYPE(sym->tag, TAG_TYPE_SYMBOL))
    {
        result = frame_set_def(sym, newval);
    }
    else if(IS_TYPE(sym->tag, TAG_TYPE_HARDLINK))
    {
        Cell* linked_def = sym->car;
        linked_def->cdr = newval;
        result = NIL;
    }

    if(IS_TYPE(result->tag, TAG_TYPE_EXCEPTION)) 
    {
        return result;
    }
    else
    {
        return newval;
    }
}

Cell* fn_let(Cell* args)
{
    Cell* variables = memory_nth(args, 0)->car;
    Cell* form = memory_nth(args, 1)->car;
    int expected = 0;

    while(!IS_NIL(variables))
    {
        Cell* varlist = variables->car;
        Cell* sym = memory_nth(varlist, 0)->car;
        Cell* val = memory_nth(varlist, 1)->car;

        val = _evaluate_sexp(val);
        if(IS_TYPE(val->tag, TAG_TYPE_EXCEPTION))
        {
            frame_pop_in(&env_top, expected);
            return val;
        }

        frame_push_def_in(&env_top, sym, val);
        expected++;
        variables = variables->cdr;
    }

    Cell* result = _evaluate_sexp(form);

    frame_pop_in(&env_top, expected);
    return result;
}

Cell* fn_nlet(Cell* args)
{
    Cell* variables = memory_nth(args, 0)->car;
    Cell* form = memory_nth(args, 1)->car;
    int expected = 0;

    while(!IS_NIL(variables))
    {
        Cell* varlist = variables->car;
        Cell* sym = memory_nth(varlist, 0)->car;
        Cell* val = memory_nth(varlist, 1)->car;

        Cell* bound = frame_push_def_in(&env_top, sym, sym)->car;
        expected++;

        val = _evaluate_sexp(val);
        if(IS_TYPE(val->tag, TAG_TYPE_EXCEPTION))
        {
            frame_pop_in(&env_top, expected);
            return val;
        }

        bound->cdr = val;
        variables = variables->cdr;
    }

    Cell* result = _evaluate_sexp(form);

    frame_pop_in(&env_top, expected);
    return result;
}

Cell* fn_macroexpand(Cell* args)
{
    return _macroexpand(args->car);
}

Cell* fn_garbage(Cell* args)
{
    int used = memory_gc();
    return memory_alloc_number(used);
}

Cell* fn_pyprint(Cell* args)
{
    while(!IS_NIL(args))
    {
        Cell* next = args->car;

        if(IS_TYPE(next->tag, TAG_TYPE_PSTRING))
        {
            printf("%s", (char*)&(next->car));
        }
        else if(IS_TYPE(next->tag, TAG_TYPE_STRING))
        {
            printf("%s", (char*)next->car);
        }
        else
        {
            _print_sexps(next);
        }

        args = args->cdr;
    }

    printf("\n");
    return NIL;
}

Cell* fn_progn(Cell* args)
{
    // Our (eval) executes these statements in order anyway - just return the last argument.
    if(args == NULL || args == NIL) { return NIL; }
    return memory_nth(args, memory_length(args) - 1)->car;
}

Cell* fn_and(Cell* args)
{
    Cell* last = T;

    while(!IS_NIL(args))
    {
        if(args->car == NIL) { return NIL; }
        last = args->car;
        args = args->cdr;
    }

    return last;
}

Cell* fn_or(Cell* args)
{
    while(!IS_NIL(args))
    {
        if(args->car != NIL) { return args->car; }
        args = args->cdr;
    }

    return NIL;
}

Cell* fn_when(Cell* args)
{
    // This can be a macro once I implement &rest in lambda-lists
    Cell* condition = args->car;
    Cell* body_list = args->cdr;
    Cell* last = NIL;

    Cell* result = _evaluate_sexp(condition);
    if(IS_NIL(result)) { return NIL; }

    Cell* todo = body_list;
    while(!IS_NIL(todo))
    {
        last = _evaluate_sexp(todo->car);
        if(IS_TYPE(last->tag, TAG_TYPE_EXCEPTION)) { return last; }
        todo = todo->cdr;
    }

    return last;
}

Cell* fn_open(Cell* args)
{
    Cell* filename = memory_nth(args, 0)->car;
    Cell* mode = memory_nth(args, 1)->car;
    
    // pain
    FILE* file = fopen(string_ptr(filename), symbol_string_ptr(mode)+1);
    if(file == NULL)
        return memory_alloc_exception(TAG_SPEC_EX_IO, memory_alloc_string("open: error opening file"));

    return memory_alloc_stream((void*)file);
}

Cell* fn_close(Cell* args)
{
    Cell* stream = args->car;
    fclose(stream->car);
    stream->car = NIL;
    return NIL;
}

Cell* fn_stderr(Cell* args)
{
    return memory_alloc_stream((void*)stderr);
}

Cell* fn_print_to(Cell* args)
{
    Cell* stream = memory_nth(args, 0)->car;
    Cell* target = memory_nth(args, 1)->car;

    _print_sexps_to_file((FILE*)stream->car, target);
    fprintf((FILE*)stream->car, "\n");

    return target; 
}

Cell* fn_pyprint_to(Cell* args)
{
    Cell* stream = args->car;
    args = args->cdr;
    FILE* file = (FILE*)stream->car;

    while(!IS_NIL(args))
    {
        Cell* next = args->car;

        if(IS_TYPE(next->tag, TAG_TYPE_PSTRING))
        {
            fprintf(file, "%s", (char*)&(next->car));
        }
        else if(IS_TYPE(next->tag, TAG_TYPE_STRING))
        {
            fprintf(file, "%s", (char*)next->car);
        }
        else
        {
            _print_sexps_to_file(file, next);
        }

        args = args->cdr;
    }

    fprintf(file, "\n");
    return NIL;
}

Cell* fn_env_root(Cell* args)
{
    return &env_root;
}

Cell* fn_apply(Cell* args)
{
    Cell* func = memory_nth(args, 0)->car;
    Cell* data = memory_nth(args, 1)->car;
    Cell* results = memory_alloc_cons(NIL, NIL);

    Cell* call = memory_alloc_cons(func, memory_alloc_cons(NIL, NIL));
    
    while(!IS_NIL(data))
    {
        Cell* tmparg = data->car;
        ((Cell*)call->cdr)->car = tmparg;

        Cell* tmpres = _evaluate_sexp(call);
        if(IS_TYPE(tmpres->tag, TAG_TYPE_EXCEPTION)) { return tmpres; }

        results->car = tmpres;
        results->cdr = memory_alloc_cons(NIL, NIL);
        results = results->cdr;

        data = data->cdr;
    }

    return results;
}

// -- -- -- -- --

int main(int argc, char** argv) 
{
    // Start up core.
    memory_init(CELL_AREA);
    frame_init();

    // Start up the frame machinery.
    frame_push_defn_in(&env_root, "t", T);
    frame_push_defn_in(&env_root, "nil", NIL);
    frame_push_defn_in(&env_root, "def", memory_alloc_builtin(fn_def, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "ldef", memory_alloc_builtin(fn_ldef, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "car", memory_alloc_builtin(fn_car, 0)); 
    frame_push_defn_in(&env_root, "cdr", memory_alloc_builtin(fn_cdr, 0)); 
    frame_push_defn_in(&env_root, "list", memory_alloc_builtin(fn_list, 0));
    frame_push_defn_in(&env_root, "quote", memory_alloc_builtin(fn_quote, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "cons", memory_alloc_builtin(fn_cons, 0));
    frame_push_defn_in(&env_root, "append", memory_alloc_builtin(fn_append, 0));
    frame_push_defn_in(&env_root, "null", memory_alloc_builtin(fn_null, 0));
    frame_push_defn_in(&env_root, "+", memory_alloc_builtin(fn_add, 0));
    frame_push_defn_in(&env_root, "-", memory_alloc_builtin(fn_sub, 0));
    frame_push_defn_in(&env_root, "*", memory_alloc_builtin(fn_mul, 0));
    frame_push_defn_in(&env_root, "/", memory_alloc_builtin(fn_div, 0));
    frame_push_defn_in(&env_root, "mod", memory_alloc_builtin(fn_mod, 0));
    frame_push_defn_in(&env_root, "lambda", memory_alloc_builtin(fn_lambda, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "macro", memory_alloc_builtin(fn_macro, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "read", memory_alloc_builtin(fn_read, 0));
    frame_push_defn_in(&env_root, "print", memory_alloc_builtin(fn_print, 0));
    frame_push_defn_in(&env_root, "eval", memory_alloc_builtin(fn_eval, 0));
    frame_push_defn_in(&env_root, "eq", memory_alloc_builtin(fn_eq, 0));
    frame_push_defn_in(&env_root, "eql", memory_alloc_builtin(fn_eql, 0));
    frame_push_defn_in(&env_root, ">", memory_alloc_builtin(fn_gt, 0));
    frame_push_defn_in(&env_root, ">=", memory_alloc_builtin(fn_gteq, 0));
    frame_push_defn_in(&env_root, "<", memory_alloc_builtin(fn_lt, 0));
    frame_push_defn_in(&env_root, "<=", memory_alloc_builtin(fn_lteq, 0));
    frame_push_defn_in(&env_root, "==", memory_alloc_builtin(fn_numeq, 0));
    frame_push_defn_in(&env_root, "/=", memory_alloc_builtin(fn_numneq, 0));
    frame_push_defn_in(&env_root, "cond", memory_alloc_builtin(fn_cond, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "go", memory_alloc_builtin(fn_go, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "tagbody", memory_alloc_builtin(fn_tagbody, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "setq", memory_alloc_builtin(fn_setq, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "let", memory_alloc_builtin(fn_let, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "nlet", memory_alloc_builtin(fn_nlet, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "macroexpand", memory_alloc_builtin(fn_macroexpand, 0));
    frame_push_defn_in(&env_root, "garbage", memory_alloc_builtin(fn_garbage, 0));
    frame_push_defn_in(&env_root, "pyprint", memory_alloc_builtin(fn_pyprint, 0));
    frame_push_defn_in(&env_root, "progn", memory_alloc_builtin(fn_progn, 0));
    frame_push_defn_in(&env_root, "and", memory_alloc_builtin(fn_and, 0));
    frame_push_defn_in(&env_root, "or", memory_alloc_builtin(fn_or, 0));
    frame_push_defn_in(&env_root, "when", memory_alloc_builtin(fn_when, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "open", memory_alloc_builtin(fn_open, 0));
    frame_push_defn_in(&env_root, "close", memory_alloc_builtin(fn_close, 0));
    frame_push_defn_in(&env_root, "print-to", memory_alloc_builtin(fn_print_to, 0));
    frame_push_defn_in(&env_root, "pyprint-to", memory_alloc_builtin(fn_pyprint_to, 0));
    frame_push_defn_in(&env_root, "env-root", memory_alloc_builtin(fn_env_root, 0));
    frame_push_defn_in(&env_root, "apply", memory_alloc_builtin(fn_apply, 0));
   
    frame_push_defn_in(&env_root, "*error-stream*", memory_alloc_stream(stderr));
    // env-root should be a variable too, but printing it causes an infinite loop, so.

    // And do some legwork.
    _evaluate_sexp(_parse_sexps("(def if (macro (cd ys no) (list 'cond (list cd ys) (list t no))))"));
    _evaluate_sexp(_parse_sexps("(def dotimes (macro (ct bd) (list 'let (list (list 'i '0)) (list 'tagbody 'g_loop bd (list 'setq 'i (list '+ 'i '1)) (list 'cond (list (list '< 'i ct) (list 'go 'g_loop))) ))))"));
    
    _evaluate_sexp(_parse_sexps("(def plusp (lambda (qu) (> qu 0)))"));
    _evaluate_sexp(_parse_sexps("(def first (lambda (qu) (car qu)))"));
    _evaluate_sexp(_parse_sexps("(def second (lambda (qu) (car (cdr qu))))"));
    _evaluate_sexp(_parse_sexps("(def rest (lambda (qu) (cdr qu)))"));
    
    _evaluate_sexp(_parse_sexps("(def not (lambda (ts) (cond (ts nil) (t t))))"));
    _evaluate_sexp(_parse_sexps("(def tak (lambda (x y z) (if (not (< y x)) z (tak (tak (- x 1) y z) (tak (- y 1) z x) (tak (- z 1) x y)))))"));
    _evaluate_sexp(_parse_sexps("(def ttak (lambda (x y z) (cond ((not (< y x)) z) (t (ttak (ttak (- x 1) y z) (ttak (- y 1) z x) (ttak (- z 1) x y))))))"));

    // Set up garbage collection.
    memory_add_root(&env_top);
    memory_add_root(&temp_root);
    memory_add_root(sym_top);

    // Set up some handlers.
#ifdef USE_READLINE
    using_history();
    signal(SIGINT, history_saver);
    read_history(HIST_FILE);

    rl_variable_bind("blink-matching-paren", "on");
    rl_variable_bind("history-size", "300");
    rl_initialize();
    rl_bind_key ('\t', rl_insert);
#endif

    printf("Thanks lisp v2\n");
    printf("Now speaking to the lisp listener.\n");
    
    while(1)
    {
        char* line_to_do = thanks_gets("* ");
        
        if(line_to_do == NULL) { break; }

        if(*line_to_do)
        {
            Cell* cell_to_do = _parse_sexps(line_to_do);
            memory_add_root(cell_to_do);

            D(printf("---\n"));

            Cell* res = _evaluate_sexp(cell_to_do);
            _print_sexps(res);
            printf("\n");

            memory_del_root(cell_to_do);
        }
    }

    printf("\nmain: used %d/%d cells (%d%%)\n", memory_get_used(), CELL_AREA, (memory_get_used() / CELL_AREA) * 100);

#ifdef USE_READLINE
    write_history(HIST_FILE);
#endif

    return 0;
}
