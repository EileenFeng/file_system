
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
//#include "fs_struct.h"
#include "file_lib.h"

/* needs to free
   1. open file table entries
   2. open file table
   3. 'inodes'
   4. data_buffer entries which are struct 'data_block' (no need to free data buffer)

*/



/*********** GLOBALS ************/
// check whether library has been initialized
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
//static struct data_block* data_buffer[20];
//static int data_buffer_count = 0;

/********** HELPER FUNCTION PROTOTYPES **********/
// helper functions
/*** create or return an existing open file entry, target is relative child path ***/
static struct file_table_entry* create_entry(int parent_fd, int child_inode, char* target, int access, int type);
/***check whether the parent directory contains the file 'targetdir' ***/
static struct dirent* checkdir_exist(int parentdir_fd, char*);
static char** parse_filepath(char* filepath);
static void update_ft(struct file_table_entry* new_entry, int new_index);
static void free_parse(char**);
/*** create a new reg or dir file, return the new file inode index ***/
/*** 'newfile_name' is relative path ***/
static int create_file(int parent_fd, char* newfile_name, int type, int permission);
static void write_newinode(struct inode* new_file_inode, int new_inode_index, int parent_inode, int type);
static struct table* get_tables(struct file_table_entry* en, struct table* t);
static int get_next_boffset(struct table* t, struct file_table_entry* en);
static void free_struct_table(struct table* t);
static int get_next_freeOffset();
// write super block back to disk
static int write_disk_sb();
// write inodes back to disk
static int write_disk_inode();
// remove the dirent from the parent dir
static int remove_dirent(int parent_fd, int parent_inode, int child_inode);
// free the inode
static int free_inode(int inode_index);
static int print_free();
static void scope(char* name);
static int check_permission(struct inode* target, int access);
void get_inode(int inode_index);
int change_mode(int mode, char* filepath);
/************************ LIB FUNCTIONS *****************************/

int f_mount(char* sourcepath) {
  // open disk
  strcpy(disk_img, sourcepath);
  cur_disk.diskfd = open("DISK", O_RDWR);
  cur_disk.uid = 0;
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
  //printf("superblock info: blocksize: %d ", cur_disk.sb.blocksize);
  //printf("inode_offset %d, data_offset %d ", cur_disk.sb.inode_offset, cur_disk.sb.data_offset);
  //printf("free_inode_head %d, free_block_head %d, data region offset %d\n", cur_disk.sb.free_inode_head, cur_disk.sb.free_block_head, cur_disk.data_region_offset);

  // read in inode region
  int inode_blocksize = (cur_disk.sb.data_offset - cur_disk.sb.inode_offset) * BLOCKSIZE;
  //printf("iniode block size is %d\n", inode_blocksize);
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
    open_ft->free_id[i] = i;
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
  open_ft->free_id[temp_index] = UNDEFINED;
  cur_disk.rootdir_fd = root_entry->fd;
  //printf("fd for root directory is %d\n", root_entry->fd);
  update_ft(root_entry, temp_index);
  /*
    open_ft->free_id[temp_index] = UNDEFINED;
    open_ft->free_fd_num --;
    open_ft->entries[open_ft->filenum] = root_entry;
    open_ft->filenum ++;
  */
  /*
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
  */

  return SUCCESS;
}


/*********************** directory functions ***********************/

int f_opendir(char* filepath) {

  char** parse_path = parse_filepath(filepath);
  int count = 0;
  char* curdir = parse_path[count];
  //printf("opendir: curdir is %s\n", curdir);
  if(curdir == NULL) {
    free_parse(parse_path);
    //printf("opendir: root directory already opened\n");
    return SUCCESS;
  }
  int parent_fd = cur_disk.rootdir_fd;
  while(curdir != NULL) {
    //printf("***opendir: curdir now is %s  and parent fd is %d\n", curdir, parent_fd);
    struct dirent* child_dirent = checkdir_exist(parent_fd, curdir);
    if(child_dirent == NULL) {
      printf("opendir: Directory %s does not exists\n", curdir);
      free_parse(parse_path);
      return FAIL;
    }
    if(child_dirent->type != DIR) {
      free_parse(parse_path);
      free(child_dirent);
      printf("opendir: Cannot open non directory files\n");
      return FAIL;
    }
    struct file_table_entry* new_entry = create_entry(parent_fd, child_dirent->inode_index, curdir, OPEN_WR, DIR);
    parent_fd = new_entry->fd;
    count ++;
    curdir = parse_path[count];
    free(child_dirent);
  }
  free_parse(parse_path);
  return parent_fd;
}


struct dirent* f_readdir(int dir_fd) {
  if(dir_fd < 0) {
    printf("f_readdir: Invalid file descriptor!\n");
    return FAIL;
  }
  struct file_table_entry* target = open_ft->entries[dir_fd];
  if(target == NULL) {
    printf("freaddir: Invalid directory file descriptor for f_readdir\n");
    return NULL;
  }
  if(target->block_offset == UNDEFINED) {
    printf("freaddir: Directory file is empty. No content to read for f_readdir\n");
    return NULL;
  }

  // read in data
  struct inode* temp_inode = (struct inode*)(cur_disk.inodes + target->inode_index * INODE_SIZE);
  int file_offset = cur_disk.data_region_offset + target->block_offset * BLOCKSIZE + target->offset;
  if(file_offset >= temp_inode->size + cur_disk.data_region_offset + temp_inode->dblocks[0] * BLOCKSIZE) {
    return NULL;
  }
  lseek(cur_disk.diskfd, file_offset, SEEK_SET);
  struct dirent* ret = (struct dirent*)malloc(sizeof(struct dirent));
  if(read(cur_disk.diskfd, ret, DIRENT_SIZE) != DIRENT_SIZE) {
    printf("Readdir: Read in dirent failed\n");
    free(ret);
    return NULL;
  }
  // update offset, block offset, block index
  struct table datatable;
  get_tables(target, &datatable);
  int new_offset = (target->offset + DIRENT_SIZE) - BLOCKSIZE;
  if(target->offset + DIRENT_SIZE >= BLOCKSIZE) {
    target->block_index ++;
    get_tables(target, &datatable);
    target->block_offset = datatable.cur_data_table[datatable.intable_index];
    target->offset = new_offset;
  } else {
    target->offset += DIRENT_SIZE;
  }
  return ret;
}


