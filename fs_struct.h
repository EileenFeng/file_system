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

enum fileseek {SSET, SCUR, SEND};
enum file_type{DIR, REG};

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
    int last_block_index; // keeps track of the last block index, useful during write when needs to allocate more blocks
}inode;

struct dirent {
    int type; // DIR OR REG
    int inode_indexs;
    char filename[MAX_NAMELEN];
}dirent;

// decides whether ctime is needed
struct fstat{
    int uid; //owner's id
    int gid;
    int filesize;
    int type;
    int permission;
    int inode_index;
} fstat;

struct file_table_entry{
    char filepath[MAX_LENGTH]; 
    int inode_index;
    int type;
    int block_index;/* index of the block relative to this file, for instance this is the 5th data
                      block for this file, then ‘block_index’ of this block is 5 */
	int block_offset; // offset of block in the data region
	int offset; //offset within a block
    int open_num; // if type == DIR, this field indicates the number of open files under this directory
}file_table_entry;

struct file_table{
    int filenum;
    int free_fd_num;
    int free_id[MAX_OPENFILE];
    struct file_table_entry entries[MAX_OPENFILE];
}file_table;

struct mounted_disk{
	int root_inode; // inode index of the root directory of the disk
	char mp_name[MAX_NAMELEN]; // mounting point absolute filepath
	int blocksize;
	int inode_region_offset;
	int data_region_offset;
	int free_block;
	int free_inode;
	int uid;
	int root_dir_inode_index;
	void* inode_region;
}mounted_disk;

