#ifndef MAPPED_FILE_H
#define MAPPED_FILE_H

#include <stdio.h>
#include <sys/types.h>

// Simple wrapper for handling memorymapped files
struct MappedFile {
    int fp;
    off_t size;
    char* map;
};

// Open file for write read and map it to memory
struct MappedFile open_input_mapped(const char* path);

// Open file [path].clean for write, resize it and map it to memory
struct MappedFile open_output_mapped(const char* path, off_t size);

// Unmap and close file pointer
// NOTE: Does not msync()
int close_mapped_file(struct MappedFile* mf);

#endif // MAPPED_FILE_H
