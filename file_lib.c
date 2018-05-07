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
//static struct data_block* data_buffer[20];
//static int data_buffer_count = 0;

/********** HELPER FUNCTION PROTOTYPES **********/
// helper functions
/*** create or return an existing open file entry, target is relative child path ***/
static struct file_table_entry* create_entry(int parent_fd, int child_inode, char* target);
/***check whether the parent directory contains the file 'targetdir' ***/
static struct dirent* checkdir_exist(int parentdir_fd, char*);
static char** parse_filepath(char* filepath);
static void update_ft(struct file_table_entry* new_entry, int new_index);
static void free_parse(char**);
/*** create a new reg or dir file, return the new file inode index ***/
/*** 'newfile_name' is relative path ***/
static int create_file(int parent_fd, char* newfile_name, int type);
static void write_newinode(struct inode* new_file_inode, int new_inode_index, int parent_inode, int type);
static struct table* get_tables(struct file_table_entry* en, struct table* t);
static int get_next_boffset(struct table* t, struct file_table_entry* en);
static void free_struct_table(struct table* t);
static int get_next_freeOffset();
//static int get_free_inode();
/************************ LIB FUNCTIONS *****************************/

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
  cur_disk.rootdir_fd = root_entry->fd;
  printf("fd for root directory is %d\n", root_entry->fd);
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


int f_opendir(char* filepath) {
  char** parse_path = parse_filepath(filepath);
  int count = 0;
  char* curdir = parse_path[count];
  printf("opendir: curdir is %s\n", curdir);
  if(curdir == NULL) {
    free_parse(parse_path);
    printf("opendir: root directory already opened\n");
    return SUCCESS;
  }
  int parent_fd = cur_disk.rootdir_fd;
  while(curdir != NULL) {
    printf("***opendir: curdir now is %s  and parent fd is %d\n", curdir, parent_fd);
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
    struct file_table_entry* new_entry = create_entry(parent_fd, child_dirent->inode_index, curdir);
    parent_fd = new_entry->fd;
    printf("opendir: Next parent dir filepath is %s and fd is %d\n", new_entry->filepath, new_entry->fd);
    count ++;
    curdir = parse_path[count];
  }
  printf("Opendir: return value is %d\n", parent_fd);
  return parent_fd;
}


