#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include <string.h>

#define BUFFER_LEN 85

int clean_file(char* path)
{
    // Open log file
    char* log_path = malloc(strlen(path) + 5);// Allocate string for path+".log"
    sprintf(log_path, "%s.log", path);
    FILE* log_file = fopen(log_path, "w");
    if (log_file == NULL) {
        printf("Opening log file \"%s\" for write failed with errno %i\n", log_path, errno);
        return -1;
    }

    // Open input file
    fprintf(log_file,"Opening file \"%s\" for read\n", path);
    FILE* input_file = fopen(path, "r");
    if (input_file == NULL) {
        fprintf(log_file, "Failed with error %i\n", errno); 
        fclose(log_file);
        return -1;
    }
    fprintf(log_file,"Success\n");

    // Open output file
    char* clean_path = malloc(strlen(path) + 7);
    sprintf(clean_path, "%s.clean", path);
    fprintf(log_file,"Opening file \"%s\" for write\n", clean_path);
    FILE* output_file = fopen(clean_path, "w");
    if (output_file == NULL) {
        fprintf(log_file, "Failed with error %i\n", errno); 
        fclose(log_file);
        fclose(input_file);
        return -1;
    }
    fprintf(log_file,"Success\n");

    // Go through input
    int in_comment_line = 0;
    int in_comment_block = 0;
    int ended_with_slash = 0;
    int ended_with_star = 0;
    int has_newline = 0;
    int line_number = 1;
    char raw_buf[BUFFER_LEN];
    while (fgets(raw_buf, BUFFER_LEN, input_file) != NULL) {
        // NOTE: Should indented line comment only -lines also be removed?
        // Count lines
        if (has_newline)
            line_number++;

        // Skip empty lines
        if (has_newline && raw_buf[0] == '\n') {
            fprintf(log_file, "Skipped empty line %i\n", line_number);
            continue;
        }
        
        // Handle last buffer ending on '/'
        if (ended_with_slash) {
            if (raw_buf[0] == '/') {
                in_comment_line = 1;
                fprintf(output_file, "\n");
                fprintf(log_file, "Removed line comment from line %i\n", line_number);
                continue;
            } else if (raw_buf[0] == '*') {
                in_comment_block = 1;
                fprintf(output_file, "\n");
                fprintf(log_file, "Comment block started in line %i\n", line_number);
                continue;
            } else
                fprintf(output_file, "/");
        }

        // Handle last buffer ending on '*'
        if (in_comment_block && ended_with_star) {
            if (raw_buf[0] == '/') {
                in_comment_block = 0;
                strcpy(raw_buf, raw_buf + 1);
                fprintf(log_file, "Comment block ended on line %i\n", line_number);
            } else
                fprintf(output_file, "*");
        }

        // Check for newline
        has_newline = strchr(raw_buf, '\n') != NULL;

        // Handle oversized line comment
        if (in_comment_line) {
            if (has_newline) {
                in_comment_line = 0;
                fprintf(output_file, "\n");
            }
            continue;
        }

        // Handle comment block
        if (in_comment_block) {
            // Check for ending tag
            char* comment_end = strchr(raw_buf, '*');
            while (comment_end != NULL) {
                // Handle the hit being the last character in buffer
                if (comment_end == raw_buf + BUFFER_LEN - 2) {
                    comment_end = NULL;
                    ended_with_star = 1;
                    break;
                }

                if (comment_end[1] == '/') {
                    in_comment_block = 0;
                    strcpy(raw_buf, comment_end + 2);
                    fprintf(log_file, "Comment block ended on line %i\n", line_number);
                    break;
                }
                comment_end = strchr(comment_end + 1, '*');
            }
            
            // Skip buffer if no ending tag was found
            if (comment_end == NULL)
                continue;
        }

        // Check for a starting comment
        char* comment_start = strchr(raw_buf, '/');
        while (comment_start != NULL) {
            // Handle the hit being the last character in buffer
            if (comment_start == raw_buf + BUFFER_LEN - 2) {
                comment_start = NULL;
                ended_with_slash = 1;
                break;
            }

            // Check following character and set "state"
            if (comment_start[1] == '/') {
                in_comment_line = !has_newline;
                fprintf(log_file, "Removed line comment from line %i\n", line_number);
                break;
            }
            else if (comment_start[1] == '*') {
                fprintf(log_file, "Comment block started in line %i\n", line_number);
                in_comment_block = 1;
                break;
            }
            comment_start = strchr(comment_start + 1, '/');
        }

        // Cut line short if comment is found
        if (comment_start != NULL) {
            if (comment_start == raw_buf) {
                raw_buf[0] = '\0';
            } else {
                comment_start[0] = '\n';
                comment_start[1] = '\0';
            }
        }

        // Write cleaned line to output
        fprintf(output_file, "%s", raw_buf);
    }
    fprintf(log_file, "File ended on line %i\n", line_number);

    // Clean up on streams and malloc'd buffers
    fclose(log_file);
    fclose(input_file);
    fclose(output_file);
    free(log_path);
    free(clean_path);
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("No file given to clean\n");
        return 1;
    }
    int err = clean_file(argv[1]);
    if (err != 0)
        return 1;
    return 0;
}
