#include "repl.h"

char scratch[256];

bool _skip_whitespace(char* buffer, int* index)
{
    // Fast-forwards a character stream past whitespace.
    int found;

    while(1)
    {
        found = buffer[(*index)];
        if(found == '\0' || found == EOF) { return false; }
        if(found != ' ' && found != '\t' && found != '\n') { return true; }
        (*index)++;
    }

    // The compiler recognizes that this function will always return a value... 
    // ...but imma force it anyway
    return false;
}

int _steal_token(char* buffer, int* index)
{
    // Grabs a token from the input stream. A token is a collection of 
    // not-whitespace characters, i.e. symbols.
    if(!_skip_whitespace(buffer, index)) { return -1; }

    int found;
    int len = 1;
    bool in_string = false;
    bool in_comment = false;

    while(1)
    {
        found = buffer[(*index)];
        if(found == '\0' || found == EOF) { return -1; }

        if(!in_string)
        {
            if(!in_comment)
            {
                if(found == ')') { return len; }
                if(found == ';') { in_comment = true; }
                if(found == '"') { in_string = true; }
            }
            else
            {
                if(found == '\n') { in_comment = false; }
            }
        }
        else
        {
        }
    }

}

Cell* _recurse_sexps(char* buffer, int* index)
{
    // TODO: the readlist
    if(!_skip_whitespace(buffer, index)) { return NULL; }
    
    Cell* this_object = NULL;
    int c = buffer[(*index)];
    int scridx = 0;

    if(c == '\0' || c == EOF) // IT'S OVER
    {
        return NULL;
    }
    else if(c == '(') // IT'S A LIST
    {
        Cell* next_object = NIL;
        this_object = NIL;

        //(*index)++;
        c = buffer[++(*index)];

        while(c != ')')
        {
            next_object = memory_extend(next_object);
            if(IS_NIL(this_object)) { this_object = next_object; }

            next_object->car = _recurse_sexps(buffer, index);

            // Abort if the child failed to convert.
            if(next_object->car == NULL) { return NULL; }

            if(!_skip_whitespace(buffer, index)) { return NULL; }
            c = buffer[(*index)];

            // Signal up the tree if this object is malformed
            if(c == '\0' || c == EOF) { return NULL; }
        }

        // Read off the end
        (*index)++;
    }
    else if(c == '-' || c == '+' || isdigit(c)) // IT'S A NUMBER (MAYBE)
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
    else if(c == '"') // IT'S A STRING
    {
        (*index)++;
        c = buffer[*index];
        bool escaped = false;
        bool looking = true;

        do
        {
            if(escaped)
            {
                escaped = false; // Is there really no better way to do this?!
                // TODO: handle codepoint insertion
                switch(c)
                {
                    case('0'):  scratch[scridx++] = '\0';   break;
                    case('a'):  scratch[scridx++] = '\a';   break;
                    case('b'):  scratch[scridx++] = '\b';   break;
                    case('e'):  scratch[scridx++] = '\x1b'; break;
                    case('f'):  scratch[scridx++] = '\f';   break;
                    case('n'):  scratch[scridx++] = '\n';   break;
                    case('r'):  scratch[scridx++] = '\r';   break;
                    case('t'):  scratch[scridx++] = '\t';   break;
                    case('v'):  scratch[scridx++] = '\v';   break;
                    case('\\'): scratch[scridx++] = '\\';   break;
                    case('\''): scratch[scridx++] = '\'';   break;
                    case('\"'): scratch[scridx++] = '\"';   break;
                    default:    scratch[scridx++] = '!';    break;
                }
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

            if(c == '\0' || c == EOF) { return NULL; }
        }
        while(looking);
        scratch[scridx] = '\0';

        this_object = memory_alloc_string(scratch, -1);
    }
    else if(c == '\'') // IT'S QUOTED
    {
        (*index)++;
        Cell* subobj = _recurse_sexps(buffer, index);
        if(subobj == NULL) { return NULL; }
        this_object = memory_alloc_cons(memory_alloc_symbol("quote"), memory_alloc_cons(subobj, NIL));
    }
    else if(c == '`') // IT'S A BACKQUOTE
    {
        (*index)++;
        Cell* subobj = _recurse_sexps(buffer, index);
        if(subobj == NULL) { return NULL; }
        this_object = memory_alloc_cons(memory_alloc_symbol("backquote"), memory_alloc_cons(subobj, NIL));
    }
    else if(c == ',') // IT'S COMMA'D
    {
        (*index)++;
        Cell* subobj = _recurse_sexps(buffer, index);
        if(subobj == NULL) { return NULL; }
        this_object = memory_alloc_cons(memory_alloc_symbol("comma"), memory_alloc_cons(subobj, NIL));
    }
    else // IT'S A SYMBOL
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
    Cell* made = _recurse_sexps(inbuf, &idx);    
    
    if(made == NULL)
    {
        return memory_alloc_exception(TAG_SPEC_EX_DATA, memory_alloc_string("read: malformed sexps", -1));
    }
    else
    {
        return made;
    }
}