/*********************** FILE functions ***********************/
int f_remove(char* filepath, int empty_dir) {
  //char name[320] __attribute__((cleanup(scope)));
  //sprintf(name, "f_remove:%s", filepath);
  char** parse_path = parse_filepath(filepath);
  int count = 0;
  char* prevdir = NULL;
  char* curdir = parse_path[count];
  int parent_fd = cur_disk.rootdir_fd;
  char parent_path[MAX_LENGTH];
  strcpy(parent_path, "");

  // check whether directories along the filepath exists
  while(curdir != NULL) {
    // get complete path
    if(strlen(parent_path) + strlen(curdir) + strlen("/") >= MAX_LENGTH) {
      free_parse(parse_path);
      printf("f_remove: filepath invalid: file path too long\n");
      return FAIL;
    }
    strcat(parent_path, "/");
    strcat(parent_path, curdir);
    // check if parent directory exists
    count ++;
    prevdir = curdir;
    curdir = parse_path[count];
    if(curdir == NULL){
      break;
    }
    parent_fd = f_opendir(parent_path);
    if(parent_fd == FAIL) {
      printf("f_remove: Directory %s along the way does not exists\n", parent_path);
      free_parse(parse_path);
      return FAIL;
    }
  }
  // now prevdir contains the file to be OPENED, parent dir is the parent Directory
  struct dirent* target_file = checkdir_exist(parent_fd, prevdir);
  // cannot remove a not existing file
  if(target_file == NULL) {
    printf("f_remove:    Cannot remove a file that does not exist. \n");
    free_parse(parse_path);
    free(target_file);
    return FAIL;
  }
  // cannot remove a directory with f_remove
  if(target_file->type != REG && empty_dir == FALSE) {
    printf("f_remove:      Cannot remove a directory file with f_remove\n");
    free_parse(parse_path);
    free(target_file);
    return FAIL;
  }

  // get the inode of target file, need to do:
  /*
    1. free all the data blocks along the way;
    2. remove the dirent entry from the parent file
    3. free the inode
    4. update the free block list, free block list head,  free inode, free inode head in sb
    5. write the updated inodes and sb back to disk!!!!!
  */

  struct inode* target = (struct inode*)(cur_disk.inodes + target_file->inode_index * INODE_SIZE);
  // 1. free all the data blocks along the way
  int filesize = target->size;
  int* block_buffer = (int*)malloc(BLOCKSIZE);
  // free dblocks
  if(filesize > 0) {
    for(int i = 0; i < N_DBLOCKS; i ++) {
      if(filesize > 0) {
        int block_offset = target->dblocks[i];
        // update the free block head
        int oldhead = cur_disk.sb.free_block_head;
        bzero(block_buffer, BLOCKSIZE);
        block_buffer[0] = oldhead;
        cur_disk.sb.free_block_head = block_offset;
        int fileoffset = cur_disk.data_region_offset + block_offset * BLOCKSIZE;
        lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
        if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
          printf("f_remove:     write free data block in dblocks failed\n");
          free(block_buffer);
	  free_parse(parse_path);
          free(target_file);
          return FAIL;
        }
        filesize -= BLOCKSIZE;
      } else {
        break;
      }
    }
    write_disk_sb();
  }
  // if filesize < 0 after dblocks, return the inode to free inodes
  if(filesize <= 0) {
    remove_dirent(parent_fd, target->parent_inode, target->inode_index);
    free_inode(target->inode_index);
    free(block_buffer);
    free(target_file);
    free_parse(parse_path);
    return SUCCESS;
  }

  // indirect level
  int* levelone = (int*)malloc(BLOCKSIZE);
  //free indirect
  if(filesize > 0) {
    for(int i = 0; i < N_IBLOCKS; i ++) {
      if(filesize <= 0) {
        break;
      }
      int datatable_offset = target->iblocks[i];
      int fileoffset = cur_disk.data_region_offset + datatable_offset * BLOCKSIZE;
      lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
      if(read(cur_disk.diskfd, (void*)levelone, BLOCKSIZE) != BLOCKSIZE) {
        printf("f_remove:     indirect:     read in data table failed\n");
        free(block_buffer);
	free_parse(parse_path);
        free(target_file);
        free(levelone);
        return FAIL;
      }

      // free the data table pointed to by iblocks[i](datatable_offset)
      int oldhead = cur_disk.sb.free_block_head;
      bzero(block_buffer, BLOCKSIZE);
      block_buffer[0] = oldhead;
      cur_disk.sb.free_block_head = datatable_offset;
      fileoffset = cur_disk.data_region_offset + datatable_offset* BLOCKSIZE;
      lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
      if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
        printf("f_remove:     free levelone[i] in indblocks failed\n");
        free(block_buffer);
	free_parse(parse_path);
        free(target_file);
        free(levelone);
        return FAIL;
      }
      write_disk_sb();

      for(int i = 0; i < TABLE_ENTRYNUM; i++) {
        if(filesize <= 0) {
          break;
        }
        int block_offset = levelone[i];
        int oldhead = cur_disk.sb.free_block_head;
        bzero(block_buffer, BLOCKSIZE);
        block_buffer[0] = oldhead;
        cur_disk.sb.free_block_head = block_offset;
        int fileoffset = cur_disk.data_region_offset + block_offset * BLOCKSIZE;


        lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
        if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
          printf("f_remove:     free data block in indblocks failed\n");
          free(block_buffer);
	  free_parse(parse_path);
          free(target_file);
          return FAIL;
        }
        filesize -= BLOCKSIZE;
      }
      write_disk_sb();
      if(filesize <= 0) {
        break;
      }
    }
  }

  // if filesize < 0 after iblocks, return the inode to free inodes
  if(filesize <= 0) {
    remove_dirent(parent_fd, target->parent_inode, target->inode_index);
    free_inode(target->inode_index);
    free_parse(parse_path);
    free(target_file);
    free(levelone);
    free(block_buffer);
    return SUCCESS;
  }

  // free blocks in level2
  int* leveltwo = (int*)malloc(BLOCKSIZE);
  if(filesize > 0) {
    bzero(levelone, BLOCKSIZE);
    int i2offset = cur_disk.data_region_offset + target->i2block * BLOCKSIZE;
    lseek(cur_disk.diskfd, i2offset, SEEK_SET);
    if(read(cur_disk.diskfd, levelone, BLOCKSIZE) != BLOCKSIZE) {
      printf("f_remove:   read in level one table failed in i2block\n");
      free_inode(target->inode_index);
      free(levelone);
      free(leveltwo);
      free_parse(parse_path);
      free(target_file);
      free(block_buffer);
      return FAIL;
    }

    // free the block pointed to by i2block
    int oldhead = cur_disk.sb.free_block_head;
    bzero(block_buffer, BLOCKSIZE);
    block_buffer[0] = oldhead;
    cur_disk.sb.free_block_head = target->i2block;
    lseek(cur_disk.diskfd, i2offset, SEEK_SET);
    //printf("f_remove: I2 offset %d     after add free head is %d\n", i2offset, cur_disk.sb.free_block_head);
    if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
      printf("f_remove:     free i2offset in i2dblocks failed\n");
      free(block_buffer);
      free_parse(parse_path);
      free(target_file);
      return FAIL;
    }
    write_disk_sb();


    for(int i = 0; i < TABLE_ENTRYNUM; i++) {
      if(filesize <= 0) {
        break;
      }
      int levelone_index = levelone[i];
      int datatable_offset = cur_disk.data_region_offset + levelone_index * BLOCKSIZE;
      lseek(cur_disk.diskfd, datatable_offset, SEEK_SET);
      //printf("f_remove: DIRECT     after add free head is %d\n", cur_disk.sb.free_block_head);
      bzero(leveltwo, BLOCKSIZE);
      if(read(cur_disk.diskfd, leveltwo, BLOCKSIZE) != BLOCKSIZE) {
        printf("f_remove:   read in LEVEL TWO data table failed in i2block\n");
        free_inode(target->inode_index);
	free_parse(parse_path);
        free(target_file);
        free(levelone);
        free(leveltwo);
        free(block_buffer);
        return FAIL;
      }

      // free the data table pointed to by levelone[i]
      int oldhead = cur_disk.sb.free_block_head;
      bzero(block_buffer, BLOCKSIZE);
      block_buffer[0] = oldhead;
      cur_disk.sb.free_block_head = levelone[i];
      lseek(cur_disk.diskfd, datatable_offset, SEEK_SET);
      //printf("f_remove:   i2block     after add free head is %d and one after %d, \n", cur_disk.sb.free_block_head, block_buffer[0]);
      if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
        printf("f_remove:     free levelone[i] in indblocks failed\n");
        free(block_buffer);
	free_parse(parse_path);
        free(target_file);
        free(levelone);
        free(leveltwo);
        return FAIL;
      }
      write_disk_sb();

      // traverse current indices table
      for(int j = 0; j < TABLE_ENTRYNUM; j++) {
        if(filesize <= 0) {
          break;
        }
        int block_offset = leveltwo[j];
        //printf("f_remove : I22 current block offset to free %d\n", block_offset);
        int oldhead = cur_disk.sb.free_block_head;
        bzero(block_buffer, BLOCKSIZE);
        block_buffer[0] = oldhead;
        cur_disk.sb.free_block_head = block_offset;
        int fileoffset = cur_disk.data_region_offset + block_offset * BLOCKSIZE;
        //printf("f_remove: I2     after add free head is %d\n", cur_disk.sb.free_block_head);
        lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
        if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
          printf("f_remove:     free data block in level2 dblocks failed\n");
	  free_parse(parse_path);
          free(target_file);
          free(block_buffer);
          free(levelone);
          free(leveltwo);
          return FAIL;
        }
        filesize -= BLOCKSIZE;
        //printf("f_remove:     i2block blocks,   updated filesize %d\n", filesize);
      }
      write_disk_sb();
      if(filesize <= 0){
        break;
      }
    }
  }

  // if filesize < 0 after i2blocks, return the inode to free inodes
  if(filesize <= 0) {
    remove_dirent(parent_fd, target->parent_inode, target->inode_index);
    free_inode(target->inode_index);
    free_parse(parse_path);
    free(target_file);
    free(levelone);
    free(leveltwo);
    free(block_buffer);
    return SUCCESS;
  }

  // level i3block
  int* levelthree = (int*)malloc(BLOCKSIZE);
  if(filesize > 0) {
    bzero(levelone, BLOCKSIZE);
    int i3offset = cur_disk.data_region_offset + target->i3block * BLOCKSIZE;
    lseek(cur_disk.diskfd, i3offset, SEEK_SET);
    if(read(cur_disk.diskfd, levelone, BLOCKSIZE) != BLOCKSIZE) {
      printf("f_remove:   read in level one table failed in i3block\n");
      free_inode(target->inode_index);
      free(levelone);
      free(leveltwo);
      free(levelthree);
      free_parse(parse_path);
      free(target_file);
      free(block_buffer);
      return FAIL;
    }

    // free the block pointed to by i3block
    int oldhead = cur_disk.sb.free_block_head;
    bzero(block_buffer, BLOCKSIZE);
    block_buffer[0] = oldhead;
    cur_disk.sb.free_block_head = target->i3block;
    lseek(cur_disk.diskfd, i3offset, SEEK_SET);
    if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
      printf("f_remove:     free i2offset in i2dblocks failed\n");
      free(levelone);
      free(leveltwo);
      free(levelthree);
      free_parse(parse_path);
      free(target_file);
      free(block_buffer);
      return FAIL;
    }
    write_disk_sb();


    // levelone
    //printf("data regioin offsert is %d\n", cur_disk.data_region_offset);
    for(int i = 0; i < TABLE_ENTRYNUM; i++) {
      if(filesize <= 0) {
        break;
      }
      int level2_index = levelone[i];
      long level2_offset = cur_disk.data_region_offset + (long)(level2_index * BLOCKSIZE);
      lseek(cur_disk.diskfd, level2_offset, SEEK_SET);
      bzero(leveltwo, BLOCKSIZE);
      //printf("data offset %d index is %d offset is %ld\n", cur_disk.data_region_offset, level2_index, level2_offset);
      //if(read(cur_disk.diskfd, leveltwo, BLOCKSIZE) != BLOCKSIZE) {
      int readin = read(cur_disk.diskfd, leveltwo, BLOCKSIZE);
      if(readin != BLOCKSIZE){
        printf("f_remove:  level3  read in LEVEL TWO data table failed in i3block %d\n", readin);
        free_inode(target->inode_index);
	free_parse(parse_path);
        free(target_file);
        free(levelone);
        free(leveltwo);
        free(levelthree);
        free(block_buffer);
        return FAIL;
      }

      // free the data table pointed to by levelone[i]
      int oldhead = cur_disk.sb.free_block_head;
      bzero(block_buffer, BLOCKSIZE);
      block_buffer[0] = oldhead;
      cur_disk.sb.free_block_head = levelone[i];
      lseek(cur_disk.diskfd, level2_offset, SEEK_SET);
      if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
        printf("f_remove:     free levelone[i] in indblocks failed\n");
        free(block_buffer);
        free_parse(parse_path);
        free(target_file);
        free(levelone);
        free(leveltwo);
        free(levelthree);
        return FAIL;
      }
      write_disk_sb();

      // leveltwo
      for(int j = 0; j < TABLE_ENTRYNUM; j++) {
        if(filesize <= 0) {
          break;
        }
        int level3_index = leveltwo[j];
        int level3_offset = cur_disk.data_region_offset + level3_index * BLOCKSIZE;
        lseek(cur_disk.diskfd, level3_offset, SEEK_SET);
        bzero(levelthree, BLOCKSIZE);
        if(read(cur_disk.diskfd, levelthree, BLOCKSIZE) != BLOCKSIZE) {
          printf("f_remove:  level3 read in LEVEL THREE data table failed in i3block\n");
          free_inode(target->inode_index);
	  free_parse(parse_path);
          free(target_file);
          free(levelone);
          free(leveltwo);
          free(levelthree);
          free(block_buffer);
          return FAIL;
        }

        // free the data table pointed to by leveltwo[i]
        int oldhead = cur_disk.sb.free_block_head;
        bzero(block_buffer, BLOCKSIZE);
        block_buffer[0] = oldhead;
        cur_disk.sb.free_block_head = leveltwo[j];
        lseek(cur_disk.diskfd, level3_offset, SEEK_SET);
        if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
          printf("f_remove:     free levelone[i] in indblocks failed\n");
          free(block_buffer);
	  free_parse(parse_path);
          free(target_file);
          free(levelone);
          free(leveltwo);
          free(levelthree);
          return FAIL;
        }
        write_disk_sb();

        // data level
        for(int z = 0; z < TABLE_ENTRYNUM; z++) {
          if(filesize <= 0) {
            break;
          }
          int block_offset = levelthree[z];
          int oldhead = cur_disk.sb.free_block_head;
          bzero(block_buffer, BLOCKSIZE);
          block_buffer[0] = oldhead;
          cur_disk.sb.free_block_head = block_offset;
          int fileoffset = cur_disk.data_region_offset + block_offset * BLOCKSIZE;
          lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
          if(write(cur_disk.diskfd, (void*)block_buffer,BLOCKSIZE) != BLOCKSIZE) {
            printf("f_remove:     level3 free data block in i3dblocks failed\n");
	    free_parse(parse_path);
            free(target_file);
            free(block_buffer);
            free(levelone);
            free(leveltwo);
            free(levelthree);
            return FAIL;
          }
          filesize -= BLOCKSIZE;
        }
        if(filesize <= 0) {
          break;
        }
      }
      if(filesize <= 0 ) {
        break;
      }
    }
  }

  // if filesize < 0 after i22blocks, return the inode to free inodes
  if(filesize <= 0) {
    remove_dirent(parent_fd, target->parent_inode, target->inode_index);
    free_inode(target->inode_index);
    free_parse(parse_path);
    free(target_file);
    free(levelone);
    free(leveltwo);
    free(levelthree);
    free(block_buffer);
    return SUCCESS;
  }

  f_close(parent_fd);
  return FAIL;
  // need to close filepath
}

