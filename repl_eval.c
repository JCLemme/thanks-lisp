#include "repl.h"

// Evaluation
// ----------


int _lambda_list_define_all(Cell* env, Cell* list, Cell* args)
{
    // Process a lambda list. Make the relevant defines and install them into the environment.
    // TODO: optionals, keys, rest
    int expected = 0;

    while(!IS_NIL(list))
    {
        frame_push_def_in(env, list->car, args->car);
        expected++;
        list = list->cdr;
        args = args->cdr;
    }

    return expected;
}

bool _is_macro_expr(Cell* list)
{
    if(!IS_TYPE(list->tag, TAG_TYPE_CONS)) { return false; }
    Cell* func = memory_nth(list, 0);
    if(!IS_TYPE(func->tag, TAG_TYPE_LAMBDA)) { return false; }
    if(!IS_SPEC(func->tag, TAG_SPEC_FUNMACRO)) { return false; }
    return true;
}

Cell* _hardlinker(Cell* target)
{
    // Recurse, replacing references to objects in the lexical environment with links thereto.
    if(!IS_TYPE(target->tag, TAG_TYPE_CONS))
    {
        if(IS_TYPE(target->tag, TAG_TYPE_SYMBOL))
        {
            // Symbol definitions are sus. Note that "find_def_in" will only look within the 
            // top level of the environment, i.e. lexical definitions only.
            Cell* found_def = frame_find_def_in(&env_top, target); 
           
            // TODO: an internal version of this function that doesn't allocate an exception. Wasteful
            if(IS_TYPE(found_def->tag, TAG_TYPE_EXCEPTION))
            {
                return target; // Must be a global - ignore it.
            }
            else
                return memory_alloc_hardlink(found_def); // Got you motherfucker
        }
        else
        {
            // Don't touch anything else.
            return target;
        }
    }
    else
    {
        // SEEK AND DESTROY
        Cell* todo = target;
        while(!IS_NIL(todo))
        {
            todo->car = _hardlinker(todo->car);
            todo = todo->cdr;
        }

        return target;
    }
}

static char errorzone[256];

