#include "filelock.h"

#include <fcntl.h>
#include <stdio.h>

int get_filelock(int fd, int type)
{
    struct flock lockinfo;
    lockinfo.l_whence = SEEK_SET;
    lockinfo.l_start = 0;
    lockinfo.l_len = 0;
    lockinfo.l_type = type;

    return fcntl(fd, F_SETLK, &lockinfo);
}
