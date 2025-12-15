#include "frame.h"

// realizing i could have made the global environment a namespace of its own
// actually how many favors would that do me really
// todid

Cell env_top;
Cell env_root;

Cell temp_root;

Cell* frame_push_in(Cell* pkg, Cell* def)
{
    if(IS_TYPE(((Cell*)def->car)->tag, TAG_TYPE_SYMBOL) && symbol_is_keyword(def->car)) 
        return memory_alloc_exception(TAG_SPEC_EX_DATA, memory_alloc_string("<internal>: can't define a keyword", -1));

    Cell* new_entry = memory_alloc_cons(def, pkg->cdr);
    pkg->cdr = new_entry;
    return new_entry;
}

Cell* frame_push_def(Cell* name, Cell* value)
{
    // A definition is a list: first item is a symbol, second is *something*.
    Cell* new_def = memory_alloc_cons(name, value);
    return frame_push_in(&env_top, new_def);
}

Cell* frame_push_defn(char* name, Cell* value)
{
    Cell* new_sym = memory_alloc_symbol(name);
    return frame_push_def(new_sym, value);
}

Cell* frame_push_def_in(Cell* pkg, Cell* name, Cell* value)
{
    // The pkg pointer is the cons cell in the frame stack that points to the package symbol.
    // Wewlad
    Cell* new_def = memory_alloc_cons(name, value);
    return frame_push_in(pkg, new_def);
}

Cell* frame_push_defn_in(Cell* pkg, char* name, Cell* value)
{
    Cell* new_sym = memory_alloc_symbol(name);
    return frame_push_def_in(pkg, new_sym, value);
}

Cell* frame_pop_in(Cell* pkg, int count)
{
    Cell* walker = pkg->cdr;
    while(!IS_NIL(walker) && (count-- > 0))
    {
        walker = walker->cdr;
    }

    if(count > 0)
        return memory_alloc_exception(TAG_SPEC_EX_DATA, memory_alloc_string("<internal>: tried to pop too many times", -1));

    pkg->cdr = walker;
    return walker;
}

Cell* frame_pop(int count)
{
    return frame_pop_in(&env_top, count);
}

Cell* frame_make_package_in(Cell* pkg, Cell* name)
{
    return frame_push_in(pkg, name);
}

Cell* frame_make_package(Cell* name)
{
    return frame_make_package_in(&env_top, name);
}

Cell* frame_make_packagen(char* name)
{
    Cell* new_name = memory_alloc_symbol(name);
    return frame_make_package(new_name);
}

Cell* frame_make_packagen_in(Cell* pkg, char* name)
{
    Cell* new_name = memory_alloc_symbol(name);
    return frame_make_package_in(pkg, new_name);
}

Cell* frame_free_package(Cell* pkg)
{
    // Remove the namespace and all definitions within.
    // Somewhat complex. Needs to walk so it can patch the hole in the linked list.

    // Not writing it yet...
    printf("frame: why did you call free_package?\n");
    abort();
}

// Note: "in" searches only within the bounds of a package
//       "from" searches the whole env starting from a package

Cell* frame_find_def_in(Cell* pkg, Cell* name)
{
    if(symbol_is_keyword(name)) 
        return memory_alloc_cons(name, name);

    Cell* walker = pkg->cdr;
    while(!IS_NIL(walker))
    {
        Cell* this_def = walker->car;

        if(IS_TYPE(this_def->tag, TAG_TYPE_CONS))
        {
            Cell* this_name = this_def->car;
            if(this_name->car == name->car)
                return this_def;
        }
        else
        {
            break;
        }

        walker = walker->cdr;
    }
    return NULL;
    //return memory_alloc_exception(TAG_SPEC_EX_DATA, memory_alloc_string("<internal>: couldn't find definition for symbol", -1));
}

Cell* frame_find_defn_in(Cell* pkg, char* name)
{
    Cell* new_name = memory_alloc_symbol(name);
    return frame_find_def_in(pkg, new_name);
}


Cell* frame_find_def_from(Cell* pkg, Cell* name)
{
    if(symbol_is_keyword(name)) 
        return memory_alloc_cons(name, name);

    Cell* walker = pkg->cdr;
    while(!IS_NIL(walker))
    {
        Cell* this_def = walker->car;

        if(IS_TYPE(this_def->tag, TAG_TYPE_CONS))
        {
            Cell* this_name = this_def->car;
            if(this_name->car == name->car)
                return this_def;
        }

        walker = walker->cdr;
    }
    return NULL;
    //return memory_alloc_exception(TAG_SPEC_EX_DATA, memory_alloc_string("<internal>: couldn't find definition for symbol", -1));
}

Cell* frame_find_defn_from(Cell* pkg, char* name)
{
    Cell* new_name = memory_alloc_symbol(name);
    return frame_find_def_in(pkg, new_name);
}

Cell* frame_find_def(Cell* name)
{
    return frame_find_def_from(&env_top, name);
}

Cell* frame_find_defn(char* name)
{
    Cell* new_name = memory_alloc_symbol(name);
    return frame_find_def_from(&env_top, new_name);
}

Cell* frame_find_package(Cell* name)
{
    Cell* walker = &env_top;
    while(!IS_NIL(walker))
    {
        Cell* this_def = walker->car;

        if(IS_TYPE(this_def->tag, TAG_TYPE_SYMBOL) && this_def->car == name->car)
            return walker;

        walker = walker->cdr;
    }
    return memory_alloc_exception(TAG_SPEC_EX_DATA, memory_alloc_string("<internal>: couldn't find package", -1));
}

Cell* frame_find_packagen(char* name)
{
    Cell* new_name = memory_alloc_symbol(name);
    return frame_find_package(new_name);
}

Cell* frame_set_def(Cell* name, Cell* nvalue)
{
    Cell* found_def = frame_find_def(name);
    if(!IS_TYPE(found_def->tag, TAG_TYPE_EXCEPTION))
        found_def->cdr = nvalue;
    return found_def;
}

Cell* frame_set_defn(char* name, Cell* nvalue)
{
    Cell* new_name = memory_alloc_symbol(name);
    return frame_set_def(new_name, nvalue);
}

Cell* frame_init()
{
    // Must build the system.
    env_root.tag = TAG_MAGIC | TAG_TYPE_CONS;
    env_root.car = memory_alloc_symbol(":root");
    env_root.cdr = NIL;

    env_top.tag = TAG_MAGIC | TAG_TYPE_CONS;
    env_top.car = memory_alloc_symbol(":top");
    env_top.cdr = &env_root;
    
    temp_root.tag = TAG_MAGIC | TAG_TYPE_CONS;
    temp_root.car = memory_alloc_symbol(":temp");
    temp_root.cdr = NIL;

    return &env_top;
}
