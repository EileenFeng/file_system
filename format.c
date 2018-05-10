#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fs_struct.h"

FILE* output_disk;

int main(int argc, char** argv) {
    struct superblock sb;
    struct inode root_inode;
    struct inode home;
    struct inode super_user;
    struct inode reg_user;
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
    sb.data_offset = 15;
    sb.free_inode_head = INODE_SETUP; // starts from the second inode,
                            // since the first one (with inode index 0) is always the root dir
    sb.free_block_head = 2; // starts from 0, used by root and home

    int sb_size = sizeof(struct superblock);
    int padding_length = BLOCKSIZE - sb_size;
    char sb_padding[padding_length];
    bzero(sb_padding, padding_length);

    if(fwrite(&sb, 1, sb_size, output_disk) != sb_size) {
        printf("Write superblock failed\n");
        return FAIL;
    } else {
        if(fwrite(&sb_padding, 1, padding_length, output_disk) != padding_length) {
            printf("Write superblock padding failed. \n");
            return FAIL;
        }
    }

    //writing root inode region
    root_inode.inode_index = ROOT_INDEX;
    root_inode.parent_inode = UNDEFINED;
    root_inode.permissions = DEFAULT_PERM; //rwxr-xr-x
    root_inode.type = DIR;
    root_inode.next_free_inode = UNDEFINED;
    root_inode.nlink = 1;
    root_inode.size = DIRENT_SIZE;
    root_inode.uid = UNDEFINED;
    root_inode.gid = UNDEFINED;
    root_inode.dblocks[0] = 0;
    for(int i = 1; i < N_DBLOCKS; i++) {
        root_inode.dblocks[i] = UNDEFINED;
    }
    for(int i = 0; i < N_IBLOCKS; i++) {
        root_inode.iblocks[i] = UNDEFINED;
    }
    root_inode.i2block = UNDEFINED;
    root_inode.i3block = UNDEFINED;
    root_inode.last_block_offset = 0;
    int inode_size = sizeof(struct inode);
    if(fwrite(&root_inode, 1, sizeof(root_inode), output_disk) != inode_size) {
        printf("Writing root directory inode failed\n");
        return FAIL;
    }

    // write home inode
    home.inode_index = HOME_INDEX;
    home.parent_inode = ROOT_INDEX; // parent inode is root inode
    home.permissions = DEFAULT_PERM; //rwxr-xr-x
    home.type = DIR;
    home.next_free_inode = UNDEFINED;
    home.nlink = 1;
    home.size = DIRENT_SIZE * 2;
    home.uid = UNDEFINED;
    home.gid = UNDEFINED;
    home.dblocks[0] = 1;
    for(int i = 1; i < N_DBLOCKS; i++) {
        home.dblocks[i] = UNDEFINED;
    }
    for(int i = 0; i < N_IBLOCKS; i++) {
        home.iblocks[i] = UNDEFINED;
    }
    home.i2block = UNDEFINED;
    home.i3block = UNDEFINED;
    home.last_block_offset = 1;
    if(fwrite(&home, 1, sizeof(home), output_disk) != inode_size) {
        printf("Writing home directory inode failed\n");
        return FAIL;
    }

    super_user.inode_index = SPRINODE;
    super_user.parent_inode = HOME_INDEX;
    super_user.permissions = DEFAULT_PERM; //rwxr-xr-x
    super_user.type = DIR;
    super_user.next_free_inode = UNDEFINED;
    super_user.nlink = 1;
    super_user.size = 0;
    super_user.uid = SPRUSER;
    super_user.gid = UNDEFINED;
    for(int i = 0; i < N_DBLOCKS; i++) {
        super_user.dblocks[i] = UNDEFINED;
    }
    for(int i = 0; i < N_IBLOCKS; i++) {
        super_user.iblocks[i] = UNDEFINED;
    }
    super_user.i2block = UNDEFINED;
    super_user.i3block = UNDEFINED;
    super_user.last_block_offset = UNDEFINED;
    if(fwrite(&super_user, 1, sizeof(super_user), output_disk) != inode_size) {
        printf("Writing super user directory inode failed\n");
        return FAIL;
    }

    reg_user.inode_index = REGINODE;
    reg_user.parent_inode = HOME_INDEX;
    reg_user.permissions = DEFAULT_PERM; //rwxr-xr-x
    reg_user.type = DIR;
    reg_user.next_free_inode = UNDEFINED;
    reg_user.nlink = 1;
    reg_user.size = 0;
    reg_user.uid = REGUSER;
    reg_user.gid = UNDEFINED;
    for(int i = 0; i < N_DBLOCKS; i++) {
        reg_user.dblocks[i] = UNDEFINED;
    }
    for(int i = 0; i < N_IBLOCKS; i++) {
        reg_user.iblocks[i] = UNDEFINED;
    }
    reg_user.i2block = UNDEFINED;
    reg_user.i3block = UNDEFINED;
    reg_user.last_block_offset = UNDEFINED;
    if(fwrite(&reg_user, 1, sizeof(reg_user), output_disk) != inode_size) {
        printf("Writing reg_user directory inode failed\n");
        return FAIL;
    }


    // write the rest of inodes
    int inode_num = (sb.data_offset - sb.inode_offset) * BLOCKSIZE / sizeof(struct inode);
    for(int i = INODE_SETUP; i < inode_num; i++) {
        struct inode temp_free;
        temp_free.inode_index = i;
        temp_free.parent_inode = i-1;
        temp_free.permissions = 777;
        temp_free.type = UNDEFINED;
        temp_free.next_free_inode = i + 1 >= inode_num ? END : i+1;
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
        temp_free.last_block_offset = UNDEFINED;
        if(fwrite(&temp_free, 1, sizeof(temp_free), output_disk) != sizeof(temp_free)) {
            printf("Write free inode %d failed\n", i);
            return FAIL;
        }
    }
    // write root block data block
    void* buffer = malloc(BLOCKSIZE);
    bzero(buffer, BLOCKSIZE);
    struct dirent* homedir = (struct dirent*)buffer;
    homedir->type = DIR;
    homedir->inode_index = HOME_INDEX;
    strcpy(homedir->filename, "home");
    if(fwrite(buffer, 1, BLOCKSIZE, output_disk) != BLOCKSIZE) {
        printf("write root dir data block failed\n");
    }

    // write home dir data block
    bzero(buffer, BLOCKSIZE);
    struct dirent* spusr = (struct dirent*)buffer;
    struct dirent* rusr= spusr + 1;
    spusr->type = DIR;
    spusr->inode_index = 2;
    strcpy(spusr->filename, "susr");
    rusr->type = DIR;
    rusr->inode_index = 3;
    strcpy(rusr->filename, "rusr");
    if(fwrite(buffer, 1, BLOCKSIZE, output_disk) != BLOCKSIZE) {
        printf("write home dir data block failed\n");
    }
    free(buffer);

    //writing the rest of blocks
    for (int i = 2; i < BLOCKNUM; i++) {
        struct free_block fb;
        fb.next_free = i+1 >= BLOCKNUM ? END : i+1;
        bzero(fb.padding, FREEB_PADDING);
        if(fwrite(&fb, 1, sizeof(fb), output_disk) != sizeof(fb)) {
            printf("Write free block %d failed\n", i);
            return FAIL;
        }
    }

    fclose(output_disk);
    return SUCCESS;

}