int f_read(void* buffer, int bsize, int fd) {
  if(fd < 0) {
    printf("f_read: Invalid file descriptor!\n");
    return FAIL;
  }
  struct file_table_entry* entry = open_ft->entries[fd];
  if(entry == NULL) {
    printf("f_read:     Invalid file descriptor!\n");
    return FAIL;
  }
  if(buffer == NULL) {
    printf("f_read:     Buffer cannot be NULL.\n");
    return FAIL;
  }
  if(bsize < 0) {
    printf("f_read:   number of bytes to read cannot be negative. \n");
    return FAIL;
  }
  if(bsize + entry->offset < BLOCKSIZE) {
    int fileoffset = cur_disk.data_region_offset + entry->block_offset * BLOCKSIZE + entry->offset;
    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
    if(read(cur_disk.diskfd, buffer, bsize) != bsize) {
      printf("f_read:   Read from file failed.\n");
      return FAIL;
    }
    entry->offset += bsize;
    return bsize;
  }

  struct table* datatable = (struct table*)malloc(sizeof(struct table));
  get_tables(entry, datatable);
  int bytes_toread = bsize;
  while(bytes_toread > 0) {
    int fileoffset = cur_disk.data_region_offset + entry->block_offset * BLOCKSIZE + entry->offset;
    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
    void* cur_buffer = buffer + (bsize - bytes_toread);
    if(bytes_toread >= BLOCKSIZE - entry->offset) {
      int cur_read = BLOCKSIZE - entry->offset;
      if(read(cur_disk.diskfd, cur_buffer, cur_read) != cur_read) {
        printf("f_read: read in the last block failed\n");
        free_struct_table(datatable);
        return FAIL;
      }
      get_next_boffset(datatable, entry);
      entry->offset = 0;
      bytes_toread -= BLOCKSIZE;
    } else {
      entry->offset += bytes_toread;
      if(read(cur_disk.diskfd, cur_buffer, bytes_toread) != bytes_toread) {
        printf("f_read: read in the last block failed\n");
        free_struct_table(datatable);
        return FAIL;
      }
      free_struct_table(datatable);
      return bsize - bytes_toread;
    }
  }
  free_struct_table(datatable);
  return bsize - bytes_toread;
}



int f_stat(int fd, struct fst* st) {
  if(fd < 0) {
    printf("f_stat: Invalid file descriptor!\n");
    return FAIL;
  }
  struct file_table_entry* entry = open_ft->entries[fd];
  if(entry == NULL) {
    printf("f_stat:     Invalid file descriptor\n");
    return FAIL;
  }
  struct inode* target = (struct inode*)(cur_disk.inodes + entry->inode_index * INODE_SIZE);
  st->uid = target->uid;
  st->gid = target->gid;
  st->filesize = target->size;
  st->type = target->type;
  st->permission = target->permissions;
  st->inode_index = target->inode_index;
  return SUCCESS;
}

int f_seek(int fd, int offset, int whence) {
  // check for invalid inputs
  if(fd < 0) {
    printf("f_seek: Invalid file descriptor!\n");
    return FAIL;
  }
  if(offset < 0) {
    printf("f_seek:   invalid offset: offset cannot be negative numbers\n");
    return FAIL;
  }
  struct file_table_entry* entry = open_ft->entries[fd];
  if(entry == NULL) {
    printf("f_seek:  Invalid file descriptor!\n");
    return FAIL;
  };
  struct inode* cur_inode = (struct inode*)(cur_disk.inodes + entry->inode_index * INODE_SIZE);
  if(whence == SEEKSET) {
    if(offset > cur_inode->size) {
      printf("fseek:    Invalid offset! Larger than file size\n");
      return FAIL;
    } else {
      entry->block_index = offset / BLOCKSIZE;
      entry->offset = offset % BLOCKSIZE;
      /*if(offset == entry->block_index * BLOCKSIZE + entry->offset) {
	return SUCCESS;
	}
      */
      struct table* datatable = (struct table*)malloc(sizeof(struct table));
      get_tables(entry, datatable);
      if(datatable->table_level == NONE) {
	free_struct_table(datatable);
	//printf("fseek:  file is empty\n");
	return SUCCESS;
      }
      entry->block_offset = datatable->cur_data_table[datatable->intable_index];
      free_struct_table(datatable);
      return SUCCESS;
    }
  } else if (whence == SEEKCUR) {
    int remain_bytes = cur_inode->size - entry->block_index * BLOCKSIZE - entry->offset;
    if(offset > remain_bytes) {
      printf("fseek:    Invalid offset! Seek outside of file range\n");
      return FAIL;
    } else {
      if(entry->offset + offset < BLOCKSIZE) {
	entry->offset += offset;
	return SUCCESS;
      } else {
	int moreoffset = offset - (BLOCKSIZE - entry->offset);
	entry->block_index += moreoffset / BLOCKSIZE;
	entry->offset = moreoffset % BLOCKSIZE;
	struct table* datatable = (struct table*)malloc(sizeof(struct table));
	get_tables(entry, datatable);
	if(datatable->table_level == NONE) {
	  free_struct_table(datatable);
	  printf("fseek:  file is empty\n");
	  return SUCCESS;
	}
	entry->block_offset = datatable->cur_data_table[datatable->intable_index];
	free_struct_table(datatable);
	return SUCCESS;
      }

    }
  }else if(whence == SEEKEND) {
    if(offset > cur_inode->size) {
      printf("fseek:    Invalid offset! Seek backwards outside of file range\n");
      return FAIL;
    } else {
      int pos = cur_inode->size - offset;
      entry->block_index = pos / BLOCKSIZE;
      entry->offset = pos % BLOCKSIZE;
      struct table* datatable = (struct table*)malloc(sizeof(struct table));
      get_tables(entry, datatable);
      if(datatable->table_level == NONE) {
	free_struct_table(datatable);
	printf("fseek:  file is empty\n");
	return SUCCESS;
      }
      entry->block_offset = datatable->cur_data_table[datatable->intable_index];
      free_struct_table(datatable);
      return SUCCESS;
    }
  } else {
    printf("fseek:    Invalid input! No matching value for 'whence'\n");
    return FAIL;
  }
  return FAIL;
}

int f_rewind(int fd) {
  if(fd < 0) {
    return FAIL;
  }
  struct file_table_entry* target = open_ft->entries[fd];
  if (target == NULL) {
    printf("f_rewind:   Invalid file descriptor\n");
    return FAIL;
  }
  struct inode* target_inode = (struct inode*)(cur_disk.inodes + target->inode_index * INODE_SIZE);
  target->block_index = 0;
  target->offset = 0;
  if(target_inode->size == 0) {
    target->block_offset = UNDEFINED;
  } else {
    target->block_offset = target_inode->dblocks[0];
  }
  return SUCCESS;
}


int f_open(char* filepath, int access, int mode) {
  if(!(access == OPEN_W || access == OPEN_R  || access == OPEN_A || access == OPEN_WR)) {
    printf("f_open:     Invalid access input\n");
    return FAIL;
  }

  char** parse_path = parse_filepath(filepath);
  int count = 0;
  char* prevdir = NULL;
  char* curdir = parse_path[count];
  int parent_fd = cur_disk.rootdir_fd;
  char parent_path[MAX_LENGTH];
  strcpy(parent_path, "");

  // check whether directories along the filepath exists
  while(curdir != NULL) {
    // get complete path
    if(strlen(parent_path) + strlen(curdir) + strlen("/") >= MAX_LENGTH) {
      free_parse(parse_path);
      printf("f_open: filepath invalid: file path too long\n");
      return FAIL;
    }
    strcat(parent_path, "/");
    strcat(parent_path, curdir);
    // check if parent directory exists
    count ++;
    prevdir = curdir;
    curdir = parse_path[count];
    if(curdir == NULL){
      break;
    }
    parent_fd = f_opendir(parent_path);
    if(parent_fd == FAIL) {
      printf("f_open: Directory %s along the way does not exists\n", parent_path);
      free_parse(parse_path);
      return FAIL;
    }
  }
  // now prevdir contains the file to be OPENED, parent dir is the parent Directory
  f_seek(parent_fd, 0, SEEK_SET);
  struct dirent* target_file = checkdir_exist(parent_fd, prevdir);

  // if file exists
  if (target_file != NULL) {
    if(target_file->type != REG) {
      printf("fopen:     file %s is not a regular file\n", prevdir);
      free_parse(parse_path);
      free(target_file);
      return FAIL;
    } else {
      if(access == OPEN_R || access == OPEN_A || access == OPEN_WR) {
        struct inode* target_inode = (struct inode*)(cur_disk.inodes + target_file->inode_index);
        // check permission
        int check = check_permission(target_inode, access);
        if (check == FAIL) {
          printf("f_open:     permission denied.\n");
          free_parse(parse_path);
          free(target_file);
          return FAIL;
        }
        struct file_table_entry* openfile = create_entry(parent_fd, target_file->inode_index, prevdir, access, REG);
        printf("fopen:     return value fd is %d\n", openfile->fd);
	free_parse(parse_path);
        free(target_file);
        return openfile->fd;
      } else if (access == OPEN_W) {
        // needs to remove the file and open and new one;
        //f_remove
        int removeres = f_remove(filepath, FALSE);
        assert(removeres == SUCCESS);
        int new_file_inode = create_file(parent_fd, prevdir, REG, mode);
        struct file_table_entry* new_entry = create_entry(parent_fd, new_file_inode, prevdir, access, REG);
        free(target_file);
	free_parse(parse_path);
        return new_entry->fd;
      }
    }

    // if file does not exists
  } else {
    // this is the case when file does not exists,needs to check access
    //createa file with create_file
    // open with create_entry and return the fd
    if(access == OPEN_R) {
      printf("f_open:     cannot read a not existing file!\n");
      free_parse(parse_path);
      free(target_file);
      return FAIL;
    }

    int new_file_inode = create_file(parent_fd, prevdir, REG, mode);
    struct file_table_entry* new_entry = create_entry(parent_fd, new_file_inode, prevdir, access, REG);
    free(target_file);
    free_parse(parse_path);
    return new_entry->fd;
  }
  free(target_file);
  free_parse(parse_path);
  return FAIL;
}


