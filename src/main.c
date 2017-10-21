#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <unistd.h>

#include "cleaner.h"

#define MAX_PROC 10

int main(int argc, char* argv[])
{
    // Disable stdout buffering to fix print order in file logging
    setbuf(stdout, NULL);

    if (argc < 2) {
        fprintf(stderr, "No file given to clean\n");
        exit(EXIT_FAILURE);
    }

    int proc;
    pid_t pid;
    // Divide cmd arguments to even blocks
    int block_size = (argc - 1) / MIN(argc - 1, MAX_PROC) + 1;
    int proc_count = (argc - 1) / block_size + 1;
    if (argc > 2) { // Only spawn children if multiple files are given
        // Spawn child processes
        for (proc = 1; proc < proc_count; proc++) {
            // Stop spawning if fork fails, parent will handle rest of the args
            if ((pid = fork()) == -1) {
                perror("fork");
                break;
            } else if (pid == 0)
                break;
        }
    } else {
        proc = 1;
        pid = 1;
    }

    // Loop over block of arguments assigned for each process
    int bloc_start = (proc - 1) * block_size;
    int block_end = pid == 0 ? proc * block_size : argc - 1;// Parent handles trailing args
    for (int i = bloc_start; i < block_end; i++) {
        char* path = argv[i + 1];

        // Open log file
        char* log_path = malloc(strlen(path) + 4);
        sprintf(log_path, "%s.log", path);
        int log_file = open(log_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        free(log_path);
        if (log_file == -1) {
            fprintf(stderr, "Error opening %s\n", log_path);
            perror("open");
            continue;
        }

        // Redirect stdout and stderr
        int old_stdout = dup(STDOUT_FILENO);
        int old_stderr = dup(STDERR_FILENO);
        if (dup2(log_file, STDOUT_FILENO) == -1 || dup2(log_file, STDERR_FILENO) == -1) {
            fprintf(stderr, "Cannot redirect stdout or stderr to log file\n");
            perror("dup2");
            close(log_file);
            continue;
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
    }

    // Parent waits for children to exit
    if (pid != 0) {
        while (wait(NULL) != -1 || errno != ECHILD);
    }

    exit(EXIT_SUCCESS);
}
