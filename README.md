# thanks

It occurred to me that I'm Old and I've never implemented a Lisp before[^1]. How embarassing

Well, off to fix that. Two days later and here is that Lisp. It's a mess (first time jitters) but yet:

* Working lambdas
* Garbage collection
* Tagbodies
* Lexical scoping
* Closures (it's a [MAN](https://en.wikipedia.org/wiki/Man_or_boy_test) baby)
* Macros
* Backquote forms
* Reader macros
* &optional and &rest lambda arguments
* File I/O
* Usually doesn't segfault

~~The goal is to see how much I can get done before Thanksgiving, at which point I'll **throw it all away** and start over.~~

On a winning streak and didn't want to stop. New goal is christmas?

---

## a reference

Well fuck me I didn't think I'd make it this far. Guess I should write some documentation.

Not much of it mind you. 

* fundamental: `(t)` `(nil)` `(lambda)` `(read)` `(print)` `(eval)`
* math: `(+)` `(-)` `(*)` `(/)` `(mod)` 
* tests: `(==)` `(!=)` `(>)` `(>=)` `(<)` `(<=)`
* logic: `(and)` `(or)` `(eq)` `(eql)` `(null)` `(plusp)`
* lists: `(car)` `(cdr)` `(list)` `(quote)` `(append)` `(cons)`
* defines: `(def)` `(let)` `(nlet)` `(setq)` `(env-root)`
* structure: `(progn)`
* looping: `(tagbody)` `(go)` `(dotimes)`
* conditionals: `(cond)` `(if)` `(when)`
* macros: `(macro)` `(macroexpand)`
* io: `(open)` `(close)` `([py]print-to)` `(pyprint)` `*error-stream*`

Most of these work like they would in Common Lisp[^2]. Some notes however:

* `(pyprint)` is a poor man's `(format)` - it prints all of its arguments on a single line, and doesn't print quotation marks around lone strings. (Like Python's `print` which accepts w/e)
 ```
* (pyprint '(1 2 3) "is a list with car" (car '(1 2 3)) "and cdr" (cdr '(1 2 3)))
(1 2 3) is a list with car 1 and cdr (2 3) 
 ```
* `([py]print-to)` expect a stream [from `(open)`] as their first arguments. `*error-stream*` returns a stream for... well.
* `(macro)` works like `(lambda)` except it generates a macro expression. ~~No backtick yet so have fun using it.~~
* Other than byte arrays (e.g. strings), all the interpreter's data structures are built from cons cells. This lets you do fun things like `(map print (env-root))` which prints all definitions in the environment[^3].
* **All numbers are secretly doubles.**

---
 
[^1]: Actually I did once. It was written in Python and it used Python lists instead of conses to build Lisp lists. Let's pretend I didn't
[^2]: Read: strictly worse than they do in CL. 
[^3]: I don't know how rare this is, but it tickles me, so.
