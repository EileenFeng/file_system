#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "fs_struct.h"
#include "file_lib.h"

/* needs to free
   1. open file table entries
   2. open file table
   3. 'inodes'
   4. data_buffer entries which are struct 'data_block' (no need to free data buffer)

*/


/*********** GLOBALS ************/
// check whether library has been initialized 
static int init_lib = FALSE;
static char disk_img[20];
static struct fs_disk cur_disk;
//int disk_fd = UNDEFINED;  
//int data_region_offset = UNDEFINED; // gives the byte offset to the data region 

//struct superblock sb;
//static struct inode* root_inode = NULL;
//static int root_fd = UNDEFINED;
//void* inodes = NULL; // inode region

static struct file_table* open_ft;
// buffer for storing data blocks
static struct data_block* data_buffer[20];
static int data_buffer_count = 0;

/********** FUNCTION PROTOTYPES **********/
// int lib_init();
// static char** parse_filepath(char* filepath);
// int f_open(char* filepath, char* access);
// void update_ft(struct file_table_entry* new_entry, int new_index);


int f_mount(char* sourcepath) {
  // open disk
  strcpy(disk_img, sourcepath); 
  cur_disk.diskfd = open(sourcepath, O_RDWR);
  if (cur_disk.diskfd == FAIL) {
    printf("Open disk image failed.\n");
    return FAIL;
  }
  // read in boot block
  void* buffer = malloc(BLOCKSIZE);
  if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
    printf("Read in disk boot block failed.\n");
    free(buffer);
    return FAIL;
  }
  //sb = (struct superblock*)malloc(sizeof(struct superblock));
  bzero(buffer, BLOCKSIZE);

  // read in superblock
  if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
    printf("Read in super block failed. \n");
    free(buffer);
    return FAIL;
  }
  struct superblock* temp = (struct superblock*)buffer;
  cur_disk.sb.blocksize = temp->blocksize;
  cur_disk.sb.inode_offset = temp->inode_offset;
  cur_disk.sb.data_offset = temp->data_offset;
  cur_disk.sb.free_inode_head = temp->free_inode_head;
  cur_disk.sb.free_block_head = temp->free_block_head;
  free(buffer);
  //get region offsets
  //cur_disk.inode_region_offset = cur.disk.sb.inode_offset * BLOCKSIZE + BLOCKSIZE * 2;
  cur_disk.data_region_offset = cur_disk.sb.data_offset * BLOCKSIZE + BLOCKSIZE * 2;
  //printf("inode region offset is %d\n", cur_disk.inode_region_offset);
  printf("superblock info: blocksize: %d ", cur_disk.sb.blocksize);
  printf("inode_offset %d, data_offset %d ", cur_disk.sb.inode_offset, cur_disk.sb.data_offset);
  printf("free_inode_head %d, free_block_head %d, data region offset %d\n", cur_disk.sb.free_inode_head, cur_disk.sb.free_block_head, cur_disk.data_region_offset);

  // read in inode region
  int inode_blocksize = (cur_disk.sb.data_offset - cur_disk.sb.inode_offset) * BLOCKSIZE;
  printf("iniode block size is %d\n", inode_blocksize);
  cur_disk.inodes = malloc(inode_blocksize);
  if(read(cur_disk.diskfd, cur_disk.inodes, inode_blocksize) != inode_blocksize) {
    printf("Read in inode region failed. \n");
    free(cur_disk.inodes);
    return FAIL;
  }

  //set up open file table
  open_ft = (struct file_table*)(malloc(sizeof(struct file_table)));  
  open_ft->filenum = 0;
  open_ft->free_fd_num = MAX_OPENFILE;
  for(int i = 0; i < MAX_OPENFILE; i++) {
    open_ft->free_id[i] = i+1;
    open_ft->entries[i] = NULL;
  }

  //create an entry for root directory in the open file table
  cur_disk.root_inode = (struct inode*)(cur_disk.inodes);
  struct file_table_entry* root_entry = malloc(sizeof(struct file_table_entry));
  strcpy(root_entry->filepath, "/");
  root_entry->inode_index = ROOT_INDEX;
  root_entry->type = DIR;
  root_entry->block_index = 0;
  root_entry->block_offset = cur_disk.root_inode->size > 0 ? cur_disk.root_inode->dblocks[0] : UNDEFINED;
  root_entry->offset = 0;
  root_entry->open_num = 0;
  //update open file table
  int temp_index = MAX_OPENFILE - open_ft->free_fd_num;
  root_entry->fd = open_ft->free_id[temp_index];
  cur_disk.rootdir_fd = root_entry->fd;
  printf("fd for root directory is %d\n", root_entry->fd);
  update_ft(root_entry, temp_index);
  /*
    open_ft->free_id[temp_index] = UNDEFINED;
    open_ft->free_fd_num --;
    open_ft->entries[open_ft->filenum] = root_entry;
    open_ft->filenum ++;
  */
  printf("data block offset for the first of root dir is %d\n", cur_disk.root_inode->dblocks[0]);
  int temp_offset = cur_disk.data_region_offset + cur_disk.root_inode->dblocks[0];
  lseek(cur_disk.diskfd, temp_offset, SEEK_SET);
  struct data_block* root_bone = (struct data_block*)(malloc(sizeof(struct data_block)));
  root_bone->block_index = cur_disk.root_inode->dblocks[0];
  if(read(cur_disk.diskfd, (void*)(root_bone->data), BLOCKSIZE) != BLOCKSIZE) {
    free(cur_disk.inodes);
    free(root_entry);
    free(open_ft);
    printf("Read root directory data block one failed\n");
    return FAIL;
  }

  return SUCCESS;
}

int f_open(char* filepath, char* access) {
  char** parse_path = parse_filepath(filepath);
  int count = 0;
  char* temp = parse_path[count];
  while(temp != NULL) {
    printf("current file token is %s\n", temp);
    count ++;
    temp = parse_path[count];
  }
  return 0;
} 

static char** parse_filepath(char* filepath) {
  char delim[2] = "/";
  int len = 20;
  int size = 20;
  int count = 0;
  char file_path[MAX_LENGTH];
  strcpy(file_path, filepath);
  char** parse_result = (char**)malloc(sizeof(char*) * size);
  char* token;
  printf("filepath is %s\n", filepath);
  token = strtok(file_path, delim);
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

static void update_ft(struct file_table_entry* new_entry, int new_index) {
  open_ft->free_id[new_index] = UNDEFINED;
  open_ft->free_fd_num --;
  open_ft->entries[open_ft->filenum] = new_entry;
  open_ft->filenum ++;
}

static struct dirent** get_dirents(int inode_index) {
  struct inode* target = (struct inode*)(inodes + inode_index * BLOCKSIZE);
  if(target->nlink == 0) {
    printf("Invalid inode! Inode is free!\n");
    return NULL;
  }
  if(target->type != DIR) {
    printf("Input file is not a directory! \n");
    return NULL;
  }
  if(target->size == 0) {
    printf("Directory does not contain any files\n");
    return NULL;
  }
  int bytestoread = target->size;
  int size = 5;
  int dirent_num = size;

  struct dirrent** res = (struct dirent**)malloc(sizeof(dirent*) * size);
  for(int i = 0; i < N_DBLOCKS; i++) {
    if(bytestoread <= 0) {
      return res;
    }
    int read_bytes = bytestoread < BLOCKSIZE ? bytestoread : BLOCKSIZE;
    int block_index = target->dblocks[i];
    
  }
  
}


/*
  1. parse
  2. open
  3. write
  4. open
*/
