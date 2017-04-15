#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    fork();
    lprintf("Hi, I am thread %d\n", gettid());
    char string_buf[10] = {0};
    string_buf[0] = '0' + gettid();
    lprintf("thread %d, before calling readline", gettid());
    readline(9, string_buf);
    lprintf("thread %d, after calling readline", gettid());
    lprintf("addr of string_buf: %p", &string_buf);
    lprintf("thread %d get input %s", gettid(), string_buf);
    while (1) {}
}