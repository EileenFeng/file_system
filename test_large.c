#include <stdlib.h>
#include <stdio.h>
#include "file_lib.h"
#include "fs_struct.h"
#define WRITEBLOCK 17000
#define WRITEBYTE 17000*512
int main() {
	srand(970409);
	f_mount("DISK");
	f_mkdir("home/feichi", RWE);
	int files[50];
	for (int i = 0; i < 50; i++) {
		files[i] = -1;
	}
	for (int i = 0; i < 40; i++) {
		char input[25];
		sprintf(input, "home/feichi/%d", rand()%40+1);
		printf("+++++++++creating file %s \n", input);
		files[rand()%40+1] = f_open(input, OPEN_W, RWE);
		printf("---------file %s created? \n", input);
	}
	int cur_file = files[4];
	f_seek(cur_file, 0, SEEKSET);
	char* buffer=malloc(WRITEBYTE);
	bzero(buffer, WRITEBYTE);
	f_write((void*)buffer, WRITEBYTE, cur_file);
	f_close(cur_file);
	//f_remove("home/feichi/4",0);
	srand(970409);
	for (int i = 1; i < 41; i++) {
		char input[25];
		sprintf(input, "home/feichi/%d", rand()%40+1);
		f_remove(input, 0);
	}
}
