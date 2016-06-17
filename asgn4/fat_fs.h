#ifndef FAT_FS_H
#define FAT_FS_H


#include <sys/stat.h>
//const int BLOCK_SIZE = 1024;
//const int word_s = 4;

// size in number of bytes
#define BLOCK_SIZE 1024
#define word_size 4
#define MAX_OPEN 30


struct stat s;


// struct stat defined in sys/stat.h

enum error_code {
   success,
   failure,
   found,
   directory,
   none
};

typedef enum error_code Error_code;



struct dir_entry {
   char file_name[6*word_size];
   int64_t creation;
   int64_t modification;
   int64_t access;
   int length;
   int first_block;
   int flags;
   int dir_entry_size;
};

void init_dir_entry(struct dir_entry * entry);

struct super_block {
   int magic_number;
   int N;
   int K;
   int block_size;
   int root_block;
};

void init_super_block(struct super_block * sb, int n, int k);


struct open_file{
   struct dir_entry entry;
   int current_block;
   int fh;
   int open;
};

void init_open_file(struct open_file *f, int file_descriptor);

struct fat_table {
   char fs_file_name[100];
   int * table;
   int fs_file;
   // block number of the current working directory file's first block 
   int working_dir; 
   struct super_block super;
   struct dir_entry *targ;
   struct open_file open_files[MAX_OPEN];
};


//void create_new_dir(struct fat_table *tbl, char* dir_name, int first_block);

void make_dirs(struct fat_table *tbl);


void error(Error_code c, char * message);

int get_k(int n);
int open_fat_fs(char* name);
int new_fat_fs(char* name, int num_blocks);

void init_fat_table(int fs_file, int num_blocks, struct fat_table * ft, 
                     char* name);
void free_fat_table(struct fat_table * tbl);

void write_table(struct fat_table * tbl);
void read_table(struct fat_table * tbl);

void load_table(struct fat_table * tbl);

void new_fat_file_system(struct fat_table * tbl, char* name, int size);
void load_file_system(struct fat_table * tbl, char* name);


void new_directory(struct fat_table *tbl, char* new_dir_name);

int fs_close(struct fat_table *tbl, int fd);

int fs_open(struct fat_table *tbl, char* path, int flags);

int fs_write(struct fat_table *tbl, int file, char* buff, int num_bytes);

int fs_read(struct fat_table *tbl, int file, char* buff, int num_bytes);

int fs_getattr(struct fat_table *tbl, char* path, struct stat *st);


#endif

