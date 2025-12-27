#include "memory.h"
#include "frame.h"
#include "builtins.h"
#include "repl.h"

/* Lisp operations
 * Math related
 */

Cell* fn_def(Cell* args) // !(def n v) Adds or replaces a definition with name n and value v
{
    Cell* evalarg = _evaluate_sexp(((Cell*)(args->cdr))->car);
    frame_push_def_in(&env_root, args->car, evalarg);
    return args->car;
}

Cell* fn_ldef(Cell* args) // !(ldef n v) Of dubious utility
{
    Cell* evalarg = _evaluate_sexp(((Cell*)(args->cdr))->car);
    frame_push_def(args->car, evalarg);
    return args->car;
}


// Real simple primitives.
Cell* fn_car(Cell* args) // (car c) Returns the car of cons c
{
    Cell* first = args->car;
    if(!IS_TYPE(first->tag, TAG_TYPE_CONS))
    {
        _print_sexps(first);
        return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("car: argument not of type 'list'", -1));
    }

    return first->car;
}

Cell* fn_cdr(Cell* args) // (cdr c) Returns the cdr of cons c
{
    // TODO: we shouldn't have to special case this...
    if(args == NULL || args == NIL) { return NIL;}
    if(args->car == NULL || args->car == NIL) { return NIL;}

    Cell* first = args->car;
    if(!IS_TYPE(first->tag, TAG_TYPE_CONS))
    {
        return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("car: argument not of type 'list'", -1));
    }

    return first->cdr;
}

Cell* fn_list(Cell* args) // (list &rest v) Constructs and returns a list from its arguments
{
    return args;
}

Cell* fn_quote(Cell* args) // !(quote v) Returns its argument v literally, i.e. without being evaluated
{
    return args->car;
}

Cell* fn_cons(Cell* args) // (cons a d) Constructs a cons cell with car a and cdr d
{
    Cell* new_car = memory_nth(args, 0)->car;
    Cell* new_cdr = memory_nth(args, 1)->car;
    return memory_alloc_cons(new_car, new_cdr);
}

Cell* fn_append(Cell* args) // (append &rest l) Constructs a new list containing the elements of all lists l
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

Cell* fn_rplaca(Cell* args) // (rplaca c a) Replaces the car of cons c with a
{
    Cell* target = memory_nth(args, 0)->car;
    Cell* insert = memory_nth(args, 1)->car;
    target->car = insert;
    return target;
}

Cell* fn_rplacd(Cell* args) // (rplacd c d) Replaces the cdr of cons c with d
{
    Cell* target = memory_nth(args, 0)->car;
    Cell* insert = memory_nth(args, 1)->car;
    target->cdr = insert;
    return target;
}

Cell* fn_nconc(Cell* args) // (nconc &rest l) Destructively concatenates all lists l, replacing the cdr of each list's last element in-place
{
    // Trying something different.
    // (Bad)
    int len = memory_length(args);

    if(len == 0) { return NIL; }
    if(len == 1) { return args->car; }
    Cell* save = args->car;

    for(int i = 0; i < len - 1; i++)
    {
        Cell* sublist = memory_nth(args, i)->car;
        Cell* taillist = memory_nth(args, i+1)->car;
        int sublen = memory_length(sublist);
        memory_nth(sublist, sublen - 1)->cdr = taillist;
    }

    return save;
}

Cell* fn_null(Cell* args) // (null c) Returns t if the cell is nil
{
    // TODO: nearly time to flip case
    if(args == NULL || args == NIL) { return T; }
    return (IS_NIL(((Cell*)args->car))) ? T : NIL;
}


Cell* fn_lambda(Cell* args) // !(lambda l b) Constructs a lambda function with lambda-list l and body b
{
    // Args: a list of arguments, and a list of operations
    Cell* arglist = args->car;
    Cell* oplist = ((Cell*)(args->cdr))->car;
    // TODO: make sure the arglist is only symbols
    Cell* result = memory_alloc_lambda(arglist, NIL, 0);            

    // Closure conversion: capture anything from the lexical environment that we need
    Cell* modlist = memory_deep_copy(oplist);
    result->cdr = _hardlinker(modlist, arglist);
    
    return result;
}

Cell* fn_macro(Cell* args) // !(macro l b) Constructs a macro with lambda-list l and body b
{
    // Args: a list of arguments, and a list of operations
    Cell* arglist = args->car;
    Cell* oplist = ((Cell*)(args->cdr))->car;
    // TODO: make sure the arglist is only symbols
    return memory_alloc_lambda(arglist, oplist, TAG_SPEC_FUNLAZY | TAG_SPEC_FUNMACRO); // TODO: use case for splitting? or at least not implying lazy
}



