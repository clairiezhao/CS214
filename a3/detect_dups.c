// add any other includes in the detetc_dups.h file
#include "detect_dups.h"

// define any other global variable you may need over here

// open ssl, this will be used to get the hash of the file
EVP_MD_CTX *mdctx;
const EVP_MD *EVP_md5(); // use md5 hash!!
hlink_node *filetree_table;

int main(int argc, char *argv[]) {
    // perform error handling, "exit" with failure incase an error occurs
    if(argc != 2) {
        fprintf(stderr, "Usage: ./detect_dups <directory>\n");
    }
    //if invalid dir given
    char *dir = argv[1];

    //initialize the other global variables you have, if any
    filetree_table = NULL;
    mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        fprintf(stderr, "%s::%d::Error allocating MD5 context %d\n", __func__, __LINE__, errno);
        return EXIT_FAILURE;
    }

    //add the nftw handler to explore the directory
    //nftw should invoke the render_file_info function
    int err = nftw(dir, render_file_info, 20, FTW_PHYS);
    
    if(err != 0) {
        fprintf(stderr, "Error %s: %s is not a valid directory\n", strerror(errno), argv[1]);
    }

    free(mdctx);

    //print out hash table
    print_filetree();
}

// render the file information invoked by nftw
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    int md5_len = 0;
    //if regular file
    if((sb->st_mode & S_IFMT) == S_IFREG) {
        int err = compute_file_hash(fpath, mdctx, hash, &md5_len);
        if (err < 0) {
            fprintf(stderr, "%s::%d::Error computing MD5 hash %d\n", __func__, __LINE__, errno);
            exit(EXIT_FAILURE);
        }
        hlink_node *new_node, *ptr;
        new_node->hash = hash;
        new_node->inode_num = sb->st_ino;
        new_node->count = 1;
        new_node->next = NULL;
        new_node->slinks = NULL;
        //add path
        path_node *new_path = {fpath, NULL};
        new_node->paths = new_path;
        //check if key already exists
        HASH_FIND(filetree_table, hash, ptr);
        if(ptr == NULL) {
            HASH_ADD(filetree_table, hash, new_node);
        }
        //add new hlink node if hard link is unique
        else {
            insert_hlink(new_node, ptr);
        }
    }

    //if soft link
    else if((sb->st_mode & S_IFMT) == S_IFLNK) {
        struct stat *lsb = (struct stat*)malloc(sizeof(struct stat));
        if(stat(fpath, lsb) == -1) {
            perror("stat\n");
            exit(EXIT_FAILURE);
        }
        //pass to md5
        int err = compute_file_hash(fpath, mdctx, hash, &md5_len);
        if (err < 0) {
            fprintf(stderr, "%s::%d::Error computing MD5 hash %d\n", __func__, __LINE__, errno);
            exit(EXIT_FAILURE);
        }

        hlink_node *ptr;
        slink_node *new_slink;
        new_slink->inode_num = sb->st_ino;
        new_slink->count = 1;
        path_node *new_path = {fpath, NULL};
        new_slink->paths = new_path;
        new_slink->next = NULL;
        HASH_FIND(filetree_table, hash, ptr);
        //if hash doesn't yet exist
        if(ptr == NULL) {
            hlink_node *new_hlink;
            new_hlink->hash = hash;
            new_hlink->inode_num = lsb->st_ino;
            new_hlink->count = 0;
            new_hlink->next = NULL;
            new_hlink->slinks = slink_node;
            new_hlink->paths = NULL;
        }
        //run through all hard links to check for inode num
        hlink_node *head = ptr;
        else {
            //ptr is head of linked list of hard links
            while(ptr != NULL) {
                if(ptr->inode_num == lsb->st_ino) {
                    insert_slink(new_slink, ptr->slinks);
                    return 0;
                }
                ptr = ptr->next;
            }
            //insert a hard link
        }
        //if DNE, add new hard link node
        hlink_node *new_hlink;
        new_hlink->hash = hash;
        new_hlink->inode_num = lsb->st_ino;
        new_hlink->count = 0;
        new_hlink->next = NULL;
        new_hlink->slinks = slink_node;
        new_hlink->paths = NULL;
        insert(new_hlink, head);
        free(lsb);
    }
    
    return 0;
}

// add any other functions you may need over here
//update duplicate info: pass in md5 hash, pathname, ref count

//insert a hard link
void insert_hlink(hlink_node *hlink, hlink_node *head) {
    if(head == NULL) {
        head = hlink;
        return;
    }
    hlink_node *ptr = head;
    //search list for inode num; if it exists, update node
    while(ptr != NULL) {
        if(ptr->inode_num == hlink->inode_num) {
            ptr->count += 1;
            insert_path(hlink->paths, ptr->paths);
            return;
        }
        ptr = ptr->next;
    }
    //if dne, add to front of list
    hlink->next = head;
    head = hlink;
}

//insert a soft link into the list
void insert_slink(slink_node *slink, slink_node *head) {
    if(head == NULL) {
        head = slink;
        return;
    }
    slink_node *ptr = head;
    //search list for inode num; if it exists, update node
    while(ptr != NULL) {
        if(ptr->inode_num == slink->inode_num) {
            ptr->count += 1;
            insert_path(slink->paths, ptr->paths);
            return;
        }
        ptr = ptr->next;
    }
    //if dne, add to front of list
    slink->next = head;
    head = slink;
}

//insert a path name into a list of path names
void insert_path(path_node *path, path_node *head) {
    if(head != NULL) {
        path->next = head;
    }
    head = path;
}

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

void print_filetree() {
    //pointers
    hlink_node *hptr;
    slink_node *sptr;
    path_node *path_ptr;
    //count number of unique md5 hashes, number of unique soft links
    int hcount = 1, scount = 1;

    //traverse every hash in hash table
    for (hlink_node *hlink = filetree_table; hlink != NULL; hlink = hlink->hh.next) {
        printf("File %d\n", count);
        printf("\tMD5 Hash: ");
        for (int i = 0; i < md5_len; i++) {
            printf("%02x", hash[i]);
        }
        printf("\n");
        //traverse linked list of unique hard links
        hptr = hlink;
        while(hptr != NULL) {
            printf("\t\tHard Link (%u): %lu\n", hptr->count, hptr->inode_num);
            path_ptr = hptr->paths;
            if(path_ptr != NULL) {
                printf("\t\t\tPaths:\t%s\n", path_ptr->path);
                path_ptr = path_ptr->next;
            }
            while(path_ptr != NULL) {
                printf("\t\t\t\t%s\n", path_ptr->path);
                path_ptr = path_ptr->next;
            }
            sptr = hlink->slinks;
            scount = 1;
            while(sptr != NULL) {
                printf("\t\t\tSoft Link %d(%u): %lu\n", scount, sptr->count, sptr->inode_num);
                path_ptr = sptr->paths;
                if(path_ptr != NULL) {
                    printf("\t\t\tPaths:\t%s\n", path_ptr->path);
                    path_ptr = path_ptr->next;
                }
                while(path_ptr != NULL) {
                    printf("\t\t\t\t%s\n", path_ptr->path);
                    path_ptr = path_ptr->next;
                }
                sptr = sptr->next;
            }
            hptr = hptr->next;
        }
    }
}