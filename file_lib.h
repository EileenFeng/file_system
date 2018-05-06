#ifndef FILE_LIB_H_
#define FILE_LIB_H_

// library functions
int f_mount(char* sourcepath); //done
int f_open(char* filepath, char* access);
int fwrite(void* buffer, int bsize, int fd);

int f_opendir(char* filepath); //done
struct dirent* f_readdir(int dir_fd); //done // malloced result
int f_rewind(int fd); //done
int f_seek(int fd, int offset, int whence);


#endif
