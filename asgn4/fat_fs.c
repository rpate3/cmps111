#define FUSE_USE_VERSION 26

// #include <fuse.h> 
#include <stdio.h>
#include <stdlib.h> // exit()
#include <stdint.h> // int32_t
#include <time.h> // time_t

#include <fcntl.h> // posix_fallocate()
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include <dirent.h>
#include "fat_fs.h"


const int magic_number = 591773423;

void create_root_dir(struct fat_table *tbl, int first_block);

/*
 *                file allocation table operations
 ******************************************************************/
 
typedef Error_code (*table_action)(struct fat_table * tbl, int val);

Error_code travers_table_cache(struct fat_table *tbl, int* block_num, table_action visit);
Error_code travers_table_disk(struct fat_table * tbl, int* block_num, table_action visit);
// writes the value at index val of the allocation table to disk
Error_code write_table_bytes(struct fat_table * tbl, int val);
// reads the value at index val of the allocation table to disk
Error_code read_table_bytes(struct fat_table * tbl, int val);

void print_table(struct fat_table *tbl);
Error_code print_table_val(struct fat_table *tbl, int val);
Error_code block_is_free(struct fat_table *tbl, int val);
Error_code get_free_block(struct fat_table *tbl, int* free_block);


/*
 *                directory operations
 ***************************************************************************/
typedef Error_code (*dir_action)(int file, struct dir_entry * entry);

Error_code 
travers_dir_file(struct fat_table *tbl, int first_block, 
                 struct dir_entry * target_entry, dir_action visit);
Error_code 
travers_dir_block(struct fat_table *tbl, int block_num,
                  struct dir_entry * target_entry, dir_action visit);

Error_code write_dir_entry(int file, struct dir_entry * entry);
Error_code read_dir_entry(int file, struct dir_entry * entry);
Error_code dir_entry_name_equals(int file, struct dir_entry * target_entry);

void init_dir_entry_2(struct dir_entry *e, char* name, int first_block, Error_code file_t);
void create_new_dir(struct fat_table *tbl, char* dir_name, int first_block);
void print_entry(struct dir_entry *entry);
void print_working_dir(struct fat_table *tbl);


/*
 * when this function returns success we can emediatly call the function
 * read_entry() to read the file entry associated with the queried file.
 */
Error_code find_dir_file(struct fat_table *tbl, char* target_name);
Error_code print_dir_entry(int file, struct dir_entry * entry);   
Error_code dir_file_find_free_space(struct fat_table *tbl, int block_num);
Error_code dir_entry_is_free(int file, struct dir_entry * target_entry);

void change_directory(struct fat_table *tbl, char* target_dir);
void new_file(struct fat_table *tbl, char* new_file_name);
void create_new_file(struct fat_table *tbl, char* file_name, int first_block);


/*Below are cases if we wish to either remove a file or directory*/
void remove_file(struct fat_table *tbl, char* rem_file_name);

void remove_directory(struct fat_table *tbl, char* rem_name_dir_name, 
      struct dir_entry * target_entry);

/*Below are cases if we need to either move or copy a file
   These passing arguments are similar to move/copy a directory*/
void move_file(struct fat_table *tbl, char* initial_target_file_location, char* new_target_file_location);
void copy_file();

/*
 *                super block operations
 ***************************************************************************/

typedef Error_code (*super_action)(struct fat_table * tbl, int attr, int* val);

void read_super(struct fat_table * tbl);
void write_super(struct fat_table * tbl);
void print_super(struct fat_table * tbl);
Error_code travers_super(struct fat_table *tbl, super_action visit);

Error_code read_super_attr(struct fat_table *tbl, int attr, int *val);
Error_code get_super_attr(struct super_block * s, int attr, int *val);
Error_code print_super_attr(struct fat_table *tbl, int attr, int *val);
Error_code write_super_attr(struct fat_table *tbl, int attr, int *val);


/*
 * ***************************************************************************
 */

