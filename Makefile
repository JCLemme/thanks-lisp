SHELL=/bin/bash
CFLAGS=-I/opt/homebrew/opt/readline/include -Wno-format -DUSE_READLINE
LDFLAGS=-L/opt/homebrew/opt/readline/lib -lreadline

all:
	gcc -g -O0 $(CFLAGS) -o thanks main.c memory.c frame.c util.c repl_read.c repl_print.c repl_eval.c $(LDFLAGS)

dbg:
	gcc -g -O0 $(CFLAGS) -o thanks -DDEBUG main.c memory.c frame.c util.c repl_read.c repl_print.c repl_eval.c $(LDFLAGS)

fast:
	gcc -O3 $(CFLAGS) -o thanks main.c memory.c frame.c util.c repl_read.c repl_print.c repl_eval.c $(LDFLAGS)

webrun:
	emcc -DUSE_SHIM -O3 frame.c main.c memory.c repl_eval.c repl_print.c repl_read.c util.c shim.c -o web/test.mjs -s FORCE_FILESYSTEM -s ASYNCIFY --js-library=web/emscripten-pty.js

test: thanks
	@echo "Diffs reported:"
	@diff canon.test <(cat testcase.lisp | ./thanks)
