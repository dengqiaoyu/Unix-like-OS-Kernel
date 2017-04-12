#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    int my_tid = gettid();
    lprintf("Hi, I am thread %d\n", my_tid);
    int ret = fork();
    if (ret != 0) {
        int j = 0;
        deschedule(&j);
    } else {
        int i = 1;
        while (i != 100000000) {i++;}
        int ret0 = make_runnable(my_tid);
        lprintf("ret0: %d\n", ret0);
    }

    if (gettid() == my_tid)
        lprintf("Hi, I am back!");
    else {
        lprintf("thread %d, should continue working now\n", my_tid);
    }
}