int f_write(void* buffer, int bsize, int fd) {
  //char name[20] __attribute__((cleanup(scope)));
  //sprintf(name, "f_write:%d ",fd);
  if(fd < 0) {
    return FAIL;
  }
  struct file_table_entry* writeto = open_ft->entries[fd];
  if(writeto == NULL) {
    printf("f_write:    invalid file descriptor\n");
  }
  struct inode* write_inode = (struct inode*)(cur_disk.inodes + writeto->inode_index * INODE_SIZE);
  // invalid fd
  if(writeto == NULL) {
    printf("f_write: invalid fd\n");
    return FAIL;
  }
  // do not have access
  if(writeto->access == OPEN_R) {
    printf("f_write:  only have reading access. \n");
    return FAIL;
  }

  if(write_inode->dblocks[0] == UNDEFINED) {
    int first_data_blockoffset = get_next_freeOffset();
    if(first_data_blockoffset == FAIL) {
      printf("f_write:   empty:      get next free block for data failed\n");
      return FAIL;
    }
    write_inode->dblocks[0] = first_data_blockoffset;
    writeto->block_index = 0;
    writeto->block_offset = first_data_blockoffset;
    write_inode->last_block_offset = first_data_blockoffset;
    writeto->offset = 0;
    write_disk_inode();
  }

  struct table* datatable = (struct table*)malloc(sizeof(struct table));
  datatable = get_tables(writeto, datatable);
  if(datatable == NULL) {
    printf("f_write: Get free blocks for f_write failed\n");
    return FAIL;
  }
  int byte_to_write = bsize;

  while(byte_to_write > 0) {
    int fileOffset = cur_disk.data_region_offset + writeto->block_offset * BLOCKSIZE + writeto->offset;
    lseek(cur_disk.diskfd, fileOffset, SEEK_SET);

    if(byte_to_write >= BLOCKSIZE - writeto->offset) {
      int cur_write_bytes = BLOCKSIZE - writeto->offset;
      void* cur_write = buffer + (bsize - byte_to_write);
      if(write(cur_disk.diskfd, cur_write, cur_write_bytes) != cur_write_bytes) {
        printf("f_write: writing %d byte failed\n", bsize - byte_to_write);
        free_struct_table(datatable);
        return FAIL;
      }
      get_next_boffset(datatable, writeto);
      byte_to_write -= cur_write_bytes;

      if(writeto->block_offset == write_inode->last_block_offset) {
        int last_inblock_offset = write_inode->size % BLOCKSIZE;
        if(writeto->offset + cur_write_bytes > last_inblock_offset) {
          write_inode->size += (cur_write_bytes + writeto->offset - last_inblock_offset);
        }
      }

    } else {
      int cur_write_bytes = byte_to_write;
      void* cur_buffer = buffer + (bsize - byte_to_write);
      int fileoffset = cur_disk.data_region_offset + writeto->block_offset * BLOCKSIZE + writeto->offset;
      lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
      if(write(cur_disk.diskfd, cur_buffer, cur_write_bytes) != cur_write_bytes) {
        printf("f_write:    write last data block failed\n");
        free_struct_table(datatable);
        return FAIL;
      }

      if(writeto->block_offset == write_inode->last_block_offset) {
        int last_inblock_offset = write_inode->size % BLOCKSIZE;
        if(writeto->offset + cur_write_bytes > last_inblock_offset) {
          write_inode->size += (cur_write_bytes + writeto->offset - last_inblock_offset);
        }
      }
      writeto->offset += byte_to_write;
      byte_to_write -= cur_write_bytes;
      f_seek(fd, 0, SEEKSET);
    }
  }
  write_disk_inode();
  free_struct_table(datatable);
  return bsize;
}


int f_close(int fd) {
  if(fd < 0) {
    printf("f_close:    Invalid file descriptor (negative)!\n");
    return FAIL;
  }
  struct file_table_entry* entry = open_ft->entries[fd];
  if(entry == NULL) {
    printf("f_close:    1:    Invalid file descriptor! \n");
    return FAIL;
  }
  if(entry->inode_index == ROOT_INDEX) {
    // need root directory to check
    return FAIL;
  }
  if(entry->type == DIR && entry->open_num > 0) {
    printf("f_close:   Cannot close directories containing opened files.\n");
    return FAIL;
  }
  char filepath[MAX_LENGTH];
  int copylength = strlen(entry->filepath);
  for(int i = strlen(entry->filepath) - 1; i >= 0; i --) {
    if(entry->filepath[i] != '/') {
      copylength --;
    } else {
      copylength --;
      break;
    }
  }
  strncpy(filepath, entry->filepath, copylength);
  filepath[copylength] = '\0';
  struct file_table_entry* parent_entry = NULL;
  if(strcmp(filepath, "/") == SUCCESS) {
    parent_entry = open_ft->entries[cur_disk.rootdir_fd];
  } else {
    for(int i = 0 ; i < MAX_OPENFILE; i++) {
      if(open_ft->entries[i] != NULL) {
        if(strcmp(open_ft->entries[i]->filepath, filepath) == SUCCESS) {
          parent_entry = open_ft->entries[i];
          break;
        }
      }
    }
  }
  if(parent_entry == NULL) {
    return FAIL;
  }
  if(parent_entry->open_num <= 0) {
    return FAIL;
  }
  parent_entry->open_num --;
  bzero(entry, sizeof(struct file_table_entry));
  free(entry);
  // update file table
  open_ft->free_id[fd] = fd;
  open_ft->free_fd_num ++;
  open_ft->filenum --;
  open_ft->entries[fd] = NULL;
  return SUCCESS;
}

int f_closedir(int dir_fd) {
  struct file_table_entry* entry = open_ft->entries[dir_fd];
  // check if not a DIR file
  if(entry->type != DIR) {
    //printf("f_closedir:     Cannot close a regular file with 'f_closedir'\n");
    return FAIL;
  }
  // check if have open files
  if(entry->open_num > 0) {
    //printf("f_closedir:     Cannot close a directory containing opened files\n");
    return FAIL;
  }
  //printf("f_closedir:   closing directory %s\n", entry->filepath);
  //printf("in open close dir before fclose\n");
  //print_openft();
  f_close(dir_fd);
  //printf("in open close dir aaafer fclose\n");
  //print_openft();
  //printf("f_closedir: end of calling 'f_close'\n");
  return SUCCESS;
}

int f_mkdir(char* filepath, int mode) {
  //print_free();
  char** parse_path = parse_filepath(filepath);
  int count = 0;
  char* prevdir = NULL;
  char* curdir = parse_path[count];
  int parent_fd = cur_disk.rootdir_fd;
  char parent_path[MAX_LENGTH];
  strcpy(parent_path, "");

  // check whether directories along the filepath exists
  while(curdir != NULL) {
    // get complete path
    if(strlen(parent_path) + strlen(curdir) + strlen("/") >= MAX_LENGTH) {
      free_parse(parse_path);
      printf("f_mkdir: filepath invalid: file path too long\n");
      return FAIL;
    } 
    strcat(parent_path, "/");
    strcat(parent_path, curdir);
    // check if parent directory exists
    count ++;
    prevdir = curdir;
    curdir = parse_path[count];
    if(curdir == NULL){
      break;
    }
    parent_fd = f_opendir(parent_path);
    if(parent_fd == FAIL) {
      printf("f_mkdir: Directory %s along the way does not exists\n", parent_path);
      free_parse(parse_path);
      return FAIL;
    }
  }
  // now prevdir contains the file to be OPENED, parent dir is the parent Directory
  f_seek(parent_fd, 0, SEEKSET);
  struct dirent* target_file = checkdir_exist(parent_fd, prevdir);
  if(target_file != NULL) {
    if(target_file->type == DIR) {
      printf("f_mkdir:      directory %s already exists\n", filepath);
      free_parse(parse_path);
      free(target_file);
      return FAIL;
    } else {
      int new_index = create_file(parent_fd, prevdir, DIR, mode);
      free(target_file);
      free_parse(parse_path);  
      return new_index;
    }
  }
  printf("f_mkdir:  Creating a new directory %s\n", filepath);
  int new_index = create_file(parent_fd, prevdir, DIR, mode);
  free(target_file);
  free_parse(parse_path);  
  return new_index;
}


int f_rmdir(char* filepath) {
  int dir_fd = f_opendir(filepath);
  if(dir_fd < 0) {
    return FAIL;
  }
  f_rewind(dir_fd);
  struct dirent* cur_dir = f_readdir(dir_fd);
  while(cur_dir != NULL) {
    if(cur_dir->type == REG) {
      char childpath[MAX_LENGTH];
      strcpy(childpath, filepath);
      strcat(childpath, "/");
      strcat(childpath, cur_dir->filename);
      printf("f_rmdir:  removing file %s\n", childpath);
      f_remove(childpath, FALSE);
    } else if(cur_dir->type == DIR) {
      char childpath[MAX_LENGTH];
      strcpy(childpath, filepath);
      strcat(childpath, cur_dir->filename);
      printf("f_rmdir:  removing directory file %s\n", childpath);
      f_rmdir(childpath);
    }
    free(cur_dir);
    cur_dir = f_readdir(dir_fd);
  }
  f_rewind(dir_fd);
  f_remove(filepath, TRUE);
  f_closedir(dir_fd);
  return SUCCESS;
}

int f_unmount() {
  /*
    1. free all entries int open file table
    2. free open file table
  */
  for(int i = 0; i < MAX_OPENFILE; i++) {
    struct file_table_entry* temp = open_ft->entries[i];
    if(temp != NULL) {
      free(temp);
    } 
  }
  free(open_ft);
  free(cur_disk.inodes);
  bzero(&(cur_disk.sb), sizeof(struct superblock));
  bzero(&(cur_disk), sizeof(struct fs_disk));
  return SUCCESS;
}

/*************************** HELPER FUNCTIONS **********************/
int print_openft(){
  for(int i = 0; i < MAX_OPENFILE; i++) {
    struct file_table_entry* temp = open_ft->entries[i];
    printf("========================  %d  ===============================\n", i);
    if(temp != NULL) {
      printf("contains file with fd %d and filename %s\n", temp->fd, temp->filepath);
    }
  }
  printf("=======================================================\n");
  return TRUE;
}

