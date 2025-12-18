#include "memory.h"

#ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif

// to ken
static Cell* mem = NULL;
static int num_cells = 0;

static Cell* free_top = NULL;
static int free_used = 0;
static int free_avail = 0;

static Cell* roots[16];
static int num_roots = 0;
static int rootshw = 0;

// A simple optimization. We throw these around as return values, so keep them allocated off the heap.
Cell nil_cell;
Cell true_cell;

Cell stex_interrupted = {TAG_MAGIC | TAG_STATIC | TAG_TYPE_EXCEPTION, &(Cell){TAG_MAGIC | TAG_STATIC | TAG_TYPE_STRING, "<>: interrupted", NIL}, NIL};

// TODO: i think I can save a mark() pass if symbols refer to the cons cell in this list
// or not since that would make holes...
Cell* sym_top = NULL;

void memory_init(int cells)
{
    num_cells = cells;
    mem = calloc(sizeof(Cell), num_cells + 2);
    free_top = mem;
    free_used = 0;
    free_avail = num_cells;

    D(printf("mem: allocated %d cells at addr %08x\n", num_cells, mem));

    for(int i = 0; i < num_cells; i++)
    {
        // Set the magic and leave everything else as zero.
        mem[i].tag = TAG_MAGIC | TAG_TYPE_CONS;
        mem[i].cdr = &(mem[i+1]);
    }

    // End of the line.
    mem[num_cells-1].cdr = NIL;

    // Preallocate cells for true and false.
    nil_cell.tag = TAG_MAGIC | TAG_STATIC | TAG_TYPE_CONS;
    nil_cell.car = NULL;
    nil_cell.cdr = NULL;

    memory_build_symbol(&true_cell, "t", -1);
    true_cell.tag |= TAG_STATIC;
}

int memory_get_used() { return free_used; }

Cell* memory_nth(Cell* begin, int place) 
{
    if(IS_NIL(begin) || place <= 0) return begin;
    return memory_nth(begin->cdr, place - 1);
}

int memory_length(Cell* begin)
{
    int acc = 0;
    while(!IS_NIL(begin))
    {
        acc++;
        begin = begin->cdr;
    }
    return acc;
}

Cell* memory_last(Cell* begin)
{
    Cell* last = begin;
    while(!IS_NIL(begin))
    {
        last = begin;
        begin = begin->cdr;
    }
    return last;
}

Cell* memory_single_copy(Cell* src)
{
    // WHAT FUCKING BUG IS THIS
    // HEISENBUG
    // FUCKING WASM POINTER TYPES SOMETHING GOD DAMNIT
    Cell* newc = memory_alloc_cons(src->car, src->cdr);
    newc->tag = src->tag;

    if(IS_TYPE(src->tag, TAG_TYPE_NUMBER))
    {
        // FINE JACKASS WE'LL DO IT YOUR WAY
        newc->num = src->num;
        newc->size = src->size;
    }

    // ok i've calmed down
    return newc;
}

Cell* memory_shallow_copy(Cell* begin)
{
    if(IS_NIL(begin)) { return NIL; }
    Cell* copy = NIL;
    Cell* save = NIL;

    while(!IS_NIL(begin))
    {
        copy = memory_extend(copy);
        if(IS_NIL(save)) { save = copy; }

        copy->car = begin->car;
        begin = begin->cdr;
    }

    return save;
}

Cell* memory_deep_copy(Cell* begin)
{
    if(IS_NIL(begin)) { return NIL; }

    if(IS_TYPE(begin->tag, TAG_TYPE_CONS))
    {
        Cell* copy = NIL;
        Cell* save = NIL;

        while(!IS_NIL(begin))
        {
            copy = memory_extend(copy);
            if(IS_NIL(save)) { save = copy; }

            copy->car = memory_deep_copy(begin->car);
            begin = begin->cdr;
        }

        return save;
    }
    else
    {
        return memory_single_copy(begin);
    }
}

Cell* memory_extend(Cell* last)
{
    if(IS_NIL(last))
    {
        return memory_alloc_cons(NULL, NIL);
    }
    else
    {
        Cell* newc = memory_alloc_cons(NULL, NIL);
        last->cdr = newc;
        return newc;
    }
}

static int lfree = 0;

