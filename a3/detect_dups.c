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
        fprintf(stderr, "Error %d: %s is not a valid directory\n", errno, argv[1]);
        return EXIT_FAILURE;
    }

    //print out hash table
    print_filetree();

    //deallocate memory
    EVP_MD_CTX_free(mdctx);
    hlink_node *hprev, *hcurr, *hlast = NULL;
    slink_node *sprev, *scurr;
    path_node *path_prev, *path_curr;
    for (hlink_node *hlink = filetree_table; hlink != NULL; hlink = hlink->hh.next) {
        if(hlast != NULL)
            free(hlast);
        hprev = hlink;
        hcurr = hlink;
        hlast = hlink;
        while(hcurr != NULL) {
            sprev = hcurr->slinks;
            scurr = hcurr->slinks;
            while(scurr != NULL) {
                path_prev = scurr->paths;
                path_curr = scurr->paths;
                while(path_curr != NULL) {
                    path_prev = path_curr;
                    path_curr = path_curr->next;
                    free(path_prev->path);
                    free(path_prev);
                }
                sprev = scurr;
                scurr = scurr->next;
                free(sprev);
            }
            path_prev = hcurr->paths;
            path_curr = hcurr->paths;
            while(path_curr != NULL) {
                path_prev = path_curr;
                path_curr = path_curr->next;
                free(path_prev->path);
                free(path_prev);
            }
            free(hcurr->hash);
            hprev = hcurr;
            hcurr = hcurr->next;
            if(hprev != hlink) {
                free(hprev);
            }
        }
    }
    if(hlast != NULL) {
        free(hlast);
    }
    return EXIT_SUCCESS;

}

// render the file information invoked by nftw
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    //printf("path: %s\n", fpath);
    unsigned char md5_hash[EVP_MAX_MD_SIZE];
    int md5_len = 0;
    //if regular file
    if((sb->st_mode & S_IFMT) == S_IFREG) {
        int err = compute_file_hash(fpath, mdctx, md5_hash, &md5_len);
        if (err < 0) {
            perror("MD5 hash error\n");
            exit(EXIT_FAILURE);
        }
        hlink_node *new_node;
        hlink_node *ptr;
        new_node = create_hlink(md5_hash, sb->st_ino, 1, md5_len, NULL, NULL, fpath);
        //check if key already exists
        HASH_FIND(hh, filetree_table, md5_hash, md5_len, ptr);
        if(ptr == NULL) {
            HASH_ADD_KEYPTR(hh, filetree_table, new_node->hash, md5_len, new_node);
        }
        //add new hlink node if hard link is unique
        else {
            insert_hlink(new_node, &ptr);
        }
    }

    //if soft link
    else if((sb->st_mode & S_IFMT) == S_IFLNK) {
        struct stat *lsb = (struct stat*)malloc(sizeof(struct stat));
        if(stat(fpath, lsb) == -1) {
            perror("Stat call error\n");
            exit(EXIT_FAILURE);
        }
        //if symbolic link points to a directory, don't process
        if((lsb->st_mode & S_IFMT) == S_IFDIR) {
            return 0;
        }
        //pass to md5
        int err = compute_file_hash(fpath, mdctx, md5_hash, &md5_len);
        if (err < 0) {
            perror("MD5 hash error\n");
            exit(EXIT_FAILURE);
        }
        hlink_node *ptr;
        //create new soft link node
        slink_node *new_slink = (slink_node*)malloc(sizeof(slink_node));
        new_slink->inode_num = sb->st_ino;
        new_slink->count = 1;
        path_node *new_path = (path_node*)malloc(sizeof(path_node));
        int path_len = strlen(fpath) + 1;
        new_path->path = (char*)malloc(path_len * sizeof(char));
        strncpy(new_path->path, fpath, path_len);
        new_path->next = NULL;
        new_slink->paths = new_path;
        new_slink->next = NULL;
        //check if hash exists in table
        HASH_FIND(hh, filetree_table, md5_hash, md5_len, ptr);
        if(ptr == NULL) {
            hlink_node *new_hlink = create_hlink(md5_hash, lsb->st_ino, 0, md5_len, NULL, new_slink, NULL);
            HASH_ADD_KEYPTR(hh, filetree_table, new_hlink->hash, md5_len, new_hlink);
        }
        else {
            //run through all hard links to check for inode num
            hlink_node *head = ptr;
            //ptr is head of linked list of hard links
            while(ptr != NULL) {
                if(ptr->inode_num == lsb->st_ino) {
                    insert_slink(new_slink, ptr);
                    free(lsb);
                    return 0;
                }
                ptr = ptr->next;
            }
            //if DNE, add new hard link node
            hlink_node *new_hlink = create_hlink(md5_hash, lsb->st_ino, 0, md5_len, NULL, new_slink, NULL);
            insert_hlink(new_hlink, &head);
            free(lsb);
        }
    }
    
    return 0;
}

