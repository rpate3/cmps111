
#Got help in Karthik's lab to format Makefile

CC = cc
CFLAGS = -g -Wall
TARGET = myshell

make:
	flex shell.l
	cc -c lex.yy.c
	cc -c myshell.c
	cc -o myshell myshell.o lex.yy.o -lfl

clean:
	rm myshell.o lex.yy.o lex.yy.c
