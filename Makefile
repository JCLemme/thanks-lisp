SHELL=/bin/bash

all:
	gcc -g -O0 -Wno-format -o thanks main.c memory.c frame.c util.c repl_read.c repl_print.c repl_eval.c

dbg:
	gcc -g -O0 -Wno-format -o thanks -DDEBUG main.c memory.c frame.c util.c repl_read.c repl_print.c repl_eval.c

fast:
	gcc -O3 -Wno-format -o thanks main.c memory.c frame.c util.c repl_read.c repl_print.c repl_eval.c

test: thanks
	@echo "Diffs reported:"
	@diff canon.test <(cat testcase.lisp | ./thanks)