int fs_getattr(struct fat_table *tbl, char* path, struct stat *st);
void of_go_to_start(struct fat_table *tbl, int file); 
void go_to_block(struct fat_table *tbl, int block_num);
void of_go_to_block(struct fat_table *tbl, int file, int block_num); 

/*
 * ***************************************************************************
 */
/*
//static struct fuse_operations hello_oper = {
//	.read = hello_read,
//}; 

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	int size1 = 1111; 

	printf("%d", size1); 

	return size1;
}

//cc -D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse  -pthread -L/usr/local/lib -lfuse -lrt fs_main.c fat_fs.c -o fat_fs
//use that to run and link fuse with our files

static struct fuse_operations hello_oper = {
        .read = hello_read,
};

*/

/*
struct dirent* fs_readdir(struct fat_table *tbl, int dh){

}*/


int get_k(int n){
   return (n/256) +1;
}


void error(Error_code c, char* message){
   if(c == failure) {
      printf("\nerror: %s\n", message);
   }
}

int open_fat_fs(char* name){
   int fat_file = open(name, O_RDWR); // O_RDONLY);//WR);
   if (fat_file < 0){
      error(failure, "file system file open failure");
   }
   return fat_file;
}



/*
 * Create a file of the specified name and size.
 */
int new_fat_fs(char* name, int num_blocks){
    // open our file system file
    int fat_file = open(name, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR|S_IROTH|S_IWOTH );
    if (fat_file < 0){
        error(failure, "file system file open failure");
    }

   // allocate the required ammount of space
    if (0 > posix_fallocate(fat_file, 0, num_blocks * BLOCK_SIZE))
        error(failure, "file space allocation failure");

    return fat_file;
}

/*
 * Initializes a new fat file system in the specified file and of the
 * specifitd size.
 */
void new_fat_file_system(struct fat_table * tbl, char* name, int size){
   int file = new_fat_fs(name, size);
   init_fat_table(file, size, tbl, name);
   
   create_root_dir(tbl, tbl->super.root_block);
   tbl->working_dir = tbl->super.root_block;
 
   print_super(tbl);
   print_table(tbl);
   print_working_dir(tbl);

   write_super(tbl);
   write_table(tbl);
   
}


void make_dirs(struct fat_table *tbl){

   char new_dir_name[24]; 
   
   strcpy(new_dir_name, "d1");
   go_to_block(tbl, tbl->working_dir); 
   new_directory(tbl, new_dir_name);


   strcpy(new_dir_name, "d2");
   go_to_block(tbl, tbl->working_dir);
   new_directory(tbl, new_dir_name);
   
   strcpy(new_dir_name, "d3");
   go_to_block(tbl, tbl->working_dir);
   new_directory(tbl, new_dir_name);
   
   int fd = fs_open(tbl, "some-file.txt", O_RDWR);

   char stuff[27];
   strcpy(stuff, "abcdefghijklmnopqrstuvwxyz");

   int w = fs_write(tbl, fd, stuff, 27);

   of_go_to_start(tbl, fd);

   char buff[100];
   int r = fs_read(tbl, fd, buff, 27);
   printf("buff: %s\n", buff);

   write_table(tbl);

   //print_table(tbl);
   //print_working_dir(tbl);

   change_directory(tbl, new_dir_name); 
   print_working_dir(tbl);

   change_directory(tbl, ".."); 
   print_working_dir(tbl);

   fs_close(tbl, fd);

   char * s = "some-file.txt";
   struct stat st;
   int y = fs_getattr(tbl, s, &st);
}


void load_file_system(struct fat_table * tbl, char* name){
   printf("\t--- LOAD FILE SYSTEM ---\n");
   tbl->fs_file = open_fat_fs(name);
   read_super(tbl);

   load_table(tbl);
   print_table(tbl);

   tbl->working_dir = tbl->super.root_block;
   print_super(tbl);

   
   char* new_dir_name = "d2";
   
   print_working_dir(tbl);
   
   //change_directory(tbl, new_dir_name); 
   print_working_dir(tbl);
}

