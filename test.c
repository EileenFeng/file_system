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
  int new_fd = f_open("home/rusr/haha.txt", "");
  printf("_______end of fopen______________\n");
  printf("new file fd is %d\n", new_fd);
  printf("\n\n_______________________read new file dir\n\n");

  struct dirent* rusrdir = f_readdir(result);
  printf("should: %s\n", rusrdir->filename);
  //f_open("/home/eileen/haha/lol.txt", "dajs");

}
