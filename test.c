#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "file_lib.h"
#include "fs_struct.h"
#define WRITEBYTE 167840
//#define WRITEBYTE 512

int main() {
  f_mount("DISK");

  //printf("readdir testing\n");
  //f_readdir(0);
  //f_readdir(0);
  printf("_____________ rewind :\n");
  f_rewind(0);
  printf("----------open dir:\n");
  int result = f_opendir("home/rusr");
  printf("1111111: open dir result is %d\n\n\n", result);
  printf("+===================================================");
  result = f_opendir("home/rusr");
  printf("222222222: open dir result is %d\n", result);
  printf("\n\n\n");
  printf("________________________ f-OPEN ___________________\n\n");
  int new_fd = f_open("home/rusr/haha.txt", OPEN_W, RWE);
  printf("_______end of fopen______________\n");
  printf("new file fd is %d\n", new_fd);
  printf("\n\n_______________________read new file dir\n\n");
  //  int again = f_open("home/rusr/hahaha.txt", "");
  //printf("again %d\n", again);
  struct dirent* rusrdir = f_readdir(result);
  printf("should: %s\n", rusrdir->filename);
  printf("\n\n______ f_close _________\n");
  f_close(new_fd);

  // second open


  printf("________________________ f-OPEN 2222 ___________________\n\n");
  int newnew_fd = f_open("home/rusr/haha.txt", OPEN_W, RWE);
  printf("_______end of fopen______________\n");
  printf("new file fd is %d\n", newnew_fd);
  printf("\n\n_______________________read new file dir\n\n");
  assert(newnew_fd == new_fd);
  //  int again = f_open("home/rusr/hahaha.txt", "");
  //printf("again %d\n", again);
  f_seek(result, 0, SEEKSET);
  rusrdir = f_readdir(result);
  printf("should: %s\n", rusrdir->filename);
  assert(strcmp(rusrdir->filename, "haha.txt") == SUCCESS);
  int buffer[WRITEBYTE];
  for(int i = 0; i < WRITEBYTE; i++) {
    buffer[i] = i;
    //printf("%d ", i);
  }
  f_seek(newnew_fd, 0, SEEKSET);
  printf("\n\n\n____________========= f_write -_________________-\n");
  int write_result = f_write((void*)buffer,WRITEBYTE * 4, newnew_fd);
  printf(" write result %d\n", write_result);
  printf("___________________________ end of f_write ___________\n\n");


  printf("\n\n\n________+++++++++++++ f_read _________________\n");
  int result_buffer[WRITEBYTE];
  bzero(result_buffer, WRITEBYTE*4);
  f_seek(newnew_fd, 0, SEEKSET);

  f_read(result_buffer,WRITEBYTE * 4, newnew_fd);
  for(int i = 0; i < WRITEBYTE; i++) {
    //    printf("read buffer %d: ", i);
    if(result_buffer[i] != i) {
      printf("read buffer %d differes: %d", i,result_buffer[i]);
    }
    //printf("%d ", result_buffer[i]);
  }

  printf("++++++++++++++++ f_stat +++++++++++++++++++\n");
  struct fst* st = malloc(sizeof(struct fst));
  f_stat(newnew_fd, st);
  printf("\n\n stat has size %d\n", st->filesize);

  // printf("\n\n\n+++++++++==========___________ f_remove ___________\n\n");
  // f_remove("home/rusr/haha.txt");
  // printf("\n\n\n __________ end of REMOVE _____________\n\n");



}
