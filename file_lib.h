#ifndef FILE_LIB_H_
#define FILE_LIB_H_

int f_mount(char* sourcepath);
int f_open(char* filepath, char* access);
 
static char** parse_filepath(char* filepath);
static void update_ft(struct file_table_entry* new_entry, int new_index);

#endif
