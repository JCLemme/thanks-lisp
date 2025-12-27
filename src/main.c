#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "memory.h"
#include "frame.h"
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
 * Lisp functions are defined in the b_*.c files (for "builtins"). They're 
 * sort of categorized. Functions written in Lisp live in builtins.c. Note that
 * the C builtins are defined automatically; you'll need to add a docstring comment
 * like the others have and rerun mkbuiltins.sh to add it to the build.
 *
 * ~Also~
 * shim.c is a two-bit Readline clone for the Emscripten target. AI wrote it.
 * I will continue to run GNU software on my Unix box like Gawd intended tyvm
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
#ifndef ON_WEB
        exit(0);
#endif
    }
}

#define CELL_AREA 409600

static char errorzone[256];




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
        executing = false;

        char* line_to_do = thanks_gets("* ");
        
        if(line_to_do == NULL) { break; }

        if(*line_to_do)
        {
            Cell* cell_to_do = _parse_sexps(line_to_do);
            memory_add_root(cell_to_do);

            D(printf("---\n"));

            executing = true;

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
