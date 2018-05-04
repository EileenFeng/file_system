CC = gcc
FLAGS = -Wall -g

format: format.c fs_struct.h
	$(CC) -o format $(FLAGS) format.c

clean:
	rm format
