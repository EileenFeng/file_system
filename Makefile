CC = gcc
FLAGS = -Wall -g

all: format testinit

format: format.c fs_struct.h
	$(CC) -o format $(FLAGS) format.c

testinit: file_lib.c fs_struct.h
	$(CC) -o testinit $(FLAGS) file_lib.c

clean:
	rm format testinit
