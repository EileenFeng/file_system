#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
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
	for(int i = 0; i < WRITEBYTE; i++) {
	  buffer[i] = 'a';
	}
	f_write((void*)buffer, WRITEBYTE, cur_file);
	bzero(buffer, WRITEBYTE);
	f_read((void*)buffer, WRITEBYTE, cur_file);
	int counter = 0;
	for(int i = 0; i < WRITEBYTE; i++) {
	  //printf("%d:  %c \t", i, buffer[i]);
	  if(buffer[i] != 'a') {
	    counter ++;
	  }
	  //assert(buffer[i] == 'a');
	  
	}
	
	f_close(cur_file);
	free(buffer);
	f_remove("home/feichi/large",0);
	printf("counter hehe %d\n", counter);
}
