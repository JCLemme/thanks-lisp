#include "repl.h"

char scratch[256];

Cell reader_macros = {TAG_MAGIC | TAG_STATIC | TAG_TYPE_CONS, NIL, NIL};


bool _skip_whitespace(char* buffer, int* index)
{
    // Fast-forwards a character stream past whitespace.
    int found;
    bool in_comment = false;

    while(1)
    {
        found = buffer[(*index)];
        if(found == '\0' || found == EOF) { return false; }

        if(!in_comment)
        {
            // Skip comments too.
            if(found == ';') { in_comment = true; }
            else if(found != ' ' && found != '\t' && found != '\n') { return true; }
        }
        else
        {
            if(found == '\n') { in_comment = false; }
        }

        (*index)++;
    }

    // The compiler recognizes that this function will always return a value... 
    // ...but imma force it anyway
    return false;
}

int _steal_token(char* buffer, int* index)
{
    // Grabs a token from the input stream. A token is a collection of 
    // not-whitespace characters, i.e. symbols. (But not comments!)
    if(!_skip_whitespace(buffer, index)) { return -1; }

    int found;
    int len = 0;
    bool in_string = false;
    bool in_comment = false;

    while(1)
    {
        found = buffer[(*index)+(len)];
        if(found == '\0' || found == EOF) { return len; }

        if(!in_string)
        {
            if(found == '(') { return 1; } 
            else if(found == ')') { return (len == 0 || len == 1) ? 1 : len; }
            else if(found == '"') { in_string = true; }
            else if(found == ' ' || found == '\t' || found == '\n') { return len; }
        }
        else
        {
            if(found == '"') { in_string = false; }
        }

        len++;
    }
}

// I was just gonna return a "1" to mark list ends, but sense got the better of me
static Cell list_end_marker;

Cell* _walk_sexps(char* buffer, int* index)
{
    int toklen = _steal_token(buffer, index);

    // Stream ended. Hopefully we were OK with that.
    if(toklen == -1 || toklen == 0) { return NULL; }

    // Let's think.
    if(toklen == 1 && buffer[(*index)] == ')')
    {
        // It's the end of a list - report up.
        (*index) += toklen;
        return &list_end_marker;
    }
    else if(toklen == 1 && buffer[(*index)] == '(')
    {
        // It's a new list.
        (*index) += toklen;
        Cell* building = NIL;
        Cell* built = NIL;

        Cell* next = _walk_sexps(buffer, index);

        while(next != &list_end_marker)
        {
            if(next == NULL) { return NULL; }
            building = memory_extend(building);
            if(built == NIL) { built = building; }

            building->car = next;
            next = _walk_sexps(buffer, index);
        }

        return built;
    }
    else
    {
        // It's an atom, which gives us more Thinking to do.

        // Run any reader macros...
        Cell* rm_walker = &reader_macros;
        while(!IS_NIL(rm_walker))
        {
            if(rm_walker->car == NIL) { break; } // Ugh
            Cell* mpair = rm_walker->car;
            int mlen = strlen(string_ptr(mpair->car));
            
            // Compare prefix.
            if(strncmp(string_ptr(mpair->car), buffer+(*index), mlen) == 0)
            {
                // Eat the token following.
                (*index) += mlen;
                Cell* todo = _walk_sexps(buffer, index);
                if(todo == NULL) { return NULL; }

                // (Oh god) evaluate it to extract the juicy macro inside
                // TODO: the garbage collector will fucking EXPLODE if you try something here
                Cell runop = {TAG_MAGIC | TAG_STATIC | TAG_TYPE_CONS, mpair->cdr, &(Cell){TAG_MAGIC | TAG_STATIC | TAG_TYPE_CONS, todo, NIL}};
                return _evaluate_sexp(&runop);
            }

            rm_walker = rm_walker->cdr;
        }

        // ...then look at default types.
        if(buffer[(*index)] == '"')
        {
            // It's a string.
            int origidx = (*index);
            (*index) += toklen;
            return memory_alloc_string(buffer+origidx+1, toklen-2); // <- math here discounts the quotation marks
        }
        else
        {
            // Might be a number, might be a symbol...
            // TODO: please no scratch buffers - need to rewrite atof()
            bool a_number = true;
            bool got_digit = false;
            bool got_sign = false;
            int ic = 0;

            // A non-number character makes a symbol, unless it's just a plus or minus.
            for(ic = 0; ic < toklen; ic++)
            {
                char c = buffer[(*index)+ic];
                scratch[ic] = c;
                if(c == '+' || c == '-') 
                { 
                    if(got_sign || got_digit) { a_number = false; }
                    else { got_sign = true; }
                }
                else if(!isdigit(c) && c != '.') 
                {
                    a_number = false; 
                }
                got_digit = true;
            }
            scratch[ic] = '\0';
            if(got_sign && toklen == 1) { a_number = false; }

            int origidx = (*index);
            (*index) += toklen;

            if(a_number) { return memory_alloc_number(atof(scratch)); }
            else { return memory_alloc_symboln(buffer+origidx, toklen); }
        }
    }
}

/*
 * to be removed 
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

            //if(c == '\0' || c == EOF) { return NULL; }
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
*/

Cell* _parse_sexps(char* inbuf)
{
    int idx = 0;
    Cell* made = _walk_sexps(inbuf, &idx);    
    
    if(made == NULL || made == &list_end_marker)
    {
        return memory_alloc_exception(TAG_SPEC_EX_DATA, memory_alloc_string("read: malformed sexps", -1));
    }
    else
    {
        return made;
    }
}