struct dirent* f_readdir(int dir_fd) {
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
  printf("Readdir: blockoffset %d, block index %d, offset %d\n", target->block_offset, target->block_index, target->offset);
  int file_offset = cur_disk.data_region_offset + target->block_offset * BLOCKSIZE + target->offset;
  printf("Readdir: file offset readdir: %d\n", file_offset);
  printf("Readdir: file size is %d\n", temp_inode->size);
  if(file_offset >= temp_inode->size + cur_disk.data_region_offset + temp_inode->dblocks[0] * BLOCKSIZE) {
    printf("Readdir: End of file\n");
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
  int new_offset = target->offset + DIRENT_SIZE;
  if(new_offset >= BLOCKSIZE) {
    int new_blockindex = target->block_index + 1;
    printf("Readdir: Old block offset is %d and new index is %d\n", target->block_offset, target->block_index);

    // set up new block_index
    if(new_blockindex < N_DBLOCKS) {
      target->block_offset = temp_inode->dblocks[new_blockindex];
    } else{
      void* buffer = malloc(BLOCKSIZE);
      // one level
      if (new_blockindex < LEVELONE) {
        int first_index = (new_blockindex - N_DBLOCKS)/TABLE_ENTRYNUM;
        int second_index = (new_blockindex - N_DBLOCKS) - first_index * TABLE_ENTRYNUM;
        printf("Readdir: sum: %d, first index %d, second index %d\n", (new_blockindex - N_DBLOCKS), first_index, second_index);
        int fileOffset = temp_inode->iblocks[first_index] * BLOCKSIZE + cur_disk.data_region_offset;
        lseek(cur_disk.diskfd, fileOffset, SEEK_SET);
        if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
          printf("Readdir: Read index data block failed in f_readdir level one\n");
          free(buffer);
          free(ret);
          return NULL;
        }
        int* dblocks = (int*)buffer;
        target->block_offset = dblocks[second_index];
        printf("Readdir: New block offset is %d\n", target->block_offset);
        // two level
      } else if (new_blockindex < LEVELTWO) {
        int first_index = (new_blockindex - LEVELONE) / TABLE_ENTRYNUM;
        int second_index = (new_blockindex - LEVELONE) - first_index * TABLE_ENTRYNUM;
        int i2offset = temp_inode->i2block * BLOCKSIZE + cur_disk.data_region_offset;
        // read in first block
        lseek(cur_disk.diskfd, i2offset, SEEK_SET);
        if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
          printf("Readdir: Read index data block failed in f_readdir level two\n");
          free(buffer);
          free(ret);
          return NULL;
        }
        // read in the second index block
        int* dblocks = (int*)buffer;
        int sec_blockindex = dblocks[first_index];
        int second_blockoffset = cur_disk.data_region_offset + sec_blockindex * BLOCKSIZE;
        lseek(cur_disk.diskfd, second_blockoffset, SEEK_SET);
        if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
          printf("Readdir: Read index data block failed in f_readdir level two\n");
          free(buffer);
          free(ret);
          return NULL;
        }
        dblocks = (int*)buffer;
        target->block_offset = dblocks[second_index];
        printf("Readdir: new block_offset is %d\n", target->block_offset);
        // three level
      } else if (new_blockindex < LEVELTHREE) {
        int first_entry_size = TABLE_ENTRYNUM * TABLE_ENTRYNUM;
        int first_index = (new_blockindex - LEVELTWO) / first_entry_size;
        int second_index = ((new_blockindex - LEVELTWO) - first_index * first_entry_size) / TABLE_ENTRYNUM;
        int third_index = (new_blockindex - LEVELTWO) - first_index * first_entry_size - second_index * TABLE_ENTRYNUM;
        printf("Readdir: three: first index %d, second %d, third %d\n", first_index, second_index, third_index);

        int i3offset = cur_disk.data_region_offset + temp_inode->i3block * BLOCKSIZE;
        lseek(cur_disk.diskfd, i3offset, SEEK_SET);
        if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
          printf("Readdir: Read first index data block failed in f_readdir level three\n");
          free(buffer);
          free(ret);
          return NULL;
        }
        int* dblocks = (int*)buffer;
        int block2_index = dblocks[first_index];
        int block2_offset = cur_disk.data_region_offset + block2_index * BLOCKSIZE;
        lseek(cur_disk.diskfd, block2_offset, SEEK_SET);
        if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
          printf("Readdir: Read second index data block failed in f_readdir level two\n");
          free(buffer);
          free(ret);
          return NULL;
        }

        dblocks = (int*)buffer;
        int block3_index = dblocks[second_index];
        int block3_offset = cur_disk.data_region_offset + block3_index * BLOCKSIZE;
        lseek(cur_disk.diskfd, block3_offset, SEEK_SET);
        if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
          printf("Readdir: Read third index data block failed in f_readdir level two\n");
          free(buffer);
          free(ret);
          return NULL;
        }
        dblocks = (int*)buffer;
        target->block_offset = dblocks[third_index];
        printf("Readdir:  new offset is %d\n", target->block_offset);
      }
      target->block_index ++;
      target->offset = new_offset - BLOCKSIZE;
      free(buffer);
    }
  } else {
    target->offset = new_offset;
  }
  printf("Readdir:  file name for the ret is %s\n", ret->filename);
  return ret;
}

