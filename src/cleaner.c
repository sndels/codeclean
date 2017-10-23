#include "cleaner.h"

#include <ctype.h>
#include <errno.h>
#include <signal.h>
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

// Signal handler
int quit_cleaner = 0;
void sig_int_cleaner(int signo)
{
    if (signo == SIGINT) quit_cleaner = 2;
}

int clean_file(char* path)
{
    // Init signal handling
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags=0;
    sig.sa_handler = sig_int_cleaner;
    sigaction(SIGINT,&sig,NULL);

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

    printf("Cleaning code\n");

    // Go through input
    StreamState state = DEFAULT;
    int line_number = 1;
    int print_char = 0;
    off_t line_start = 0;
    off_t out_off = 0;
    for (off_t in_off = 0; in_off < input_file.size - 1; in_off++) {
        // Check if user has interrupted
        if (quit_cleaner) break;

        // Read relevant characters
        char current_char = input_file.map[in_off];
        char next_char = input_file.map[in_off + 1];

        // Flags for special cases
        int newline = current_char == '\n';
        int comment_line_start = current_char == '/' && next_char == '/';
        int comment_block_start = current_char == '/' && next_char == '*';
        int comment_block_end = current_char == '*' && next_char == '/';

        // Keep count of input lines, reset line status
        if (newline) {
            if (!print_char && state != COMMENT_BLOCK)
                printf("Skipped empty line %i\n", line_number);
            line_number++;
            if (state == COMMENT_LINE)
                state = DEFAULT;
            if (state != COMMENT_BLOCK) {
                // Include first of multiple newlines
                if (print_char) {
                    output_file.map[out_off] = current_char;
                    out_off++;
                }
            }
            print_char = 0;
            line_start = in_off + 1;
        } else {
            if (state == COMMENT_LINE) {
                continue;
            } else if (state == COMMENT_BLOCK) { // Wait for */ to end comment block
                if (comment_block_end) {
                    state = DEFAULT;
                    printf("Comment block ended on line %i\n", line_number);
                    in_off += 2;
                    print_char = 1;
                }
                continue;
            } else { // Check for starting comment
                if (comment_line_start){
                    state = COMMENT_LINE;
                    printf("Comment at the end of line %i\n", line_number);
                    in_off++;
                    continue;
                } else if (comment_block_start) {
                    state = COMMENT_BLOCK;
                    printf("Comment block started on line %i\n", line_number);
                    in_off++;
                    continue;
                } else {
                    // Print indentation for populated line and reset 
                    if (!print_char && !isspace(current_char)) {
                        for (off_t c = line_start; c < in_off; c++, out_off++)
                            output_file.map[out_off] = input_file.map[c];
                        print_char = 1;
                    }
                }
            }
        }

        if (print_char) {
            output_file.map[out_off] = current_char;
            out_off++;
        }
    }
    // Print last character if necessary
    char prev_char = input_file.map[input_file.size - 2];
    char cur_char = input_file.map[input_file.size - 1];
    if (!quit_cleaner && ((state == COMMENT_LINE && cur_char == '\n') ||
        (state == DEFAULT && print_char && !(prev_char == '*' && cur_char == '/'))))
        output_file.map[out_off] = cur_char;
    else
        out_off--;

    printf("Finished processing file %s\n", path);

    int ret_val = 0;
    // Sync output to disk, resize file first
    printf("Syncing cleaned file\n");
    if (ftruncate(output_file.fp, out_off + 1) != 0) {
        printf("ftruncate: %s\n", strerror(errno));
        ret_val = 1;
    } else {
        if (msync(output_file.map, out_off + 1, MS_SYNC) != 0) {
            printf("msync: %s\n", strerror(errno));
            ret_val = 1;
        }
    }

    printf("Cleaning up\n");

    // Clean up on files, maps and malloc'd buffers
    close_mapped_file(&input_file);
    close_mapped_file(&output_file);

    printf("Done");
    return ret_val + quit_cleaner;
}
