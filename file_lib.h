#ifndef FILE_LIB_H_
#define FILE_LIB_H_

// library functions
int f_mount(char* sourcepath); //done
int f_open(char* filepath, char* access);
int f_opendir(char* filepath);
struct dirent* f_readdir(int dir_fd); //done
int f_rewind(int fd); //done
int f_seek(int fd, int offset, int whence);
// helper functions
static char** parse_filepath(char* filepath);
static void update_ft(struct file_table_entry* new_entry, int new_index);
static void free_parse(char**);

#endif