int f_seek(int fd, int offset, int whence) {
  // check for invalid inputs
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
      if(offset == entry->block_index * BLOCKSIZE + entry->offset) {
        return SUCCESS;
      }
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
    printf("s1\n");
    if(offset > cur_inode->size) {
      printf("fseek:    Invalid offset! Seek backwards outside of file range\n");
      return FAIL;
    } else {
      printf("s2\n");
      int pos = cur_inode->size - offset;
      entry->block_index = pos / BLOCKSIZE;
      entry->offset = pos % BLOCKSIZE;
      struct table* datatable = (struct table*)malloc(sizeof(struct table));
      printf("s3\n");
      get_tables(entry, datatable);
      printf("s4\n");
      if(datatable->table_level == NONE) {
        free_struct_table(datatable);
        printf("fseek:  file is empty\n");
        return SUCCESS;
      }
      entry->block_offset = datatable->cur_data_table[datatable->intable_index];
      printf("s5\n");
      free_struct_table(datatable);
      printf("s6\n");
      return SUCCESS;
    }
  } else {
    printf("fseek:    Invalid input! No matching value for 'whence'\n");
    return FAIL;
  }
  return FAIL;
}

int f_rewind(int fd) {
  struct file_table_entry* target = open_ft->entries[fd];
  if (target == NULL) {
    printf("Invalid file descriptor\n");
    return FAIL;
  }
  struct inode* target_inode = (struct inode*)(cur_disk.inodes + target->inode_index * INODE_SIZE);
  target->block_index = 0;
  target->offset = 0;
  if(target_inode->size == 0) {
    printf("Rewinding an empty file\n");
    target->block_offset = UNDEFINED;
  } else {
    target->block_offset = target_inode->dblocks[0];
  }
  printf("new block_offset is %d\n", open_ft->entries[fd]->block_offset);
  return SUCCESS;
}


int f_open(char* filepath, char* access) {
  char** parse_path = parse_filepath(filepath);
  int count = 0;
  char* prevdir = NULL;
  char* curdir = parse_path[count];
  int parent_fd = cur_disk.rootdir_fd;
  char parent_path[MAX_LENGTH];
  printf("fopen:  )))))))))))))))))))) 1\n");
  strcpy(parent_path, "");
  while(curdir != NULL) {
    printf("\nf_open:   ^^^^^^ current file token is %s, parent path is %s and parent fd %d\n", curdir, parent_path, parent_fd);
    // get complete path
    if(strlen(parent_path) + strlen(curdir) + strlen("/") >= MAX_LENGTH) {
      free_parse(parse_path);
      printf("f_open: filepath invalid: file path too long\n");
      return FAIL;
    }
    //if(parent_fd != cur_disk.rootdir_fd){
    strcat(parent_path, "/");
    //}
    strcat(parent_path, curdir);
    // check if parent directory exists
    count ++;
    printf("f_open: parent path for fopen is: %s\n", parent_path);
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
    printf("f_open:   ##### parent_fd is %d\n", parent_fd);
  }
  printf("f_open:  )))))))))))))))))))) 3  parent fd is %d\n", parent_fd);
  // now prevdir contains the file to be OPENED, parent dir is the parent Directory
  printf("f_open: checking whether file %s exists in directory with fd %d\n", prevdir, parent_fd);
  struct dirent* target_file = checkdir_exist(parent_fd, prevdir);
  if (target_file != NULL) {
    if(target_file->type != REG) {
      printf("fopen: file %s is not a regular file\n", prevdir);
      free_parse(parse_path);
      free(target_file);
      return FAIL;
    } else {
      struct file_table_entry* openfile = create_entry(parent_fd, target_file->inode_index, prevdir);
      printf("fopen: return value fd is %d\n", openfile->fd);
      return openfile->fd;
    }
  } else {
    // this is the case when file does not exists,needs to check access
    //createa file with create_file
    // open with create_entry and return the fd
    printf("f_open:\t ccccccreating new file\n");
    int new_file_inode = create_file(parent_fd, prevdir, REG);
    struct file_table_entry* new_entry = create_entry(parent_fd, new_file_inode, prevdir);
    
    return new_entry->fd;
  }
  printf("fopen:  the end\n");
  return FAIL;
}


