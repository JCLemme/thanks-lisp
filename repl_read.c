#include "repl.h"

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
                next_object = memory_alloc_cons(NIL, NIL);
                this_object = next_object;
            }
            else
            {
                next_object->cdr = memory_alloc_cons(NIL, NIL);
                next_object = next_object->cdr;
            }

            next_object->car = _recurse_sexps(buffer, index);

            while(buffer[(*index)] == ' ') {(*index)++;}
            c = buffer[(*index)];
        }

        // Nil won't set this_object - do it manually
        if(this_object == NULL) this_object = NIL;

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
            //printf("%c\n", scratch[1]);
            goto was_symbol_actually;
        }

        // TODO: an input of just "+" becomes 0 - I think it's the null terminator detector breaking
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
                else if(c == 'x')
                    scratch[scridx++] = '\x1b';
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

Cell* _parse_sexps(char* inbuf)
{
    int idx = 0;
    return _recurse_sexps(inbuf, &idx);    
}