void get_inode(int inode_index) {
  struct inode* target = (struct inode*)(cur_disk.inodes + inode_index * INODE_SIZE);
  char permission[20];
  if(target->permissions == R) {
    strcpy(permission, "r--");
  } else if(target->permissions == RW) {
    strcpy(permission, "rw-");
  } else if(target->permissions == RWE) {
    strcpy(permission, "rwx");
  }else if(target->permissions == W) {
    strcpy(permission, "-w-");
  }else if(target->permissions == WE) {
    strcpy(permission, "-wx");
  }else if(target->permissions == N) {
    strcpy(permission, "---");
  }else if(target->permissions == RE) {
    strcpy(permission, "r-x");
  }else if(target->permissions == E) {
    strcpy(permission, "--x");
  }
  if(target->type == DIR) {
    printf("[type]: DIR \t[permission]: %s \t [filesize]: %d \t[uid]: %d\t [gid]: %d\t", permission, target->size, target->uid,target->gid);
  } else if(target->type == REG) {
    printf("[type]: REG \t[permission]: %s \t [filesize]: %d \t[uid]: %d\t [gid]: %d\t", permission, target->size, target->uid,target->gid);
  }
}

static char** parse_filepath(char* filepath) {
  char delim[2] = "/";
  int len = 20;
  int size = 50;
  int count = 0;
  char file_path[MAX_LENGTH];
  strcpy(file_path, filepath);
  char** parse_result = (char**)malloc(sizeof(char*) * size);
  bzero(parse_result, size * sizeof(char*));
  char* token;
  //printf("parse file path: filepath is %s\n", filepath);
  token = strtok(file_path, delim);
  while(token != NULL) {
    if(count >= size) {
      size += len;
      parse_result = realloc(parse_result, sizeof(char*) * size);
    }
    char* temp = malloc(sizeof(char) * (strlen(token) + 1));
    strcpy(temp, token);
    parse_result[count] = temp;
    //printf("parse file path: parse result %d is %s\n", count, parse_result[count]);
    token = strtok(NULL, delim);
    count ++;
  }
  parse_result[count] = NULL;
  return parse_result;
}



static void free_parse(char** parse_result) {
  int count = 0;
  char* token = parse_result[count];
  while(token != NULL) {
    free(token);
    count ++;
    token = parse_result[count];
  }
  free(parse_result);
}

static struct dirent* checkdir_exist(int parentdir_fd, char* target) {
  f_seek(parentdir_fd, 0, SEEKSET);
  struct file_table_entry* origin = open_ft->entries[parentdir_fd];
  int org_offset = origin->offset;
  int org_block_index = origin->block_index;
  int org_block_offset = origin->block_offset;
  struct dirent* temp = f_readdir(parentdir_fd);
  while(temp != NULL) {
    if(strcmp(temp->filename, target) == SUCCESS) {
      origin->offset = org_offset;
      origin->block_index = org_block_index;
      origin->block_offset = org_block_offset;
      return temp;
    }
    free(temp);
    temp = f_readdir(parentdir_fd);
  }
  origin->offset = org_offset;
  origin->block_index = org_block_index;
  origin->block_offset = org_block_offset;
  free(temp);
  return NULL;
}


static void update_ft(struct file_table_entry* new_entry, int new_index) {
  open_ft->free_id[new_index] = UNDEFINED;
  open_ft->free_fd_num --;
  open_ft->entries[open_ft->filenum] = new_entry;
  open_ft->filenum ++;
}

static int get_free_fd_index() {
  for(int i = 0; i < MAX_OPENFILE; i++) {
    if(open_ft->free_id[i] > 0) {
      return i;
    }
  }
  return FAIL;
}

static struct file_table_entry* create_entry(int parent_fd, int child_inode, char* childpath, int access, int type){
  struct file_table_entry* parent_entry = open_ft->entries[parent_fd];
  //printf("create entry chiled inode pased in is %d\n", child_inode);
  struct inode* result_inode = (struct inode*)(cur_disk.inodes + child_inode * INODE_SIZE);
  //printf("create entry: inode index is %d and size is %d\n", result_inode->inode_index, result_inode->size);

  // heyt the children path, and check whether the target file is already opened
  char resultpath[MAX_LENGTH];
  strcpy(resultpath, parent_entry->filepath);
  if(parent_fd != cur_disk.rootdir_fd){
    strcat(resultpath, "/");
  }
  strcat(resultpath, childpath);
  for(int i = 0; i < MAX_OPENFILE; i++) {
    struct file_table_entry* temp = open_ft->entries[i];
    if(temp == NULL) {
      continue;
    }
    //printf("create entry: \t checking entry with fd %d\n", temp->fd);
    if(strcmp(temp->filepath, resultpath) == SUCCESS) {
      //printf("Create entry: file %s already OPENED!!!!\n", resultpath);
      return temp;
    }
  }

  // create a new entry
  //printf("create_entry:   _____ file %s not open ____ needs to open a new one\n", resultpath);
  //printf("create_entry: curparent is %s and number of open file%d\n", parent_entry->filepath, parent_entry->open_num);
  struct file_table_entry* result = (struct file_table_entry*)malloc(sizeof(struct file_table_entry));
  strcpy(result->filepath, resultpath);
  result->access = access;
  result->inode_index = child_inode;
  result->type = type;
  if(type == DIR) {
    result->open_num = 0;
  } else {
    result->open_num = UNDEFINED;
  }
  if(access == OPEN_R || access == OPEN_W || access == OPEN_WR) {
    result->block_index = 0;
    result->offset = 0;
    result->block_offset = result_inode->size > 0 ? result_inode->dblocks[0] : UNDEFINED;
  } else if (access == OPEN_A) {
    //printf("create_entry:     appending\n");
    struct table* datatable = (struct table*)malloc(sizeof(struct table));
    result->block_index = result_inode->size % BLOCKSIZE == 0 ? result_inode->size / BLOCKSIZE : result_inode->size / BLOCKSIZE + 1;
    result->offset = result_inode->size % BLOCKSIZE;
    get_tables(result, datatable);
    result->block_offset = datatable->cur_data_table[datatable->intable_index];
  }

  //printf("create entry: childname is %s child inode is %d, blockoffset is %d\n", result->filepath, result->inode_index, result->block_offset);
  // assigning fd
  int freefd_index = get_free_fd_index();
  //printf("freeeeee entry is %d\n", freefd_index);
  result->fd = open_ft->free_id[freefd_index];
  open_ft->free_id[freefd_index] = UNDEFINED;
  update_ft(result, freefd_index);
  parent_entry->open_num ++;
  //open_ft->entries[result->fd] = result;
  return result;
}


static int create_file(int parent_fd, char* newfile_name, int type, int permission){
  //printf("____________ creating file creating %s in parent fd %d __________\n", newfile_name, parent_fd);
  struct file_table_entry* parent_entry = open_ft->entries[parent_fd];
  //printf("___________creating file: parent filepaht is %s\n", parent_entry->filepath);
  if(parent_entry == NULL) {
    printf("create file: invalid parent file descriptor. \n");
    return FAIL;
  }

  int new_inode_index = cur_disk.sb.free_inode_head;
  //printf("create file: new inode index is %d\n", new_inode_index);
  if(new_inode_index == END) {
    printf("create file: No more available inodes.\n");
    return FAIL;
  }
  struct inode* new_file_inode = (struct inode*)(cur_disk.inodes + new_inode_index * INODE_SIZE);
  cur_disk.sb.free_inode_head = new_file_inode->next_free_inode;
  //printf("\n\n\n\n\ncurrent free inode head is %d\n", cur_disk.sb.free_inode_head);
  write_newinode(new_file_inode, new_inode_index, parent_entry->inode_index, type);
  new_file_inode->permissions = permission;
  // need to write new entries into the parent directory file
  // needs to seek to the parent file end!!!!!!!!!!!!!!!!!!!
  write_disk_inode();
  write_disk_sb();
  //printf("create file:    before seeking!!\n");
  f_seek(parent_fd, 0, SEEKEND);
  //printf("create file:    aaaaaafter SEEKING result is %d\n", seek_res);
  struct dirent new_dirent;
  bzero(&new_dirent, sizeof(struct dirent));
  new_dirent.type = type;
  new_dirent.inode_index = new_inode_index;
  strcpy(new_dirent.filename, newfile_name);
  //printf("create_file:      nnnnnnnnn file name is %s\n", new_dirent.filename);
  f_write(&new_dirent, sizeof(struct dirent), parent_fd);
  //printf("create file:      wrote %d bytes\n", write);
  return new_inode_index;
}

static void write_newinode(struct inode* new_file_inode, int new_inode_index, int parent_inode, int type) {
  new_file_inode->inode_index = new_inode_index;
  new_file_inode->parent_inode = parent_inode;
  new_file_inode->permissions = DEFAULT_PERM;
  new_file_inode->type = type;
  new_file_inode->next_free_inode = UNDEFINED;
  new_file_inode->nlink = 1;
  new_file_inode->size = 0;
  new_file_inode->uid = cur_disk.uid;
  new_file_inode->gid = UNDEFINED;
  for(int i = 0; i < N_DBLOCKS; i++) {
    new_file_inode->dblocks[i] = UNDEFINED;
  }
  for(int i = 0; i < N_IBLOCKS; i++) {
    new_file_inode->iblocks[i] = UNDEFINED;
  }
  new_file_inode->i2block = UNDEFINED;
  new_file_inode->i3block = UNDEFINED;
  new_file_inode->last_block_offset = UNDEFINED;
  write_disk_inode();
}


static struct table* get_tables(struct file_table_entry* en, struct table* t) {
  int block_index = en->block_index;
  //struct table* t = (struct table*)malloc(sizeof(struct table));
  struct inode* node = (struct inode*)(cur_disk.inodes + en->inode_index * INODE_SIZE);
  // need to handle the case when file is emtyp and has ZERO data blocks assigned
  if(node->dblocks[0] == UNDEFINED) {
    t->table_level = NONE;
    return t;
  }

  int inblock_offset = node->size % BLOCKSIZE;
  t->inblock_offset = inblock_offset;
  // level direct:
  if(block_index < N_DBLOCKS) {
    //printf("Get tables level DDDDIRECT\n");

    t->table_level = DIRECT;
    t->level_one = node->dblocks;
    t->cur_data_table = node->dblocks;
    t->level_one_index = block_index;
    t->level_two = NULL;
    t->level_two_index = UNDEFINED;
    t->level_three = NULL;
    t->level_three_index = UNDEFINED;
    t->intable_index = block_index;
    t->cur_table_size = N_DBLOCKS;
    return t;
  }

