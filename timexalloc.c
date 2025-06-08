#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

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
    int* mem;
    struct timeval start, end, diff;
    unsigned long long int limit = pow(2, 60);
    char* state[] = {"malloc", "memset", "naive", "free", "calloc", "free"};

    // Loop over exponentially larger array sizes.
    for(int i = 1; i < limit; i = i << 1) {

        // Loop over the program states. Measure and report the time in each state.
        for(int j = 0; j < 6; j++) {

            gettimeofday(&start, NULL);

            switch(j) {

                case 0:
                mem = malloc(i * sizeof(int));
                break;

                case 1:
                memset(mem, 0, i * sizeof(int));

                case 2:
                for(int n = 0; n < i * sizeof(int); n++) {
                    mem[i] = 1;
                }

                case 4:
                mem = calloc(i, sizeof(int));
                break;

                case 3:
                case 5:
                free(mem);
                break;
            }

            gettimeofday(&end, NULL);

            timeval_subtract(&diff, &end, &start);

            printf("%s\t%d\t%ld.%06ld\n",
                    state[j],
                    i,
                    diff.tv_sec,
                    diff.tv_usec);
        }
    }

    return 0;
}
