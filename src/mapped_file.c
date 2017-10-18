#include "mapped_file.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct MappedFile open_input_mapped(char* path, FILE* log_file)
{
    fprintf(log_file,"Mapping file \"%s\" for read\n", path);
    struct MappedFile input_file;
    // Open file
    input_file.fp = open(path, O_RDONLY);
    if (input_file.fp == -1) {
        fprintf(log_file, "open failed with error %i\n", errno); 
        return input_file;
    }
    // Check validity
    struct stat sb;
    if (fstat(input_file.fp, &sb) == -1) {
        fprintf(log_file, "fstat failed with error %i\n", errno); 
        close(input_file.fp);
        input_file.fp = -1;
        return input_file;
    }
    if (!S_ISREG(sb.st_mode)) {
        fprintf(log_file, "%s is not a file\n", path); 
        close(input_file.fp);
        input_file.fp = -1;
        return input_file;
    }
    // Map to memory
    input_file.size = sb.st_size;
    input_file.map = mmap(0, input_file.size, PROT_READ, MAP_SHARED, input_file.fp, 0);
    if (input_file.map == MAP_FAILED) {
        fprintf(log_file, "mmap failed with error %i\n", errno); 
        close(input_file.fp);
        input_file.fp = -1;
    } else
        fprintf(log_file,"Success\n");

    return input_file;
}

struct MappedFile open_output_mapped(char* path, off_t size, FILE* log_file)
{
    char* clean_path = malloc(strlen(path) + 7);
    sprintf(clean_path, "%s.clean", path);
    fprintf(log_file,"Mapping file \"%s\" for write\n", clean_path);
    // Open file for write
    struct MappedFile output_file;
    output_file.fp = open(clean_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    free(clean_path);
    if (output_file.fp == -1) {
        fprintf(log_file, "open failed with error %i\n", errno); 
        return output_file;
    }
    // Set size (ftruncate adds zero padding)
    output_file.size = size;
    if (ftruncate(output_file.fp, size) != 0) {
        fprintf(log_file, "ftruncate failed with error %i\n", errno); 
        close(output_file.fp);
        output_file.fp = -1;
        return output_file;
    }
    // Map to memory
    output_file.map = mmap(0, output_file.size, PROT_WRITE, MAP_SHARED, output_file.fp, 0);
    if (output_file.map == MAP_FAILED) {
        fprintf(log_file, "mmap failed with error %i\n", errno); 
        close(output_file.fp);
        free(clean_path);
        output_file.fp = -1;
    } else
        fprintf(log_file,"Success\n");

    return output_file;
}

int close_mapped_file(struct MappedFile* mf, FILE* log_file)
{
    int ret_val = 0;
    if (munmap(mf->map, mf->size) == -1) {
        fprintf(log_file, "munmap failed with error %i\n", errno); 
        ret_val = 1;
    }
    if (close(mf->fp) == -1) {
        fprintf(log_file, "closefailed with error %i\n", errno); 
        ret_val = 1;
    }
    return ret_val;
}
