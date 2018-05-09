CC = gcc
FLAGS = -Wall -g

all: libfilesys.so format testinit testmkdir

libfilesys.so: file_lib.o
	$(CC) -o libfilesys.so file_lib.o -shared

file_lib.o: file_lib.c file_lib.h
	$(CC) -c -fpic $(FLAGS) file_lib.c

format: format.c fs_struct.h
	$(CC) -o format $(FLAGS) format.c

testinit: test.c libfilesys.so
	$(CC) -o testinit $(FLAGS) test.c libfilesys.so

testmkdir: test_mkdir.c libfilesys.so
	$(CC) -o testmkdir $(FLAGS) test_mkdir.c libfilesys.so

clean:
	rm format testinit testmkdir file_lib.o libfilesys.so
