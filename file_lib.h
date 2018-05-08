#ifndef FILE_LIB_H_
#define FILE_LIB_H_
//#include "fs_struct.h"

// library functions
int f_mount(char* sourcepath); //done
int f_open(char* filepath, int access); // partially done, need to handle access (write remove)
int f_write(void* buffer, int bsize, int fd); // done NEED TO TEST
int f_read(void* buffer, int bsize, int fd);
int f_close(int fd); //done
int f_remove(char* filepath);


int f_rewind(int fd); //done
int f_stat(int fd, struct fStat* st);
/*  when SEEK_END, seek backwards */
int f_seek(int fd, int offset, int whence); // done


int f_opendir(char* filepath); //done
struct dirent* f_readdir(int dir_fd); //done // malloced result



#endif
