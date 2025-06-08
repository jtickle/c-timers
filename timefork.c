// C program to demonstrate use of fork() and pipe()
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <sched.h>

int main()
{
    cpu_set_t set;

    int fd1[2]; // Used to store two ends of first pipe

    pid_t p;

    if (pipe(fd1) == -1) {
        fprintf(stderr, "Pipe Failed");
        return 1;
    }

    CPU_ZERO(&set);

    p = fork();

    if (p < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }

    // Parent process
    else if (p > 0) {
        CPU_SET(0, &set);
        if(sched_setaffinity(getpid(), sizeof(set), &set) == -1)
            perror("sched_setaffinity");

        close(fd1[0]); // Close reading end of first pipe
        
        // Get the time
        struct timeval t;
        gettimeofday(&t, NULL);
        suseconds_t start = t.tv_usec;

        // Write the time to the pipe
        write(fd1[1], &start, sizeof(start));
        close(fd1[1]);

        // Show the time
        printf("Start time was %ld\n", start);

        // Wait for child to send a string
        wait(NULL);
    }

    // child process
    else {
        CPU_SET(0, &set);
        if(sched_setaffinity(getpid(), sizeof(set), &set) == -1)
            perror("sched_setaffinity");

        close(fd1[1]); // Close writing end of first pipe

        // Read a string using first pipe
        suseconds_t start;
        read(fd1[0], &start, sizeof(start));

        struct timeval t;
        gettimeofday(&t, NULL);
        suseconds_t end = t.tv_usec;

        // Close both reading ends
        close(fd1[0]);

        printf("End time was %ld\n", end);

        printf("Difference was %ld\n", end - start);

        exit(0);
    }
}