// add any other functions you may need over here
//update duplicate info: pass in md5 hash, pathname, ref count

hlink_node* create_hlink(unsigned char *hash, unsigned long int inode, unsigned int count, unsigned int hash_len, hlink_node *next, slink_node *slinks, char* path_name) {
    hlink_node *new_node = (hlink_node*)malloc(sizeof(hlink_node));;
    new_node->hash = (unsigned char*)malloc((hash_len) * sizeof(unsigned char));
    strncpy(new_node->hash, hash, hash_len);
    new_node->inode_num = inode;
    new_node->count = count;
    new_node->hash_len = hash_len;
    new_node->next = next;
    new_node->slinks = slinks;
    if(path_name == NULL)
        new_node->paths = NULL;
    else {
        int path_len = strlen(path_name) + 1;
        path_node *new_path = (path_node*)malloc(sizeof(path_node));
        new_path->path = (char*)malloc(path_len * sizeof(char));
        strncpy(new_path->path, path_name, path_len);
        new_path->next = NULL;
        new_node->paths = new_path;
    }
    return new_node;
}

//insert a hard link
void insert_hlink(hlink_node *hlink, hlink_node **head) {
    //head cannot be null
    if((*head) == NULL) {
        return;
    }
    hlink_node *ptr = (*head), *prev = (*head);
    //search list for inode num; if it exists, update node
    while(ptr != NULL) {
        if(ptr->inode_num == hlink->inode_num) {
            ptr->count += 1;
            //insert new path name
            if(ptr->paths != NULL) {
                hlink->paths->next = ptr->paths;
            }
            ptr->paths = hlink->paths;
            return;
        }
        prev = ptr;
        ptr = ptr->next;
    }
    //if dne, add to end of list
    prev->next = hlink;
}

//insert a soft link into the list
void insert_slink(slink_node *slink, hlink_node *hlink) {
    if(hlink->slinks == NULL) {
        hlink->slinks = slink;
        return;
    }

    //search list for inode num; if it exists, update node
    slink_node *ptr = hlink->slinks;
    while(ptr != NULL) {
        if(ptr->inode_num == slink->inode_num) {
            ptr->count += 1;
            if(ptr->paths != NULL) {
                slink->paths->next = ptr->paths;
            }
            ptr->paths = slink->paths;
            return;
        }
        ptr = ptr->next;
    }
    //if dne, add to front of list
    slink->next = hlink->slinks;
    hlink->slinks = slink;
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
        printf("File %d\n", hcount);
        printf("\tMD5 Hash: ");
        for (int i = 0; i < hlink->hash_len; i++) {
            printf("%02x", (hlink->hash)[i]);
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
            sptr = hptr->slinks;
            scount = 1;
            while(sptr != NULL) {
                printf("\t\t\tSoft Link %d(%u): %lu\n", scount, sptr->count, sptr->inode_num);
                path_ptr = sptr->paths;
                if(path_ptr != NULL) {
                    printf("\t\t\t\tPaths:\t%s\n", path_ptr->path);
                    path_ptr = path_ptr->next;
                }
                while(path_ptr != NULL) {
                    printf("\t\t\t\t\t%s\n", path_ptr->path);
                    path_ptr = path_ptr->next;
                }
                sptr = sptr->next;
                scount++;
            }
            hptr = hptr->next;
        }
        hcount++;
    }
}