  // level one:
  if(block_index < LEVELONE) {
    int first_index = (block_index - N_DBLOCKS) / TABLE_ENTRYNUM;
    int second_index = (block_index - N_DBLOCKS) - first_index * TABLE_ENTRYNUM;

    int first_offset = node->iblocks[first_index];
    int first_foffset = cur_disk.data_region_offset + first_offset * BLOCKSIZE;
    int* datablocks = (int*)malloc(BLOCKSIZE);
    lseek(cur_disk.diskfd, first_foffset, SEEK_SET);
    if(read(cur_disk.diskfd, (void*)datablocks, BLOCKSIZE) != BLOCKSIZE) {
      printf("get_tables:     level 1   read in dblock failed\n");
      free(datablocks);
      free(t);
      return NULL;
    }
    t->table_level = I1;
    t->level_one = node->iblocks;
    t->level_one_index = first_index;
    t->level_two = datablocks;
    t->cur_data_table = datablocks;
    t->level_two_index = second_index;
    t->level_three = NULL;
    t->level_three_index = UNDEFINED;
    t->intable_index = second_index;
    t->cur_table_size = TABLE_ENTRYNUM;
    return t;
  }

  // level two
  if(block_index < LEVELTWO) {
    int first_index = (block_index - LEVELONE) / TABLE_ENTRYNUM;
    int second_index = (block_index - LEVELONE) - first_index * TABLE_ENTRYNUM;
    int i2offset = node->i2block * BLOCKSIZE + cur_disk.data_region_offset;

    // read in level one table
    int* first_table = (int*)malloc(BLOCKSIZE);
    lseek(cur_disk.diskfd, i2offset, SEEK_SET);
    if(read(cur_disk.diskfd, (void*)first_table, BLOCKSIZE) != BLOCKSIZE) {
      printf("get_tables:     level 2   read in first table failed\n");
      free(first_table);
      free(t);
      return NULL;
    }

    // read in level two table
    int secondt_offset = cur_disk.data_region_offset + first_table[first_index] * BLOCKSIZE;
    int* second_table = (int*)malloc(BLOCKSIZE);
    lseek(cur_disk.diskfd, secondt_offset, SEEK_SET);
    if(read(cur_disk.diskfd, (void*)second_table, BLOCKSIZE) != BLOCKSIZE) {
      printf("get_tables:     level 2   read in dblock failed\n");
      free(second_table);
      free(t);
      return NULL;
    }
    t->table_level = I2;
    t->level_one = first_table;
    t->level_one_index = first_index;
    t->level_two = second_table;
    t->level_two_index = second_index;
    t->cur_data_table = second_table;
    t->level_three = NULL;
    t->level_three_index = UNDEFINED;
    t->intable_index = second_index;
    t->cur_table_size = TABLE_ENTRYNUM;
    return t;
  }

  if(block_index < LEVELTHREE) {
    int firstlevel_size = TABLE_ENTRYNUM * TABLE_ENTRYNUM;
    int first_index = (block_index - LEVELTWO) / firstlevel_size;
    int second_index = ((block_index - LEVELTWO) - first_index * firstlevel_size)  / TABLE_ENTRYNUM;
    int third_index = (block_index - LEVELTWO) - first_index * firstlevel_size - second_index * TABLE_ENTRYNUM;

    int i3offset = cur_disk.data_region_offset + node->i3block * BLOCKSIZE;
    lseek(cur_disk.diskfd, i3offset, SEEK_SET);
    // read in level one table
    int* first_table = (int*)malloc(BLOCKSIZE);
    if(read(cur_disk.diskfd, (void*)first_table, BLOCKSIZE) != BLOCKSIZE) {
      printf("get_tables:     level 3   read in first table failed\n");
      free(first_table);
      free(t);
      return NULL;
    }

    // read in level two table
    int second_foffset = cur_disk.data_region_offset + first_table[first_index];
    lseek(cur_disk.diskfd, second_foffset, SEEK_SET);
    int* second_table = (int*)malloc(BLOCKSIZE);
    if(read(cur_disk.diskfd, (void*)second_table, BLOCKSIZE) != BLOCKSIZE) {
      printf("get_tables:     level 3   read in 2nd table failed\n");
      free(second_table);
      free(t);
      return NULL;
    }

    int third_foffset = cur_disk.data_region_offset + second_table[second_index] * BLOCKSIZE;
    lseek(cur_disk.diskfd, third_foffset, SEEK_SET);
    int* third_table = (int*)malloc(BLOCKSIZE);
    if(read(cur_disk.diskfd, (void*)third_table, BLOCKSIZE) != BLOCKSIZE) {
      printf("get_tables:     level 3   read in 3rd table failed\n");
      free(third_table);
      free(t);
      return NULL;
    }

    t->table_level = I3;
    t->level_one = first_table;
    t->level_one_index = first_index;
    t->level_two = second_table;
    t->level_two_index = second_index;
    t->level_three = third_table;
    t->level_three_index = third_index;
    t->intable_index = third_index;
    t->cur_data_table = third_table;
    t->cur_table_size = TABLE_ENTRYNUM;
    return t;
  }
  return NULL;
}

// get the next block offset (used or new block)

