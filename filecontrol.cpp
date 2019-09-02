#include "filecontrol.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <windows.h>


superblock nowsb;   //the current superblock
inode nowin;        //the current directory
dir_block nowdir;   //the subdirectories of current directory
FILE * f;           //for write and read from disk
HANDLE hCon;

enum Color { DARKBLUE = 1, DARKGREEN, DARKTEAL, DARKRED, DARKPINK,
             DARKYELLOW, GRAY, DARKGRAY, BLUE, GREEN, TEAL, RED, PINK, YELLOW, WHITE };

void SetColor(Color c){
        if(hCon == NULL)
                hCon = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hCon, c);
}
// to change the color of words
int superblock::set_bitmap_true(int p)
{
    int x=p/2;
    int y=p%2;
    if(y==1)
        bitmap[x]|=0x01;
    else
        bitmap[x]|=0x10;
}
//make the pth block true in bitmap
int superblock::set_bitmap_false(int p)
{
    int x=p/2;
    int y=p%2;
    if(y==1)
        bitmap[x]&=0x10;
    else
        bitmap[x]&=0x01;
}
//make teh pth block false in bitmap
bool superblock::get_bitmap(int p)
{
    int x=p/2;
    int y=p%2;
    if(y==1)
        return bitmap[x]&0x01;
    else
        return (bitmap[x]&0x10)>>1;
}
//get the value of bitmap of pth block
int superblock::get_size(int p)
{
    return p*block_size;
}
//for seek to the position of pth block
int superblock::get_empty_block()
{
    for(int i=0;i<BLOCK_NUM;i++)
        if(!this->get_bitmap(i))
        {
            blocks_count++;
            free_blocks_count--;
            return i;
        }
    SetColor(RED);
    printf("there has been no empty block to create\n");
    SetColor(GRAY);
    return -1;
}
//get a empty block
int superblock::get_empty_inode()
{
    if(this->free_inodes_count<=0)
    {
        SetColor(RED);
        printf("there has been no empty space to create an inode\n");
        SetColor(GRAY);
        return -1;
    }
    for(int i=0;i<TABLE_SIZE;i++)
    {
        if(strlen(nowdir[i].filename)==0)
        {
            inodes_count++;
            free_inodes_count--;
            return i;
        }
    }
    SetColor(RED);
    printf("there has been no empty space to create an inode\n");
    SetColor(GRAY);
    return -1;
}
//get a empty inode
int set_inode_table(int i, char * name, int bnum)
{
    strcpy(nowdir[i].filename,name);
    nowdir[i].inumber=bnum;
    return 1;
}
//used in creating directory
int name_exist(char *name)
{
    for(int i=0;i<TABLE_SIZE;i++)
    {
        if(strcmp(nowdir[i].filename,name)==0)
            return i;
    }
    return -1;
}
//to check whether the name has been used
int format(char filename[]) //to format the disk
{
    superblock sb;
    inode root;
    dir_block root_block;
    sb.block_size=BLOCK_SIZE;
    sb.blocks_count=3;          //the superblock, the inode of root and the block of root.
    sb.inodes_count=1;          //the inode of root
    sb.free_blocks_count=BLOCK_SIZE-sb.blocks_count;
    sb.free_inodes_count=BLOCK_SIZE*INODE_RATIO-sb.inodes_count;
    sb.first_data_block=1;
    sb.magic=EXT2_MAGIC;        //for checking whether the disk is correct
    memset(sb.bitmap,0,sizeof(sb.bitmap));
    nowsb=sb;
    nowsb.set_bitmap_true(0);
    nowsb.set_bitmap_true(1);
    nowsb.set_bitmap_true(2);
    f=fopen(filename,"wb+");
    if(!f)
    {
        SetColor(RED);
        printf("cannot open the file\n");
        SetColor(GRAY);
        return -1;
    }
    fseek(f,0,SEEK_SET);
    fwrite(&nowsb,sizeof(superblock),1,f);      //set the superblock as the 0th block
    root.isroot=true;
    root.isdir=true;
    root.size=1;
    root.data=2;
    root.block=1;
    fseek(f,1*nowsb.block_size,SEEK_SET);
    fwrite(&root,sizeof(inode),1,f);
    memset(root_block,0,sizeof(root_block));
    strcpy(root_block[0].filename,".");         //point to root itself
    root_block[0].inumber=1;                    //set the root as the 1st block
    fseek(f,2*nowsb.block_size,SEEK_SET);
    fwrite(&root_block,sizeof(dir_block),1,f);
    data_block d;
    memset(d,0,sizeof(d));
    fseek(f,3*nowsb.block_size,SEEK_SET);
    for(int i=3;i<nowsb.blocks_count+nowsb.free_blocks_count;i++)
    {
        fwrite(&d,sizeof(data_block),1,f);
    }
    SetColor(YELLOW);
    printf("block_size=%d\n",nowsb.block_size);     //print the status of the disk
    printf("inodes_count=%d\tblocks_count=%d\n",nowsb.inodes_count,nowsb.blocks_count);
    printf("free_inodes_count=%d\tfree_block_count=%d\n",nowsb.free_inodes_count,nowsb.free_blocks_count);
    printf("format ok!\n");
    SetColor(GRAY);
    fclose(f);
    return 1;
}
int load(char filename[])   //to open the disk
{
    superblock sb;
    inode root;
    dir_block root_block;
    f=fopen(filename,"rb+");
    if(!f)
    {
        SetColor(RED);
        printf("cannot open the file\n");
        SetColor(GRAY);
        return -1;
    }
    fseek(f,0,SEEK_SET);
    fread(&sb,sizeof(superblock),1,f);
    if(sb.magic!=EXT2_MAGIC)            //check the magic value
    {
        SetColor(RED);
        printf("not correct disk!\n");
        SetColor(GRAY);
        return -1;
    }
    nowsb=sb;
    fseek(f,nowsb.get_size(nowsb.first_data_block),SEEK_SET);
    fread(&root,sizeof(inode),1,f);
    if  (!root.isdir)
    {
        SetColor(RED);
        printf("not a directory!\n");
        SetColor(GRAY);
        return -1;
    }
    nowin=root;             //make the current directory inode as the root
    fseek(f,nowsb.get_size(root.data),SEEK_SET);
    fread(&root_block,sizeof(dir_block),1,f);
    for(int i=0;i<TABLE_SIZE;i++)
        nowdir[i]=root_block[i];
    SetColor(YELLOW);
    printf("block_size=%d\n",nowsb.block_size);     //print the status of the disk
    printf("inodes_count=%d\tblocks_count=%d\n",nowsb.inodes_count,nowsb.blocks_count);
    printf("free_inodes_count=%d\tfree_block_count=%d\n",nowsb.free_inodes_count,nowsb.free_blocks_count);
    printf("load file ok!\n");
    SetColor(GRAY);
    return 1;
}
int create_dir(char dirname[])  //for the command "mkdir"
{
    int space,inode_ptr,newdir_ptr;
    inode in;
    dir_block newdir;
    if(strcmp(dirname,"..")==0||strcmp(dirname,".")==0)
    {
        SetColor(RED);
        printf("cannot create a directory called %s\n",dirname);
        SetColor(GRAY);
        return -1;
    }//the .. and . directories cannot be created
    else if(name_exist(dirname)!=-1)
    {
        SetColor(RED);
        printf("directory with name \"%s\" has existed\n",dirname);
        SetColor(GRAY);
        return -1;
    }
    space=nowsb.get_empty_inode();
    if(space==-1)
        return -1;          //no empty inode
    inode_ptr=nowsb.get_empty_block();
    if(inode_ptr==-1)
        return -1;          //no empty block
    set_inode_table(space,dirname,inode_ptr);
    nowsb.set_bitmap_true(inode_ptr);           //set the bitmap for inode
    nowin.size++;
    in.isroot=false;
    in.isdir=true;
    in.block=inode_ptr;
    newdir_ptr=nowsb.get_empty_block();         //get a data block
    nowsb.set_bitmap_true(newdir_ptr);          //set the bitmap for data block
    if(newdir_ptr==-1)
        return -1;
    in.data=newdir_ptr;
    in.size=2;                          //including . and .. directories
    memset(newdir,0,sizeof(newdir));
    strcpy(newdir[0].filename,".");     //point to the new directory itself
    newdir[0].inumber=inode_ptr;
    strcpy(newdir[1].filename,"..");    //point to the father directory
    newdir[1].inumber=nowin.block;

    fseek(f,0,SEEK_SET);
    fwrite(&nowsb,sizeof(superblock),1,f);

    fseek(f,nowsb.get_size(nowin.block),SEEK_SET);
    fwrite(&nowin,sizeof(inode),1,f);

    fseek(f,nowsb.get_size(nowin.data),SEEK_SET);
    fwrite(nowdir,sizeof(dir_block),1,f);

    fseek(f,nowsb.get_size(inode_ptr),SEEK_SET);
    fwrite(&in,sizeof(inode),1,f);

    fseek(f,nowsb.get_size(newdir_ptr),SEEK_SET);
    fwrite(newdir,sizeof(dir_block),1,f);
    fflush(f);
    return 1;
}
int remove_dir(char dirname[])      //for the command "rmdir"
{
    int ptr;
    inode in;
    ptr=name_exist(dirname);
    if((ptr<2&&(ptr==0||!(ptr==1&&nowin.isroot==true)))||strcmp(dirname,"..")==0||strcmp(dirname,".")==0)
    {
        SetColor(RED);
        printf("the father and directory itself cannot be removed!\n");
        SetColor(GRAY);
        return -1;
    }//the .. and . directory cannot be removed
    if(ptr==-1)
    {
        SetColor(RED);
        printf("directory %s does not exit!\n",dirname);
        SetColor(GRAY);
        return -1;
    }//check the name whether exits
    fseek(f,nowsb.get_size(nowdir[ptr].inumber),SEEK_SET);
    fread(&in,sizeof(inode),1,f);
    if(!in.isdir)
    {
        SetColor(RED);
        printf("%s is not a directory!\n",dirname);
        SetColor(GRAY);
        return -1;
    }//if not a file ,return
    if(in.size!=2)
    {
        SetColor(RED);
        printf("there are still files in directory %s\n",dirname);
        SetColor(GRAY);
        return -1;
    }//can just remove the directory with nothing inside
    nowsb.set_bitmap_false(in.data);
    strcpy(nowdir[ptr].filename,"");
    nowsb.set_bitmap_false(nowdir[ptr].inumber);
    nowdir[ptr].inumber=0;
    nowin.size--;                   //the number of size decreased by one
    nowsb.blocks_count-=2;          //the number of blocks decreased by two
    nowsb.inodes_count--;           //the number of inodes decreased by one
    nowsb.free_blocks_count+=2;     //meanwhile the free_blocks and free_inodes increased
    nowsb.free_inodes_count++;

    fseek(f,0,SEEK_SET);
    fwrite(&nowsb,sizeof(superblock),1,f);

    fseek(f,nowsb.get_size(nowin.block),SEEK_SET);
    fwrite(&nowin,sizeof(inode),1,f);

    fseek(f,nowsb.get_size(nowin.data),SEEK_SET);
    fwrite(nowdir,sizeof(dir_block),1,f);
    fflush(f);
    return 1;
}
int change_dir(char dirname[])  //for the command "cd"
{
    int ptr;
    inode in;
    ptr=name_exist(dirname);
    if(ptr==-1)
    {
        SetColor(RED);
        printf("directory %s does not exit\n",dirname);
        SetColor(GRAY);
        return -1;
    }
    fseek(f,nowsb.get_size(nowdir[ptr].inumber),SEEK_SET);
    fread(&in,sizeof(inode),1,f);
    if(in.isdir==false)
    {
        SetColor(RED);
        printf("%s is not a dirctory\n",dirname);
        SetColor(GRAY);
        return -1;
    }
    nowin=in;
    fseek(f,nowsb.get_size(nowin.data),SEEK_SET);
    fread(nowdir,sizeof(dir_block),1,f);
    return 1;
}
int create_file(char filename[])    //for the command "touch"
{
    int space;
    int inode_ptr,newfile_ptr;
    inode in;
    data_block newdata;
    if(strcmp(filename,"..")==0||strcmp(filename,".")==0)       //file .. and . should not be created
    {
        SetColor(RED);
        printf("cannot create a file called %s\n",filename);
        SetColor(GRAY);
        return -1;
    }
    else if(name_exist(filename)!=-1)
    {
        SetColor(RED);
        printf("file with name \"%s\" has existed\n",filename);
        SetColor(GRAY);
        return -1;
    }
    space=nowsb.get_empty_inode();      //get a empty inode
    if(space==-1)
        return -1;
    inode_ptr=nowsb.get_empty_block();      //get the block of the inode
    if(inode_ptr==-1)
        return -1;
    set_inode_table(space,filename,inode_ptr);
    nowsb.set_bitmap_true(inode_ptr);       //set the bitmap of inode true
    nowin.size++;
    in.isroot=false;
    in.isdir=false;                         //not a directory,but a file
    in.block=inode_ptr;
    newfile_ptr=nowsb.get_empty_block();        //get a block for data
    if(newfile_ptr==-1)
        return -1;
    in.data=newfile_ptr;
    nowsb.set_bitmap_true(newfile_ptr);         //set the data block true
    memset(newdata,0,sizeof(newdata));
    printf("input the content\n");              //get the content of the file
    fflush(stdin);
    gets(newdata);

    fseek(f,0,SEEK_SET);
    fwrite(&nowsb,sizeof(superblock),1,f);

    fseek(f,nowsb.get_size(nowin.block),SEEK_SET);
    fwrite(&nowin,sizeof(inode),1,f);

    fseek(f,nowsb.get_size(nowin.data),SEEK_SET);
    fwrite(nowdir,sizeof(dir_block),1,f);

    fseek(f,nowsb.get_size(inode_ptr),SEEK_SET);
    fwrite(&in,sizeof(inode),1,f);

    fseek(f,nowsb.get_size(newfile_ptr),SEEK_SET);
    fwrite(newdata,sizeof(data_block),1,f);
    fflush(f);
    return 1;
}
int write_file(char filename[])//for the command "write" to change the content of the file
{
    data_block newdata;
    int ptr;
    inode in;
    ptr=name_exist(filename);
    if(ptr==-1)
    {
        SetColor(RED);
        printf("file %s does not exist!\n",filename);
        SetColor(GRAY);
        return -1;
    }//jugde the name whether exit
    fseek(f,nowsb.get_size(nowdir[ptr].inumber),SEEK_SET);
    fread(&in,sizeof(inode),1,f);
    if(in.isdir==true)          //judege whether it is a file
    {
        SetColor(RED);
        printf("%s is not a file!\n",filename);
        SetColor(GRAY);
        return -1;
    }//this function only for file
    printf("input the content\n");
    gets(newdata);              //get content of file
    fseek(f,nowsb.get_size(in.data),SEEK_SET);
    fwrite(newdata,sizeof(data_block),1,f);
    fflush(f);
    return 1;
}

