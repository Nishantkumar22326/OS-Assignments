all:
	gcc  -g -m32 -no-pie -nostdlib -o fib fib.c
	gcc  -g -m32 -no-pie -nostdlib -o sum sum.c
	gcc -g -m32 -o loader loader.c 

clean:
	-@rm -f fib sum loader
