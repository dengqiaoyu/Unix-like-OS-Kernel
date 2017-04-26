/** @file 410user/progs/ck1.c
 *  @author ?
 *  @author elly1
 *  @brief Runs the checkpoint 1 script.
 *  @public yes
 *  @for p3
 *  @covers gettid
 *  @status done
 */

#include <syscall.h>  /* gettid() */
#include <simics.h>   /* lprintf() */
#include <stdio.h>

char *noop(char *buf)
{
    return buf;
}

/* Main */
int main(int argc, char *argv[])
{
    sim_ck1();

    int tid = gettid();
    lprintf("%d\n", tid);
    lprintf("real exec\n");
    lprintf("argc: %d\n", argc);
    lprintf(argv[0]);
    lprintf(argv[1]);
    if (argv[2] == 0) lprintf("that is all");

    char buf[16];
    sprintf(buf, "pls 6 = %d", 6);
    lprintf("i want %s", buf);

    while (1) {
        continue;
    }
}