/*=================================================
 *                                  Table Functions
 *=================================================
 */

/*
 * Initialize a new fat_table struct.
 */
void init_fat_table(int fs_file, int num_blocks, struct fat_table * tbl,
                     char* name){
   
   strcpy((char*) &tbl->fs_file_name, name);
   
   int k = get_k(num_blocks);
   init_super_block(&tbl->super, num_blocks, k);
   tbl->fs_file = fs_file;
   tbl->working_dir = tbl->super.root_block;

   printf("fat fs init()\n");

   tbl->table = (int *) malloc(1 + k*sizeof(int));
   if (tbl->table == 0){
      error(failure, "fat_table->table allocation failure");
   }

   int i;
   for(i = 0; i < k+1; i++)
      tbl->table[i] = -1;

   for(i = k+1; i < num_blocks; i++)
      tbl->table[i] = 0;

   for(i = 0; i < MAX_OPEN; i++)
      tbl->open_files[i].open = 0;
}  


void free_fat_table(struct fat_table * tbl){
   printf("free fat table()\n");
   free(tbl->table);
   free(tbl);
}


/*
 * loads a previously initialized fat file system file into a
 * fat_table struct.
 *
 * PRE: tbl->super is already initialized.
 */
void load_table(struct fat_table * tbl){
   int k = tbl->super.K;
   tbl->table = (int *) malloc(1 + k*sizeof(int));
   if (tbl->table == 0){
      error(failure, "fat_table->table allocation failure");
   }
   read_table(tbl);
}


Error_code travers_table_cache(struct fat_table *tbl, int* block_num, table_action visit){

   go_to_block(tbl, 1);
   int n = tbl->super.N;
   int i;
   Error_code result = failure;
   
   for (i=0; i < n; i++){
      result = visit(tbl, i);
      if(result == found) {
         *block_num = i;
         break;
      }
   }
   return result;
}

Error_code travers_table_disk(struct fat_table * tbl, int *block_num, table_action visit){
   go_to_block(tbl, 1);
   int n = tbl->super.N;
   int i;
   Error_code result = failure;
   
   for (i=0; i < n; i++){
      result = visit(tbl, i);
      if(result == found) {
         *block_num = i;
         break;
      }
   }
   return result;
}


Error_code write_table_bytes(struct fat_table * tbl, int val){
      int x = tbl->table[val];
      write( tbl->fs_file, &x, 4);
      return success;
}


Error_code read_table_bytes(struct fat_table * tbl, int val){
      read( tbl->fs_file, &tbl->table[val],  4);
      return success;
}


Error_code print_table_val(struct fat_table *tbl, int val){
   printf("table[%d] = %d\n", val, tbl->table[val]);
   return success;
}


Error_code block_is_free(struct fat_table *tbl, int val){
      if(0 == tbl->table[val])
         return found;
      else
         return success;
}

/*
 * Writes our fat table to the file system file.
 *
 * We can use this to keep the file allocation table consistant
 * between the version in cache and the version on disk.
 */
void write_table(struct fat_table * tbl){
   int x;
   
   off_t f_place = lseek(tbl->fs_file, 0, SEEK_CUR);
   travers_table_disk(tbl, &x, write_table_bytes); 
   f_place = lseek(tbl->fs_file, f_place, SEEK_SET);
}


void read_table(struct fat_table * tbl){ 
   int x;
   travers_table_disk(tbl, &x, read_table_bytes);
}

void print_table(struct fat_table *tbl){  
   printf("\t--- ALLOCATION TABLE --- %s\n", tbl->fs_file_name);
   int x;
   travers_table_disk(tbl, &x, print_table_val);
   printf("\n");
}

Error_code get_free_block(struct fat_table *tbl, int* free_block){
   if(found == travers_table_disk(tbl, free_block, block_is_free)) 
      return success;
   else
      return failure;
}


void go_to_block(struct fat_table *tbl, int block_num){
   off_t block_start = block_num * tbl->super.block_size;
   off_t f_place = lseek(tbl->fs_file, block_start, SEEK_SET);
}

