#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cleaner.h"

int main(int argc, char* argv[])
{
    // Disable stdout buffering to fix print order in file logging
    setbuf(stdout, NULL);

    if (argc < 2) {
        fprintf(stderr, "No file given to clean\n");
        exit(EXIT_FAILURE);
    }

    // Spawn a child to handle each given file 
    for (int proc = 0; proc < argc - 1; proc++) {
        pid_t pid;
        if ((pid = fork()) == -1) {
            perror("fork");
        }
        if (pid == 0) {
            char* path = argv[proc + 1];

            // Open log file
            char* log_path = malloc(strlen(path) + 4);
            sprintf(log_path, "%s.log", path);
            int log_file = open(log_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            free(log_path);
            if (log_file == -1) {
                fprintf(stderr, "Error opening %s\n", log_path);
                perror("open");
                exit(EXIT_FAILURE);
            }

            // Redirect stdout and stderr
            int old_stdout = dup(STDOUT_FILENO);
            int old_stderr = dup(STDERR_FILENO);
            if (dup2(log_file, STDOUT_FILENO) == -1 || dup2(log_file, STDERR_FILENO) == -1) {
                perror("cannot redirect stdout or stderr");
                close(log_file);
                exit(EXIT_FAILURE);
            }
            close(log_file);

            // Clean file
            int err = clean_file(path);

            // Restore stdout and stderr 
            fflush(stdout);
            fflush(stderr);
            dup2(old_stdout, STDOUT_FILENO);
            dup2(old_stderr, STDERR_FILENO);

            if (err == 1)
                fprintf(stderr, "Process for cleaning %s failed\n", path);

            exit(err == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
        }
    }

    // Wait for children to exit
    while (wait(NULL) != -1 || errno != ECHILD);

    exit(EXIT_SUCCESS);
}