int f_write(void* buffer, int bsize, int fd) {
  struct file_table_entry* writeto = open_ft->entries[fd];
  struct inode* write_inode = (struct inode*)(cur_disk.inodes + writeto->inode_index * INODE_SIZE);
  if(writeto == NULL) {
    printf("f_write: invalid fd\n");
    return FAIL;
  }
  printf("f_write:     first:  fd is %d cur data block index %d offset %d\n", fd, writeto->block_index, writeto->offset);
  if(write_inode->dblocks[0] == UNDEFINED) {                                                                   
        int first_data_blockoffset = get_next_freeOffset();
	if(first_data_blockoffset == FAIL) {
	  printf("f_write:   empty:      get next free block for data failed\n");
	  return FAIL;
	}
	printf("fwrite:   --------------- gettting a new block? %d\n", first_data_blockoffset);
	write_inode->dblocks[0] = first_data_blockoffset;
	printf("File inode %d dblocks is %d\n", write_inode->inode_index, write_inode->dblocks[0]);
	writeto->block_index = 0;
	writeto->block_offset = first_data_blockoffset;
	write_inode->last_block_offset = first_data_blockoffset;
	writeto->offset = 0;
	
  }    
  
  struct table* datatable = (struct table*)malloc(sizeof(struct table));
  datatable = get_tables(writeto, datatable);
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
      // need to fill up the current block, and then get the next block, update block_index, block_offset, offset = 0; and possible
      // last_block_offset, and file size
      //get next boffset
      //1. write the data into the current block
      //2. call get_next_boffset, which will update datatable and entry, and inode when applciable
      ////not need 3. update file entry's block_index ++, blockoffset to return valueof getnextboffset, and offset = 0
    } else {
      /*
      if(write_inode->dblocks[0] == UNDEFINED) {
        int first_data_blockoffset = get_next_freeOffset();
        if(first_data_blockoffset == FAIL) {
          printf("f_write:   empty:      get next free block for data failed\n");
          return FAIL;
        }
        printf("fwrite:   gettting a new block?\n");
        write_inode->dblocks[0] = first_data_blockoffset;
        writeto->block_index = 0;
        writeto->block_offset = first_data_blockoffset;
        write_inode->last_block_offset = first_data_blockoffset;
        writeto->offset = 0;
      }
      */
      printf("f_write:    final block to write.\n");
      int cur_write_bytes = byte_to_write;
      void* cur_buffer = buffer + (bsize - byte_to_write);
      // hard code:
      struct dirent* temp = (struct dirent*)cur_buffer;
      printf("fwriteL hahahhaha buffer: %s\n", temp->filename);

      printf("f_write:     cur data block index %d offset %d\n", writeto->block_index, writeto->offset);
      printf("f_write:   bsize %d     cur_write_bytes is %d\n", bsize, cur_write_bytes);
      int fileoffset = cur_disk.data_region_offset + writeto->block_offset * BLOCKSIZE + writeto->offset;
      lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
      if(write(cur_disk.diskfd, cur_buffer, cur_write_bytes) != cur_write_bytes) {
        printf("f_write:    2:   write data block failed\n");
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
      printf("f_write:\t before sssssseek block index %d and offset %d\n", writeto->block_index, writeto->offset);
      f_seek(fd, 0, SEEKSET);
      printf("f_write:\t after ssssseek block index %d and offset %d\n", writeto->block_index, writeto->offset);    
    }
  }
  return bsize;
}