/*=================================================
 *                                  Super Block Functions
 *=================================================
 */


void init_super_block(struct super_block * sb, int n, int k){
   sb->magic_number = magic_number;
   sb->N = n;
   sb->K = k;
   sb->block_size = BLOCK_SIZE;
   sb->root_block = k+1;
}


Error_code travers_super(struct fat_table *tbl, super_action visit){
   go_to_block(tbl, 0);
   int num_attrs = 5;
   int i, j;
   Error_code result;
   for (i=0; i < num_attrs; i++){
      result = visit(tbl, i, &j);
      if (result == found)
         break;
   }
   return result;
}


void read_super(struct fat_table * tbl){
   Error_code r = travers_super(tbl, read_super_attr);
   
}

void write_super(struct fat_table * tbl){
   travers_super(tbl, write_super_attr);
}

void print_super(struct fat_table * tbl){
   printf("\t--- SUPER BLOCK ---\n");
   travers_super(tbl, print_super_attr);
   printf("\n");
}


Error_code read_super_attr(struct fat_table *tbl, int attr, int *val){
   int x;
   int r = read( tbl->fs_file, val,  4);
   //printf("val = %d\n", x); 
   switch (attr){
      case 0: tbl->super.magic_number = *val;   break;
      case 1: tbl->super.N = *val;  break;
      case 2: tbl->super.K = *val;  break;
      case 3: tbl->super.block_size = *val;  break;
      case 4: tbl->super.root_block = *val;  break;
      default:
              return failure;
   }
   return success;
}


Error_code get_super_attr(struct super_block * s, int attr, int *val){
   switch (attr){
      case 0: *val = s->magic_number;   break;
      case 1: *val = s->N;  break;
      case 2: *val = s->K;  break;
      case 3: *val = s->block_size;   break;
      case 4: *val = s->root_block;   break;
      default:
              return failure;
   }
   return success;
}


Error_code write_super_attr(struct fat_table *tbl, int attr,  int *val){
   Error_code result = get_super_attr(&tbl->super, attr, val);
   result = write( tbl->fs_file, val,  4);
   return result;
}


Error_code  print_super_attr(struct fat_table *tbl, int attr, int *val){
   struct super_block *s;
   s = &tbl->super;
   switch (attr){
      case 0:
         printf("magic number: %d\n", s->magic_number);
         break;
      case 1:
         printf("N: %d\n", s->N);
         break;
      case 2:
         printf("K: %d\n", s->K);
         break;
      case 3:
         printf("block size: %d\n", s->block_size);
         break;
      case 4:
         printf("fs root block: %d\n", s->root_block);
         break;
      default:
         return failure;
   }
   return success;
}

/*=================================================
 *                                  Directory Entry Functions
 *=================================================
 */


void init_dir_entry(struct dir_entry * entry){
   // we set the size of a directory entry here so that we can easilly
   // add or remove future entry attributes.
   entry->dir_entry_size = 64; // size measured in bytes
}


/*
 * PRE: 
 *
 * POST: file will be at begging of the last visited file entry. 
 */
Error_code travers_dir_block(struct fat_table *tbl, int block_num,  
                            struct dir_entry *target_entry, dir_action visit){
   struct dir_entry visited_entry;
   init_dir_entry(&visited_entry);
   
   // go to beggining of block
   go_to_block(tbl, block_num);

   int max_entries = tbl->super.block_size / visited_entry.dir_entry_size; 
   int i;
   // loop through block and visit each file entry
   for(i = 0; i < max_entries; i++){ 
      Error_code result = visit(tbl->fs_file, target_entry);
      if(result == success){
         // set file position to beggining of most recently visitted entry
         lseek(tbl->fs_file, -visited_entry.dir_entry_size, SEEK_CUR); 
         return result;
      }
   }
   return failure;
}