int remove_file(char filename[])    //for command "rm"
{
    int ptr;
    inode in;
    ptr=name_exist(filename);
    if(ptr==-1)
    {
        SetColor(RED);
        printf("file %s does not exit!\n",filename);
        SetColor(GRAY);
        return -1;
    }
    fseek(f,nowsb.get_size(nowdir[ptr].inumber),SEEK_SET);
    fread(&in,sizeof(inode),1,f);
    if(in.isdir)
    {
        SetColor(RED);
        printf("%s is not a file!\n",filename);
        SetColor(GRAY);
        return -1;
    }
    nowsb.set_bitmap_false(in.data);                //set the bitmap of data block false
    strcpy(nowdir[ptr].filename,"");                //delete the file from current directory
    nowsb.set_bitmap_false(nowdir[ptr].inumber);    //set the bitmap of inode false
    nowdir[ptr].inumber=0;
    nowin.size--;
    nowsb.blocks_count-=2;
    nowsb.free_blocks_count+=2;
    nowsb.inodes_count--;
    nowsb.free_inodes_count++;

    fseek(f,0,SEEK_SET);
    fwrite(&nowsb,sizeof(superblock),1,f);

    fseek(f,nowsb.get_size(nowin.block),SEEK_SET);
    fwrite(&nowin,sizeof(inode),1,f);

    fseek(f,nowsb.get_size(nowin.data),SEEK_SET);
    fwrite(nowdir,sizeof(dir_block),1,f);
    fflush(f);
}
int list(void)      //for the command "ls"
{
    inode in;
    int bias=nowin.isroot?1:2;      //check whether a root
    SetColor(BLUE);
    if(bias==1)
        printf(".\t");
    else
        printf(".\t..\t");
    SetColor(GRAY);
    for(int i=bias;i<TABLE_SIZE;i++)
    {
        if(strlen(nowdir[i].filename)!=0)
        {
            fseek(f,nowsb.get_size(nowdir[i].inumber),SEEK_SET);
            fread(&in,sizeof(inode),1,f);
            if(in.isdir)
            {
                SetColor(BLUE);
                printf("%s\t",nowdir[i].filename);
                SetColor(GRAY);
            }
            else
                printf("%s\t",nowdir[i].filename);

        }
    }
    printf("\n");
    return 1;
}
int concatenate(char filename[])    //for the command "cat"
{
    int ptr;
    inode in;
    data_block data;
    ptr=name_exist(filename);
    if(ptr==-1)
    {
        SetColor(RED);
        printf("there is no file called %s\n",filename);
        SetColor(GRAY);
        return -1;
    }
    fseek(f,nowsb.get_size(nowdir[ptr].inumber),SEEK_SET);
    fread(&in,sizeof(inode),1,f);
    if(in.isdir==true)
    {
        SetColor(RED);
        printf("%s is not a file\n",filename);
        SetColor(GRAY);
        return -1;
    }
    fseek(f,nowsb.get_size(in.data),SEEK_SET);
    fread(&data,sizeof(data_block),1,f);
    printf("%s\n",data);
    return 1;
}
void split(char cmd[],char x[],char y[])    //to split a string by a ' '
{
    int flag=0;
    int j=0;
    for(int i=0;cmd[i]!='\0';i++)
    {
        if(flag==0&&cmd[i]==' ')
        {
            flag=1;
            x[i]='\0';
            continue;
        }
        if(flag==0)
            x[i]=cmd[i];
        else
            y[j++]=cmd[i];
    }
    y[j]='\0';
}
void previous_dir(char dir[])           //the make the directory address right when "cd .."
{
    int i;
    int j;
    for(i=0;dir[i]!='\0';i++);
    for(j=i-2;j>=0;j--)
    {
        if(dir[j]=='\\')
        {
            dir[j+1]='\0';
            //printf("j=%d\n",j);
            break;
        }
    }

}
void console()          //the console
{
    char cmd[300];
    bool flag=false;
    char current_dir[50]="\\";
    while(true)
    {
        fflush(stdin);
        //printf("%s\n",cmd);
        if(flag==false)
        {
            printf("input \"create diskname\" to create a new disk\n");     //create new disk and format
            printf("input \"load diskname\" to load a disk\n");             //load the disk
        }
        if(flag==false)
            printf(">>");
        else
        {
            printf("%s",current_dir);               //print the address of current directory
            printf(">");
        }
        fflush(stdin);
        gets(cmd);
        char x[100],y[100];
        split(cmd,x,y);
        if(flag==false&&strcmp(x,"create")==0)
        {
            if(format(y)==-1)
                continue;
        }
        else if(flag==false&&strcmp(x,"load")==0)
        {
            if(load(y)==-1)
                continue;
            flag=true;
        }
        else if(flag==true)
        {
            if(strcmp(cmd,"q")==0||strcmp(cmd,"quit")==0)
            {
                printf("byebye :-)\n");
                return;
            }
            if(strcmp(cmd,"ls")==0)
                list();
            else
            {
                split(cmd,x,y);
                if(strcmp(x,"touch")==0)
                    create_file(y);
                else if(strcmp(x,"rm")==0)
                    remove_file(y);
                else if(strcmp(x,"cat")==0)
                    concatenate(y);
                else if(strcmp(x,"mkdir")==0)
                    create_dir(y);
                else if(strcmp(x,"rmdir")==0)
                    remove_dir(y);
                else if(strcmp(x,"write")==0)
                    write_file(y);
                else if(strcmp(x,"cd")==0)
                {
                    if(strcmp(y,".")==0||change_dir(y)==-1)
                        continue;
                    if(strcmp(y,"..")!=0)
                    {
                        strcat(current_dir,y);
                        strcat(current_dir,"\\");
                    }
                    else
                    {
                        previous_dir(current_dir);
                    }

                }
                else
                {
                    SetColor(RED);
                    printf("illegal command!\n");
                    SetColor(GRAY);
                }
            }
        }
    }
}

