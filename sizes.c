#include <sys/types.h>
#include <stdio.h>

int main() {
    printf("int: %lu\n", sizeof(int));
    printf("char: %lu\n", sizeof(char));
    printf("long: %lu\n", sizeof(long));
    printf("long long: %lu\n", sizeof(long long));
    return 0;
}