Error_code read_dir_entry(int file, struct dir_entry * entry){
   int r[7];
   r[0] = read( file, entry->file_name, 6*word_size);
   r[1] = read( file, &entry->creation, 2*word_size);
   r[2] = read( file, &entry->modification, 2*word_size);
   r[3] = read( file, &entry->access, 2*word_size);
   r[4] = read( file, &entry->length, 1*word_size);
   r[5] = read( file, &entry->first_block, 1*word_size);
   r[6] = read( file, &entry->flags, 1*word_size);
   lseek(file, word_size, SEEK_CUR); 

   //printf("\n\t%s\n", entry->file_name);

   return success;
}


Error_code write_dir_entry(int file, struct dir_entry * entry){
   int w[7];
   w[0] = write( file, entry->file_name, 6*word_size);
   w[1] = write( file, &entry->creation, 2*word_size);
   w[2] = write( file, &entry->modification, 2*word_size);
   w[3] = write( file, &entry->access, 2*word_size);
   w[4] = write( file, &entry->length, 1*word_size);
   w[5] = write( file, &entry->first_block, 1*word_size);
   w[6] = write( file, &entry->flags, 1*word_size);
   lseek(file, word_size, SEEK_CUR); 
   return success;
}

//return the local time for the current located directory
void to_time(int64_t* int_seconds, char time_string[], size_t size){
   struct tm* time_s;
   const time_t * seconds = (const time_t*)int_seconds;
   time_s = localtime( seconds );
   strftime(time_string, size, "%b %d %H:%M", time_s);
}

void print_entry(struct dir_entry *entry){
   size_t n = 100;
   char s[n];
   to_time(&entry->creation, s, n);
   printf("%2dkb %15s %5s\n", entry->length, s,entry->file_name);
/*
   printf("\nentry name: %s\n", entry->file_name);
   printf( "creation time: %s", to_time(&entry->creation));
   printf("modification time: %s", to_time(&entry->modification));
   printf( "access time: %s", to_time(&entry->access));
   printf( "length: %dkb\n", entry->length);
   printf( "first block: %d\n", entry->first_block);
   printf( "dir flag: %d\n", entry->flags);
*/
}

Error_code print_dir_entry(int file, struct dir_entry * entry){
   Error_code result = read_dir_entry(file, entry); 
   if(entry->first_block > 1){
      print_entry(entry);
   }
   return failure;
}


void print_working_dir(struct fat_table *tbl){
   printf("\t --- WORKING DIRECTORY, block %d ---\n", tbl->working_dir);
   struct dir_entry e;
   init_dir_entry(&e);   
   Error_code result = travers_dir_file(tbl, tbl->working_dir, &e, print_dir_entry); 
   printf("\n");
}

Error_code travers_dir_file(struct fat_table *tbl, int first_block, 
                            struct dir_entry * target_entry, dir_action visit){

   Error_code block_result = failure;
   int current_block = first_block;
   while(1){
      block_result = travers_dir_block(tbl, current_block, target_entry, visit);
      if(block_result == success)
         break;

      // we are done once we traverse the last block in the file
      int next_block = tbl->table[current_block]; 
      if (next_block < 0)
         break;

      // the directory file can span multiple blocks
      go_to_block(tbl, next_block);
      current_block = next_block;
   }
   // return found/not found, target file block number
   return block_result;
}



Error_code dir_entry_name_equals(int file, struct dir_entry *target_entry){
   struct dir_entry e;
   init_dir_entry(&e);
   Error_code result = read_dir_entry(file, &e);
 
   int r = strcmp((const char*) &target_entry->file_name, e.file_name); 
   if (r == 0)
      return success;
   return failure;
}

/*
 * when this function returns success we can emediatly call the function
 * read_entry() to read the file entry associated with the queried file.
 */
Error_code find_dir_file(struct fat_table *tbl, char* target_name){
   // search working directory for target file name
   struct dir_entry target;
   init_dir_entry(&target); 
   strcpy((char*) &target.file_name, target_name);
   return  travers_dir_file(tbl, tbl->working_dir, &target, dir_entry_name_equals);
}


