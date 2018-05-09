#ifndef FILE_LIB_H_
#define FILE_LIB_H_
//#include "fs_struct.h"

// library functions
int f_mount(char* sourcepath); //done
int f_unmount();

int f_open(char* filepath, int access, int mode); // done
int f_read(void* buffer, int bsize, int fd); // done
int f_write(void* buffer, int bsize, int fd); // done NEED TO TEST
int f_close(int fd); //done
int f_seek(int fd, int offset, int whence); // done
int f_rewind(int fd); //done
int f_stat(int fd, struct fStat* st); //done
/*  when SEEK_END, seek backwards */
int f_remove(char* filepath, int empty_dir); // done


int f_opendir(char* filepath); //done
struct dirent* f_readdir(int dir_fd); //done // malloced result
int f_closedir(int dir_fd);// need to test
int f_mkdir(char* filepath, int mode); // done
int f_rmdir(char* filepath);
/*
f_closedir
:  close an open directory file
12.
f_mkdir
:  make a new directory at the specified location
13.
f_rmdir
:  delete a specified directory.  Be sure to remove entire contents and the contents of
all subdirectorys from the filesystem.  Do NOT simply remove pointer to directory.
14.
f_mount
:  mount a specified file system into your directory tree at a specified location.
15.
f_umount
:  unmount a specified file system

*/

#endif