Cell* memory_alloc_cons(Cell* ar, Cell* dr)
{
    if(IS_NIL(((Cell*)free_top->cdr)) || free_avail <= 0)
    {
        printf("mem: out of cells!\n");
        return NULL;
    }

    free_used++;
    free_avail--;
    Cell* next = free_top;
    free_top = next->cdr;

    // A valid, empty cons.
    next->tag = 0; // why isn't it set by calloc????!?
    next->tag = OF_TYPE(next->tag, TAG_TYPE_CONS);
    next->car = ar;
    next->cdr = dr;

    D(printf("memory: allocated cons of (%08x, %08x) in cell %08x\n", next->car, next->cdr, next));

    return next;
}

void memory_build_number(Cell* found, double value)
{
    // If the cell is invalid, we'll just segfault.
    found->num = value;
    found->tag = OF_TYPE(found->tag, TAG_TYPE_NUMBER);
    D(printf("memory: built number %f in cell %08x\n", found->num, found));
}

Cell* memory_alloc_number(double value)
{
    // If the cell is invalid, we'll just segfault.
    Cell* found = memory_alloc_cons(NIL, NIL);
    memory_build_number(found, value);
    return found;
}

void memory_build_string(Cell* found, char* src, int len)
{
    if(len < 0) { len = strlen(src); }
    found->size = len;

    if(len < 8)
    {
        found->data = 0;
        strncpy((char*)&(found->data), src, len);
        found->tag = OF_TYPE(found->tag, TAG_TYPE_PSTRING);
        D(printf("memory: built packed string '%s' in cell %08x\n", &(found->data), found));
    }
    else
    {
        // TODO: write our own allocator to avoid the malloc
        // likely will combine with the vector allocator
        found->car = malloc(len + 2);
        strncpy(found->car, src, len);
        found->tag = OF_TYPE(found->tag, TAG_TYPE_STRING);
        D(printf("memory: built string '%s' at addr %08x in cell %08x\n", found->car, found->car, found));
    }
}

Cell* memory_alloc_string(char* src, int len)
{
    Cell* found = memory_alloc_cons(NIL, NIL);
    memory_build_string(found, src, len);
    return found;
}

void memory_build_symbol(Cell* found, char* src, int len)
{
    // Walk the symbol table please.
    Cell* sym_current = sym_top;
    Cell* sym_last = sym_top;
    len = (len < 0) ? strlen(src) : len;

    found->tag = OF_TYPE(found->tag, TAG_TYPE_SYMBOL);
    D(printf("memory: building new symbol in cell %08x\n", found));
    D(printf("memory: looking for symstr '%.*s'...\n", len, src));

    while(sym_current != NULL)
    {
        // Attempt to match strings.
        Cell* str_current = sym_current->car;
        
        if(str_current->size == len)
        {
            if(str_current->tag == TAG_TYPE_PSTRING)
            {
                // It's packed.
                if(strncmp(src, (char*)&(str_current->car), len) == 0)
                {
                    D(printf("memory: ...found packed in cell %08x\n", str_current));
                    found->car = str_current;
                    return;
                }
            }
            else
            {
                // It's not packed.
                if(strncmp(src, str_current->car, len) == 0)
                {
                    D(printf("memory: ...found in cell %08x\n", str_current));
                    found->car = str_current;
                    return;
                }
            }
        }

        // Otherwise walk it.
        sym_last = sym_current;
        sym_current = sym_current->cdr;
    }

    // It wasn't found, so make it.
    Cell* new_str = memory_alloc_string(src, len);
    Cell* new_sym = memory_alloc_cons(new_str, NULL);

    if(sym_top == NULL)
        sym_top = new_sym;
    else
        sym_last->cdr = new_sym;

    found->car = new_str;
    D(printf("memory: ...new symstr created in cell %08x\n", new_sym));
}

Cell* memory_alloc_symbol(char* src)
{
    Cell* found = memory_alloc_cons(NIL, NIL);
    memory_build_symbol(found, src, -1);
    return found;
}

Cell* memory_alloc_symboln(char* src, int len)
{
    Cell* found = memory_alloc_cons(NIL, NIL);
    memory_build_symbol(found, src, len);
    return found;
}