/* *e is initialized as a directory entry that can be either the super block for the
 main file system or the first block that is located wihtin a sub-direoctory under root*/

/* The directory entry *e contains a struct where we can point to the access, modification,
 and length for the meta deta*/

/* If we had an error within our directory, set a flag to initialize out current state */

void init_dir_entry_2(struct dir_entry *e, char* name, int first_block, Error_code file_t){
   init_dir_entry(e);
   e->first_block = first_block;
   strcpy(e->file_name, name);
   
   time_t current_time = time(NULL);


   e->creation = current_time;
   e->modification = current_time;
   e->access = current_time;
   e->length = 1;
   if(file_t == directory)
      e->flags = 1;
   else 
      e->flags = 0;
}


void create_root_dir(struct fat_table *tbl, int first_block){
      // add new file to allocation table
      tbl->table[first_block] = -2;
      go_to_block(tbl, first_block);

      struct dir_entry e;
      init_dir_entry_2(&e, ".", first_block, directory);
      Error_code result = write_dir_entry(tbl->fs_file, &e);
}

/*
 * PRE: file is in correct place to write a new file entry.
 */
void create_new_dir(struct fat_table *tbl, char* dir_name, int first_block){
      //printf("create_new_dir()\n");
      //init_dir_entry(dir_name, first_block)
      struct dir_entry e;
      init_dir_entry_2(&e, dir_name, first_block, directory);
      
      // add new file to allocation table
      tbl->table[first_block] = -2;

      Error_code result;
      result = write_dir_entry(tbl->fs_file, &e);

      // add . and .. file entries
      go_to_block(tbl, first_block);
      init_dir_entry_2(&e, "..", tbl->working_dir, directory);
      result = write_dir_entry(tbl->fs_file, &e);

      init_dir_entry_2(&e, ".", first_block, directory);
      result = write_dir_entry(tbl->fs_file, &e);
      
      go_to_block(tbl, tbl->working_dir);
}


/*
 * PRE: file is in correct place to write a new file entry.
 */
void create_new_file(struct fat_table *tbl, char* file_name, int first_block){
      struct dir_entry e;
      init_dir_entry_2(&e, file_name, first_block, none);
      
      // add new file to allocation table
      tbl->table[first_block] = -2;

      Error_code result;
      result = write_dir_entry(tbl->fs_file, &e);      
      off_t n = lseek(tbl->fs_file, -e.dir_entry_size , SEEK_CUR); 
}


void change_directory(struct fat_table *tbl, char* target_dir){
   printf("-----------------  change_directory ==> %s  -----------------\n",
         target_dir);
   // search working directory for target directory file
   Error_code result = find_dir_file(tbl, target_dir);
   
   if(result == failure)
      error(failure, "could not find directory");
   
   else{
      struct dir_entry e;
      init_dir_entry(&e);
      result = read_dir_entry(tbl->fs_file, &e);
      // make sure target is a directory
      if (e.flags == 0)
         error(failure, "target_dir is not a directory");
      else{
         printf("going to block: %d\n", e.first_block);
         tbl->working_dir = e.first_block;
         go_to_block(tbl, e.first_block);
      }
   } 
}


Error_code grow_file(struct fat_table *tbl, int file_block_number){

   // search allocation table for next available free bock
   int free_block;
   Error_code result = get_free_block(tbl, &free_block);

   if(found != result) 
      return failure;
   else{
      tbl->table[file_block_number] = free_block;
      tbl->table[free_block] = -2;
      go_to_block(tbl, free_block);

      write_table(tbl);
      return success;
   }
}

Error_code dir_entry_is_free(int file, struct dir_entry * target_entry){
   Error_code result = read_dir_entry(file, target_entry);
   if (target_entry->first_block == 0)
      return success;
   else 
      return failure;
}

Error_code dir_file_find_free_space(struct fat_table *tbl, int block_num){ 
   struct dir_entry e;
   init_dir_entry(&e);
   return travers_dir_file(tbl, block_num, &e, dir_entry_is_free);
}


