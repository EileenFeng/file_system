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
static struct file_table_entry* create_entry(int parent_fd, int child_inode, char* target);
static struct dirent* checkdir_exist(int parentdir_fd, char* targetdir);
static char** parse_filepath(char* filepath);
static void update_ft(struct file_table_entry* new_entry, int new_index);
static void free_parse(char**);


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
      free_parse(parse_path);
      printf("opendir: Directory %s does not exists\n", curdir);
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
  int file_offset = cur_disk.data_region_offset + target->block_offset * 512 + target->offset;
  printf("Readdir: file offset readdir: %d\n", file_offset);
  if(file_offset > temp_inode->size + cur_disk.data_region_offset + temp_inode->dblocks[0] * BLOCKSIZE) {
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
      target->offset = 0;
      free(buffer);
    }
  } else {
    target->offset = new_offset;
  }
  printf("Readdir:  file name for the ret is %s\n", ret->filename);
  return ret;
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
    printf("[arse file path: parse result %d is %s\n", count, parse_result[count]);
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

static struct dirent* checkdir_exist(int parentdir_fd, char* targetdir) {
  struct dirent* temp = f_readdir(parentdir_fd);
  printf("checkdir after readdir\n");
  struct file_table_entry* origin = open_ft->entries[parentdir_fd];
  printf("checkdir getting fd %d origin is  NULL? %d\n", parentdir_fd, origin==NULL);
  int org_offset = origin->offset;
  int org_block_index = origin->block_index;
  int org_block_offset = origin->block_offset;
  printf("checkdir exists: parentdir fd is %d\n", parentdir_fd);
  while(temp != NULL) {
    if(strcmp(temp->filename, targetdir) == SUCCESS) {
      printf("**** In check, exists cur dirent filename %s, and target name %s\n", temp->filename, targetdir);
      return temp;
    }
    free(temp);
    printf(" checkdir exists assigning new dirent\n");
    temp = f_readdir(parentdir_fd);
    printf("checkdir exists after assign temp\n");
  }
  origin->offset = org_offset;
  origin->block_index = org_block_index;
  origin->block_offset = org_block_offset;
  return NULL;
}

static struct file_table_entry* create_entry(int parent_fd, int child_inode, char* childpath){
  struct file_table_entry* parent_entry = open_ft->entries[parent_fd];
  struct file_table_entry* result = (struct file_table_entry*)malloc(sizeof(struct file_table_entry));
  printf("create entry chiled inode pased in is %d\n", child_inode);
  struct inode* result_inode = (struct inode*)(cur_disk.inodes + child_inode * INODE_SIZE);
  printf("create entry: inode index is %d and size is %d\n", result_inode->inode_index, result_inode->size);
  strcpy(result->filepath, parent_entry->filepath);
  if(parent_fd != cur_disk.rootdir_fd){
    strcat(result->filepath, "/");
  }
  strcat(result->filepath, childpath);
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