Cell* fn_read(Cell* args) // (read s) BORKED Parses an s-expression from a string s
{
    return _parse_sexps(string_ptr(args->car));
}

Cell* fn_print(Cell* args) // (print v) Prints an s-expression v to the console
{
    Cell* arg = args->car;
    _print_sexps(arg);
    printf("\n");
    return arg;
}

Cell* fn_eval(Cell* args) // (eval v) Evaluates an s-expression v and returns the result
{
    return _evaluate_sexp(args->car);
}

Cell* fn_eq(Cell* args) // (eq a b) Returns true if cells a and b have the same car, i.e. refer to the same object
{
    // Semantics - in CL, "eq" is a pointer comparison - is this the same object?
    Cell* a = memory_nth(args, 0);
    Cell* b = memory_nth(args, 1);
    return (a->car == b->car) ? T : NIL;
}

Cell* fn_eql(Cell* args) // (eql a b) Returns true if the cars of cells a and b have the same value
{
    // However, "eql" is a value comparison - do these objects mean the same thing?
    Cell* a = memory_nth(args, 0)->car;
    Cell* b = memory_nth(args, 1)->car;
    return (a->data == b->data) ? T : NIL;
}
Cell* fn_cond(Cell* args) // !(cond &rest c) Evaluates a set of test-body pairs c, returning the result of the first match
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

Cell* fn_go(Cell* args) // !(go l) Changes the control flow in a tagbody to the label l
{
    Cell* target = args->car;
    if(!IS_TYPE(target->tag, TAG_TYPE_SYMBOL)) 
    {
        return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("go: label wasn't a symbol", -1));
    }
    else
    {
        return memory_alloc_exception(TAG_SPEC_EX_LABEL, args->car);
    }
}

Cell* fn_tagbody(Cell* args) // !(tagbody &rest s) Evaluates its arguments s in order, optionally changing control flow with the use of (go)
{
    Cell* hunting_for = NULL;
    Cell* current_step = args;
    //frame_push_in(&temp_root, current_step);

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
                        //frame_pop_in(&temp_root, 1);
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
        //frame_pop_in(&temp_root, 1);
        return memory_alloc_exception(TAG_SPEC_EX_DATA, memory_alloc_string("tagbody: label not found", -1));
    }
    else
    {
        //frame_pop_in(&temp_root, 1);
        return NIL;
    }
}

Cell* fn_setq(Cell* args) // !(setq n v) Sets the value of symbol s in the environment to value v
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

Cell* fn_let(Cell* args) // !(let a f) Locally defines a list of name-value pairs a, then evaluates form f
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

Cell* fn_nlet(Cell* args) // !(nlet a f) BORKED Like (let) but allows referencing names within their own definitions
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

Cell* fn_macroexpand(Cell* args) // (macroexpand m) Expands a macro form m to its most primitive version
{
    return _macroexpand(args->car);
}

Cell* fn_progn(Cell* args) // (progn &rest f) Processes its arguments f in order, returning the result of the last
{
    // Our (eval) executes these statements in order anyway - just return the last argument.
    if(args == NULL || args == NIL) { return NIL; }
    return memory_nth(args, memory_length(args) - 1)->car;
}

Cell* fn_and(Cell* args) // (and &rest t) Returns true if all its arguments t are truthy
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

Cell* fn_or(Cell* args) // (or &rest t) Returns true if any of its arguments t are truthy
{
    while(!IS_NIL(args))
    {
        if(args->car != NIL) { return args->car; }
        args = args->cdr;
    }

    return NIL;
}

Cell* fn_when(Cell* args) // !(when t b) Evaluates a test form t, and evaluates body b if the result is true
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

Cell* fn_map(Cell* args) // (map f l) Applies a function f to the cars of list l, returning a list of the results
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

