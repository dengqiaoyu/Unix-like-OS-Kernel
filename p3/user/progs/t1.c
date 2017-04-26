#include <syscall.h>  /* gettid() */
#include <simics.h>   /* lprintf() */

/* Main */
int main() {
    int i = 0;
    lprintf("fam\n");
    while (1) {
        i++;
        // if (i % 1000000 == 0) lprintf("t1 : %d\n", i);
    }
}
