#include <stdlib.h>
#include <stdio.h>
#include "file_lib.h"
#include "fs_struct.h"
#define WRITEBLOCK 17000
#define WRITEBYTE 17000*512
int main() {
	f_mount("DISK");
	f_mkdir("home/feichi", RWE);
	int cur_file = f_open("home/feichi/large", OPEN_W, RWE);
	f_seek(cur_file, 0, SEEKSET);
	char* buffer=malloc(WRITEBYTE);
	bzero(buffer, WRITEBYTE);
	f_write((void*)buffer, WRITEBYTE, cur_file);
	f_close(cur_file);
	f_remove("home/feichi/large",0);
}
