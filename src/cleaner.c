#include "cleaner.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mapped_file.h"

typedef enum {
    DEFAULT,
    COMMENT_LINE,
    COMMENT_BLOCK
} StreamState;

int clean_file(char* path)
{
    // Open input file
    struct MappedFile input_file = open_input_mapped(path);
    if (input_file.fp == -1) {
        return 1;
    }

    // Open output file
    struct MappedFile output_file = open_output_mapped(path, input_file.size);
    if (output_file.fp == -1) {
        close_mapped_file(&input_file);
        return 1;
    }

    // Go through input
    StreamState state = DEFAULT;
    int line_number = 1;
    off_t out_off = 0;
    for (off_t in_off = 0; in_off < input_file.size - 1; in_off++) {
        // Keep count of input lines
        if (input_file.map[in_off] == '\n')
            line_number++;

        if (state == COMMENT_LINE) { // Wait for newline to end comment line
            if (input_file.map[in_off] == '\n')
                state = DEFAULT;
            else
                continue;
        } else if (state == COMMENT_BLOCK) { // Wait for */ to end comment block
            if (input_file.map[in_off] == '*' && input_file.map[in_off + 1] == '/') {
                state = DEFAULT;
                printf("Comment block ended on line %i\n", line_number);
                in_off += 2;
            } else
                continue;
        } else { // Check for starting comment
            if (input_file.map[in_off] == '/') {
                if (input_file.map[in_off + 1] == '/') {
                    state = COMMENT_LINE;
                    printf("Comment at the end of line %i\n", line_number);
                    in_off++;
                    continue;
                } else if (input_file.map[in_off + 1] == '*') {
                    state = COMMENT_BLOCK;
                    printf("Comment block started on line %i\n", line_number);
                    in_off++;
                    continue;
                }
            }
        }

        // Skip empty lines and lines starting with a comment
        if (input_file.map[in_off] == '\n') {
            if (input_file.map[in_off + 1] == '\n') {
                printf("Skipped empty line %i\n", line_number);
                continue;
            } else if (input_file.map[in_off + 1] == '/') {
                if (input_file.map[in_off + 2] == '/' || input_file.map[in_off + 2] == '*') {
                    printf("Skipped empty line %i\n", line_number);
                    continue;
                }
            }
        }
        output_file.map[out_off] = input_file.map[in_off];
        out_off++;
    }
    // Print last character if necessary
    if (state == DEFAULT)
        output_file.map[out_off] = input_file.map[input_file.size - 1];

    printf("Finished processing file %s\n", path);

    int ret_val = 0;
    // Sync output to disk, resize file first
    printf("Syncing cleaned file\n");
    if (ftruncate(output_file.fp, out_off + 1) != 0) {
        perror("ftruncate");
        ret_val = 1;
    } else {
        if (msync(output_file.map, out_off + 1, MS_SYNC) == -1) {
            perror("msync");
            ret_val = 1;
        }
    }

    printf("Cleaning up\n");
    
    // Clean up on files, maps and malloc'd buffers
    close_mapped_file(&input_file);
    close_mapped_file(&output_file);
    return ret_val;
}
