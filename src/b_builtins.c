#include "builtins.h"



void _define_lisp()
{
    // Reminder: these are evaluated *after* the C builtins.

    // Variables, etc. 
    frame_push_defn_in(&env_root, "*error-stream*", memory_alloc_stream(stderr));
    frame_push_defn_in(&env_root, "*reader-macros*", &reader_macros);

    // Quoting is handled with this disgusting macro. Note the use of "rplaca".
    _evaldef("(rplaca *reader-macros* (cons \"'\" (macro (x) (list (quote quote) (list (quote quote) x)))))");

    // Same idea, but for backquote.
    _evaldef("(nconc *reader-macros* (list (cons \"`\" (macro (x) (list 'quote (list 'backquote x))))))");
    _evaldef("(nconc *reader-macros* (list (cons \",\" (macro (x) (list 'quote (list 'comma x))))))");

    // Fun wrappers around primitive conditionals and loops.
    _evaldef("(def if (macro (cd ys no) `(cond (,cd ,ys) (t ,no)) ))");
    _evaldef("(def dotimes (macro (ct bd) `(let ((i 0)) (tagbody g_loop ,bd (setq i (+ i 1)) (cond ((< i ,ct) (go g_loop))))) ))");
    _evaldef("(def let-dotimes (macro (cv ct bd) `(let ((,cv 0)) (tagbody g_loop ,bd (setq ,cv (+ ,cv 1)) (cond ((< ,cv ,ct) (go g_loop))))) ))");
    _evaldef("(def while (macro (cd bd) `(tagbody g_loop ,bd (cond (,cd (go g_loop))))))");

    // Helper shims for benchmarks - might make it into the standard library, might not.
    _evaldef("(def plusp (lambda (qu) (> qu 0)))");
    _evaldef("(def first (lambda (qu) (car qu)))");
    _evaldef("(def second (lambda (qu) (car (cdr qu))))");
    _evaldef("(def rest (lambda (qu) (cdr qu)))");
    _evaldef("(def not (lambda (ts) (cond (ts nil) (t t))))");
    
    // Benchmarks to test speed with macros vs without.
    _evaldef("(def tak (lambda (x y z) (if (not (< y x)) z (tak (tak (- x 1) y z) (tak (- y 1) z x) (tak (- z 1) x y)))))");
    _evaldef("(def ttak (lambda (x y z) (cond ((not (< y x)) z) (t (ttak (ttak (- x 1) y z) (ttak (- y 1) z x) (ttak (- z 1) x y))))))");

}



// These are hyperspecific thanks functions and I don't know where they should go.
Cell* fn_garbage(Cell* args)
{
    int used = memory_gc();
    return memory_alloc_number(used);
}

Cell* fn_env_root(Cell* args)
{
    return &env_root;
}


Cell* fn_fast(Cell* args)
{
    Cell* form = memory_deep_copy(args->car);
    form->cdr = _hardlink_dynamic(form->cdr);
    return form;
}

void _define_c()
{
    frame_push_defn_in(&env_root, "t", T);
    frame_push_defn_in(&env_root, "nil", NIL);

    // See above...
    frame_push_defn_in(&env_root, "garbage", memory_alloc_builtin(fn_garbage, 0));
    frame_push_defn_in(&env_root, "env-root", memory_alloc_builtin(fn_env_root, 0));
    frame_push_defn_in(&env_root, "fast", memory_alloc_builtin(fn_fast, 0));
}

// ----- ----- ----- ----- -----
// You shouldn't touch anything
// under this line...
// ----- ----- ----- ----- -----

void _evaldef(char* code)
{
    Cell* res = _evaluate_sexp(_parse_sexps(code));
    if(IS_TYPE(res->tag, TAG_TYPE_EXCEPTION))
    {
        printf("Woops error in builtin def '%s'\n", code);
        printf("Unrecoverable, aborting.\n");
        abort();
    }
}

void builtins_init()
{
    _define_auto();
    _define_c();
    _define_lisp();
}


