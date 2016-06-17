# Makefile for asgn4
#
# Vrindavan Harrison
# 
# CS111, Spring
# ASGn4 Makefile

GCC= cc -g -O0 -Wall -Wextra -std=c99 -o
LINK= cc -g -O0 -Wall -Wextra -std=c99 -c -o

SOURCES = fs_main.c fat_fs.c
HEADERS = fat_fs.h
FS_FILE= fs.fat
FS_DIR= mountdir

ALL_SRC= ${HEADERS} ${SOURCES}
OBJECTS = ${SOURCES:.c=.o} 

EXECBIN = fat_fs
FLAGS = -I


fuse: 
	cc -D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse -pthread -L/usr/local/lib -lfuse -lrt fs_main.c fat_fs.c -o fat_fs 

all : ${EXECBIN}

${EXECBIN}:  ${OBJECTS}
	${GCC} ${EXECBIN} ${OBJECTS} 	
	- rm ${OBJECTS}  

%.o : %.c
	${LINK} $@ $< 

run: 
	./${EXECBIN} ${FS_DIR}
	


clean : 
	- rm ${OBJECTS} *~ 

spotless : clean
	- rm ${EXECBIN}	${FS_FILE} *.core




