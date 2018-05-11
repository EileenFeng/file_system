#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "file_lib.h"
//#include "fs_struct.h"
#define WRITEBYTE 67840
//#define WRITEBYTE 512

int main() {
  f_mount("DISK");
  f_mkdir("home/hehe", RWE);
  int hehe = f_opendir("/home/hehe");
  int feichi = f_open("home/hehe/feichi.txt", OPEN_W, RWE);
  f_seek(hehe, 0, SEEKSET);
  int buffer[WRITEBYTE];
  for(int i = 0; i < WRITEBYTE; i++) {
    buffer[i] = i;
  }
  f_seek(feichi, 0, SEEKSET);
  int write_result = f_write((void*)buffer,WRITEBYTE * 4, feichi);
  printf(" write result %d\n", write_result);

  int result_buffer[WRITEBYTE];
  bzero(result_buffer, WRITEBYTE);
  f_rewind(feichi);

  f_read(result_buffer,WRITEBYTE * 4, feichi);
  for(int i = 0; i < WRITEBYTE; i++) {
    //    printf("read buffer %d: ", i);
    if(result_buffer[i] != i) {
      printf("read buffer %d differes: %d", i,result_buffer[i]);
    }
    assert(result_buffer[i] == i);
  }

  struct fst* st = malloc(sizeof(struct fst));
  f_stat(feichi, st);
  printf("\n\n stat has size %d\n", st->filesize);
  //f_closedir(hehe);
  f_close(feichi);
  f_closedir(hehe);
  f_rmdir("home/hehe");
  feichi = f_open("home/hehe/feichi.txt", OPEN_W, RWE);
  f_mkdir("home/hehe", RWE);
  feichi = f_open("home/hehe/feichi.txt", OPEN_W, RWE);
  write_result = f_write((void*)buffer,WRITEBYTE * 4, feichi);
  bzero(result_buffer, WRITEBYTE);
  f_rewind(feichi);
  f_read(result_buffer,WRITEBYTE * 4, feichi);
  for(int i = 0; i < WRITEBYTE; i++) {
    if(result_buffer[i] != i) {
      printf("read buffer %d differes: %d", i,result_buffer[i]);
    }
    assert(result_buffer[i] == i);
  }
  f_unmount();
  free(st);
}
