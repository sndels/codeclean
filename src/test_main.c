#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cleaner.h"
#include "filelock.h"

#define TEST_FILES 3

static const char* test_files[] = {"tests/test_blocks", "tests/test_lines",
                                   "tests/test_combined"};
static const char* reference_files[] = {"tests/test_blocks.correct",
                                        "tests/test_lines.correct",
                                        "tests/test_combined.correct"};
static const char* output_files[] = {"tests/test_blocks.clean",
                                     "tests/test_lines.clean",
                                     "tests/test_combined.clean"};

int main(int argc, char* argv[])
{
    // Disable stdout buffering to fix print order in file logging
    setbuf(stdout, NULL);

    for (int i = 0; i < TEST_FILES; i++) {
        const char* path = test_files[i];

        // Get current stdout
        int old_stdout = dup(STDOUT_FILENO);

        // Open log file
        char* log_path = malloc(strlen(path) + 4);
        sprintf(log_path, "%s.log", path);
        int log_file = open(log_path, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        if (log_file == -1) {
            fprintf(stderr, "Error opening \"%s\"\n", log_path);
            perror("open");
            fprintf(stderr, "Continuing to next file\n");
            continue;
        }
        // Acquire write lock
        if (get_filelock(log_file, F_WRLCK) == -1){
            fprintf(stderr, "Couldn't get a write lock on \"%s\"\n", log_path);
            perror("fcntl");
            close(log_file);
            fprintf(stderr, "Continuing to next file\n");
            continue;
        }
        // Clear log file
        if (ftruncate(log_file, 0) != 0) {
            fprintf(stderr, "Error clearing \"%s\"\n", log_path);
            perror("ftruncate");
            close(log_file);
            fprintf(stderr, "Continuing to next file\n");
            continue;
        }
        // Redirect stdout and stderr
        if (dup2(log_file, STDOUT_FILENO) == -1) {
            fprintf(stderr, "Redirecting stdout to \"%s\" failed\n", log_path);
            perror("dup2");
            close(log_file);
            fprintf(stderr, "Continuing to next file\n");
            continue;
        }
        close(log_file);
        free(log_path);

        // Clean file
        int err = clean_file(path);

        int should_break = 0;
        if (err == 1)
            fprintf(stderr, "Process for cleaning \"%s\" ended in error\n", path);
        else if (err == 2) {
            fprintf(stderr, "Process for cleaning \"%s\" interrupted by user\n", path);
            should_break = 1;
        } else if (err == 3) {
            fprintf(stderr, "Process for cleaning \"%s\" interrupted by user and ended in error\n", path);
            should_break = 1;
        }

        // Restore stdout
        fflush(stdout);
        if (dup2(old_stdout, STDOUT_FILENO) == -1) {
            fprintf(stderr, "Redirecting stdout back to original failed\n");
            perror("dup2");
            should_break = 1;
        }

        // Compare with loop to get line and char positions on errors
        FILE* reference_file = fopen(reference_files[i], "r");
        FILE* output_file = fopen(output_files[i], "r");
        if (reference_file == NULL) {
            printf("Failed to open file %s\n", reference_files[i]);
            continue;
        }
        if (output_file == NULL) {
            printf("Failed to open file %s\n", output_files[i]);
            continue;
        }
        char cor_c;
        char out_c;
        int line = 1;
        int reported = 0;
        while ((cor_c = fgetc(reference_file)) != EOF && (out_c = fgetc(output_file)) != EOF) {
            if (cor_c != out_c && !reported) {
                printf("First error on line %i\n", line);
                reported = 1;
                break;
            }
            if (out_c == '\n') line++;
        }
        if (!reported && (out_c == EOF || (out_c = fgetc(output_file)) != EOF))
            printf("File endings differ\n");

        if (should_break) break;
    }

    exit(EXIT_SUCCESS);
}
