#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "file_lib.h"
#include "fs_struct.h"
#define WRITEBYTE 67840
//#define WRITEBYTE 512

int main() {
  f_mount("DISK");

  // printf("_____________ rewind :\n");
  // f_rewind(0);
  // printf("----------open dir:\n");
  /*
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
  //  int again = f_open("home/rusr/hahaha.txt", "");
  //printf("again %d\n", again);
  struct dirent* rusrdir = f_readdir(result);
  printf("should: %s\n", rusrdir->filename);
  printf("\n\n______ f_close _________\n");
  f_close(new_fd);
  */
  // second open


  printf("\n________________________ f-OPEN 2222 ___________________\n\n");
  f_mkdir("home/hehe", RWE);
  int hehe = f_opendir("/home/hehe");
  int feichi = f_open("home/hehe/feichi.txt", OPEN_W, RWE);
  printf("Ugh %d\n", feichi);
  printf("_______end of fopen______________\n");
  printf("new file fd is %d\n", feichi);
  printf("\n_______________________read new file dir\n\n");
  //assert(newnew_fd == new_fd);
  //  int again = f_open("home/rusr/hahaha.txt", "");
  //printf("again %d\n", again);
  f_seek(hehe, 0, SEEKSET);
  struct dirent* fhehe = f_readdir(hehe);
  printf("should: %s\n", fhehe->filename);
  assert(strcmp(fhehe->filename, "feichi.txt") == SUCCESS);
  int buffer[WRITEBYTE];
  for(int i = 0; i < WRITEBYTE; i++) {
    buffer[i] = i;
    //printf("%d ", i);
  }
  f_seek(feichi, 0, SEEKSET);
  printf("\n\n\n____________========= f_write -_________________-\n");
  int write_result = f_write((void*)buffer,WRITEBYTE * 4, feichi);
  printf(" write result %d\n", write_result);
  printf("___________________________ end of f_write ___________\n\n");


  printf("\n\n\n________+++++++++++++ f_read _________________\n");
  int result_buffer[WRITEBYTE];
  bzero(result_buffer, WRITEBYTE);
  //f_seek(feichi, 0, SEEKSET);
  f_rewind(feichi);

  f_read(result_buffer,WRITEBYTE * 4, feichi);
  for(int i = 0; i < WRITEBYTE; i++) {
    //    printf("read buffer %d: ", i);
    if(result_buffer[i] != i) {
      printf("read buffer %d differes: %d", i,result_buffer[i]);
    }
    assert(result_buffer[i] == i);
    //printf("%d ", result_buffer[i]);
  }

  printf("++++++++++++++++ f_stat +++++++++++++++++++\n");
  struct fStat* st = malloc(sizeof(struct fStat));
  f_stat(feichi, st);
  printf("\n\n stat has size %d\n", st->filesize);
  //f_closedir(hehe);
  printf("\n\nClose dir again!!\n\n");
  f_close(feichi);
  f_closedir(hehe);
  printf("after locsing \n\n");
  print_openft();
  printf("stat should fail for hehe\n");
  f_stat(feichi, st);
  printf("after stat\n\n\n");
  print_openft();
  printf("\n\n_______________- rmdir __________________\n\n\n");
  printf("before remove dir\n");
  print_openft();
  f_rmdir("home/hehe");
  printf("after rmdir\n");
  print_openft();
  printf("\n\n\nrmdir finished\n");
  // printf("\n\n\n+++++++++==========___________ f_remove ___________\n\n");
  //f_remove("home/rusr/haha.txt");
  // printf("\n\n\n __________ end of REMOVE _____________\n\n");
  // printf("\n\n\n\n\n\n\n makinh aging\n\n\n\n\n");
  //f_mkdir("home/hehe", RWE);
  printf("\n\n\n open again\n");
  feichi = f_open("home/hehe/feichi.txt", OPEN_W, RWE);
  printf("Ugh 2.0 %d\n", feichi);
  printf("_______end of fopen______________\n\n\n");
  printf("nownownow\n\n\n");
  print_openft();
  f_mkdir("home/hehe", RWE);
  printf("watatatta makdir\n");
  print_openft();
  feichi = f_open("home/hehe/feichi.txt", OPEN_W, RWE);
  printf("Ugh 3.0 %d\n", feichi);
  printf("_______end of fopen______________\n\n\n");
  print_openft();
  write_result = f_write((void*)buffer,WRITEBYTE * 4, feichi);                                                             
  printf(" write result %d\n", write_result);
  printf("___________________________ end of f_write ___________\n\n");

  
  printf("\n\n\n________+++++++++++++ f_read _________________\n");

  
  bzero(result_buffer, WRITEBYTE);
  
  //f_seek(feichi, 0, SEEKSET);
  
  f_rewind(feichi);
  


  
  f_read(result_buffer,WRITEBYTE * 4, feichi);
  

  for(int i = 0; i < WRITEBYTE; i++) {
    
    //    printf("read buffer %d: ", i);
    
    if(result_buffer[i] != i) {
      
      printf("read buffer %d differes: %d", i,result_buffer[i]);
      
    }
    
    assert(result_buffer[i] == i);
    

    printf("%d ", result_buffer[i]);
    
  }                                   


  f_unmount();
  free(st);
  free(fhehe);
}
