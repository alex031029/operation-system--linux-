#ifndef FILECONTROL_H
#define FILECONTROL_H


const int TABLE_SIZE=64;
const int DATA_SIZE=1024;
const int BLOCK_SIZE=1024;
const int BLOCK_NUM=1024;
const double INODE_RATIO=0.1;
const int EXT2_MAGIC=0xef53;

class superblock{
public:
    int inodes_count;       //the number of used inodes
    int blocks_count;       //the number of used blocks
    int free_blocks_count;  //the number of free blocks
    int free_inodes_count;  //the number of free inodes
    int first_data_block;   //the 1st data block
    int magic;              //the check whether the disk is correct format
    int block_size;         //the size of block
    char bitmap[BLOCK_NUM/2];   //the bitmap
    int set_bitmap_true(int);
    int set_bitmap_false(int);
    bool get_bitmap(int);
    int get_size(int);
    int get_empty_inode(void);
    int get_empty_block(void);
};
class inode{
public:
    int size;       //the number of directories in this inode,including . and ..
    int data;       //record the place of beginning address of the data of this inode in blocks
    bool isroot;    //return true when root
    bool isdir;     //return true when directory
    int block;      //record the place of this inode in blocks

};
struct inode_map{
    char filename[12];      //the name of subdirectory
    int inumber;            //record the place of subdirectory in blocks
};


typedef char data_block[DATA_SIZE];     //to record the data
typedef inode_map dir_block[TABLE_SIZE];    //the subdirectories of a directory

void console(void);

#endif // FILECONTROL_H

