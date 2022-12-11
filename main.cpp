
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace std;

#define Block_size 5120
#define no_of_blocks 102400
#define no_of_inodes 61440
#define no_of_file_descriptors 16

struct file_map
{
    int inode_num;
    char filename[20];
};
struct inode
{
    /* data */
    int file_size;
    int file_desc;
    int permissions;
    int start_block;
};
struct data_block
{
    int next_block;
    int size;
};
struct superblock
{
    int no_of_blocks_of_superblock=ceil(((float)sizeof(superblock))/Block_size);
    int no_of_blocks_of_filemap=ceil(((float)sizeof(file_map)*no_of_inodes)/Block_size);
    int no_of_blocks_of_inode_data=ceil(((float)sizeof(inode)*no_of_inodes)/Block_size);
    int no_of_blocks_of_block_data=ceil(((float)sizeof(data_block)*no_of_inodes)/Block_size);

    int start_pos_of_filemap=no_of_blocks_of_superblock*Block_size;
    int start_pos_of_inode_data=start_pos_of_filemap+no_of_blocks_of_filemap*Block_size;
    int start_pos_of_block_data=start_pos_of_inode_data+no_of_blocks_of_inode_data*Block_size;

    int start_block_of_filemap=no_of_blocks_of_superblock;
    int start_block_of_inode_data=start_block_of_filemap+no_of_blocks_of_filemap;
    int start_block_of_block_data=start_block_of_inode_data+no_of_blocks_of_inode_data;

    bool free_inodes[no_of_inodes];
    bool free_data_blocks[no_of_blocks];
};



struct superblock sb;
struct inode inodes[no_of_inodes];
struct data_block dblocks[no_of_blocks];
struct file_map fmaps[no_of_inodes];

FILE *fp;
map<string,int> file_inode_map;
vector<int> free_file_descriptors(no_of_file_descriptors,-1);
int mounted=0;

