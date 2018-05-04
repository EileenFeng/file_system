#include <stdlib.h>
#include <stdio.h>
#include "fs_struct.h"

/*********** globals ************/
// check whether 
int init_lib = FALSE;
struct file_table open_ftable;


/*
    1. parse
    2. open
    3. write
    4. open
*/