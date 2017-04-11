#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    lprintf("Hi I am thread %d\n", gettid());
    int j = 0;
    int ret = deschedule(&j);
    lprintf("ret: %d\n", ret);
    lprintf("You should never see me %d\n", gettid());
}