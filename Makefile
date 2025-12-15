SHELL=/bin/bash
CFLAGS=-I/opt/homebrew/opt/readline/include -Wno-format -DUSE_READLINE
LDFLAGS=-L/opt/homebrew/opt/readline/lib -lreadline

SRCFILES=main.c memory.c frame.c util.c repl_read.c repl_print.c repl_eval.c

all:
	gcc -g -O0 $(CFLAGS) -o thanks $(SRCFILES) $(LDFLAGS)

dbg:
	gcc -g -O0 $(CFLAGS) -o thanks -DDEBUG $(SRCFILES) $(LDFLAGS)

fast:
	gcc -O3 $(CFLAGS) -o thanks $(SRCFILES) $(LDFLAGS)

webrun:
	emcc -DUSE_SHIM -O0 -g $(SRCFILES) shim.c -o web/test.mjs -s FORCE_FILESYSTEM -s ASYNCIFY --js-library=web/emscripten-pty.js

test: thanks
	@echo "Diffs reported:"
	@diff canon.test <(cat testcase.lisp | ./thanks --testmode)

canonize-test: thanks
	cp canon.test canon.test.last
	cat testcase.lisp | ./thanks --testmode > canon.test