void memory_build_lambda(Cell* found, Cell* args, Cell* body, int tags)
{
    found->tag = OF_TYPE(found->tag, TAG_TYPE_LAMBDA);
    found->tag |= tags;

    D(printf("memory: built new lambda in cell %08x%s%s\n", found, tags & TAG_SPEC_FUNLAZY ? " (lazy)" : "", tags & TAG_SPEC_FUNMACRO ? " (macro)" : ""));

    found->car = args;
    found->cdr = body;
}

Cell* memory_alloc_lambda(Cell* args, Cell* body, int tags)
{
    Cell* found = memory_alloc_cons(NIL, NIL);
    memory_build_lambda(found, args, body, tags);
    return found;
}

void memory_build_builtin(Cell* found, Cell* (*primfunc)(Cell*), int tags)
{
    found->tag = OF_TYPE(found->tag, TAG_TYPE_BUILTIN);
    found->tag |= tags;

    D(printf("memory: built new builtin in cell %08x%s%s\n", found, tags & TAG_SPEC_FUNLAZY ? " (lazy)" : "", tags & TAG_SPEC_FUNMACRO ? " (macro)" : ""));

    found->car = primfunc;
}

Cell* memory_alloc_builtin(Cell* (*primfunc)(Cell*), int tags)
{
    Cell* found = memory_alloc_cons(NIL, NIL);
    memory_build_builtin(found, primfunc, tags);
    return found;
}

void memory_build_exception(Cell* found, int kind, Cell* data)
{
    // TODO: replace with enumtype?
    found->tag = OF_TYPE(found->tag, TAG_TYPE_EXCEPTION);
    found->tag = OF_SPEC(found->tag, kind);
    found->car = data;
}

Cell* memory_alloc_exception(int kind, Cell* data)
{
    Cell* found = memory_alloc_cons(NIL, NIL);
    memory_build_exception(found, kind, data);
    return found;
}

void memory_build_hardlink(Cell* found, Cell* data)
{
    found->tag = OF_TYPE(found->tag, TAG_TYPE_HARDLINK);
    found->car = data;
}

Cell* memory_alloc_hardlink(Cell* data)
{
    Cell* found = memory_alloc_cons(NIL, NIL);
    memory_build_hardlink(found, data);
    return found;
}

void memory_build_stream(Cell* found, void* repr)
{
    found->tag = OF_TYPE(found->tag, TAG_TYPE_STREAM);
    found->car = repr;
}

Cell* memory_alloc_stream(void* repr)
{
    Cell* found = memory_alloc_cons(NIL, NIL);
    memory_build_stream(found, repr);
    return found;
}

// --- 

char* symbol_string_ptr(Cell* sym)
{
    if(!IS_TYPE(sym->tag, TAG_TYPE_SYMBOL)) { return NULL; }
    Cell* symstr = sym->car;
    if(IS_TYPE(symstr->tag, TAG_TYPE_PSTRING))
        return (char*)&(symstr->car);
    else
        return (char*)symstr->car;

}

char* string_ptr(Cell* str)
{
    if(IS_TYPE(str->tag, TAG_TYPE_PSTRING))
        return (char*)&(str->car);
    else if(IS_TYPE(str->tag, TAG_TYPE_STRING))
        return (char*)str->car;
    else
        return NULL;
}

bool symbol_is_keyword(Cell* sym)
{
    if(!IS_TYPE(sym->tag, TAG_TYPE_SYMBOL)) { return false; }
    char* str = symbol_string_ptr(sym);
    return (str[0] == ':');
}

bool symbol_matches(Cell* sym, char* name)
{
    if(!IS_TYPE(sym->tag, TAG_TYPE_SYMBOL)) { return false; }
    return (strcmp(symbol_string_ptr(sym), name) == 0);
}

// ---

void memory_free(Cell* target)
{
    // Manually free a cell we know we won't need again.
    // Dragons: you bet
    if(IS_NIL(target)) { return; }

    if(IS_TYPE(target->tag, TAG_TYPE_CONS))
    {
        memory_free(target->car);
        memory_free(target->cdr);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_LAMBDA))
    {
        memory_free(target->car);
        memory_free(target->cdr);
    }
    else if(IS_TYPE(target->tag, TAG_TYPE_STRING))
    {
        free(target->car);
    }

    target->tag = TAG_MAGIC | TAG_TYPE_CONS;
    target->car = NULL;
    target->cdr = free_top;
    free_top = target;
}

