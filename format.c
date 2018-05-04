#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fs_struct.h"


#define BLOCKSIZE 512
#define FALSE 0
#define TRUE 1
#define FAIL -1
#define SUCCESS 0

FILE* output_disk;

int main(int argc, char** argv) {
    struct superblock sb;
    struct inode root_inode;
    char bootbuffer[BLOCKSIZE];

    // open output disk image for writing 
    output_disk = fopen("DISK", "wb+");
    if (output_disk == NULL) {
        printf("Create 'DISK' file failed.\n");
        return FAIL;
    }

    // writing boot block
    bzero(bootbuffer, BLOCKSIZE);
    if(fwrite(bootbuffer, 1, BLOCKSIZE, output_disk) != BLOCKSIZE) {
        printf("Write boot block failed\n");
        return FAIL;
    }

    //writing super block
    sb.blocksize = BLOCKSIZE;
    sb.inode_offset = 0; // 16 inodes in total
    sb.data_offset = 3;
    sb.free_inode_head = 1; // starts from the second inode, 
                            // since the first one (with inode index 0) is always the root dir
    sb.free_block_head = 0; // starts from 0
    
    int sb_size = sizeof(struct superblock);
    int padding_length = BLOCKSIZE - sb_size;
    char sb_padding[padding_length];
    bzero(sb_padding, padding_length);
    printf("padding length is %d\n", padding_length);
    printf("super block size is %d\n", sb_size);
    printf("sum of sb size and padding is %d\n", sb_size + padding_length);

    if(fwrite(&sb, 1, sb_size, output_disk) != sb_size) {
        printf("Write superblock failed\n");
        return FAIL;
    } else {
        if(fwrite(&sb_padding, 1, padding_length, output_disk) != padding_length) {
            printf("Write superblock padding failed. \n");
            return FAIL;
        }
    }

    //writing inode region
    root_inode.inode_index = 0;
    root_inode.parent_inode = -1;
    root_inode.permissions = 755; //rwxr-xr-x
    root_inode.type = DIR;
    root_inode.next_free_inode = UNDEFINED;
    root_inode.nlink = 1;
    root_inode.size = 0;
    root_inode.uid = UNDEFINED;
    root_inode.gid = UNDEFINED;
    for(int i = 0; i < N_DBLOCKS; i++) {
        root_inode.dblocks[i] = UNDEFINED;
    }
    for(int i = 0; i < N_IBLOCKS; i++) {
        root_inode.iblocks[i] = UNDEFINED;
    }
    root_inode.i2block = UNDEFINED;
    root_inode.i3block = UNDEFINED;
    root_inode.last_block_index = UNDEFINED;
    int inode_size = sizeof(struct inode);
    printf("inode size is %d and root inodes %d\n", inode_size, sizeof(root_inode));
    if(fwrite(&root_inode, 1, sizeof(root_inode), output_disk) != inode_size) {
        printf("Writing root directory inode failed\n");
        return FAIL;
    }

    int inode_num = (sb.data_offset - sb.inode_offset) * BLOCKSIZE / sizeof(struct inode);
    printf("Number of inodes is %d\n", inode_num);
    for(int i = 1; i < inode_num; i++) {
        struct inode temp_free;
        temp_free.inode_index = i;
        temp_free.parent_inode = i-1;
        temp_free.permissions = 777;
        temp_free.type = UNDEFINED;
        temp_free.next_free_inode = i + 1 >= inode_num ? END : i+1;
        printf("temp_free next is %d for inode %d\n", temp_free.next_free_inode, i);
        temp_free.nlink = 0;
        temp_free.size = 0;
        temp_free.uid = UNDEFINED;
        temp_free.gid = UNDEFINED;
        for(int j = 0; j < N_DBLOCKS; j++) {
            temp_free.dblocks[j] = UNDEFINED;
        }
        for(int j = 0; j < N_IBLOCKS; j++) {
            temp_free.iblocks[j] = UNDEFINED;
        }
        temp_free.i2block = UNDEFINED;
        temp_free.i3block = UNDEFINED;
        temp_free.last_block_index = UNDEFINED;
        if(fwrite(&temp_free, 1, sizeof(temp_free), output_disk) != sizeof(temp_free)) {
            printf("Write free inode %d failed\n", i);
            return FAIL;
        }
        printf("Finish writing free inode %d\n", i);
    }

    for (int i = 0; i < BLOCKNUM; i++) {
        struct free_block fb;
        printf("free block size is %d\n", sizeof(struct free_block));
        fb.next_free = i+1 >= TESTNUM ? END : i+1;
        printf("free block %d next free is %d\n", i, fb.next_free);
        bzero(fb.padding, FREEB_PADDING);
        if(fwrite(&fb, 1, sizeof(fb), output_disk) != sizeof(fb)) {
            printf("Write free block %d failed\n", i);
            return FAIL;
        }
        printf("Finish writing free block %d\n", i);
    }

    fclose(output_disk);
    return SUCCESS;

}
