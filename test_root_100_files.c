#include <stdlib.h>
#include <stdio.h>
#include "file_lib.h"
#include "fs_struct.h"
#define WRITEBYTE 17635

int main() {
	f_mount("DISK");
	f_mkdir("feichi", RWE);
	for (int i = 0; i < 80; i++) {
		char input[20];
		sprintf(input, "feichi/%d", i);
		int result = f_mkdir(input, RWE);
	}
	//result = f_opendir("home/rusr");
	//int new_fd = f_open("home/rusr/haha.txt", OPEN_W);
	//struct dirent* rusrdir = f_readdir(result);
	//f_close(new_fd);
	//int newnew_fd = f_open("home/rusr/haha.txt", OPEN_W);
	//  int again = f_open("home/rusr/hahaha.txt", "");
}
