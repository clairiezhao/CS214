#define _XOPEN_SOURCE 500
#include <ftw.h>
/*
    Add any other includes you may need over here...
*/
#include <openssl/evp.h>
#include "uthash.h"

// define the structure required to store the file paths

// process nftw files using this function
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);

// add any other function you may need over here