int memory_mark(Cell* begin)
{
    if(begin == NULL) { return 0; }

    // Reachable NIL cells still need to be marked.
    // Note the above case: a NULL NIL cell can't be marked...
    if(IS_NIL(begin)) 
    { 
        begin->tag |= TAG_MARKED;
        return 0; 
    }

    int found = 1;
    if(begin->tag & TAG_MARKED ) {

        return 0;
    }

    if(!(begin->tag & TAG_STATIC))
        begin->tag |= TAG_MARKED;

    if(IS_TYPE(begin->tag, TAG_TYPE_CONS))
    {
        // Cons point to other cells, so they need to be marked.
        found += memory_mark(begin->car);
        found += memory_mark(begin->cdr);
    }
    else if(IS_TYPE(begin->tag, TAG_TYPE_LAMBDA))
    {
        // Lambdas point to other cells too.
        found += memory_mark(begin->car);
        found += memory_mark(begin->cdr);
    }
    else if(IS_TYPE(begin->tag, TAG_TYPE_SYMBOL))
    {
        // Symbols have just one cell reference.
        found += memory_mark(begin->car);
    }
    else if(IS_TYPE(begin->tag, TAG_TYPE_EXCEPTION))
    {
        // Exceptions have one also.
        found += memory_mark(begin->car);
    }
    else if(IS_TYPE(begin->tag, TAG_TYPE_HARDLINK))
    {
        // Same for hardlinks.
        found += memory_mark(begin->car);
    }

    return found;
}

int memory_sweep()
{
    Cell* new_free_list = NULL;
    Cell* this_free_list = NULL;
    int new_avail = 0;
    int new_used = 0; // idk why I keep separating these, they're derivable from each other

    D(printf("memory: using %d cells pre-sweep\n", free_used));

    for(int i = 0; i < num_cells; i++)
    {
        Cell* this_cell = &(mem[i]);
        if(this_cell->tag & TAG_MARKED)
        {
            // Safe
            this_cell->tag &= ~TAG_MARKED;
            new_used++;
        }
        else
        {
            // Out
            if(IS_TYPE(this_cell->tag, TAG_TYPE_STRING))
                free(this_cell->car);

            this_cell->car = NULL;
            this_cell->cdr = NULL;
            this_cell->tag = TAG_MAGIC | TAG_TYPE_CONS;

            if(new_free_list == NULL)
            {
                this_free_list = this_cell;
                new_free_list = this_free_list;
            }
            else
            {
                this_free_list->cdr = this_cell;
                this_free_list = this_cell;
            }

            new_avail++;
        }
    }

    D(printf("memory: sweep found %d used, now %d avail\n", new_used, new_avail));
    free_avail = new_avail;
    free_used = new_used;
    free_top = new_free_list;

    return free_used;
}

void memory_add_root(Cell* nroot)
{
    for(int i = 0; i < sizeof(roots)/sizeof(roots[0]); i++)
    {
        if(roots[i] == NULL)
        {
            roots[i] = nroot;
            num_roots++;
            
            if(num_roots > rootshw)
                rootshw = num_roots;

            return;
        }
    }
}

void memory_del_root(Cell* nroot)
{
    for(int i = 0; i < sizeof(roots)/sizeof(roots[0]); i++)
    {
        if(roots[i] == nroot)
        {
            roots[i] = NULL;
            num_roots--;
            return;
        }
    }
}

int memory_gc()
{
    int marked = 0;

    for(int i = 0; i < sizeof(roots)/sizeof(roots[0]); i++)
    {
        if(roots[i] != NULL)
            marked += memory_mark(roots[i]);
    }

    D(printf("memory: gc: marked %d cells from roots\n", marked));
    return memory_sweep();
}

int memory_gc_thresh(double pct)
{
    int too_many = (double)num_cells * pct;
    int used = 0;

    if(free_used > too_many)
    {
        used = memory_gc();
    }

    if(free_used > too_many)
    {
        printf("memory: can't gc our way out of this one\n");
        abort();
    }

    return used;
}
