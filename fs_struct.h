#define N_DBLOCKS 4
#define N_IBLOCKS 8
#define BLOCKNUM 17408
#define TESTNUM 20
#define UNDEFINED -10
#define END -1
#define FREEB_PADDING 508
#define BLOCKSIZE 512
#define FALSE 0
#define TRUE 1
#define FAIL -1
#define SUCCESS 0
#define MAX_NAMELEN 56
#define MAX_LENGTH 300
#define MAX_OPENFILE 50
#define DIRENT_SIZE 64
#define SUPERUSER 1
#define REGUSER 2
#define INODE_SETUP 4
#define ROOT_INDEX 0
#define HOME_INDEX 1
#define DEFAULT_PERM 755 //rwxr-xr-x
#define TABLE_ENTRYNUM 128
#define INODE_SIZE 96
#define LEVELONE 520
#define LEVELTWO 16904
#define LEVELTHREE 2114056

enum fileseek {SEEKSET, SEEKCUR, SEEKEND};
enum file_type{DIR, REG};
enum table_level{DIRECT, I1, I2, I3};

/*
1. inode index starting from 0
2. root directory always have inode index 0 (the first inode)
3. there are 16 inodes in total
4. empty directories has no data blocks assigned
*/

struct user {
  int uid;
  int permission_value;
  int home_inode; //inode index for the user's home directory
  char username[20];
  char password[20];
}user;

struct superblock {
  int blocksize; /* size of blocks in bytes */
  int inode_offset; /* offset of inode region in blocks */
  int data_offset; /* data region offset in blocks */
  int free_inode_head; /* head of free inode list, index */
  int free_block_head; /* head of free block list, index */
  //int swap_offset; /* swap region offset in blocks */
}superblock;

struct free_block {
  int next_free;
  char padding[FREEB_PADDING];
}free_block;

// filename is inside of dirent; also needs to decide wheter c, a, mtime is needed
struct inode {
  int inode_index; //starting from 0
  int parent_inode; // inode index to the parent inode
  int permissions; // file permission, uses 000 - 777 number to represent
  int type; // type of the file
  int next_free_inode; // only used if the current inode is free
  int nlink; // equals 0 if this inode is not used
  int size; // file size
  int uid;
  int gid;
  int dblocks[N_DBLOCKS];
  int iblocks[N_IBLOCKS];
  int i2block;
  int i3block;
  int last_block_offset; // keeps track of the last block index, useful during write when needs to allocate more blocks
}inode;

// size 64 bytes
struct dirent {
  int type; // DIR OR REG
  int inode_index;
  char filename[MAX_NAMELEN];
}dirent;

// decides whether ctime is needed
struct f_stat{
  int uid; //owner's id
  int gid;
  int filesize;
  int type;
  int permission;
  int inode_index;
} f_stat;

struct file_table_entry{
  int fd;
  int inode_index;
  int type;
  int block_index;/* index of the block relative to this file, for instance this is the 5th data
  block for this file, then ‘block_index’ of this block is 5 */
  int block_offset; // offset index of block in the data region
  int offset; //offset within a block; 0 <= offset <512
  //once hit 512, update offset = 0 as well as block_offset, incre block_index
  int open_num; // if type == DIR, this field indicates the number of open files under this directory
  char filepath[MAX_LENGTH];
}file_table_entry;

struct file_table{
  // file descriptor starts from 1
  int filenum;
  int free_fd_num;
  int free_id[MAX_OPENFILE];
  struct file_table_entry* entries[MAX_OPENFILE];
}file_table;

//shell
struct mounted_disk {
  char mp_name[MAX_NAMELEN]; // mounting point absolute filepath
  int diskfd;
}mounted_disk;

// file sytem
struct fs_disk{
  int diskfd;
  int uid;
  int data_region_offset; // byte offset
  //int inode_region_offset; // byte offset
  int rootdir_fd;
  struct inode* root_inode;
  struct superblock sb;
  void* inodes;
  //char mp_name[MAX_NAMELEN]; // mounting point absolute filepath
  //int root_inode; // inode index of the root directory of the disk
  //int blocksize;
  //int inode_region_offset;
  //int free_block;
  //int free_inode;
  //int root_dir_inode_index;
}fs_disk;

struct data_block{
  int block_index; // data block index relative to the data region
  char data[BLOCKSIZE];
}data_block;

struct table {
  int table_level;
  int inblock_offset;
  int intable_index; // position of the current block: index to the data block in data table, not the data block offset
  int cur_table_size;
  int* cur_data_table;
  int* level_one;
  int level_one_index; // index of table level_one to obtain level_two table
  int* level_two;
  int level_two_index;
  int* level_three;
  int level_three_index;
}table;

/*
direct: data: level one
indirect: data: level two
i2: data: level two
i3: data: level three
*/
