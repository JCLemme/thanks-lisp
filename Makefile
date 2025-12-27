SHELL=/bin/bash
HOST=$(shell uname -s)

CFLAGS=-Wno-format -DUSE_READLINE
LDFLAGS=-lreadline

ifeq ($(HOST),Darwin)
	CFLAGS += -I/opt/homebrew/opt/readline/include
	LDFLAGS += -L/opt/homebrew/opt/readline/lib
endif

SRCFILES=src/main.c src/memory.c src/frame.c src/repl_read.c src/repl_print.c src/repl_eval.c \
		 src/b_math.c src/b_io.c src/b_lisp.c src/b_string.c src/b_builtins.c src/b_auto.c

all:
	gcc -g -O0 $(CFLAGS) -o thanks $(SRCFILES) $(LDFLAGS)

dbg:
	gcc -g -O0 $(CFLAGS) -o thanks -DDEBUG $(SRCFILES) $(LDFLAGS)

fast:
	gcc -O3 $(CFLAGS) -o thanks $(SRCFILES) $(LDFLAGS)

webrun:
	emcc -DUSE_SHIM -DON_WEB -O0 -g $(SRCFILES) src/shim.c -o web/thanks.mjs -s FORCE_FILESYSTEM -s ASYNCIFY -s ASYNCIFY_STACK_SIZE=32768 -s EXPORTED_FUNCTIONS=_main,_inthandler -s EXPORTED_RUNTIME_METHODS=["FS"] --js-library=web/emscripten-pty.js --embed-file examples

test: thanks
	@echo "Diffs reported:"
	@diff test/canon.test <(cat test/testcase.lisp | ./thanks --testmode)

canonize-test: thanks
	cp test/canon.test test/canon.test.last
	cat test/testcase.lisp | ./thanks --testmode > test/canon.test


clean:
	rm -r *.dSYM web/*.data web/*.mjs web/*.wasm thanks 