Cell* fn_typep(Cell* args) // (typep t c) Returns true if the type of cell c matches type keyword t
{
    Cell* type = memory_nth(args, 0)->car;
    Cell* target = memory_nth(args, 1)->car;

    if(symbol_matches(type, ":cons")) { return (IS_TYPE(target->tag, TAG_TYPE_CONS)) ? T : NIL; }
    else if(symbol_matches(type, ":number")) { return (IS_TYPE(target->tag, TAG_TYPE_NUMBER)) ? T : NIL; }
    else if(symbol_matches(type, ":lambda")) { return (IS_TYPE(target->tag, TAG_TYPE_LAMBDA)) ? T : NIL; }
    else if(symbol_matches(type, ":symbol")) { return (IS_TYPE(target->tag, TAG_TYPE_SYMBOL)) ? T : NIL; }
    else if(symbol_matches(type, ":array")) { return (IS_TYPE(target->tag, TAG_TYPE_ARRAY)) ? T : NIL; }
    else if(symbol_matches(type, ":builtin")) { return (IS_TYPE(target->tag, TAG_TYPE_BUILTIN)) ? T : NIL; }
    else if(symbol_matches(type, ":hardlink")) { return (IS_TYPE(target->tag, TAG_TYPE_HARDLINK)) ? T : NIL; }
    else if(symbol_matches(type, ":exception")) { return (IS_TYPE(target->tag, TAG_TYPE_EXCEPTION)) ? T : NIL; }
    else if(symbol_matches(type, ":stream")) { return (IS_TYPE(target->tag, TAG_TYPE_STREAM)) ? T : NIL; }
    else if(symbol_matches(type, ":string")) { return (IS_TYPE(target->tag, TAG_TYPE_STRING) || IS_TYPE(target->tag, TAG_TYPE_PSTRING)) ? T : NIL; }
    else { return NIL; }
}

Cell* fn_type_of(Cell* args) // (type-of c) Returns the type keyword of cell c
{
    Cell* target = memory_nth(args, 0)->car;

    if(IS_TYPE(target->tag, TAG_TYPE_CONS)) { return memory_alloc_symbol(":cons"); }
    if(IS_TYPE(target->tag, TAG_TYPE_NUMBER)) { return memory_alloc_symbol(":number"); }
    if(IS_TYPE(target->tag, TAG_TYPE_LAMBDA)) { return memory_alloc_symbol(":lambda"); }
    if(IS_TYPE(target->tag, TAG_TYPE_SYMBOL)) { return memory_alloc_symbol(":symbol"); }
    if(IS_TYPE(target->tag, TAG_TYPE_ARRAY)) { return memory_alloc_symbol(":array"); }
    if(IS_TYPE(target->tag, TAG_TYPE_BUILTIN)) { return memory_alloc_symbol(":builtin"); }
    if(IS_TYPE(target->tag, TAG_TYPE_HARDLINK)) { return memory_alloc_symbol(":hardlink"); }
    if(IS_TYPE(target->tag, TAG_TYPE_EXCEPTION)) { return memory_alloc_symbol(":exception"); }
    if(IS_TYPE(target->tag, TAG_TYPE_STREAM)) { return memory_alloc_symbol(":stream"); }
    if(IS_TYPE(target->tag, TAG_TYPE_STRING) || IS_TYPE(target->tag, TAG_TYPE_PSTRING)) { return memory_alloc_symbol(":string"); }
    else { return NIL; }
}

Cell* _do_backquote(Cell* target)
{
    if(IS_TYPE(target->tag, TAG_TYPE_CONS))
    {
        // We have one special case: if it's a comma-form [i.e. (comma ...)], let it eval.
        // Note that this function checks for type internally - non-symbols will gracefully fail out.
        if(symbol_matches(target->car, "comma"))
        {
            return _evaluate_sexp(target);
        }
        else
        {
            Cell* built = NIL;
            Cell* saved = NIL;

            while(!IS_NIL(target))
            {
                // TODO: ,@ and ,. (append and nconc)
                built = memory_extend(built);
                built->car = NIL;
                if(IS_NIL(saved)) 
                { 
                    saved = built; 
                    frame_push_in(&temp_root, saved);
                }

                built->car = _do_backquote(target->car);

                target = target->cdr;
            }
            
            if(!IS_NIL(saved)) { frame_pop_in(&temp_root, 1); }
            return saved;
        }
    }
    else
    {
        // If it's not a list, act like it's (quote) and do nothing.
        return target;
    }
}

Cell* fn_backquote(Cell* args) // !(backquote f) Returns form f as-is, like quote; however, forms can be selectively evaluated within
{
    // Opposite-eval: return quoted forms *except* when asked not to.
    return _do_backquote(args->car);
}

Cell* fn_comma(Cell* args) // (comma f) Evaluates and returns the result of form f; will evaulate even within a backquote form
{
    // Do nothing.
    return args->car;
}


