// C program to demonstrate use of fork() and pipe()
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/file.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sched.h>
#include <limits.h>

#define GIGABYTE 1072741824

void getrandom(void* dest, size_t elsize, size_t count) {
    FILE *f = fopen("/dev/urandom", "rb");
    int nread = fread(dest, elsize, count, f);
    fclose(f);
    printf("Read %d bytes from urandom, expected %d\n", nread, GIGABYTE);
}

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.
   Thanks to
   https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.5/html_node/Elapsed-Time.html */
int
timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

int main()
{
    cpu_set_t set;

    int ctrlpipe[2]; // Our pipe
    // The protocol is:
    // time_t for start time
    // unsigned long for data count
    // data
    // once count is reached, look for start time again
    // if start time is zero, clean it up
    
    printf("PIPE_BUF: %d\n", PIPE_BUF);

    pid_t p;

    if (pipe(ctrlpipe) == -1) {
        perror("Control pipe failed");
        return 1;
    }

    CPU_ZERO(&set);

    p = fork();

    if (p < 0) {
        perror("Fork failed");
        return 1;
    }

    // Parent process
    else if (p > 0) {

        struct timeval start, end, diff;

        // Read a gigabyte from /dev/urandom and time it
        gettimeofday(&start, NULL);
        char* random = malloc(GIGABYTE * sizeof(char));
        getrandom(random, sizeof(char), GIGABYTE);
        gettimeofday(&end, NULL);
        timeval_subtract(&diff, &end, &start);
        printf("Took %lu.%lu to read a gigabyte of random data\n",
                diff.tv_sec,
                diff.tv_usec);

        CPU_SET(0, &set);
        if(sched_setaffinity(getpid(), sizeof(set), &set) == -1)
            perror("sched_setaffinity");

        close(ctrlpipe[0]); // Close reading end of first pipe

        unsigned long limit = pow(2, 60);
        unsigned long buflen, i;
        ssize_t ret;

        struct timeval zero = {
            .tv_sec = 0, 
            .tv_usec = 0,
        };

        // Loop over exponentially larger buffer sizes.
        for(buflen = 256; buflen < limit; buflen = buflen << 1) {
        
            // Get the time
            gettimeofday(&start, NULL);

            //printf("parent sent start time %ld.%ld\n", start.tv_sec, start.tv_usec);
            //printf("parent sending %lu which will happen %lu times\n", buflen, GIGABYTE/buflen);
            //fflush(stdout);

            // If we're trying to write more than is available, forget about it
            if(GIGABYTE / buflen < 1) {
                break;
            }

            // Write the time to the pipe
            ret = write(ctrlpipe[1], &start, sizeof(struct timeval));
            if(ret < 0) {
                perror("write time to pipe");
                fflush(stderr);
            }

            // Write the length to he pipe
            write(ctrlpipe[1], &buflen, sizeof(buflen));
            if(ret < 0) {
                perror("write length to pipe");
                fflush(stderr);
            }

            // Write data to the pipe buflen at a time
            for(i = 0; i < GIGABYTE / buflen; i++) {
                ret = write(ctrlpipe[1], &random[i * buflen], buflen * sizeof(char));
                if(ret < 0) {
                    perror("write data to pipe");
                    fflush(stderr);
                } else if(ret != buflen * sizeof(char)) {
                    //printf("Supposed to write %lu bytes but only wrote %lu bytes\n", buflen * sizeof(char), ret);
                    //fflush(stdout);
                }
            }
            //printf("parent wrote %lu times\n", i);
            //fflush(stdout);

            // That's it start over again
        }

        // We're done here, send a zero
        write(ctrlpipe[1], &zero, sizeof(struct timeval));

        // Wait for child to wreck it
        wait(NULL);
    }

    // child process
    else {
        CPU_SET(0, &set);
        if(sched_setaffinity(getpid(), sizeof(set), &set) == -1)
            perror("sched_setaffinity");

        close(ctrlpipe[1]); // Close writing end of first pipe

        char* random = malloc(GIGABYTE * sizeof(char));
        unsigned long buflen, i;
        ssize_t ret, have_read;
        struct timeval start, end, diff;

        // Receive commands
        while(1) {

            // Read the start time from the pipe
            ret = read(ctrlpipe[0], &start, sizeof(struct timeval));
            if(ret < 0) {
                perror("read time from pipe");
                fflush(stderr);
            }

            // Quit if a zero is received here
            if(start.tv_sec == 0 && start.tv_usec == 0) {
                break;
            }

            // Read the buffer length from the pipe
            ret = read(ctrlpipe[0], &buflen, sizeof(buflen));
            if(ret < 0) {
                perror("read length from pipe");
                fflush(stderr);
            }

            //printf("Child received start time %ld.%ld\n", start.tv_sec, start.tv_usec);
            //printf("Child received buflen %lu which will happen %lu times\n", buflen, GIGABYTE/buflen);
            //fflush(stdout);

            // Oops
            if (buflen > GIGABYTE) {
                printf("That ain't right...\n");
                break;
            }

            // Read data from the pipe buflen at a time
            for(i = 0; i < GIGABYTE / buflen; i++) {
                ret = -2;
                have_read = 0;
                while(have_read < buflen * sizeof(char)) {
                    ret = read(ctrlpipe[0], &random[have_read + (i * buflen)], (buflen * sizeof(char)) - have_read);
                    if(ret < 0) {
                        perror("read data from pipe");
                        fflush(stderr);
                    } else if(ret != buflen * sizeof(char)) {
                        //printf("Have read %lu; Expected to read %lu but only read %lu\n", have_read, buflen * sizeof(char), ret);
                        //fflush(stdout);
                    }
                    have_read += ret;
                }
            }
            // Get the time
            gettimeofday(&end, NULL);

            //printf("child read %lu times\n", i);
            //fflush(stdout);

            timeval_subtract(&diff, &end, &start);

            // Report on what happened
            printf("%lu,%ld.%ld\n",
                    buflen,
                    diff.tv_sec,
                    diff.tv_usec);
            fflush(stdout);
        }

        // Close both reading ends
        close(ctrlpipe[0]);
    }
    exit(0);
}
