#ifndef FILE_LIB_H_
#define FILE_LIB_H_
#include "fs_struct.h"

// library functions
int f_mount(char* sourcepath); //done
int f_unmount();

int f_open(char* filepath, int access, int mode); // done
int f_read(void* buffer, int bsize, int fd); // done
int f_write(void* buffer, int bsize, int fd); // done NEED TO TEST
int f_close(int fd); //done
int f_seek(int fd, int offset, int whence); // done
int f_rewind(int fd); //done
int f_stat(int fd, struct fst* st); //done
/*  when SEEK_END, seek backwards */
int f_remove(char* filepath, int empty_dir); // done


int f_opendir(char* filepath); //done
struct dirent* f_readdir(int dir_fd); //done // malloced result
int f_closedir(int dir_fd);// need to test
int f_mkdir(char* filepath, int mode); // done
int f_rmdir(char* filepath);
int f_unmount();

// helper for shell
void get_inode(int inode_index);
int change_mode(int mode, char* filepath);
#endif
