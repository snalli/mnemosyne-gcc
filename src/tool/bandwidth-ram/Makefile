
CFLAGS= 
CC=gcc


bandwidth64:	main.c routines64.s
	nasm -f elf64 routines64.s -o routines64.o
	${CC} routines64.o main.c ${CFLAGS} -o bandwidth64

bandwidth32:	main.c routines32.s 
	nasm -f elf routines32.s -o routines32.o
	${CC} routines32.o main.c ${CFLAGS} -o bandwidth32

clean:
	rm -f bandwidth bandwidth32 bandwidth64 routines64.o routines32.o