/*
 * Creates a new directory in current working directory.
 */
void new_directory(struct fat_table *tbl, char* new_dir_name){
   printf("new_directory( %s )\n", new_dir_name);
   if (success == find_dir_file(tbl, new_dir_name)){
      error(failure, "cant make new_dir_name: a directory already exists with than name ");
      return;
   }
   
   int free_block;
   Error_code found_free = get_free_block(tbl, &free_block);
   error(found_free, "no available free data blocks");
   
   if(found_free == success)    
      // search working directory file for unused space to put the new file entry
      found_free = dir_file_find_free_space(tbl, tbl->working_dir);

   if (found_free == failure){ 
      // if we didnt find space and this is last block then we need to grow the file
      found_free = grow_file(tbl, tbl->working_dir);
      error(found_free, "could not grow file");
   }
 
   if (found_free == success){
      // create dir entry for new file 
      create_new_dir(tbl, new_dir_name, free_block);
   } 
   if (found_free != success)
      error(failure, "out of space: no more available data blocks");
   
   write_table(tbl);
   return;
}



/*
Search for the file using find_dir_file

If the directory is not found{
   display error
}
else{
   (1) read the file entry
   (2) inside the file entry we have access to the first block (data block)
   (3) In order to delete the file entry, we set the first block to zero
}
*/
void remove_directory(struct fat_table *tbl, char* rem_name_dir_name, struct dir_entry * target_entry){
   printf("remove_directory( %s )\n", rem_name_dir_name);


   /*Precondion to see if the directory isn't found*/
   if (success != find_dir_file(tbl, rem_name_dir_name)){
      error(failure, "Directory doesn't exists ");
      return;
   }
   int free_block;
   Error_code found_free = get_free_block(tbl, &free_block);

   /* Handle cases if either we have blocks for the entire file. */
   if(found_free == success){
      //file is already deleted or empty being in this state. 
      error(failure, "out of space: no more available data blocks");
   }  

   /*/* This is where we want to find any free space within a block */
   /*This is where we have the target_entry-> first_block = 1 */  
   if (found_free == failure){ 

      /*In order to delete the file entry, we set the first block to zero*/
      // target_entry->first_block == 0;

      error(failure, "out of space: no more available data blocks");
   }
   if(free_block == 0){
     error(failure, "remove directory failure"); 
   }
   write_table(tbl);
   return;
}

int open_dir(struct fat_table *tbl, char* dir_name){
   Error_code result = find_dir_file(tbl, dir_name);
   if (success == result){
      struct dir_entry * entry;

      Error_code r =  read_dir_entry(tbl->fs_file, entry);

      int fat_file = open(tbl->fs_file_name,O_RDONLY );

      of_go_to_block(tbl, fat_file, entry->first_block); 
      return fat_file;
   }
   error(failure, "open_dir failure");
   return 0;

}




void new_file(struct fat_table *tbl, char* new_file_name){
   printf("new_file( %s )\n", new_file_name);
   if (success == find_dir_file(tbl, new_file_name)){
      error(failure, "cant make new_file: a file already exists with than name ");
      return;
   }
   
   int free_block;
   Error_code found_free = get_free_block(tbl, &free_block);
   error(found_free, "no available free data blocks");
   
   if(found_free == success)    
      // search working directory file for unused space to put the new file entry
      found_free = dir_file_find_free_space(tbl, tbl->working_dir);

   if (found_free == failure){ 
      error(found_free, "did not find free space in dir file");
      // if we didnt find space and this is last block then we need to grow the file
      found_free = grow_file(tbl, tbl->working_dir);
      error(found_free, "could not grow file");
   }
 
   if (found_free == success){
      // create dir entry for new file 
      create_new_file(tbl, new_file_name, free_block);
   } 
   if (found_free != success)
      error(failure, "out of space: no more available data blocks");

   write_table(tbl);
   return;
}


/*
 * ===================================================
 */

