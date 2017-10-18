#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cleaner.h"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("No file given to clean\n");
        exit(1);
    }

    int err = clean_file(argv[1]);
    if (err != 0) {
        printf("clean_file returned with error\n");
        exit(1);
    }
    exit(0);
}
