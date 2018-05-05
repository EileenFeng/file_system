int lib_init();
static char** parse_filepath(char* filepath);
int f_open(char* filepath, char* access);
void update_ft(struct file_table_entry* new_entry, int new_index);
