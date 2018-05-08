#include <stdlib.h>
#include <stdio.h>
#include "file_lib.h"
#include "fs_struct.h"

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
  int new_fd = f_open("home/rusr/haha.txt", OPEN_W);
  printf("_______end of fopen______________\n");
  printf("new file fd is %d\n", new_fd);
  printf("\n\n_______________________read new file dir\n\n");
  //  int again = f_open("home/rusr/hahaha.txt", "");
  //printf("again %d\n", again);
  struct dirent* rusrdir = f_readdir(result);
  printf("should: %s\n", rusrdir->filename);
  printf("\n\n______ f_close _________\n");
  f_close(new_fd);
  printf("________________________ f-OPEN 2222 ___________________\n\n");
  int newnew_fd = f_open("home/rusr/haha.txt", OPEN_W);
  printf("_______end of fopen______________\n");
  printf("new file fd is %d\n", newnew_fd);
  printf("\n\n_______________________read new file dir\n\n");
  //  int again = f_open("home/rusr/hahaha.txt", "");
  //printf("again %d\n", again);
  f_seek(result, 0, SEEKSET);
  rusrdir = f_readdir(result);
  printf("should: %s\n", rusrdir->filename);
  int* buffer = (int*)malloc(516);
  for(int i = 0; i < 129; i++) {
    buffer[i] = i;
    printf("%d ", i);
  }
  
  printf("\n\n\n____________========= f_write -_________________-\n");
  int write_result = f_write((void*)buffer, 512, newnew_fd);
  printf(" write result %d\n", write_result);
  printf("___________________________ end of f_write ___________\n\n");

  
  printf("\n\n\n________+++++++++++++ f_read _________________\n");
  int* result_buffer = (int*)malloc(516);
  f_seek(newnew_fd, 0, SEEKSET);
  
  f_read(result_buffer, 512, newnew_fd);
  for(int i = 0; i < 128; i++) {
    printf("read buffer %d\n", i);
     printf("%d\n", result_buffer[i]);
  }
  
  
  
}
