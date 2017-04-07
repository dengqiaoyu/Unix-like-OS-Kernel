#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    lprintf("I am thread %d", gettid());
    if (fork() == 0) {
        lprintf("I am thread %d", gettid());
    }
    if (fork() == 0) {
        lprintf("I am thread %d", gettid());
    }
    if (fork() == 0) {
        lprintf("I am thread %d", gettid());
    }
    if (fork() == 0) {
        lprintf("I am thread %d", gettid());
    }
    if (fork() == 0) {
        lprintf("I am thread %d", gettid());
    }
    if (fork() == 0) {
        lprintf("I am thread %d", gettid());
    }
    while (1) {}
}
