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
    mem[num_cells-1].cdr = NULL;
}

int memory_get_used() { return free_used; }

Cell* memory_alloc_cons(Cell* ar, Cell* dr)
{
    if(free_top->cdr == NULL || free_avail <= 0)
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
    Cell* found = memory_alloc_cons(NULL, NULL);
    memory_build_number(found, value);
    return found;
}

void memory_build_string(Cell* found, char* src)
{
    int len = strlen(src);
    found->size = len;

    if(len < 8)
    {
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

Cell* memory_alloc_string(char* src)
{
    Cell* found = memory_alloc_cons(NULL, NULL);
    memory_build_string(found, src);
    return found;
}

void memory_build_symbol(Cell* found, char* src)
{
    // Walk the symbol table please.
    Cell* sym_current = sym_top;
    Cell* sym_last = sym_top;

    found->tag = OF_TYPE(found->tag, TAG_TYPE_SYMBOL);
    D(printf("memory: building new symbol in cell %08x\n", found));
    D(printf("memory: looking for symstr '%s'...\n", src));

    while(sym_current != NULL)
    {
        // Attempt to match strings.
        Cell* str_current = sym_current->car;
        if(str_current->tag == TAG_TYPE_PSTRING)
        {
            // It's packed.
            if(strncmp(src, (char*)&(str_current->car), 7) == 0)
            {
                D(printf("memory: ...found packed in cell %08x\n", str_current));
                found->car = str_current;
                return;
            }
        }
        else
        {
            // It's not packed.
            if(strcmp(src, str_current->car) == 0)
            {
                D(printf("memory: ...found in cell %08x\n", str_current));
                found->car = str_current;
                return;
            }
        }

        // Otherwise walk it.
        sym_last = sym_current;
        sym_current = sym_current->cdr;
    }

    // It wasn't found, so make it.
    Cell* new_str = memory_alloc_string(src);
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
    Cell* found = memory_alloc_cons(NULL, NULL);
    memory_build_symbol(found, src);
    return found;
}


void memory_build_lambda(Cell* found, Cell* args, Cell* body, bool lazy)
{
    found->tag = OF_TYPE(found->tag, TAG_TYPE_LAMBDA);
    if(lazy) found->tag |= TAG_LAZY;

    D(printf("memory: built new lambda in cell %08x%s\n", found, lazy ? " (lazy)" : ""));

    found->car = args;
    found->cdr = body;
}

Cell* memory_alloc_lambda(Cell* args, Cell* body, bool lazy)
{
    Cell* found = memory_alloc_cons(NULL, NULL);
    memory_build_lambda(found, args, body, lazy);
    return found;
}

Cell* memory_build_builtin(Cell* found, Cell* (*primfunc)(Cell*), bool lazy)
{
    found->tag = OF_TYPE(found->tag, TAG_TYPE_BUILTIN);
    if(lazy) found->tag |= TAG_LAZY;

    D(printf("memory: built new builtin in cell %08x%s\n", found, lazy ? " (lazy)" : ""));

    found->car = primfunc;
    return found;
}

Cell* memory_alloc_builtin(Cell* (*primfunc)(Cell*), bool lazy)
{
    Cell* found = memory_alloc_cons(NULL, NULL);
    memory_build_builtin(found, primfunc, lazy);
    return found;
}


int memory_mark(Cell* begin)
{
    if(begin == NULL) { return 0; }

    int found = 1;
    begin->tag |= TAG_MARKED;

    if(IS_TYPE(begin->tag, TAG_TYPE_CONS))
    {
        // Cons point to other cells, so they need to be marked.
        found += memory_mark(begin->car);
        found += memory_mark(begin->cdr);
    }
    if(IS_TYPE(begin->tag, TAG_TYPE_LAMBDA))
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

    return found;
}

int memory_sweep()
{
    Cell* new_free_list = NULL;
    Cell* this_free_list = NULL;
    int new_avail = 0;
    int new_used = 0; // idk why I keep separating these, they're derivable from each other

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
            this_cell->tag = TAG_MAGIC;

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

    return free_avail + free_used;
}
