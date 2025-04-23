#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <errno.h>
#include <openssl/evp.h>
#include "uthash.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// define the structure required to store the file paths
typedef struct hlink_node {
    unsigned char *hash;
    //inode number, curr count of hard links with that num
    unsigned long int inode_num;
    unsigned int count, hash_len;
    //next unique hard link
    struct hlink_node *next;
    //head of linked list of soft links
    struct slink_node *slinks;
    //head of linked list of hard link path names
    struct path_node *paths;
    UT_hash_handle hh;
} hlink_node;

typedef struct slink_node {
    //inode number, curr count of soft links with that num
    unsigned long int inode_num;
    unsigned int count;
    struct path_node *paths;
    //next unique soft link
    struct slink_node *next;
} slink_node;

typedef struct path_node {
    char *path;
    struct path_node *next;
} path_node;

// process nftw files using this function
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);

int compute_file_hash(const char *path, EVP_MD_CTX *mdctx, unsigned char *md_value, unsigned int *md5_len);
void print_filetree();

//linked list operations
hlink_node* create_hlink(unsigned char *hash, unsigned long int inode, unsigned int count, unsigned int hash_len, hlink_node *next, slink_node *slinks, char *path_name);
void insert_hlink(hlink_node *hlink, hlink_node **head);
void insert_slink(slink_node *slink, hlink_node *hlink);

// add any other function you may need over here