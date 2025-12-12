#include "repl.h"

// TODO: so many strlen calls that should be culled

static char write_buffer[256];

int _write_to_stdout(char* src, int len, void* state)
{
    if(len < 0) { len = strlen(src); }
    return printf("%.*s", len, src);
}

struct writer_string_state
{
    char* dest;
    int written;
    int size;
};

int _write_to_string(char* src, int len, void* state)
{
    // TODO: find the bugs I know are in here
    struct writer_string_state* info = (struct writer_string_state*)state;
    if(len < 0) { len = strlen(src); }

    info->written += snprintf(info->dest + info->written, info->size - info->written, "%.*s", len, src);
    return info->written;
}

int _write_to_file(char* src, int len, void* state)
{
    FILE* file = (FILE*)state;
    if(len < 0) { len = strlen(src); }
    return fprintf(file, "%.*s", len, src);
}

void _print_sexps_internal(write_callback_t writer, Cell* target, void* state)
{
    if(IS_NIL(target)) { writer("()", -1, state); return; }

    if(IS_TYPE(target->tag, TAG_TYPE_CONS))
    {
        writer("(", -1, state);
        bool first = true;

        while(!IS_NIL(target))
        {
            // Handle conses
            if(!IS_TYPE(target->tag, TAG_TYPE_CONS))
            {
                writer(" . ", -1, state);
                _print_sexps_internal(writer, target, state);
                target = NULL;
            }
            else
            {
                if(!first) { writer(" ", -1, state); }
                first = false;
                _print_sexps_internal(writer, target->car, state);
                target = target->cdr;
            }
        }
        writer(")", -1, state); 
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_NUMBER))
    {
        int last = snprintf(write_buffer, sizeof(write_buffer), "%.6f", target->num);

        while(write_buffer[--last] == '0') {}

        if(write_buffer[last] == '.') { write_buffer[last] = '\0'; }
        write_buffer[last+1] = '\0';

        writer(write_buffer, -1, state);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_LAMBDA))
    {
        writer("(lambda ", -1, state);
        _print_sexps_internal(writer, target->car, state);
        writer(" ", -1, state);
        _print_sexps_internal(writer, target->cdr, state);
        writer(")", -1, state);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_SYMBOL))
    {
        Cell* symstr = target->car;
        // TODO: auto type crushing for strings
        if(IS_TYPE(symstr->tag, TAG_TYPE_PSTRING))
            writer((char*)&(symstr->data), symstr->size, state);
        else
            writer((char*)symstr->car, symstr->size, state); // TODO: if strings explode look here first
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_PSTRING))
    {
        writer("\"", -1, state);
        writer((char*)&(target->data), target->size, state);
        writer("\"", -1, state);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_STRING))
    {
        writer("\"", -1, state);
        writer((char*)target->car, target->size, state); // TODO: if strings explode look here first
        writer("\"", -1, state);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_ARRAY))
    {
        snprintf(write_buffer, sizeof(write_buffer), "<array at %8p>", target->car);
        writer(write_buffer, -1, state);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_BUILTIN))
    {
        snprintf(write_buffer, sizeof(write_buffer), "<builtin at %8p>", target->car);
        writer(write_buffer, -1, state);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_EXCEPTION))
    {
        snprintf(write_buffer, sizeof(write_buffer), "Exception (type %d) in %s", (int)((target->tag & TAG_SPEC_MASK) >> 8), (char*)((Cell*)target->car)->car);
        writer(write_buffer, -1, state);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_HARDLINK))
    {
        Cell* trapped = target->car;

        writer("[->", -1, state);
        _print_sexps_internal(writer, trapped->car, state);
        printf(" %p", target->car);
        writer("]", -1, state);
    }
    else
    {
        snprintf(write_buffer, sizeof(write_buffer), "<unknown type %d at %8p>", (int)target->tag, target);
        writer(write_buffer, -1, state);
    }
}


void _print_sexps(Cell* target)
{
    _print_sexps_internal(_write_to_stdout, target, NULL);
}

void _print_sexps_to(char* dest, int size, Cell* target)
{
    struct writer_string_state info = {dest, 0, size};
    _print_sexps_internal(_write_to_string, target, &info);
}

void _print_sexps_to_file(FILE* dest, Cell* target)
{
    _print_sexps_internal(_write_to_file, target, (void*)dest);
}
