#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    lprintf("Hi, I am thread %d in user space\n", gettid());
    int i = fork();
    lprintf("fork() returns %d\n", i);
    while (1) {}
}