all: shell.c
	gcc shell.c -o shell
all-GDB: shell.c
	gcc -g shell.c -o shell