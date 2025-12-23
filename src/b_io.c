#include "memory.h"
#include "builtins.h"
#include "frame.h"
#include "repl.h"

/* Lisp operations
 * Math related
 */

#ifdef ON_WEB
#include <emscripten.h>
#endif
char* thanks_gets(char*);

Cell* fn_collect(Cell* args) // (collect) Prompts the user for a string
{
    char* got = thanks_gets("");
    Cell* made = memory_alloc_string(got, -1);
    // Note that _gets frees the string for us.
    return made;
}

Cell* fn_pyprint(Cell* args) // (pyprint &rest v) Successively prints its arguments v, with no quotes around strings and spaces between values
{
    while(!IS_NIL(args))
    {
        Cell* next = args->car;

        if(IS_TYPE(next->tag, TAG_TYPE_PSTRING))
        {
            printf("%s", (char*)&(next->data));
        }
        else if(IS_TYPE(next->tag, TAG_TYPE_STRING))
        {
            printf("%s", (char*)next->car);
        }
        else
        {
            _print_sexps(next);
        }

        printf(" ");
        args = args->cdr;
    }

    printf("\n");
    return NIL;
}

Cell* fn_open(Cell* args) // (open f m) Opens filename f with mode m and returns a stream
{
    Cell* filename = memory_nth(args, 0)->car;
    Cell* mode = memory_nth(args, 1)->car;
    
    // pain
    FILE* file = fopen(string_ptr(filename), symbol_string_ptr(mode)+1);
    if(file == NULL)
        return memory_alloc_exception(TAG_SPEC_EX_IO, memory_alloc_string("open: error opening file", -1));

    return memory_alloc_stream((void*)file);
}

Cell* fn_close(Cell* args) // (close s) Closes a stream s
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

Cell* fn_print_to(Cell* args) // (print-to s f) Prints a form f to stream s
{
    Cell* stream = memory_nth(args, 0)->car;
    Cell* target = memory_nth(args, 1)->car;

    _print_sexps_to_file((FILE*)stream->car, target);
    fprintf((FILE*)stream->car, "\n");

    return target; 
}

Cell* fn_pyprint_to(Cell* args) // (pyprint-to s &rest v) Like (pyprint), but prints to stream s
{
    Cell* stream = args->car;
    args = args->cdr;
    FILE* file = (FILE*)stream->car;

    while(!IS_NIL(args))
    {
        Cell* next = args->car;

        if(IS_TYPE(next->tag, TAG_TYPE_PSTRING))
        {
            fprintf(file, "%s", string_ptr(next));
        }
        else if(IS_TYPE(next->tag, TAG_TYPE_STRING))
        {
            fprintf(file, "%s", string_ptr(next));
        }
        else
        {
            _print_sexps_to_file(file, next);
        }

        fprintf(file, " ");
        args = args->cdr;
    }

    fprintf(file, "\n");
    fflush(file);
    return NIL;
}

Cell* fn_time(Cell* args) // !(time f) Evaluates form f, printing the elapsed evaluation time to the console
{
    // TODO: utility of making the time available to lisp?
    clock_t start = clock();
    Cell* result = _evaluate_sexp(args->car);
    clock_t stop = clock() - start;
    double len = 1000.0 * stop / CLOCKS_PER_SEC;
    printf("time: %fms\n", len);
    return result;
}
Cell* fn_sleep(Cell* args) // (sleep s) Pauses Lisp execution for s seconds
{
    Cell* sec = args->car;
#ifdef ON_WEB
    emscripten_sleep(sec->num * 1000.0);
#else
    usleep(sec->num * 1000.0 * 1000.0);
#endif
    return sec;
}

Cell* fn_decoded_time(Cell* args) // (decoded-time) Returns a list containing the system local time (s m h d m y w)
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    Cell* built = NIL;
    Cell* saved = NIL;

    built = memory_extend(built); built->car = memory_alloc_number(timeinfo->tm_sec); saved = built;
    built = memory_extend(built); built->car = memory_alloc_number(timeinfo->tm_min);
    built = memory_extend(built); built->car = memory_alloc_number(timeinfo->tm_hour);
    built = memory_extend(built); built->car = memory_alloc_number(timeinfo->tm_mday);
    built = memory_extend(built); built->car = memory_alloc_number(timeinfo->tm_mon);
    built = memory_extend(built); built->car = memory_alloc_number(timeinfo->tm_year);

    return saved;
}
Cell* fn_load(Cell* args) // (load f) Opens, reads, and evaluates the contents of filename f; returns the last result
{
    Cell* filename = args->car;
    int len;
    FILE* target = fopen(string_ptr(filename), "r");

    fseek(target, 0, SEEK_END);
    len = ftell(target);
    fseek(target, 0, SEEK_SET);
    char* buffer = malloc(len);
    
    if(buffer)
    {
        fread(buffer, 1, len, target);
        fclose(target);
    }
    else
    {
        fclose(target);
        return memory_alloc_exception(TAG_SPEC_EX_IO, memory_alloc_string("load: error opening file", -1));
    }

    int idx = 0;
    Cell* last = NIL;

    while(idx <= len && last != NULL)
    {
        Cell* next = _walk_sexps(buffer, &idx);
        if(next == NULL) { return last; }
        frame_push_in(&temp_root, next);
        last = _evaluate_sexp(next);
        frame_pop_in(&temp_root, 1);
        if(IS_TYPE(last->tag, TAG_TYPE_EXCEPTION)) { return last; }
    }

    return last;
}