// if getting to a new level, cannot memcpy, needs to malloc new buffer
static int get_next_boffset(struct table* t, struct file_table_entry* en) {
  //printf("\nGet_next_BoffsetL ************ block index is %d\n", en->block_index);
  //printf("getboffset 1 cur level %d table space %d and intable_index + 1 %d\n\n", t->table_level, t->cur_table_size, t->intable_index + 1);

  struct inode* node = (struct inode*)(cur_disk.inodes + en->inode_index * INODE_SIZE);
  //printf("inode last block offset %d and current block offset %d\n\n", node->last_block_offset, t->cur_data_table[t->intable_index]);
  if(t->table_level == NONE) {
    //printf("Get next boffset: none\n");
    int first_data_blockoffset = get_next_freeOffset();
    if(first_data_blockoffset == FAIL) {
      printf("Get next boffset:   None:      get next free block for data failed\n");
      return FAIL;
    }
    t->table_level = DIRECT;
    t->inblock_offset = 0;
    t->intable_index = 0;
    t->cur_table_size = N_DBLOCKS;
    t->cur_data_table = node->dblocks;
    t->level_one = node->dblocks;
    t->level_one_index = 0;
    t->level_two = NULL;
    t->level_two_index = UNDEFINED;
    t->level_three = NULL;
    t->level_three_index = UNDEFINED;
    //return t;
  }


  if(t->cur_data_table[t->intable_index] == node->last_block_offset) {
    // needs to assign new blocks
    //    printf("getboffset 1 cur level %d table space %d and intable_index + 1 %d\n", t->table_level, t->cur_table_size, t->intable_index + 1);
    if(t->intable_index + 1 < t->cur_table_size) {
      int new_datablock_offset = get_next_freeOffset();
      //printf("getboffset new block in same table:  next free is %d\n", new_datablock_offset);
      if(new_datablock_offset < 0) {
        printf("Get next boffset:   1:      get next free block for data failed\n");
        return FAIL;
      }
      //printf("same table new block 2\n");
      en->block_index ++;
      en->block_offset = new_datablock_offset;
      en->offset = 0;

      t->intable_index ++;
      // need to write this back to disk
      t->cur_data_table[t->intable_index] = new_datablock_offset;
      if(t->table_level != DIRECT) {
        int tableoffset = UNDEFINED;
        if(t->table_level == I1) {
          tableoffset = t->level_one[t->level_one_index];
        } else if (t->table_level == I2) {
          tableoffset = t->level_one[t->level_one_index];
        } else if(t->table_level == I3) {
          tableoffset = t->level_two[t->level_two_index];
        }
        // write the table data for the table back to disk
        //printf("same table table level %d and table offset %d and index in table %d contains value: %d\n", t->table_level, tableoffset, t->intable_index, t->cur_data_table[t->intable_index]);
        int table_fileOffset = cur_disk.data_region_offset + tableoffset * BLOCKSIZE;
        lseek(cur_disk.diskfd, table_fileOffset, SEEK_SET);
        void* buffer = (void*)(t->cur_data_table);
        if(write(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
          printf("get_next_offset:    2:  update index table failed\n");
          return FAIL;
        }
      }
      //update inode and write back to disk
      node->last_block_offset = new_datablock_offset;
      //printf("after write cur data table has intable index %d and value %d\n", t->intable_index, t->cur_data_table[t->intable_index]);
      /*
	if(t->table_level == DIRECT) {
	node->dblocks[t->intable_index] = new_datablock_offset;
	}
      */
      write_disk_inode();
      return new_datablock_offset;
    } else {
      //printf("boffset :     GGGGGEtting new table\n");
      //need to create new data tables
      if(t->table_level == DIRECT) {
	//printf("finished DDDDirect and create new table for I1\n");
	int new_table_offset = get_next_freeOffset();
	if(new_table_offset == FAIL) {
	  printf("Get next boffset:   get next free block for table failed\n");
	  return FAIL;
	}
	// update inode
	//printf("first ___Iblocks___ data table offset is %d\n", new_table_offset);
	node->iblocks[0] = new_table_offset;

	int* new_table = malloc(BLOCKSIZE);
	bzero(new_table, BLOCKSIZE);
	int new_data_offset = get_next_freeOffset();
	if(new_data_offset == FAIL) {
	  free(new_table);
	  printf("Get next boffset:   get next free block for data failed\n");
	  return FAIL;
	}
	new_table[0] = new_data_offset;
	// write new offset table
	int newtable_offset = cur_disk.data_region_offset + new_table_offset * BLOCKSIZE;
	lseek(cur_disk.diskfd, newtable_offset, SEEK_SET);
	if(write(cur_disk.diskfd, (void*)new_table, BLOCKSIZE) != BLOCKSIZE) {
	  free(new_table);
	  printf("Get next boffset:   1:  write new table block failed\n");
	  return FAIL;
	}
	//printf("updating to I1\n");
	// update entry
	en->block_index ++;
	en->block_offset = new_data_offset;
	en->offset = 0;
	// update table:
	t->table_level = I1;
	t->inblock_offset = 0;
	t->intable_index = 0;
	t->cur_table_size = TABLE_ENTRYNUM;
	t->level_one = node->iblocks;
	t->level_one_index = 0;
	//memcpy(t->level_two, new_table, BLOCKSIZE);
	t->level_two = new_table;
	//printf("&&&&&&&&&&&&& leveltwo first value %d\n", t->level_two[0]);
	t->level_two_index = 0;
	t->level_three = NULL;
	t->level_three_index = UNDEFINED;
	t->cur_data_table = t->level_two;
	node->last_block_offset = new_data_offset;
	write_disk_inode();
	return new_data_offset;
      }

      int new_i2 = FALSE;
      if(t->table_level == I1) {
	//printf("In I1  \t\t\tCreating table levelone index %d and level two %d\n",t->level_one_index, t->level_two_index );
	if(t->level_one_index == N_IBLOCKS - 1) {
	  //t->table_level = I2;
	  //printf("Finished IIIII1111 moving to _________2_________\n");
	  new_i2 = TRUE;
	} else {
	  int new_table_offset = get_next_freeOffset();
	  if(new_table_offset < 0) {
	    printf("get next boffset:    i2 get new table offset failed\n");
	    return FAIL;
	  }

	  t->level_one_index ++;
	  t->level_one[t->level_one_index] = new_table_offset;
	  int new_data_offset = get_next_freeOffset();
	  if(new_data_offset < 0) {
	    printf("get next boffset:     i2 get new data offset failed\n");
	    return FAIL;
	  }

	  int* new_table = (int*)malloc(BLOCKSIZE);
	  bzero(new_table, BLOCKSIZE);
	  new_table[0] = new_data_offset;
	  int fileoffset = cur_disk.data_region_offset + new_table_offset * BLOCKSIZE;
	  lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	  if(write(cur_disk.diskfd, (void*)new_table, BLOCKSIZE) != BLOCKSIZE) {
	    free(new_table);
	    printf("Get next boffset:     write new table block failed\n");
	    return FAIL;
	  }

	  // update table:
	  t->table_level = I1;
	  t->inblock_offset = 0;
	  t->intable_index = 0;
	  t->cur_table_size = TABLE_ENTRYNUM;
	  //t->level_one = node->iblocks;
	  //t->level_one_index = 0;
	  t->level_two = malloc(BLOCKSIZE);
	  memcpy(t->level_two, new_table, BLOCKSIZE);
	  t->level_two_index = 0;
	  t->level_three = NULL;
	  t->level_three_index = UNDEFINED;
	  t->cur_data_table = t->level_two;
	  // update entry
	  en->block_index ++;
	  en->offset = 0;
	  en->block_offset = new_data_offset;
	  node->last_block_offset = new_data_offset;
	  write_disk_inode();
	  free(new_table);
	  return new_data_offset;
	}
      }

      int new_i3 = FALSE;
      if(t->table_level == I2 || new_i2) {
	if(t->level_one_index == (TABLE_ENTRYNUM - 1) && new_i2 == FALSE) {
	  new_i3 = TRUE;
	  //t->table_level = I3;
	} else {
	  if(new_i2) {
	    t->table_level = I2;
	    int i2 = get_next_freeOffset();
	    if(i2 < 0) {
	      printf("Get next boffset:   5 I2:  write new table block failed\n");
	      return FAIL;
	    }
	    // update inode
	    node->i2block = i2;
	    int i2block_offset = get_next_freeOffset();
	    int i2data_offset = get_next_freeOffset(); // return value
	    if(i2block_offset < 0 || i2data_offset < 0 ) {
	      printf("Get next boffset:   6 I2:  write new table block failed\n");
	      return FAIL;
	    }
	    // write first level
	    //void* buffer = malloc(BLOCKSIZE);
	    int* level1 = (int*)malloc(BLOCKSIZE);
	    bzero(level1, BLOCKSIZE);
	    level1[0] = i2block_offset;
	    int fileoffset = cur_disk.data_region_offset + i2 * BLOCKSIZE;
	    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	    if(write(cur_disk.diskfd, level1, BLOCKSIZE) != BLOCKSIZE) {
	      free(level1);
	      printf("Get next boffset:   1 I2:  write new table block failed\n");
	      return FAIL;
	    }
	    //memcpy(t->level_one, buffer, BLOCKSIZE);
	    t->level_one = level1;
	    t->level_one_index = 0;

	    // write second level

	    int* level2 = (int*)malloc(BLOCKSIZE);
	    bzero(level2, BLOCKSIZE);
	    level2[0] = i2data_offset;
	    fileoffset = cur_disk.data_region_offset + i2block_offset * BLOCKSIZE;
	    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	    if(write(cur_disk.diskfd, level2, BLOCKSIZE) != BLOCKSIZE) {
	      free(level2);
	      printf("Get next boffset:   2 I2:  write new table block failed\n");
	      return FAIL;
	    }
	    //memcpy(t->level_two, level2, BLOCKSIZE);
	    t->level_two = level2;
	    t->level_two_index = 0;
	    t->level_three = NULL;
	    t->level_three_index = UNDEFINED;
	    // update rest of table:
	    t->inblock_offset = 0;
	    t->intable_index = 0;
	    t->cur_table_size = TABLE_ENTRYNUM;
	    t->cur_data_table = t->level_two;
	    // update entry:
	    en->block_index++;
	    en->block_offset = i2data_offset;
	    en->offset = 0;
	    node->last_block_offset = i2data_offset;
	    write_disk_inode();
	    return i2data_offset;
	  } else {
	    int i2block_offset = get_next_freeOffset();
	    int i2data_offset = get_next_freeOffset(); // return value
	    if(i2block_offset < 0 || i2data_offset < 0 ) {
	      printf("Get next boffset:   6 I2:  write new table block failed\n");
	      return FAIL;
	    }
	    t->level_one_index ++;
	    t->level_one[t->level_one_index] = i2block_offset;
	    // write the level_one back to disk
	    int fileoffset = cur_disk.data_region_offset + node->i2block * BLOCKSIZE;
	    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	    if(write(cur_disk.diskfd, t->level_one, BLOCKSIZE) != BLOCKSIZE) {
	      printf("get_next_offset:    I2:  1:     update level one index table failed\n");
	      return FAIL;
	    }

	    int* level2 = (int*)malloc(BLOCKSIZE);
	    bzero(level2, BLOCKSIZE);
	    level2[0] = i2data_offset;
	    fileoffset = cur_disk.data_region_offset + i2block_offset * BLOCKSIZE;
	    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	    if(write(cur_disk.diskfd, level2, BLOCKSIZE) != BLOCKSIZE) {
	      printf("get_next_offset:    I2:  1:     update level one index table failed\n");
	      return FAIL;
	    }

	    //update table:
	    t->inblock_offset = 0;
	    t->intable_index = 0;
	    t->cur_table_size = TABLE_ENTRYNUM;
	    t->cur_data_table = t->level_two;
	    memcpy(t->level_two, level2, BLOCKSIZE);
	    t->level_two_index = 0;
	    t->level_three = NULL;
	    t->level_three_index = UNDEFINED;
	    //update entry:
	    en->block_index ++;
	    en->offset = 0;
	    en->block_offset = i2data_offset;
	    node->last_block_offset = i2data_offset;
	    write_disk_inode();
	    free(level2);
	    return i2data_offset;
	  }
	}
      }

      // i3
      if(t->table_level == I3 || new_i3) {
	if(t->level_one_index == (TABLE_ENTRYNUM - 1) && new_i3 == FALSE) {
	  printf("get_next_offset:  I3  1:  exceeds file size\n");
	  return FAIL;
	}
	if(new_i3) {
	  t->table_level = I3;
	  int i3 = get_next_freeOffset();
	  if(i3 < 0) {
	    printf("Get next boffset:   I3  2:  GET I3BLOCK failed\n");
	    return FAIL;
	  }
	  node->i3block = i3;
	  int i3_level2 = get_next_freeOffset();
	  int i3_level3 = get_next_freeOffset();
	  int new_data_offset = get_next_freeOffset(); // return value
	  if(i3_level2 < 0 || i3_level3 < 0 || new_data_offset < 0 ) {
	    printf("Get next boffset:  I3   3:  get new offsets failed\n");
	    return FAIL;
	  }

	  // write first level
	  int* level1 = (int*)malloc(BLOCKSIZE);
	  bzero(level1, BLOCKSIZE);
	  level1[0] = i3_level2;
	  int fileoffset = cur_disk.data_region_offset + i3 * BLOCKSIZE;
	  lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	  if(write(cur_disk.diskfd, level1, BLOCKSIZE) != BLOCKSIZE) {
	    free(level1);
	    printf("Get next boffset:   i3  4:  write new table block failed\n");
	    return FAIL;
	  }
	  t->level_one = level1;
	  t->level_one_index = 0;

	  // write second level
	  int* level2 = (int*)malloc(BLOCKSIZE);
	  bzero(level2, BLOCKSIZE);
	  level2[0] = i3_level3;
	  fileoffset = cur_disk.data_region_offset + i3_level2 * BLOCKSIZE;
	  lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	  if(write(cur_disk.diskfd, level2, BLOCKSIZE) != BLOCKSIZE) {
	    free(level2);
	    printf("Get next boffset:   I3  5:  write new table block failed\n");
	    return FAIL;
	  }
	  t->level_two = level2;
	  t->level_two_index = 0;

	  // write third level
	  int* level3 = (int*)malloc(BLOCKSIZE);
	  bzero(level3, BLOCKSIZE);
	  level3[0] = new_data_offset;
	  fileoffset = cur_disk.data_region_offset + i3_level3 * BLOCKSIZE;
	  lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	  if(write(cur_disk.diskfd, level3, BLOCKSIZE) != BLOCKSIZE) {
	    free(level3);
	    printf("Get next boffset:   I3  6:  write new table block failed\n");
	    return FAIL;
	  }
	  t->level_three = level3;
	  t->level_three_index = 0;

	  //update rest of table:
	  t->inblock_offset = 0;
	  t->intable_index = 0;
	  t->cur_table_size = TABLE_ENTRYNUM;
	  t->cur_data_table = t->level_three;
	  // update entry:
	  en->block_index++;
	  en->block_offset = new_data_offset;
	  en->offset = 0;
	  node->last_block_offset = new_data_offset;
	  write_disk_inode();
	  return new_data_offset;
	} else {

	  if(t->level_two_index == TABLE_ENTRYNUM - 1) {
	    //need new level_two table
	    int i3_level2 = get_next_freeOffset();
	    int i3_level3 = get_next_freeOffset();
	    int new_data_offset = get_next_freeOffset(); // return value
	    if(i3_level2 < 0 || i3_level3 < 0 || new_data_offset < 0 ) {
	      printf("Get next boffset:  I3   10:  get new offsets failed\n");
	      return FAIL;
	    }
	    // update level one
	    t->level_one_index ++;
	    t->level_one[t->level_one_index] = i3_level2;
	    int fileoffset = cur_disk.data_region_offset + node->i3block * BLOCKSIZE;
	    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	    if(write(cur_disk.diskfd, t->level_one, BLOCKSIZE) != BLOCKSIZE) {
	      printf("Get next boffset:   I3   11:  UPDATE table block failed\n");
	      return FAIL;
	    }

	    // write level 2
	    int* level2 = (int*)malloc(BLOCKSIZE);
	    bzero(level2, BLOCKSIZE);
	    level2[0] = i3_level3;
	    fileoffset = cur_disk.data_region_offset + i3_level2 * BLOCKSIZE;
	    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	    if(write(cur_disk.diskfd, level2, BLOCKSIZE) != BLOCKSIZE) {
	      free(level2);
	      printf("Get next boffset:   I3  12:  write new table block failed\n");
	      return FAIL;
	    }
	    memcpy(t->level_two, level2, BLOCKSIZE);
	    t->level_two_index = 0;
	    free(level2);

	    //write level 3
	    int* level3 = (int*)malloc(BLOCKSIZE);
	    bzero(level3, BLOCKSIZE);
	    level3[0] = new_data_offset;
	    fileoffset = cur_disk.data_region_offset + i3_level3 * BLOCKSIZE;
	    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	    if(write(cur_disk.diskfd, level3, BLOCKSIZE) != BLOCKSIZE) {
	      free(level3);
	      printf("Get next boffset:   I3  13:  write new table block failed\n");
	      return FAIL;
	    }
	    memcpy(t->level_three, level3, BLOCKSIZE);
	    t->level_three_index = 0;
	    free(level3);
	    // update the rest of table
	    t->inblock_offset = 0;
	    t->intable_index = 0;
	    t->cur_table_size = TABLE_ENTRYNUM;
	    t->cur_data_table = t->level_two;
	    //update entry:
	    en->block_index ++;
	    en->offset = 0;
	    en->block_offset = new_data_offset;
	    node->last_block_offset = new_data_offset;
	    write_disk_inode();
	    return new_data_offset;
	  } else {
	    int i3level3 = get_next_freeOffset();
	    int new_data_offset = get_next_freeOffset();
	    if(i3level3 < 0 || new_data_offset < 0 ) {
	      printf("Get next boffset:  I3   7:  get new offsets failed\n");
	      return FAIL;
	    }
	    // update old level2
	    t->level_two_index ++;
	    t->level_two[t->level_two_index] = i3level3;
	    int fileoffset = cur_disk.data_region_offset + t->level_one[t->level_one_index] * BLOCKSIZE;
	    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	    if(write(cur_disk.diskfd, t->level_two, BLOCKSIZE) != BLOCKSIZE) {
	      printf("get_next_offset:    I3:  8:     update level two index table failed\n");
	      return FAIL;
	    }
	    // write new level3
	    int* level3 = (int*)malloc(BLOCKSIZE);
	    bzero(level3, BLOCKSIZE);
	    level3[0] = new_data_offset;
	    fileoffset = cur_disk.data_region_offset + i3level3 * BLOCKSIZE;
	    lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
	    if(write(cur_disk.diskfd, level3, BLOCKSIZE) != BLOCKSIZE) {
	      printf("get_next_offset:    I3:  9:     write new level3 failed. \n");
	      free(level3);
	      return FAIL;
	    }

	    // UPDATE TABLE:
	    t->inblock_offset = 0;
	    t->intable_index = 0;
	    t->cur_table_size = TABLE_ENTRYNUM;
	    memcpy(t->level_three, level3, BLOCKSIZE);
	    t->level_three_index = 0;
	    t->cur_data_table = t->level_three;
	    // update entry:
	    en->block_index ++;
	    en->offset = 0;
	    en->block_offset = new_data_offset;
	    node->last_block_offset = new_data_offset;
	    write_disk_inode();
	    free(level3);
	    return new_data_offset;
	  }
	}
      }
    }
  } else {
    // find the file's inner(not new) next block
    //  printf("just return the next one\n");
    int new_index = t->intable_index + 1;
    if(new_index < t->cur_table_size) {
      t->intable_index = new_index;
      en->block_index ++;
      en->block_offset = t->cur_data_table[new_index];
      //printf("getboffset table level %d intable index %d resutn val: %d\n", t->table_level, t->intable_index, t->cur_data_table[t->intable_index]);
      return t->cur_data_table[new_index];
    } else {
      en->block_index ++;
      t = get_tables(en, t);
      en->block_offset = t->cur_data_table[t->intable_index];
      en->offset = 0;
      //printf("2: table level is %d and  intable index %d resutn val: %d\n", t->table_level, t->intable_index, t->cur_data_table[t->intable_index]);
      return t->cur_data_table[t->intable_index];
    }
    //printf("get next boffset:   This is not possible\n");
    return FAIL;
  }
  return FAIL;
}

static void free_struct_table(struct table* t) {
  if(t->table_level == I1) {
    free(t->level_two);
  } else if (t->table_level == I2) {
    free(t->level_one);
    free(t->level_two);
  } else if(t->table_level == I3) {
    free(t->level_one);
    free(t->level_two);
    free(t->level_three);
  }
  free(t);
}

static int get_next_freeOffset() {
  int ret = cur_disk.sb.free_block_head;
  if(ret < 0) {
    return FAIL;
  }
  int freeOffset = cur_disk.data_region_offset + ret * BLOCKSIZE;
  lseek(cur_disk.diskfd, freeOffset, SEEK_SET);
  void* buffer = malloc(BLOCKSIZE);
  if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
    printf("getnextfree:    get next free Offset failed. \n");
    free(buffer);
    return FAIL;
  }
  struct free_block* oldhead = (struct free_block*)buffer;
  cur_disk.sb.free_block_head = oldhead->next_free;
  write_disk_sb();
  free(buffer);
  return ret;
}

