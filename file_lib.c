#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "fs_struct.h"

/*********** GLOBALS ************/
// check whether library has been initialized 
static int init_lib = FALSE;
char disk_img[5] = "DISK";
int disk_fd = UNDEFINED;   

struct superblock* sb = NULL;
void* inodes = NULL;
static struct file_table* open_ft;

/********** FUNCTION PROTOTYPES **********/
int lib_init();
static char** parse_filepath(char* filepath);
int f_open(char* filepath, char* access);


int lib_init() {
    // open disk
    disk_fd = open(disk_img, O_RDWR);
    if (disk_fd == FAIL) {
        printf("Open disk image failed.\n");
        return FAIL;
    }
    // read in boot block
    void* buffer = malloc(BLOCKSIZE);
    if(read(disk_fd, buffer, BLOCKSIZE) != BLOCKSIZE) {
        printf("Read in disk boot block failed.\n");
        free(buffer);
        return FAIL;
    }
    sb = (struct superblock*)malloc(sizeof(struct superblock));
    bzero(buffer, BLOCKSIZE);

    // read in superblock
    if(read(disk_fd, buffer, BLOCKSIZE) != BLOCKSIZE) {
        printf("Read in super block failed. \n");
        free(buffer);
        return FAIL;
    }

    open_ft = (struct file_table*)(malloc(sizeof(struct file_table)));
    struct superblock* temp = (struct superblock*)buffer;
    sb->blocksize = temp->blocksize;
    sb->inode_offset = temp->inode_offset;
    sb->data_offset = temp->data_offset;
    sb->free_inode_head = temp->free_inode_head;
    sb->free_block_head = temp->free_block_head;
    free(buffer);
    printf("superblock info: blocksize: %d ", sb->blocksize);
    printf("inode_offset %d, data_offset %d ", sb->inode_offset, sb->data_offset);
    printf("free_inode_head %d, free_block_head %d\n", sb->free_inode_head, sb->free_block_head);

    // read in inode region
    int inode_blocksize = (sb->data_offset - sb->inode_offset) * BLOCKSIZE;
    printf("iniode block size is %d\n", inode_blocksize);
    inodes = malloc(inode_blocksize);
    if(read(disk_fd, inodes, inode_blocksize) != inode_blocksize) {
        printf("Read in inode region failed. \n");
        free(inodes);
        return FAIL;
    }
    return SUCCESS;
}

static char** parse_filepath(char* filepath) {
    char delim[2] = "/";
    int len = 20;
    int size = 20;
    int count = 0;
    char** parse_result = (char**)malloc(sizeof(char*) * size);
    char* token = strtok(filepath, delim);
    while(token != NULL) {
        if(count >= size) {
            size += len;
            parse_result = realloc(parse_result, sizeof(char*) * size);
        }
        char* temp = malloc(sizeof(char) * (strlen(token) + 1));
        strcpy(temp, token);
        parse_result[count] = temp;
        printf("parse result %d is %s\n", count, parse_result[count]);
        token = strtok(NULL, delim);
        count ++;
    }
    parse_result[count] = NULL;
    return parse_result;
}

int f_open(char* filepath, char* access) {
    char** parse_path = parse_filepath(filepath);
    return 0;
}

int main() {
    lib_init();
}




/*
    1. parse
    2. open
    3. write
    4. open
*/