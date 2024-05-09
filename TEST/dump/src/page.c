#include <unistd.h>
#include <stdio.h>

int main() {
    long pagesize = sysconf(_SC_PAGESIZE);
    printf("Page size: %ld bytes\n", pagesize);
    return 0;
}