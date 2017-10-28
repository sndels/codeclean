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
static int quit = 0;
static void sig_int(int signo)
{
    if (signo == SIGINT) quit = 2;
}

int clean_file(const char* path)
{
    // Init signal handling
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags=0;
    sig.sa_handler = sig_int;
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

    // Reserve memory for first pass
    char* tmp_output = malloc(input_file.size);
    if (tmp_output == NULL) {
        printf("Failed to reserve memory for intermediate cleanup result");
        close_mapped_file(&input_file);
        close_mapped_file(&output_file);
        return 1;
    }

    printf("Cleaning comments\n");

    // Remove comments
    StreamState state = DEFAULT;
    int line_number = 1;
    int tmp_last= 0;
    for (off_t in_off = 0; in_off < input_file.size - 1; in_off++) {
        // Check if user has interrupted
        if (quit) break;

        // Read relevant characters
        char current_char = input_file.map[in_off];
        char next_char = input_file.map[in_off + 1];

        // Flags for special cases
        int newline = current_char == '\n';
        int comment_line_start = current_char == '/' && next_char == '/';
        int comment_block_start = current_char == '/' && next_char == '*';
        int comment_block_end = current_char == '*' && next_char == '/';

        // Keep count of input lines, reset comment status
        if (newline) {
            line_number++;
            if (state == COMMENT_LINE)
                state = DEFAULT;
        }

        if (state == COMMENT_LINE) {
            continue;
        } else if (state == COMMENT_BLOCK) { // Wait for */ to end comment block
            if (comment_block_end) {
                state = DEFAULT;
                printf("Comment block ended on line %i\n", line_number);
                in_off++;
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
            }
        }

        tmp_output[tmp_last] = current_char;
        tmp_last++;
    }
    // Copy last character if necessary
    char prev_char = input_file.map[input_file.size - 2];
    char cur_char = input_file.map[input_file.size - 1];
    if (!quit && state == DEFAULT && !(prev_char == '*' && cur_char == '/'))
        tmp_output[tmp_last] = cur_char;
    else
        tmp_last--;

    printf("Cleaning empty lines\n");

    // Write cleaned code to mapped output without empty lines
    off_t out_last = 0;
    int line_start = 0;
    int print_char = 0;
    for (int tmp_off = 0; tmp_off <= tmp_last; tmp_off++) {
        if (quit) break;

        char cur_char = tmp_output[tmp_off];

        // Check if a non-whitespace character is encountered
        if (!print_char) {
            if (!isspace(cur_char)) {
                print_char = 1;
                // Write indentation
                for (int c = line_start; c < tmp_off; c++, out_last++)
                    output_file.map[out_last] = tmp_output[c];
            }
        }

        // Write current character if on a non-empty line
        if (print_char) {
            output_file.map[out_last] = cur_char;
            out_last++;
        }

        // Write first of sequential newline
        if (cur_char == '\n') {
            print_char = 0;
            line_start = tmp_off + 1;
        }
    }

    // Remove trailing newlines
    while (output_file.map[--out_last] == '\n')
        output_file.map[out_last] = '\0';

    printf("Finished processing file %s\n", path);

    // Sync output to disk, resize file first
    int ret_val = 0;
    printf("Syncing cleaned file\n");
    if (ftruncate(output_file.fp, out_last + 1) != 0) {
        printf("ftruncate: %s\n", strerror(errno));
        ret_val = 1;
    } else {
        if (msync(output_file.map, out_last + 1, MS_SYNC) != 0) {
            printf("msync: %s\n", strerror(errno));
            ret_val = 1;
        }
    }

    printf("Cleaning up\n");

    // Clean up on files, maps and malloc'd buffers
    free(tmp_output);
    close_mapped_file(&input_file);
    close_mapped_file(&output_file);

    printf("Done");
    return ret_val + quit;
}
