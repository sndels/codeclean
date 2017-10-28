#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cleaner.h"
#include "filelock.h"

// Number of maximum processes including the parent
#define MAX_PROC 10

// Main signal handler
static int quit = 0;
static void sig_int(int signo)
{
    if (signo == SIGINT) quit = 1;
}

int main(int argc, char* argv[])
{
    // Check if no argument was given
    if (argc < 2) {
        fprintf(stderr, "No file given to clean\n");
        exit(EXIT_FAILURE);
    }

    // Init signal handling
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags=0;
    sig.sa_handler = sig_int;
    sigaction(SIGINT,&sig,NULL);

    // Disable stdout buffering to fix print order in file logging
    setbuf(stdout, NULL);

    int proc;
    pid_t pid;
    // Divide cmd arguments to even blocks
    int block_size = (argc - 1) / MIN(argc - 1, MAX_PROC) + 1;
    int proc_count = (argc - 1) / block_size + 1;
    // Only spawn children if multiple files are given
    if (argc > 2) {
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

    // Loop over each process' block of args, parent handles trailing
    int bloc_start = (proc - 1) * block_size;
    int block_end = pid == 0 ? proc * block_size : argc - 1;
    for (int arg = bloc_start; arg < block_end; arg++) {
        const char* path = argv[arg + 1];

        // Check if user has interrupted
        if (quit) {
            fprintf(stderr, "Process interrupted by user\n");
            for (int i = arg; i < block_end; i++)
                fprintf(stderr, "Cleaning of \"%s\" skipped\n", argv[i + 1]);
            break;
        }

        // Save current stdout
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

        // Clean the file
        int err = clean_file(path);
        // Rebind main signal handler
        sigaction(SIGINT, &sig, NULL);

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

        // Interrupt by user or failure to restore stdout will end the process
        if (should_break) {
            // Print out names of files left uncleaned
            for (int i = arg + 1; i < block_end; i++)
                fprintf(stderr, "Cleaning of \"%s\" skipped\n", argv[i + 1]);
            break;
        }
    }

    // Parent waits for children to exit
    if (pid != 0) {
        while (wait(NULL) != -1 || errno != ECHILD);
    }

    exit(EXIT_SUCCESS);
}
