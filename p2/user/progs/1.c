#include <stdio.h>
#include <stdlib.h>
#include <thread.h>
#include <syscall.h> /* for PAGE_SIZE */

void *
waiter(void *p) {
    int status;

    printf("waiter exited\n");
    exit(1);
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
    exit(1);

    while (1)
        continue; /* placate compiler portably */
}