size_t min(size_t a,size_t b)
{
    if(a<b)
        return a;
    return b;
}
int getFreeInode()
{
    for(int i=0;i<no_of_inodes;i++)
    {
        if(sb.free_inodes[i])
            return i;
    }
    return -1;
}
int getFreeDataBlock()
{
    for(int i=0;i<no_of_blocks;i++)
    {
        if(sb.free_data_blocks[i])
            return i;
    }
    return -1;
}
int getFreeFileDescriptor()
{
    for(int i=0;i<free_file_descriptors.size();i++)
    {
        if(free_file_descriptors[i]==-1)
            return i;
    }
    return -1;
}
string getMode(int mode)
{
    if(mode==0)
        return " Read ";
    if(mode==1)
        return " Write ";
    return " Append ";
}
void create_disk(string disk_name)
{
    //Find if disk already exists
    if(!access(disk_name.c_str(),F_OK))
    {
        cout<<"Disk already exists"<<endl;
        return;
    }


    fp=fopen(disk_name.c_str(),"wb");
    char data[Block_size]={0};

    //Entire disk to be filled with 0's
    for(int i=0;i<no_of_blocks;i++)
        fwrite(data,1,Block_size,fp);

    for(int i=0;i<no_of_inodes;i++)
    {
        sb.free_inodes[i]=true;
    }
    for(int i=0;i<no_of_blocks;i++)
    {
        sb.free_data_blocks[i]=true;
    }
    for(int i=0;i<sb.start_block_of_block_data;i++)
    {
        sb.free_data_blocks[i]=false;
    }

    // Writing superblock into disk
    int sb_size=sizeof(struct superblock);
    char sb_data[sb_size]={0};
    fseek(fp,0,SEEK_SET);
    memset(sb_data,0,sb_size);
    memcpy(sb_data,&sb,sb_size);
    fwrite(sb_data,sb_size,1,fp);


    // Writing filemaps into disk
    int filemap_size=sizeof(struct file_map);
    // fmaps=(struct file_map*)malloc(filemap_size*no_of_inodes);
    for(int i=0;i<no_of_inodes;i++)
    {
        fmaps[i].inode_num=-1;
    }
    fseek(fp,sb.start_pos_of_filemap,SEEK_SET);
    fwrite(fmaps,filemap_size,no_of_inodes,fp);
    

    // Writing inodes data into disk
    int inode_size=sizeof(struct inode);
    // inodes=(struct inode*)malloc(inode_size*no_of_inodes);
    for(int i=0;i<no_of_inodes;i++)
    {
        inodes[i].file_desc=-1;
        inodes[i].file_size=-1;
        inodes[i].permissions=-1;
        inodes[i].start_block=-1;
    }
    fseek(fp,sb.start_pos_of_inode_data,SEEK_SET);
    fwrite(inodes,inode_size,no_of_inodes,fp);


    // Writing data blocks metadata into disk
    int db_size=sizeof(struct data_block);
    // dblocks=(struct data_block*)malloc(db_size*no_of_blocks);
    for(int i=0;i<no_of_blocks;i++)
    {
        dblocks[i].next_block=-1;
        dblocks[i].size=0;
    }
    fseek(fp,sb.start_pos_of_block_data,SEEK_SET);
    fwrite(dblocks,db_size,no_of_blocks,fp);

    fclose(fp);
    cout<<"Disk created"<<endl;
}
int mount_disk(string disk_name)
{
    if(mounted==1)
    {
        cout<<"Some Disk has already been mounted"<<endl;
        return 0;
    }
    // check if the disk exists
    if(access(disk_name.c_str(),F_OK))
    {
        cout<<"No such Disk exists"<<endl;
        return 0;
    }


    fp=fopen(disk_name.c_str(),"rb+");
    // Read superblock from disk
    fseek(fp,0,SEEK_SET);
    fread(&sb,sizeof(struct superblock),1,fp);


    // Read filemaps from disk
    int filemap_size=sizeof(struct file_map);
    fseek(fp,sb.start_pos_of_filemap,SEEK_SET);

    fread(fmaps,filemap_size,no_of_inodes,fp);


    

    for(int i=0;i<no_of_inodes;i++)
    {
        if(fmaps[i].inode_num!=-1)
        {
            file_inode_map[string(fmaps[i].filename)]=fmaps[i].inode_num;
        }
    }

    // Read inodes data from disk
    int inode_size=sizeof(struct inode);
    fseek(fp,sb.start_pos_of_inode_data,SEEK_SET);
    fread(inodes,inode_size,no_of_inodes,fp);


    // Read data blocks metadata from disk
    int db_size=sizeof(struct data_block);
    fseek(fp,sb.start_pos_of_block_data,SEEK_SET);
    fread(dblocks,db_size,no_of_blocks,fp);

    // fclose(fp);
    mounted=1;
    cout<<"Disk Mounted"<<endl;
    return 1;
}
void create_file(string filename)
{
    if(mounted==0)
    {
        cout<<"Disk not mounted"<<endl;
        return;
    }
    if(file_inode_map.find(filename)!=file_inode_map.end())
    {
        cout<<"File already exists"<<endl;
        return;
    }


    int free_inode=getFreeInode();
    if(free_inode==-1)
    {
        cout<<"No Free Inodes available"<<endl;
        return;
    }


    int free_data_block=getFreeDataBlock();
    if(free_data_block==-1)
    {
        cout<<"No Free Data blocks available"<<endl;
        return;
    }


    sb.free_data_blocks[free_data_block]=false;
    sb.free_inodes[free_inode]=false;
    file_inode_map[filename]=free_inode;


    fmaps[free_inode].inode_num=free_inode;
    strcpy(fmaps[free_inode].filename,filename.c_str());

    inodes[free_inode].file_desc=-1;
    inodes[free_inode].file_size=0;
    inodes[free_inode].permissions=0;
    inodes[free_inode].start_block=free_data_block;
    
    cout<<"File created"<<endl;
}
void open_file(string filename,int mode)
{
    if(mounted==0)
    {
        cout<<"Disk not mounted"<<endl;
        return;
    }
    if(file_inode_map.find(filename)==file_inode_map.end())
    {
        cout<<"File does not exists"<<endl;
        return;
    }
    int inode=file_inode_map[filename];
    if(inodes[inode].file_desc!=-1)
    {
        cout<<"File already opened in "<<getMode(mode)<<" with file desc: "<<inodes[inode].file_desc<<endl;
        return;
    }
    inodes[inode].permissions=mode;
    inodes[inode].file_desc=getFreeFileDescriptor();
    if(inodes[inode].file_desc==-1)
    {
        cout<<"No Free File Descriptors available"<<endl;
        return;
    }
    free_file_descriptors[inodes[inode].file_desc]=inode;
    cout<<"File Descriptor: "<<inodes[inode].file_desc<<endl;
}
void read_file(int filedesc)
{
    if(mounted==0)
    {
        cout<<"Disk not mounted"<<endl;
        return;
    }
    if(filedesc<0||filedesc>16)
    {
        cout<<"Invalid File descriptor"<<endl;
        return;
    }
    int inode=free_file_descriptors[filedesc];
    if(inode==-1)
    {
        cout<<"Invalid File descriptor"<<endl;
        return;
    }
    if(inodes[inode].permissions!=0)
    {
        cout<<"You do not have read permission"<<endl;
        return;
    }
    int block=inodes[inode].start_block;
    while(block!=-1)
    {
        //Read the blocks data
        // cout<<"Reading at block "<<block<<endl;
        char buffer[dblocks[block].size+1]={'\0'};
        fseek(fp,sb.start_pos_of_block_data+block*Block_size,SEEK_SET);
        fread(buffer,1,dblocks[block].size,fp);
        cout<<buffer<<endl;
        block=dblocks[block].next_block;
    }
    cout<<"!!!!!File read!!!!"<<endl;
}
void write_file(int filedesc)
{
    if(mounted==0)
    {
        cout<<"Disk not mounted"<<endl;
        return;
    }
    if(filedesc<0||filedesc>16)
    {
        cout<<"Invalid File descriptor"<<endl;
        return;
    }
    int inode=free_file_descriptors[filedesc];
    if(inode==-1)
    {
        cout<<"Invalid File descriptor"<<endl;
        return;
    }
    if(inodes[inode].permissions!=1)
    {
        cout<<"You do not have Write permission"<<endl;
        return;
    }
    int block=inodes[inode].start_block;
    int temp;
    while(block!=-1)
    {
        temp=block;
        dblocks[block].size=0;
        sb.free_data_blocks[block]=true;
        block=dblocks[block].next_block;
        dblocks[temp].next_block=-1;
    }
    string input;
    cout<<"Enter input terminated by $"<<endl;
    cin>>ws;
    getline(cin,input,'$');  //Take input till user presses ctrl+D

    // cout<<"##################"<<endl;
    // cout<<input<<endl;
    // input.push_back('\0');
    size_t totalwritesize=input.size();

    cout<<"Written size:"<<totalwritesize<<endl;

    block=getFreeDataBlock();
    inodes[inode].start_block=block;
    int prev;
    while(Block_size<totalwritesize)
    {
        sb.free_data_blocks[block]=false;
        fseek(fp,sb.start_pos_of_block_data+block*Block_size,SEEK_SET);
        fwrite(input.c_str(),1,Block_size,fp);
        cout<<"file at block "<<block<<endl;
        totalwritesize-=Block_size;
        int nextblock=getFreeDataBlock();
        prev=block;
        dblocks[block].next_block=nextblock;
        dblocks[block].size=Block_size;
        block=nextblock;
        input=input.substr(Block_size);
    }
    if(totalwritesize>0)
    {
        sb.free_data_blocks[block]=false;
        fseek(fp,sb.start_pos_of_block_data+block*Block_size,SEEK_SET);
        cout<<"file at block "<<block<<endl;
        fwrite(input.c_str(),1,totalwritesize,fp);
    }
    dblocks[block].next_block=-1;
    dblocks[block].size+=totalwritesize;

    cout<<"File written"<<endl;

}
void append_file(int filedesc)
{
    if(mounted==0)
    {
        cout<<"Disk not mounted"<<endl;
        return;
    }
    if(filedesc<0||filedesc>16)
    {
        cout<<"Invalid File descriptor"<<endl;
        return;
    }
    int inode=free_file_descriptors[filedesc];
    if(inode==-1)
    {
        cout<<"Invalid File descriptor"<<endl;
        return;
    }
    if(inodes[inode].permissions!=2)
    {
        cout<<"You do not have Append permission"<<endl;
        return;
    }
    int block=inodes[inode].start_block;
    while(dblocks[block].next_block!=-1)
    {
        block=dblocks[block].next_block;
    }
    if(dblocks[block].size==Block_size)
    {
        dblocks[block].next_block=getFreeDataBlock();
        block=dblocks[block].next_block;
        sb.free_data_blocks[block]=false;
    }
    string input;
    cout<<"Enter input terminated by $"<<endl;
    cin>>ws;
    getline(cin,input,'$');  //Take input till user presses ctrl+D
    // input.push_back('\0');
    size_t totalwritesize=input.size();

    size_t writesize=min(Block_size-dblocks[block].size,totalwritesize);
    fseek(fp,sb.start_pos_of_block_data+block*Block_size+dblocks[block].size,SEEK_SET);
    fwrite(input.c_str(),1,writesize,fp);
    dblocks[block].size+=writesize;
    totalwritesize-=writesize;
    input=input.substr(writesize);
    if(totalwritesize>0)
    {
        dblocks[block].next_block=getFreeDataBlock();
        block=dblocks[block].next_block;
    }
    


    while(Block_size<totalwritesize)
    {
        sb.free_data_blocks[block]=false;
        fseek(fp,sb.start_pos_of_block_data+block*Block_size,SEEK_SET);
        fwrite(input.c_str(),1,Block_size,fp);
        // cout<<"file at block "<<block<<endl;
        totalwritesize-=Block_size;
        int nextblock=getFreeDataBlock();
        dblocks[block].next_block=nextblock;
        dblocks[block].size=Block_size;
        block=nextblock;
        input=input.substr(Block_size);
    }
    if(totalwritesize>0)
    {
        sb.free_data_blocks[block]=false;
        fseek(fp,sb.start_pos_of_block_data+block*Block_size,SEEK_SET);
        // cout<<"file at block "<<block<<endl;
        fwrite(input.c_str(),1,totalwritesize,fp);
    }
    dblocks[block].next_block=-1;
    dblocks[block].size+=totalwritesize;

    cout<<"Data Appended"<<endl;




}
void close_file(int filedesc)
{
    if(mounted==0)
    {
        cout<<"Disk not mounted"<<endl;
        return;
    }
    if(filedesc<0||filedesc>16)
    {
        cout<<"Invalid File descriptor"<<endl;
        return;
    }
    int inode=free_file_descriptors[filedesc];
    if(inode==-1)
    {
        cout<<"Invalid File descriptor or File not opened"<<endl;
 
 
 
        return;
    }
    free_file_descriptors[inodes[inode].file_desc]=-1;
    inodes[inode].file_desc=-1;
    cout<<"!!!File Closed!!!"<<endl;
}
void delete_file(string filename)
{
    if(mounted==0)
    {
        cout<<"Disk not mounted"<<endl;
        return;
    }
    if(file_inode_map.find(filename)==file_inode_map.end())
    {
        cout<<"No such File exists"<<endl;
        return;
    }
    int inode=file_inode_map[filename];
    if(inodes[inode].file_desc!=-1)
    {
        cout<<"File opened. Close the file and delete"<<endl;
        return;
    }
    int block=inodes[inode].start_block;
    int temp;
    while(block!=-1)
    {
        temp=block;
        dblocks[block].size=0;
        sb.free_data_blocks[block]=true;
        block=dblocks[block].next_block;
        dblocks[temp].next_block=-1;
    }



    sb.free_inodes[inode]=true;
    file_inode_map.erase(filename);


    fmaps[inode].inode_num=-1;
    // strcpy(fmaps[inode].filename," ");
    // memset(fmaps[inode].filename,0,20);
    // int filedesc=inodes[inode].file_desc;
    // free_file_descriptors[filedesc]=-1;

    inodes[inode].file_desc=-1;
    inodes[inode].file_size=0;
    inodes[inode].permissions=-1;
    inodes[inode].start_block=-1;

    cout<<"!!!File Deleted!!!"<<endl;
}
void list_of_files()
{
    if(mounted==0)
    {
        cout<<"Disk not mounted"<<endl;
        return;
    }
    cout<<"#### List of Files ####"<<endl;
    for(auto it=file_inode_map.begin();it!=file_inode_map.end();it++)
    {
        cout<<it->first<<endl;
    }
}
void list_of_opened_files()
{
    if(mounted==0)
    {
        cout<<"Disk not mounted"<<endl;
        return;
    }
    cout<<"#### List of Opened Files ####"<<endl;
    for(auto it=file_inode_map.begin();it!=file_inode_map.end();it++)
    {
        int inode=it->second;
        if(inodes[inode].file_desc!=-1)
        {
            cout<<it->first<<endl;
        }
    }
}
void unmount()
{
    if(mounted==0)
    {
        cout<<"No Disk mounted"<<endl;
        return;
    }
    // Write the structs data to disk
    // Writing superblock into disk
    int sb_size=sizeof(struct superblock);
    char sb_data[sb_size]={0};
    fseek(fp,0,SEEK_SET);
    memset(sb_data,0,sb_size);
    memcpy(sb_data,&sb,sb_size);
    fwrite(sb_data,sb_size,1,fp);


    // Writing filemaps into disk
    int filemap_size=sizeof(struct file_map);
    fseek(fp,sb.start_pos_of_filemap,SEEK_SET);
    fwrite(fmaps,filemap_size,no_of_inodes,fp);
    

    // Writing inodes data into disk
    int inode_size=sizeof(struct inode);
    fseek(fp,sb.start_pos_of_inode_data,SEEK_SET);
    fwrite(inodes,inode_size,no_of_inodes,fp);


    // Writing data blocks metadata into disk
    int db_size=sizeof(struct data_block);
    fseek(fp,sb.start_pos_of_block_data,SEEK_SET);
    fwrite(dblocks,db_size,no_of_blocks,fp);



    file_inode_map.clear();
    fill(free_file_descriptors.begin(),free_file_descriptors.end(),-1);
    mounted=0;
    fclose(fp);
    cout<<"Disk unmounted"<<endl;
}
void fileoperations()
{
    int choice;
    string filename;
    int filedesc;
    while(1)
    {
        cout<<"###########################"<<endl;
        cout<<"1: create file"<<endl;
        cout<<"2: open file"<<endl;
        cout<<"3: read file"<<endl;
        cout<<"4: write file"<<endl;
        cout<<"5: append file"<<endl;
        cout<<"6: close file"<<endl;
        cout<<"7: delete file"<<endl;
        cout<<"8: list of files"<<endl;
        cout<<"9: list of opened files"<<endl;
        cout<<"10: unmount"<<endl;
        cout<<"Enter choice:";
        cin>>choice;
        if(choice==10)
        {
            unmount();
            break;
        }
        switch(choice)
        {
            case 1:
            cout<<"Enter the file name to create:";
            cin>>filename;
            create_file(filename);
            break;
            case 2:
            int mode;
            cout<<"Enter the file name to open:";
            cin>>filename;
            cout<<"0: read mode"<<endl;
            cout<<"1: write mode"<<endl;
            cout<<"2: append mode"<<endl;
            cout<<"Enter the mode:";
            cin>>mode;
            open_file(filename,mode);
            break;
            case 3:
            cout<<"Enter the file descriptor:";
            cin>>filedesc;
            read_file(filedesc);
            break;
            case 4:
            cout<<"Enter the file descriptor:";
            cin>>filedesc;
            write_file(filedesc);
            break;
            case 5:
            cout<<"Enter the file descriptor:";
            cin>>filedesc;
            append_file(filedesc);
            break;
            case 6:
            cout<<"Enter the file descriptor:";
            cin>>filedesc;
            close_file(filedesc);
            break;
            case 7:
            cout<<"Enter the file name to delete:";
            cin>>filename;
            delete_file(filename);
            break;
            case 8:
            list_of_files();
            break;
            case 9:
            list_of_opened_files();
            break;
            default:cout<<"Enter valid choice"<<endl;
        }
    }
}
int main()
{
    int choice;
    string diskname;
    while(1)
    {
        cout<<"1: create disk"<<endl;
        cout<<"2: mount disk"<<endl;
        cout<<"3: exit"<<endl;
        cout<<"Enter choice:";
        cin>>choice;
        if(choice==3)
            break;
        switch(choice)
        {
            case 1:
            cout<<"Enter the disk name to create:";
            cin>>diskname;
            create_disk(diskname);
            break;
            case 2:
            cout<<"Enter the disk name to mount:";
            cin>>diskname;
            if(mount_disk(diskname))
            {
                fileoperations();
            }
            break;
        }
    }
    return 0;
}