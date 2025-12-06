SHELL=/bin/bash

all:
	gcc -g -O0 -Wno-format -o thanks main.c memory.c frame.c util.c 

dbg:
	gcc -g -O0 -Wno-format -o thanks -DDEBUG main.c memory.c frame.c util.c

fast:
	gcc -O3 -Wno-format -o thanks main.c memory.c frame.c util.c

test: thanks
	@echo "Diffs reported:"
	@diff canon.test <(cat testcase.lisp | ./thanks)
