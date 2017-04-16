#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    lprintf("Hellp I am thread %d", gettid());
}