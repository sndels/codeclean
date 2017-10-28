#ifndef FILELOCK_H
#define FILELOCK_H

// Try getting lock, return fnctl's value
int get_filelock(int fd, int type);

#endif // FILELOCK_H
