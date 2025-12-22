#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "memory.h"
#include "frame.h"
#include "util.h"
#include "repl.h"
#include "builtins.h"

#ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif

/* T H A N K S  L I S P
 *
 * You probably opened this file first to figure out wtf is going on.
 * Here's a map:
 *
 * memory.c controls cells and the heap. Cell allocation and garbage collection
 * live here, plus the symbol table and a bunch of cell type helpers.
 *
 * frame.c contains the environment system, built from conses and symbols.
 *
 * repl_*.c hold the main Lisp operations - reading, printing, and evaluating 
 * S-expressions. 
 *
 * The rest of the source files contain the Lisp builtins. C functions live in 
 * the categorized C files (math.c, etc.), while Lisp-only functions/macros
 * get defined in builtins.c.
 *
 * Enjoy~
 */


#ifdef ON_WEB
#include <emscripten.h>
#endif

#if defined(USE_READLINE)
#include <readline/readline.h>
#include <readline/history.h>
#elif defined(USE_SHIM)
#include "shim.h"
#endif

#if defined(USE_READLINE) || defined(USE_SHIM)
char* last_line = NULL;
bool save_history = true;

char* thanks_gets(char* prompt)
{
    if(last_line) { free(last_line); }
    last_line = readline(prompt);

    if(last_line && *last_line && save_history) { add_history(last_line); }

    return last_line;
}

#define HIST_FILE "./.thanks_history"

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


bool executing = false;
void inthandler(int sig)
{
    if(executing)
    {
        got_interrupt = true;
    }
    else
    {
#ifdef USE_READLINE
        write_history(HIST_FILE);
#endif
        exit(0);
    }
}

#define CELL_AREA 409600

static char errorzone[256];




Cell* fn_garbage(Cell* args)
{
    int used = memory_gc();
    return memory_alloc_number(used);
}

Cell* fn_env_root(Cell* args)
{
    return &env_root;
}


Cell* fn_fast(Cell* args)
{
    Cell* form = memory_deep_copy(args->car);
    form->cdr = _hardlink_dynamic(form->cdr);
    return form;
}




// -- -- -- -- --

int main(int argc, char** argv) 
{
    // Basic, basic support for arguments.
    if(argc > 1)
    {
        for(int a = 1; a < argc; a++)
        {
            // Just disabling history could be done, I think, with env variables alone.
            // But I might want to add other features to test mode, so.
            if(strcmp(argv[a], "--testmode") == 0)
            {
#ifdef USE_READLINE
                save_history = false;
#endif
            }
        }
    }

    // Start up core.
    memory_init(CELL_AREA);
    frame_init();
    builtins_init();

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
    frame_push_defn_in(&env_root, "rplaca", memory_alloc_builtin(fn_rplaca, 0));
    frame_push_defn_in(&env_root, "rplacd", memory_alloc_builtin(fn_rplacd, 0));
    frame_push_defn_in(&env_root, "nconc", memory_alloc_builtin(fn_nconc, 0));
    frame_push_defn_in(&env_root, "null", memory_alloc_builtin(fn_null, 0));
    frame_push_defn_in(&env_root, "+", memory_alloc_builtin(fn_add, 0));
    frame_push_defn_in(&env_root, "-", memory_alloc_builtin(fn_sub, 0));
    frame_push_defn_in(&env_root, "*", memory_alloc_builtin(fn_mul, 0));
    frame_push_defn_in(&env_root, "/", memory_alloc_builtin(fn_div, 0));
    frame_push_defn_in(&env_root, "mod", memory_alloc_builtin(fn_mod, 0));
    frame_push_defn_in(&env_root, "sin", memory_alloc_builtin(fn_sin, 0));
    frame_push_defn_in(&env_root, "cos", memory_alloc_builtin(fn_cos, 0));
    frame_push_defn_in(&env_root, "tan", memory_alloc_builtin(fn_tan, 0));
    frame_push_defn_in(&env_root, "sqrt", memory_alloc_builtin(fn_sqrt, 0));
    frame_push_defn_in(&env_root, "pow", memory_alloc_builtin(fn_pow, 0));
    frame_push_defn_in(&env_root, "truncate", memory_alloc_builtin(fn_truncate, 0));
    frame_push_defn_in(&env_root, "lambda", memory_alloc_builtin(fn_lambda, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "macro", memory_alloc_builtin(fn_macro, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "read", memory_alloc_builtin(fn_read, 0));
    frame_push_defn_in(&env_root, "collect", memory_alloc_builtin(fn_collect, 0));
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
    frame_push_defn_in(&env_root, "map", memory_alloc_builtin(fn_map, 0));
    frame_push_defn_in(&env_root, "typep", memory_alloc_builtin(fn_typep, 0));
    frame_push_defn_in(&env_root, "type-of", memory_alloc_builtin(fn_type_of, 0));
    frame_push_defn_in(&env_root, "backquote", memory_alloc_builtin(fn_backquote, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "comma", memory_alloc_builtin(fn_comma, 0));
    frame_push_defn_in(&env_root, "time", memory_alloc_builtin(fn_time, TAG_SPEC_FUNLAZY));
    frame_push_defn_in(&env_root, "fast", memory_alloc_builtin(fn_fast, 0));
    frame_push_defn_in(&env_root, "sleep", memory_alloc_builtin(fn_sleep, 0));
    frame_push_defn_in(&env_root, "decoded-time", memory_alloc_builtin(fn_decoded_time, 0));
    frame_push_defn_in(&env_root, "load", memory_alloc_builtin(fn_load, 0));
    frame_push_defn_in(&env_root, "symbol-name", memory_alloc_builtin(fn_symbol_name, 0));
    frame_push_defn_in(&env_root, "read-from-string", memory_alloc_builtin(fn_read_from_string, 0));
    frame_push_defn_in(&env_root, "substr", memory_alloc_builtin(fn_substr, 0));
    

    // env-root should be a variable too, but printing it causes an infinite loop, so.

    // Make some standard defines.


    // Set up garbage collection.
    // TODO: can these be eliminated by adding them to the env? (some maybe)
    memory_add_root(&env_top);
    memory_add_root(&temp_root);
    memory_add_root(sym_top);
    memory_add_root(&reader_macros);

    // Set up some handlers.
    struct sigaction sa;
    sa.sa_handler = inthandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

#ifdef USE_READLINE
    using_history();
    read_history(HIST_FILE);

    rl_variable_bind("blink-matching-paren", "on");
    rl_variable_bind("history-size", "300");
    rl_initialize();
    rl_bind_key ('\t', rl_insert);
    //rl_bind_key ('\n', sexp_ready);
#endif

    printf("Thanks lisp v2\n");
    printf("Now speaking to the lisp listener.\n");
    
    while(1)
    {
        got_interrupt = false;
#ifdef USE_READLINE
        executing = false;
#endif

        char* line_to_do = thanks_gets("* ");
        
        if(line_to_do == NULL) { break; }

        if(*line_to_do)
        {
            Cell* cell_to_do = _parse_sexps(line_to_do);
            memory_add_root(cell_to_do);

            D(printf("---\n"));

#ifdef USE_READLINE
            executing = true;
#endif

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
