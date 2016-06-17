
#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdint.h> // int32_t
#include <stdlib.h>
#include <string.h>

#include <fcntl.h> // posix_fallocate()
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include <fuse.h>
#include "fat_fs.h"


/*
    This macro fetches our fat_table for us.

    We will need to get our fat_table object whenever one of our
    functions are called.                                */


#define MY_FAT ((struct fat_table *) fuse_get_context()->private_data)

int myfat_open (const char * path, struct fuse_file_info * fuse_info){
   int flags = fuse_info->flags;
   fuse_info->fh = fs_open(MY_FAT, (char*)path, flags);
   return fuse_info->fh;
}

int myfat_read (const char *path, char *buf, size_t size, off_t offset, 
               struct fuse_file_info *fuse_info){
   
   return fs_read(MY_FAT, fuse_info->fh, buf, size);
}

int myfat_write (const char *path, const char *buf, size_t size, off_t offset, 
                  struct fuse_file_info *fuse_info){
    
   return fs_write(MY_FAT, fuse_info->fh, (char*)buf, size);
}

int myfat_mkdir (const char *path, mode_t mode){
   //this is a void function for us
   new_directory(MY_FAT, (char*)path);
   return 1;
}

int myfat_getattr (const char *path, struct stat *stats){
    return fs_getattr(MY_FAT, (char*)path, stats);
}

int myfat_raddir(const char *path, mode_t mode){

   return 1;
}
/*
int myfat_opendir (const char * path, struct fuse_file_info * fuse_info){

   fuse_info->fh = fs_open(MY_FAT, (char*)path, flags);
   return fuse_info->fh;
}
*/


/*
we set up the fuse opperations here.
we need to assign our functoins to the fuse object before
before we call fuse_main().
*/

static struct fuse_operations myfat_oper = {
    //all the functions name we need to implement called here
    .open = myfat_open,
    .read = myfat_read,
    .write = myfat_write,
    .getattr = myfat_getattr, 
    //.readlink = myfat_readlick, 
    .mkdir = myfat_mkdir, 
};



int main(int argc, char* argv[])
{

   // get and parse command line args
   int N = 20; 
   char* fname = "fs.fat"; 
   // validate arguments

   // declare our fat file system
   struct fat_table * my_fat;
   
   // allocate resources for our fat file system
   my_fat = (struct fat_table*) malloc( sizeof(struct fat_table) );

   // initialize our fat file system
   new_fat_file_system(my_fat, fname, N);
   
   //load_file_system(my_fat, fname);
   //make_dirs(my_fat);
   
   //free_fat_table(my_fat);

   // pass our fat file system into fuse main
   return fuse_main(argc, argv, &myfat_oper, my_fat);
   
   //return 0;
   
}

