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
    if (argc < 2) {
        fprintf(stderr, "No file given to clean\n");
        exit(1);
    }

    // Spawn a child to handle each given file 
    for (int proc = 0; proc < argc - 1; proc++) {
        pid_t pid;
        if ((pid = fork()) == -1) {
            perror("fork");
        }
        if (pid > 0) {
            char* path = argv[proc + 1];

            // Open log file
            char* log_path = malloc(strlen(path) + 4);
            sprintf(log_path, "%s.log", path);
            int log_file = open(log_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if (log_file == -1) {
                fprintf(stderr, "Error opening %s\n", log_path);
                perror("open");
                exit(1);
            }

            // Redirect stdout and stderr
            int old_stdout = dup(fileno(stdout));
            int old_stderr = dup(fileno(stderr));
            if (dup2(log_file, fileno(stdout)) == -1 || dup2(log_file, fileno(stderr)) == -1)
                perror("cannot redirect stdout or stderr");

            // Clean file
            int err = clean_file(path);

            // Restore stdout and stderr 
            fflush(stdout);
            fflush(stderr);
            close(log_file);
            dup2(old_stdout, fileno(stdout));
            dup2(old_stderr, fileno(stderr));

            if (err == 1)
                fprintf(stderr, "Process for cleaning %s failed\n", path);

            exit(err);
        }
    }

    // Wait for children to exit
    while (wait(NULL) != -1 || errno != ECHILD);

    exit(0);
}
