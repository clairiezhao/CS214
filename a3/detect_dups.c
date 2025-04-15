// add any other includes in the detetc_dups.h file
#include "detect_dups.h"

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
        printf("Usage: ./detect_dups <directory>\n");
    }
    //if invalid dir given

    // initialize the other global variables you have, if any
    file_node *filetree_hash_table = NULL;

    // add the nftw handler to explore the directory
    // nftw should invoke the render_file_info function

    //print out list of md5 hashes
}

// render the file information invoked by nftw
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    // perform the inode operations over here

    // invoke any function that you may need to render the file information
    unsigned char *hash;
    compute_file_hash(fpath, mdctx, hash, md5_len);
    //if correctly computed hash
    //add to hash table
    //check if key already exists
    //add file to head of linked list
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