/* handled below
// if write within the same block
if(bsize + writeto->offset < BLOCKSIZE) {
if(write_inode->last_block_offset == writeto->block_offset) {
// might exceeds file size
int offset_inblock = write_inode->size % BLOCKSIZE;
if(bsize + writeto->offset > offset_inblock) {
write_inode->size += (bsize + writeto->offset - offset_inblock);
}
}
int fileoffset = cur_disk.data_region_offset + writeto->block_offset * BLOCKSIZE + writeto->offset;
lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
if(write(cur_disk.diskfd, buffer, bsize) != bsize) {
printf("f_write: 1: write to data block failed\n");
return FAIL;
}
writeto->offset += bsize;
return bsize;



} else {
*/
/*
  int total_blocknum = write_inode->size % BLOCKSIZE == 0? write_inode->size / BLOCKSIZE : write_inode->size / BLOCKSIZE + 1;

  struct table* datatable = get_tables(writeto);
  int add_block_num = ((datatable->inblock_offset + bsize - BLOCKSIZE) % BLOCKSIZE) == 0 ? (datatable->inblock_offset + bsize - BLOCKSIZE) / BLOCKSIZE : (datatable->inblock_offset + bsize - BLOCKSIZE) / BLOCKSIZE + 1;
  printf("f_write: needs to write %d more data blocks.\n", add_block_num);



  if(datatable->intable_index + add_block_num < TABLE_ENTRYNUM) {
  // need not to add new data tables
  } else {
  // need to add data blocks
  }
  // now write blocks
  }
*/