void of_go_to_start(struct fat_table *tbl, int file){ 
   int b = tbl->open_files[file].entry.first_block;
   of_go_to_block(tbl, file, b);
}

void of_go_to_block(struct fat_table *tbl, int file, int block_num){ 
   off_t block_start = block_num * tbl->super.block_size;
   off_t f_place = lseek(file, block_start, SEEK_SET);
   tbl->open_files[file].current_block = block_num;
}

void init_open_file(struct open_file *f,  int file_descriptor){
   f->current_block = 0;
   f->fh= file_descriptor;
   f->open = 1; 
}

int fs_open(struct fat_table *tbl,  char * path, int flags){
   
   // open our file system file
   int fat_file = open(tbl->fs_file_name, flags);
   if (fat_file < 0){
      error(failure, "fs_open, file system file open failure");
   }
   if (MAX_OPEN <= fat_file){
      close(fat_file);
      error(failure, "fs_open, cannot open file: MAX_OPEN has been reached");
   }

   if(success !=  find_dir_file(tbl, path)){
      // create file if it does not exist
      new_file(tbl, path);
   }

   struct open_file * new_f = &tbl->open_files[fat_file]; 
   
   Error_code r =  read_dir_entry(tbl->fs_file, &new_f->entry);
   
   init_open_file(new_f, fat_file); 
   of_go_to_block(tbl, fat_file, new_f->entry.first_block); 
   return fat_file;
} 

int fs_close(struct fat_table *tbl, int file){
   tbl->open_files[file].open = 0;
   return close(file);
}

int fs_write(struct fat_table *tbl, int file, char* buff, int num_bytes){
   if(tbl->open_files[file].open == 0)
      error(failure, "fs_write, writting to a file that is not open");
   
   //TODO: update the modifiation time timestamp for this file

   int written = 0; 
   off_t po = lseek(file, 0, SEEK_CUR);
   off_t b_size = tbl->super.block_size;
   int current_block = tbl->open_files[file].current_block;
   off_t m = ( current_block * b_size) + b_size;
   
   if(po + num_bytes > m){
      int n = po + num_bytes - m;
      char* buff2 = &buff[n+1];

      written += write(file, buff, n);
      
      if( tbl->table[current_block] > 0)
         of_go_to_block(tbl, file, tbl->table[current_block]);
      else
         grow_file(tbl, current_block);
      written += write(file, buff2, num_bytes -n);
   }else
      written += write(file, buff, num_bytes);

   return written;
}


int fs_read(struct fat_table *tbl, int file, char* buff, int num_bytes){ 
   if(tbl->open_files[file].open == 0)
      error(failure, "fs_write, writting to a file that is not open");
   
   printf("\t--- READING FILE: %s ---\n", tbl->open_files[file].entry.file_name);
   
   int num_read = 0;
   off_t po = lseek(file, 0, SEEK_CUR);

   off_t b_size = tbl->super.block_size; 
   int current_block = tbl->open_files[file].current_block;
   off_t m = ( current_block * b_size) + b_size;
   
   if(po + num_bytes > m){
      int n = po + num_bytes - m;
      char* buff2 = &buff[n+1];
      num_read += read(file, buff, n);
      
      if( tbl->table[current_block] > 0)
         of_go_to_block(tbl, file, tbl->table[current_block]);
      
      num_read += read(file, buff2, num_bytes -n);
   }else
      num_read += read(file, buff, num_bytes);

   return num_read;
}

void dir_entry_2_stat(struct dir_entry * entry, struct stat *st){
   st->st_size  = (off_t)entry->length;
   st->st_atime = (time_t)entry->access;
   st->st_mtime = entry->modification;
   st->st_ctime = entry->creation;
}

int fs_getattr(struct fat_table *tbl, char* path, struct stat *st){
   error(find_dir_file(tbl, (char*)path), "fs_getattr, could not find file"); 
   struct dir_entry e;
   Error_code r =  read_dir_entry(tbl->fs_file, &e);
   dir_entry_2_stat(&e, st);
   return 1;
}
