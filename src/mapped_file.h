#ifndef MAPPED_FILE_H
#define MAPPED_FILE_H

#include <stdio.h>
#include <sys/types.h>

struct MappedFile {
    int fp;
    off_t size;
    char* map;
};

struct MappedFile open_input_mapped(char* path, FILE* log_file);
struct MappedFile open_output_mapped(char* path, off_t size, FILE* log_file);
int close_mapped_file(struct MappedFile* mf, FILE* log_file);

#endif // MAPPED_FILE_H
