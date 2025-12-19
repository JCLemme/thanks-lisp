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
	emcc -DUSE_SHIM -DON_WEB -O0 -g $(SRCFILES) shim.c -o web/test.mjs -s FORCE_FILESYSTEM -s ASYNCIFY -s ASYNCIFY_STACK_SIZE=32768  -s EXPORTED_RUNTIME_METHODS=FS --js-library=web/emscripten-pty.js --preload-file examples

test: thanks
	@echo "Diffs reported:"
	@diff canon.test <(cat testcase.lisp | ./thanks --testmode)

canonize-test: thanks
	cp canon.test canon.test.last
	cat testcase.lisp | ./thanks --testmode > canon.test
