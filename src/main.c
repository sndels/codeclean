#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef enum {
    DEFAULT,
    COMMENT_LINE,
    COMMENT_BLOCK
} StreamState;

int clean_file(char* path)
{
    // Open log file
    char* log_path = malloc(strlen(path) + 5);// Allocate string for path+".log"
    sprintf(log_path, "%s.log", path);
    FILE* log_file = fopen(log_path, "w");
    if (log_file == NULL) {
        printf("Opening log file \"%s\" for write failed with errno %i\n", log_path, errno);
        free(log_path);
        return 1;
    }

    // Open input file
    fprintf(log_file,"Mapping file \"%s\" for read\n", path);
    int input_file = open(path, O_RDONLY);
    if (input_file == -1) {
        fprintf(log_file, "open failed with error %i\n", errno); 
        fclose(log_file);
        free(log_path);
        return 1;
    }
    // Check validity
    struct stat sb;
    if (fstat(input_file, &sb) == -1) {
        fprintf(log_file, "fstat failed with error %i\n", errno); 
        fclose(log_file);
        close(input_file);
        free(log_path);
        return 1;
    }
    if (!S_ISREG(sb.st_mode)) {
        fprintf(log_file, "%s is not a file\n", path); 
        fclose(log_file);
        close(input_file);
        free(log_path);
        return 1;
    }
    // Map to memory
    off_t input_len = sb.st_size;
    char* input_mapped = mmap(0, input_len, PROT_READ, MAP_SHARED, input_file, 0);
    if (input_mapped == MAP_FAILED) {
        fprintf(log_file, "mmap failed with error %i\n", errno); 
        fclose(log_file);
        close(input_file);
        free(log_path);
        return 1;
    }
    fprintf(log_file,"Success\n");

    // Open output file
    char* clean_path = malloc(strlen(path) + 7);
    sprintf(clean_path, "%s.clean", path);
    fprintf(log_file,"Mapping file \"%s\" for write\n", clean_path);
    int output_file = open(clean_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (output_file == -1) {
        fprintf(log_file, "open failed with error %i\n", errno); 
        fclose(log_file);
        munmap(input_mapped, input_len);
        close(input_file);
        free(log_path);
        free(clean_path);
        return 1;
    }
    // Set size (ftruncate adds zero padding)
    if (ftruncate(output_file, input_len) != 0) {
        fprintf(log_file, "ftruncate failed with error %i\n", errno); 
        fclose(log_file);
        munmap(input_mapped, input_len);
        close(input_file);
        close(output_file);
        free(log_path);
        free(clean_path);
        return 1;
    }
    // Map to memory
    char* output_mapped = mmap(0, input_len, PROT_WRITE, MAP_SHARED, output_file, 0);
    if (output_mapped == MAP_FAILED) {
        fprintf(log_file, "mmap failed with error %i\n", errno); 
        fclose(log_file);
        munmap(input_mapped, input_len);
        close(input_file);
        close(output_file);
        free(log_path);
        free(clean_path);
        return 1;
    }
    fprintf(log_file,"Success\n");

    // Go through input
    StreamState state = DEFAULT;
    int line_number = 1;
    off_t out_off = 0;
    for (off_t in_off = 0; in_off < input_len - 1; in_off++) {
        // Keep count of input lines
        if (input_mapped[in_off] == '\n')
            line_number++;

        if (state == COMMENT_LINE) { // Wait for newline to end comment line
            if (input_mapped[in_off] == '\n')
                state = DEFAULT;
            else
                continue;
        } else if (state == COMMENT_BLOCK) { // Wait for */ to end comment block
            if (input_mapped[in_off] == '*' && input_mapped[in_off + 1] == '/') {
                state = DEFAULT;
                fprintf(log_file, "Comment block ended on line %i\n", line_number);
                in_off += 2;
            } else
                continue;
        } else { // Check for starting comment
            if (input_mapped[in_off] == '/') {
                if (input_mapped[in_off + 1] == '/') {
                    state = COMMENT_LINE;
                    fprintf(log_file, "Comment at the end of line %i\n", line_number);
                    in_off++;
                    continue;
                } else if (input_mapped[in_off + 1] == '*') {
                    state = COMMENT_BLOCK;
                    fprintf(log_file, "Comment block started on line %i\n", line_number);
                    in_off++;
                    continue;
                }
            }
        }

        // Skip empty lines and lines starting with a comment
        if (input_mapped[in_off] == '\n') {
            if (input_mapped[in_off + 1] == '\n') {
                fprintf(log_file, "Skipped empty line %i\n", line_number);
                continue;
            } else if (input_mapped[in_off + 1] == '/') {
                if (input_mapped[in_off + 2] == '/' || input_mapped[in_off + 2] == '*') {
                    fprintf(log_file, "Skipped empty line %i\n", line_number);
                    continue;
                }
            }
        }
        output_mapped[out_off] = input_mapped[in_off];
        out_off++;
    }
    // Print last character if necessary
    if (state == DEFAULT)
        output_mapped[out_off] = input_mapped[input_len - 1];

    fprintf(log_file, "Finished processing file %s\n", path);

    int ret_val = 0;
    // Sync output to disk, resize file first
    if (ftruncate(output_file, out_off + 1) != 0) {
        fprintf(log_file, "ftruncate failed with error %i\n", errno); 
        ret_val = 1;
    } else {
        if (msync(output_mapped, out_off + 1, MS_SYNC) == -1) {
            fprintf(log_file, "msync failed with error %i\n", errno); 
            ret_val = 1;
        }
    }

    // Clean up on files, maps and malloc'd buffers
    fclose(log_file);
    munmap(input_mapped, input_len);
    munmap(output_mapped, input_len);
    close(input_file);
    close(output_file);
    free(log_path);
    free(clean_path);
    return ret_val;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("No file given to clean\n");
        exit(1);
    }

    int err = clean_file(argv[1]);
    if (err != 0) {
        printf("clean_file returned with error\n");
        exit(1);
    }
    exit(0);
}