// These functions are too similar not to merge somehow... time will tell if this is the right way to do it
Cell* _evaluate_sexp_macrocontrol(Cell* target, bool eval_macro_result, bool finish_macro_expand)
{
    memory_gc_thresh(0.80);

    if(IS_NIL(target)) { return NIL; } // TODO: maybe unnecessary

    if(!IS_TYPE(target->tag, TAG_TYPE_CONS))
    {
        if(IS_TYPE(target->tag, TAG_TYPE_SYMBOL))
        {
            // An optimization. (I couldn't help myself.)
            // frame_find_def will alloc a cell to fake a keyword definition.
            // If we catch them here, we avoid an unnecessary alloc.
            if(symbol_is_keyword(target)) { return target; }

            // Look it up first
            Cell* found = frame_find_def(target); // sus 
            // TODO: no fucking exceptions!!!
            if(IS_TYPE(found->tag, TAG_TYPE_EXCEPTION))
            {
                snprintf(errorzone, sizeof(errorzone), "eval: symbol '%s' undefined", symbol_string_ptr(target));
                return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string(errorzone));
            }
            else
            {
                return found->cdr;
            }
        }
        else if(IS_TYPE(target->tag, TAG_TYPE_HARDLINK))
        {
            // Hardlink - it's storing a reference to the atom we want. 
            // Specifically, a reference to the *definition* for the atom we want.
            Cell* linked_def = target->car;
            return linked_def->cdr;
        }
        else
        {
            // It's an atom - return it
            return target;
        }
    }
    else if(!IS_NIL(((Cell*)target->cdr)) && !IS_TYPE(((Cell*)target->cdr)->tag, TAG_TYPE_CONS))
    {
        // It's a cons, but not a list.
        return target;
    }
    else
    {
        // It's a list - evaluate
        Cell* func = target->car;
        Cell* args = target->cdr;
        int argc = 0;
        
        if(IS_NIL(target)) return NIL;

        // Process and extract the function to run.
        func = _evaluate_sexp(func); 
        if(IS_TYPE(func->tag, TAG_TYPE_EXCEPTION)) { return func; }

        // Dup check, so we error out before evaluating the arglist.
        // TODO: refactor
        if(!IS_TYPE(func->tag, TAG_TYPE_BUILTIN) && !IS_TYPE(func->tag, TAG_TYPE_LAMBDA))
        {
            return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("eval: not a function"));
        }

        if(!IS_SPEC(func->tag, TAG_SPEC_FUNLAZY))
        {
            // Normal function - evaluate arguments first
            Cell* proc_args = NULL;
            Cell* curr_arg = NULL;
 
            while(!IS_NIL(args))
            {
                if(curr_arg == NULL)
                {
                    curr_arg = memory_alloc_cons(NIL, NIL);
                    proc_args = curr_arg;
                    frame_push_in(&temp_root, curr_arg);
                }
                else
                {
                    curr_arg->cdr = memory_alloc_cons(NIL, NIL);
                    curr_arg = curr_arg->cdr;
                }

                argc++;

                curr_arg->car = _evaluate_sexp(args->car); // see above
                if(IS_TYPE(((Cell*)curr_arg->car)->tag, TAG_TYPE_EXCEPTION)) 
                { 
                    frame_pop_in(&temp_root, 1);//argc);
                    return curr_arg->car; 
                }

                args = args->cdr;
            }

            args = proc_args;
        }
        else
        {
            // Literally just counting.
            argc = memory_length(args);
        }

        if(IS_TYPE(func->tag, TAG_TYPE_BUILTIN))
        {
            // An oversight? Builtins don't get a lexical environment automatically, unless they do it themselves.
            Cell* result = ((Cell* (*)(Cell*))func->car)(args);
            if(!IS_SPEC(func->tag, TAG_SPEC_FUNLAZY)) { frame_pop_in(&temp_root, 1); } //argc); }
            return result;
        }
        else if(IS_TYPE(func->tag, TAG_TYPE_LAMBDA))
        {
            // Set up the lexical environment.
            int expected = _lambda_list_define_all(&env_top, func->car, args);

            Cell* body = func->cdr;
            Cell* result = NULL;

            // Macros need to be expanded first, which leads to potential GC. Hence this ugly.
            if(IS_SPEC(func->tag, TAG_SPEC_FUNMACRO))
            {
                result = body;
                do {
                    frame_push_in(&temp_root, result);
                    result = _evaluate_sexp(result);
                    frame_pop_in(&temp_root, 1);
                } while(_is_macro_expr(result) && finish_macro_expand);
                
                if(eval_macro_result)
                {
                    frame_push_in(&temp_root, result);
                    result = _evaluate_sexp(result);
                    frame_pop_in(&temp_root, 1);
                }
            }
            else
            {
                result = _evaluate_sexp(body); // hmmm
            }

            // Closure conversion: trap any references to things in the lexical environment.
            /*if(IS_TYPE(result->tag, TAG_TYPE_LAMBDA))
            {
                // To be clear: expand within the lambda body.
                result->cdr = _hardlinker(result->cdr);
            }*/
            // Moved to fn_lambda, so it would capture if returned by a builtin.

            // Unwind the lexical environment.
            frame_pop(expected);
            if(!IS_SPEC(func->tag, TAG_SPEC_FUNLAZY)) { frame_pop_in(&temp_root, 1); } //argc); }

            return result;
        }
        else
        {
            if(!IS_SPEC(func->tag, TAG_SPEC_FUNLAZY)) { frame_pop_in(&temp_root, 1); }//argc); }
            return memory_alloc_exception(TAG_SPEC_EX_TYPE, memory_alloc_string("eval: not a function"));
        }
    }
}

Cell* _evaluate_sexp(Cell* target)
{
    return _evaluate_sexp_macrocontrol(target, true, true);
}

Cell* _macroexpand_one(Cell* expr)
{
    return _evaluate_sexp_macrocontrol(expr, false, false); 
}

Cell* _macroexpand(Cell* expr)
{
    return _evaluate_sexp_macrocontrol(expr, false, true); 
}
