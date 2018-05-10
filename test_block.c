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
		sprintf(input, "home/feichi/%d", i);
		printf("+++++++++creating file %s \n", input);
		f_open(input, OPEN_W, RWE);
		printf("---------file %s created? \n", input);
	}
	for (int i = 0; i < 40; i++) {
		char input[25];
		sprintf(input, "home/feichi/%d", i);
		printf("+++++++++remove file %s \n", input);
		f_remove(input, 0);
		printf("---------file %s removed? \n", input);
	}
}
