all:
	gcc -g -O0 -Wno-format -o thanks main.c memory.c util.c 

dbg:
	gcc -g -O0 -Wno-format -o thanks -DDEBUG main.c memory.c util.c