/*************************** HELPER FUNCTIONS **********************/
static char** parse_filepath(char* filepath) {
  char delim[2] = "/";
  int len = 20;
  int size = 20;
  int count = 0;
  char file_path[MAX_LENGTH];
  strcpy(file_path, filepath);
  char** parse_result = (char**)malloc(sizeof(char*) * size);
  char* token;
  printf("parse file path: filepath is %s\n", filepath);
  token = strtok(file_path, delim);
  while(token != NULL) {
    if(count >= size) {
      size += len;
      parse_result = realloc(parse_result, sizeof(char*) * size);
    }
    char* temp = malloc(sizeof(char) * (strlen(token) + 1));
    strcpy(temp, token);
    parse_result[count] = temp;
    printf("parse file path: parse result %d is %s\n", count, parse_result[count]);
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



static void update_ft(struct file_table_entry* new_entry, int new_index) {
  open_ft->free_id[new_index] = UNDEFINED;
  open_ft->free_fd_num --;
  open_ft->entries[open_ft->filenum] = new_entry;
  open_ft->filenum ++;
}



static struct dirent* checkdir_exist(int parentdir_fd, char* target) {
  printf("========== checking %s exists in fd %d\n", target, parentdir_fd);
  struct file_table_entry* origin = open_ft->entries[parentdir_fd];
  //printf("checkdir getting fd %d origin is  NULL? %d\n", parentdir_fd, origin==NULL);
  int org_offset = origin->offset;
  int org_block_index = origin->block_index;
  int org_block_offset = origin->block_offset;
  printf("\ncheckdir: \t !!!!!!!!!!cur block_offset %d and offset %d\n\n", origin->block_offset, org_offset);

  struct dirent* temp = f_readdir(parentdir_fd);
  printf("\ncheckdir: \t ---- afte readdir !!!!!!!!!!cur block_index %d and offset %d\n\n", org_block_index, org_offset);   
  //printf("checkdir exists: OOOOFset parentdir fd is %d origin offset is %d\n", parentdir_fd, origin->offset);
  while(temp != NULL) {
    printf("Now checking %s\n", temp->filename);
    if(strcmp(temp->filename, target) == SUCCESS) {
      printf("*|*|*|* In check, exists cur dirent filename %s, and target name %s\n", temp->filename, target);
      origin->offset = org_offset;
      origin->block_index = org_block_index;
      origin->block_offset = org_block_offset;
      printf("?????? org_block_offset %d\n", org_block_offset);
      printf("+++++++++++++----- checkdir end offset %d blockindex %d blockoffset %d for fd %d\n", origin->offset, origin->block_index, origin->block_offset, parentdir_fd);
      return temp;
    }
    free(temp);
    printf(" checkdir exists:   %s not match assigning new dirent\n", temp->filename);
    temp = f_readdir(parentdir_fd);
    printf("checkdir exists after assign temp\n");
  }
  printf("?????? ----- org_block_offset %d\n", org_block_offset); 
  origin->offset = org_offset;
  origin->block_index = org_block_index;
  origin->block_offset = org_block_offset;
  printf("?????? org_block_offset %d\n", org_block_offset);   
  printf("+++++++++++++ checkdir end offset %d blockindex %d blockoffset %d for fd %d\n", origin->offset, origin->block_index, origin->block_offset, parentdir_fd);
  return NULL;
}



static struct file_table_entry* create_entry(int parent_fd, int child_inode, char* childpath){
  struct file_table_entry* parent_entry = open_ft->entries[parent_fd];
  printf("create entry chiled inode pased in is %d\n", child_inode);
  struct inode* result_inode = (struct inode*)(cur_disk.inodes + child_inode * INODE_SIZE);
  printf("create entry: inode index is %d and size is %d\n", result_inode->inode_index, result_inode->size);
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
    if(strcmp(temp->filepath, resultpath) == SUCCESS) {
      printf("Create entry: file %s already OPENED!!!!\n", resultpath);
      return temp;
    }
  }
  struct file_table_entry* result = (struct file_table_entry*)malloc(sizeof(struct file_table_entry));
  strcpy(result->filepath, resultpath);
  result->inode_index = child_inode;
  result->type = DIR;
  result->block_index = 0;
  result->offset = 0;
  result->open_num = 0;
  result->block_offset = result_inode->size > 0 ? result_inode->dblocks[0] : UNDEFINED;
  printf("create entry: childname is %s child inode is %d, blockoffset is %d\n", result->filepath, result->inode_index, result->block_offset);
  // assigning fd
  int freefd_index = MAX_OPENFILE - open_ft->free_fd_num;
  result->fd = open_ft->free_id[freefd_index];
  update_ft(result, freefd_index);
  parent_entry->open_num ++;
  return result;
}


static int create_file(int parent_fd, char* newfile_name, int type){
  printf("____________ creating file creating %s in parent fd %d __________\n", newfile_name, parent_fd);
  struct file_table_entry* parent_entry = open_ft->entries[parent_fd];
  printf("___________creating file: parent filepaht is %s\n", parent_entry->filepath);
  if(parent_entry == NULL) {
    printf("create file: invalid parent file descriptor. \n");
    return FAIL;
  }

  int new_inode_index = cur_disk.sb.free_inode_head;
  printf("create file: new inode index is %d\n", new_inode_index);
  if(new_inode_index == END) {
    printf("create file: No more available inodes.\n");
    return FAIL;
  }
  struct inode* new_file_inode = (struct inode*)(cur_disk.inodes + new_inode_index * INODE_SIZE);
  cur_disk.sb.free_inode_head = new_file_inode->next_free_inode;
  write_newinode(new_file_inode, new_inode_index, parent_entry->inode_index, type);
  // need to write new entries into the parent directory file
  // needs to seek to the parent file end!!!!!!!!!!!!!!!!!!!
  printf("create file:    before seeking!!\n");
  int seek_res = f_seek(parent_fd, 0, SEEKEND);
  printf("create file:    aaaaaafter SEEKING result is %d\n", seek_res);
  struct dirent new_dirent;
  new_dirent.type = REG;
  new_dirent.inode_index = new_inode_index;
  strcpy(new_dirent.filename, newfile_name);
  printf("create_file:      nnnnnnnnn file name is %s\n", new_dirent.filename);
  f_write(&new_dirent, sizeof(struct dirent), parent_fd);
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
}


static struct table* get_tables(struct file_table_entry* en, struct table* t) {
  int block_index = en->block_index;
  //struct table* t = (struct table*)malloc(sizeof(struct table));
  struct inode* node = (struct inode*)(cur_disk.inodes + en->inode_index * INODE_SIZE);
  // need to handle the case when file is emtyp and has ZERO data blocks assigned
  if(node->dblocks[0] == UNDEFINED) {
    t->table_level = NONE;
    printf("get_tables: EMPTY\n");
    return t;
  }

  int inblock_offset = node->size % BLOCKSIZE;
  t->inblock_offset = inblock_offset;
  // level direct:
  if(block_index < N_DBLOCKS) {
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
  struct inode* node = (struct inode*)(cur_disk.inodes + en->inode_index * INODE_SIZE);
  if(t->table_level == NONE) {
    int first_data_blockoffset = get_next_freeOffset();
    if(first_data_blockoffset == FAIL) {
      printf("Get next boffset:   None:      get next free block for data failed\n");
      return FAIL;
    }
    printf("get_next_boffset:   first_table \n");
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
    return t;
  }

  if(t->cur_data_table[t->intable_index] == node->last_block_offset) {
    // needs to assign new blocks
    if(t->intable_index + 1 < t->cur_table_size) {
      int new_datablock_offset = get_next_freeOffset();
      if(new_datablock_offset == FAIL) {
        printf("Get next boffset:   1:      get next free block for data failed\n");
        return FAIL;
      }
      en->block_index ++;
      en->block_offset = new_datablock_offset;
      en->offset = 0;

      t->intable_index ++;
      // need to write this back to disk
      t->cur_data_table[t->intable_index] = new_datablock_offset;
      int tableoffset = UNDEFINED;
      if(t->table_level == I1) {
        tableoffset = t->level_one_index;
      } else if (t->table_level == I2) {
        tableoffset = t->level_one_index;
      } else if(t->table_level == I3) {
        tableoffset = t->level_two_index;
      }
      // write the table data for the table back to disk
      int table_fileOffset = cur_disk.data_region_offset + tableoffset * BLOCKSIZE;
      lseek(cur_disk.diskfd, table_fileOffset, SEEK_SET);
      if(write(cur_disk.diskfd, t->cur_data_table, BLOCKSIZE) != BLOCKSIZE) {
        printf("get_next_offset:    2:  update index table failed\n");
        return FAIL;
      }
      //update inode and write back to disk
      node->last_block_offset = new_datablock_offset;
      if(t->table_level == DIRECT) {
        node->dblocks[t->intable_index] = new_datablock_offset;
      }

      return new_datablock_offset;
    } else {
      //need to create new data tables
      if(t->table_level == DIRECT) {
        int new_table_offset = get_next_freeOffset();
        if(new_table_offset == FAIL) {
          printf("Get next boffset:   get next free block for table failed\n");
          return FAIL;
        }
        // update inode
        node->iblocks[0] = new_table_offset;

        int* new_table = (int*)malloc(BLOCKSIZE);
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
        t->level_two_index = 0;
        t->level_three = NULL;
        t->level_three_index = UNDEFINED;
        t->cur_data_table = t->level_two;

        return new_data_offset;
      }

      int new_i2 = FALSE;
      if(t->table_level == I1) {
        if(t->level_one_index == N_IBLOCKS - 1 && t->level_two_index == TABLE_ENTRYNUM - 1) {
          //t->table_level = I2;
          new_i2 = TRUE;
        } else {
          int new_table_offset = get_next_freeOffset();
          if(new_table_offset < 0) {
            printf("get next boffset:   3:  i2 get new table offset failed\n");
            return FAIL;
          }

          t->level_one_index ++;
          t->level_one[t->level_one_index] = new_table_offset;
          int new_data_offset = get_next_freeOffset();
          if(new_data_offset < 0) {
            printf("get next boffset:   4:  i2 get new data offset failed\n");
            return FAIL;
          }

          int* new_table = (int*)malloc(BLOCKSIZE);
          new_table[0] = new_data_offset;
          int fileoffset = cur_disk.data_region_offset + new_table_offset * BLOCKSIZE;
          lseek(cur_disk.diskfd, fileoffset, SEEK_SET);
          if(write(cur_disk.diskfd, new_table, BLOCKSIZE) != BLOCKSIZE) {
            free(new_table);
            printf("Get next boffset:   2:  write new table block failed\n");
            return FAIL;
          }

          // update table:
          t->table_level = I1;
          t->inblock_offset = 0;
          t->intable_index = 0;
          t->cur_table_size = TABLE_ENTRYNUM;
          //t->level_one = node->iblocks;
          //t->level_one_index = 0;
          memcpy(t->level_two, new_table, BLOCKSIZE);
          t->level_two_index = 0;
          t->level_three = NULL;
          t->level_three_index = UNDEFINED;
          t->cur_data_table = t->level_two;
          // update entry
          en->block_index ++;
          en->offset = 0;
          en->block_offset = new_data_offset;
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
            free(level3);
            return new_data_offset;
          }
        }
      }
    }
  } else {
    // find the file's inner(not new) next block
    int new_index = t->intable_index + 1;
    if(new_index < t->cur_table_size) {
      t->intable_index = new_index;
      return t->cur_data_table[new_index];
    } else {
      en->block_index ++;
      t = get_tables(en, t);
      en->block_offset = t->cur_data_table[t->intable_index];
      en->offset = 0;
      return t->cur_data_table[t->intable_index];
    }
    printf("get next boffset:   This is not possible\n");
    return FAIL;
  }



  /*
    int new_index = t->intable_index + 1;
    if(new_index < t->cur_table_size) {
    if(t->table_level == DIRECT) {
    return t->level_one[new_index];
    }
    if(t->table_level == I1) {
    return t->level_two[new_index];
    }
    if(t->table_level == I2) {
    return t->level_two[new_index];
    }
    if(t->table_level == I3) {
    return t->level_three[new_index];
    }
    } else {
    //need new data blocks
    if(t->table_level == DIRECT) {
    // assign new table for iblocks[0]
    int new_table_offset = get_next_freeOffset();
    if(new_table_offset == FAIL) {
    printf("Get next boffset:   get next free block for table failed\n");
    return FAIL;
    }
    node->iblocks[0] = new_table_offset;

    int* new_table = (int*)malloc(BLOCKSIZE);
    int new_data_offset = get_next_freeOffset();
    if(new_data_offset == FAIL) {
    printf("Get next boffset:   get next free block for data failed\n");
    return FAIL;
    }


    }
    if(t->table_level == I1) {
    return t->level_two[new_index];
    }
    if(t->table_level == I2) {
    return t->level_two[new_index];
    }
    if(t->table_level == I3) {
    return t->level_three[new_index];
    }
    }
  */
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
  printf("getnextfree:    ret value is %d     and new head %d\n", ret, oldhead->next_free);
  cur_disk.sb.free_block_head = oldhead->next_free;
  printf("getnextfree: \tnew free_block_head %d\n",cur_disk.sb.free_block_head); 
  return ret;
}


/*
  static struct dirent** get_dirents(int inode_index) {
  struct inode* target = (struct inode*)(cur_disk.inodes + inode_index * BLOCKSIZE);
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

  struct dirent** res = (struct dirent**)malloc(sizeof(struct dirent*) * size);
  void* buffer = malloc(BLOCKSIZE);
  for(int i = 0; i < N_DBLOCKS; i++) {
  if(bytestoread <= 0) {
  free(buffer);
  return res;
  }
  bzero(buffer, BLOCKSIZE);
  int read_bytes = bytestoread < BLOCKSIZE ? bytestoread : BLOCKSIZE;
  int block_index = target->dblocks[i];
  lseek(cur_disk.diskfd, cur_disk.data_region_offset + block_index * BLOCKSIZE, SEEK_SET);
  if(read(cur_disk.diskfd, buffer, BLOCKSIZE) != BLOCKSIZE) {
  printf("read in data block in get dirents failed in dblocks\n");
  free(buffer);

  return NULL;
  }
  }

  }
*/

/*
  1. parse
  2. open
  3. write
  4. open
*/
