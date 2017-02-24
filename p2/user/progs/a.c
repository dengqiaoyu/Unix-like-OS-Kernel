#include <stdio.h>
#include <thread.h>
#include <syscall.h> /* for PAGE_SIZE */
#include <simics.h>
#include <assert.h>

void *
waiter(void *p) {
    int status;

    // int zero = 0;
    // int divided_by_zero = 1 / zero;
    // printf("impossible to get %d\n", divided_by_zero);

    // int *NULL_ptr = NULL;
    // assert(0 == 1);
    // *NULL_ptr = 2;

    // int overflow = (2147483647);
    // overflow++;

    sim_breakpoint();


    thr_join((int)p, (void **)&status);
    printf("Thread %d exited '%c'\n", (int) p, (char)status);

    thr_exit((void *) 0);

    while (1)
        continue; /* placate compiler portably */
}

int main() {
    thr_init(16 * PAGE_SIZE);

    (void) thr_create(waiter, (void *) thr_getid());

    sleep(10); /* optional, of course!! */

    thr_exit((void *)'!');

    while (1)
        continue; /* placate compiler portably */
}
