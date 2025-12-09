#include "repl.h"

void _print_sexps(Cell* target)
{
    if(IS_NIL(target)) { printf("()"); return; }

    if(IS_TYPE(target->tag, TAG_TYPE_CONS))
    {
        printf("(");
        while(!IS_NIL(target))
        {
            // Handle conses
            if(!IS_TYPE(target->tag, TAG_TYPE_CONS))
            {
                printf(". ");
                _print_sexps(target);
                printf(" ");
                target = NULL;
            }
            else
            {
                _print_sexps(target->car);
                printf(" ");
                target = target->cdr;
            }
        }
        printf("\b)");
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_NUMBER))
    {
        print_double(target->num);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_LAMBDA))
    {
        //printf("<lambda at %08x>", target->car);
        printf("(<lambda> ");
        _print_sexps(target->car);
        printf(" ");
        _print_sexps(target->cdr);
        printf(")");
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_SYMBOL))
    {
        Cell* symstr = target->car;
        // TODO: auto type crushing for strings
        if(IS_TYPE(symstr->tag, TAG_TYPE_PSTRING))
            printf("%s", (char*)&(symstr->car));
        else
            printf("%s", (char*)symstr->car);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_PSTRING))
    {
        printf("\"%s\"", (char*)&(target->car));
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_STRING))
    {
        printf("\"%s\"", (char*)target->car);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_ARRAY))
    {
        printf("<array at %08x>", target->car);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_BUILTIN))
    {
        printf("<builtin at %08x>", target->car);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_EXCEPTION))
    {
        printf("Exception (type %d) in %s", ((target->tag & TAG_SPEC_MASK) >> 8), ((Cell*)target->car)->car);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_HARDLINK))
    {
        printf("[->");
        _print_sexps(target->car);
        printf("]");
    }
    else
    {
        printf("<unknown type %d at %08x>", target->tag, target);
    }
}


