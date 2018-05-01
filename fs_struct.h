#define N_DBLOCKS 4
#define N_IBLOCKS 10

enum file_permission{};
enum file_type{DIR, REG};

struct user {
    int uid;
    int permission_value;
    int home_inode; //inode index for the user's home directory
    char username[20];
    char password[20];
}user;

// filename is inside of dirent; also needs to decide wheter c, a, mtime is needed
struct inode {
    int inode_index;
    int parent_inode; // inode index to the parent inode
    int permissions; // file permission
    int type;
    int next_inode;
    int nlink;
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
    char filename[56];
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
    int free_id[30];
    struct file_table_entry entries[30];
}file_table;

struct mounted_disk{
	int root_inode; // inode index of the root directory of the disk
	char mp_name[65]; // mounting point absolute filepath
	int blocksize;
	int inode_region_offset;
	int data_region_offset;
	int free_block;
	int free_inode;
	int uid;
	int root_dir_inode_index;
	void* inode_region;
}mounted_disk;

