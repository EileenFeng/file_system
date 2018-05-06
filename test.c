#include <stdlib.h>
#include <stdio.h>
#include "file_lib.h"

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
  //f_open("/home/eileen/haha/lol.txt", "dajs");

}
