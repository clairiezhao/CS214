// add any other includes in the detetc_dups.h file
#include "detect_dups.h"
#define _XOPEN_SOURCE = 500

// define any other global variable you may need over here

// open ssl, this will be used to get the hash of the file
EVP_MD_CTX *mdctx;
const EVP_MD *EVP_md5(); // use md5 hash!!

typedef struct file_node {
    unsigned char *hash;
    char *path;
    //0 if soft link, 1 if hard link
    int hard_link;
    file_node *next;
    UT_hash_handle hh;
} file_node;

int main(int argc, char *argv[]) {
    // perform error handling, "exit" with failure incase an error occurs
    if(argc != 2) {
        fprintf(stderr, "Usage: ./detect_dups <directory>\n");
    }
    //if invalid dir given
    char *dir = argv[1];

    // initialize the other global variables you have, if any
    file_node *filetree_table = NULL;

    // add the nftw handler to explore the directory
    // nftw should invoke the render_file_info function
    int err = nftw(dir, render_file_info, 20, 0);

    //print out hash table
    print_filetree(filetree_table);
}

// render the file information invoked by nftw
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    // perform the inode operations over here
    if((sb->st_mode & S_IFMT) == S_IFLINK) {
        //regular file
        printf(" ;symbolic link\n");
    }

    // invoke any function that you may need to render the file information
    unsigned char *hash[EVP_MAX_MD_SIZE];
    int md5_len = 0;
    mdctx = EVP_MD_CTX_new();
    int err = compute_file_hash(fpath, mdctx, hash, md5_len);

    if (err < 0) {
        fprintf(stderr, "%s::%d::Error computing MD5 hash %d\n", __func__, __LINE__, errno);
        exit(EXIT_FAILURE);
    }

    file_node new_node = {hash, fpath, 0, NULL, hh};
    file_node ptr;
    //check if key already exists
    HASH_FIND(filetree_table, hash, ptr);
    if(ptr == NULL) {
        HASH_ADD(filetree_table, hash, new_node);
    }
    //add file to head of linked list
    //set file number to current number of items in hash table + 1
    else {
        new_node.next = ptr;
        //replace
        HASH_DEL(filetree_table, ptr);
        HASH_ADD(filetree_table, hash, new_node);
    }
    
}

// add any other functions you may need over here
//update duplicate info: pass in md5 hash, pathname, ref count

//*md_value is a string with the final md5 hashcode
int compute_file_hash(const char *path, EVP_MD_CTX *mdctx, unsigned char *md_value, unsigned int *md5_len) {
    FILE *fd = fopen(path, "rb");
    if (fd == NULL) {
        fprintf(stderr, "%s::%d::Error opening file %d: %s\n", __func__, __LINE__, errno, path);
        return -1;
    }
    char buffer[4096];
    unsigned int n;
    //initialize mdctx structure
    EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);
    //read chunks of bytes (max size 4096) from file
    while ((n = fread(buffer, 1, 4096, fd))) {
        EVP_DigestUpdate(mdctx, buffer, n);
    }
    //calculate final hash code;
    EVP_DigestFinal_ex(mdctx, md_value, md5_len);
    //reset mdctx to compute other files
    EVP_MD_CTX_reset(mdctx);
    fclose(fd);
    return 0;
}

void print_filetree(file_node *filetree_table) {
    file_node *file;

    for (file = filetree_table; file != NULL; file = file->hh.next) {
        printf("File <number>:");
        printf("\tMD5 Hash: <hash>");
        //traverse linked list with head = file
        printf("\t\tHard Link (<count>): <inode>");
        printf("\t\t\tPaths:\t<Path 1>");
        printf("\t\t\t\t<Path N>");
        printf("\t\t\tSoft Link <number>(<count>): <inode>");
        printf("\t\t\t\tPaths:\t<Path 1>");
        printf("\t\t\t\t\t<Path N>");
    }
}