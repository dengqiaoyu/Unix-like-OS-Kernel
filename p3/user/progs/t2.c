/** @file 410user/progs/idle.c
 *  @author ?
 *  @brief Idle program.
 *  @public yes
 *  @for p2 p3
 *  @covers
 *  @status done
 */

#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    char *execname = "cp1";
    char *argvec[2];
    argvec[0] = execname;
    argvec[1] = "try me";
    argvec[2] = NULL;

    lprintf("trying\n");
    int ret = exec(execname, argvec);
    lprintf("%d\n", ret);

    // test code
    // while (1) continue;

    int i = 0;
    while (1) {
        i++;
        if (i % 1000000 == 0)
            lprintf("t2 : %d\n", i);
    }
}