static int write_disk_sb() {
  lseek(cur_disk.diskfd, BLOCKSIZE, SEEK_SET);
  if(write(cur_disk.diskfd, &(cur_disk.sb), BLOCKSIZE) != BLOCKSIZE) {
    printf("Write_disk_sb:  failed\n");
    return FAIL;
  }
  return SUCCESS;
}

static int write_disk_inode() {
  lseek(cur_disk.diskfd, BLOCKSIZE * 2, SEEK_SET);
  int size = (cur_disk.sb.data_offset - cur_disk.sb.inode_offset) * BLOCKSIZE;
  if(write(cur_disk.diskfd, cur_disk.inodes, size) != size) {
    printf("Write_disk_inodes:  failed\n");
    return FAIL;
  }
  return SUCCESS;
}
// free the inode
static int free_inode(int inode_index) {
  struct inode* tofree = (struct inode*)(cur_disk.inodes + inode_index * INODE_SIZE);
  for(int i = 0; i < N_DBLOCKS; i++) {
    tofree->dblocks[i] = UNDEFINED;
  }
  for(int i = 0; i < N_IBLOCKS; i++) {
    tofree->iblocks[i] = UNDEFINED;
  }
  int old_free_head = cur_disk.sb.free_inode_head;
  tofree->nlink = 0;
  tofree->next_free_inode = old_free_head;
  cur_disk.sb.free_inode_head = inode_index;
  write_disk_inode();
  write_disk_sb();
  return SUCCESS;
}

// remove the dirent from the parent dir
static int remove_dirent(int parent_fd, int parent_inode, int child_inode) {
  /*
    1. get parent's last dirent;
    2. traverse and get the block offset for the target entry (check the inode)
    3. replace the target entry
    4. write back to disk
  */
  struct inode* parentinode = (struct inode*)(cur_disk.inodes + parent_inode * INODE_SIZE);
  struct file_table_entry* parent_entry = open_ft->entries[parent_fd];
  int new_size = parentinode->size - sizeof(struct dirent);
  // read in all the dirent
  f_seek(parent_fd, 0, SEEKSET);
  char buffer[parentinode->size];
  f_read((void*)buffer, parentinode->size, parent_fd);
  // find the target, and update with the last one, if last one is the target, then do nothing
  struct dirent* cur = (struct dirent*)buffer;
  struct dirent* end = (struct dirent*)(buffer + new_size);
  int dirent_num = parentinode->size / sizeof(struct dirent);
  for(int i = 0; i < dirent_num; i++) {
    if(cur->inode_index == child_inode) {
      if(i != dirent_num - 1) {
        memcpy(cur, end, sizeof(struct dirent));
        break;
      } else {
        cur ++;
        break;
      }
    }
  }
  f_seek(parent_fd, 0, SEEKSET);
  f_write((void*)buffer, new_size, parent_fd);
  // update file size to oldsize - sizeof(dirent), update last block offset of inode
  parentinode->size -= sizeof(struct dirent);
  f_seek(parent_fd, new_size, SEEKSET);
  struct table* datatable = malloc(sizeof(struct table));
  get_tables(parent_entry, datatable);
  parentinode->last_block_offset = datatable->cur_data_table[datatable->intable_index];
  free_struct_table(datatable);
  return SUCCESS;
}

static int check_permission(struct inode* target, int access) {
  if(target->permissions == R) {
    if(access != OPEN_R) {
      return FAIL;
    }
  }
  if(target->permissions == E) {
    return FAIL;
  }
  if(target->permissions == RE) {
    if(access == OPEN_W || access == OPEN_WR) {
      return FAIL;
    }
  }
  if(target->permissions == WE) {
    if(access == OPEN_R || access == OPEN_WR) {
      return FAIL;
    }
  }
  return SUCCESS;
}

static int print_free() {
  int count = 0;
  int fb = cur_disk.sb.free_block_head;
  while (fb != FAIL) {
    count++;
    int freeOffset = cur_disk.data_region_offset + fb * BLOCKSIZE;
    lseek(cur_disk.diskfd, freeOffset, SEEK_SET);
    char buffer[BLOCKSIZE];
    if (read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
      printf("getnextfree:    get next free Offset failed. \n");
      return FAIL;
    }
    struct free_block* oldhead = (struct free_block*)buffer;
    fb = oldhead->next_free;
  }
  printf("\n__________________ Totol free blocks: %d ________________\n\n", count);
  return count;
}

void scope(char* name) {
  //printf("\n**********%s finishes\n", name);
  //print_free();
}

int change_mode(int mode, char* filepath) {
  int fd = f_open(filepath, OPEN_R, R);
  if(fd == FAIL) {
    fd = f_opendir(filepath);
  }
  if(fd < 0) {
    printf("Changing the mode for file %s failed. \n", filepath);
    return TRUE;
  }
  struct file_table_entry* entry = open_ft->entries[fd];
  struct inode* target = (struct inode*)(cur_disk.inodes + entry->inode_index * INODE_SIZE);
  target->permissions = mode;
  write_disk_inode();
  f_close(fd);
  return TRUE;
}
