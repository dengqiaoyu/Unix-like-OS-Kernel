#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    fork();
    printf("Hi, I am thread %d\n", gettid());
    while (1) {}
}
